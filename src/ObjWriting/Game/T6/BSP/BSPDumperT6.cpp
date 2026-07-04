#pragma once

#include "BSPDumperT6.h"

#include "BSP/BSP.h"
#include "BSP/BSPUtil.h"
#include "Game/T6/CommonT6.h"
#include "Gltf/JsonGltf.h"
#include "Utils/Logging/Log.h"
#include "Utils/Pack.h"
#include "XModel/Gltf/GltfBinOutput.h"
#include "XModel/Gltf/GltfTextOutput.h"
#include "XModel/Gltf/GltfWriter.h"

#pragma warning(push, 0)
#include <Eigen>
#pragma warning(pop)

#include <QuickHull.hpp>
#include <deque>
#include <unordered_set>

using namespace T6;
using namespace BSP;
using namespace BSPFlags;
using namespace gltf;
using namespace quickhull;

namespace
{
    unsigned m_total_accessor_types = 5;
    unsigned m_position_accessor_start = 0u;
    unsigned m_normal_accessor_start = 0u;
    unsigned m_uv_accessor_start = 0u;
    unsigned m_color_accessor_start = 0u;
    unsigned m_index_accessor_start = 0u;

    unsigned m_vertex_buffer_view = 0u;
    unsigned m_index_buffer_view = 0u;

    constexpr vec4_t whiteColour = {1.0f, 1.0f, 1.0f, 1.0f};

    void LhcToRhcCoordinates(float (&coords)[3])
    {
        const float two[3]{coords[0], coords[1], coords[2]};

        coords[0] = two[0];
        coords[1] = two[2];
        coords[2] = -two[1];
    }

    void LhcToRhcQuaternion(float (&quat)[4])
    {
        const float two[4]{quat[0], quat[1], quat[2], quat[3]};

        quat[0] = two[0];
        quat[1] = two[2];
        quat[2] = -two[1];
        quat[3] = two[3];
    }

    void LhcToRhcIndices(unsigned short* indices)
    {
        const unsigned short two[3]{indices[0], indices[1], indices[2]};

        indices[0] = two[2];
        indices[1] = two[1];
        indices[2] = two[0];
    }

    bool flagsMatchExact(int flag1, int flag2)
    {
        return (flag1 & flag2) == flag1;
    }

    bool flagsMatchAny(int flag1, int flag2)
    {
        return (flag1 & flag2) != 0;
    }

    int getSurfaceTypeFromFlags(int surfaceFlags)
    {
        return ((surfaceFlags >> 20) & 0x3F);
    }

    std::string convertFlagsToString(int surfaceflags, int contentflags)
    {
        std::string result;

        for (size_t surfType = 0; surfType < BSP_SURF_TYPE_CLIPMISSILE; surfType++)
        {
            s_SurfaceTypeFlags sFlags = surfaceTypeToFlagMap[surfType];
            if (getSurfaceTypeFromFlags(surfaceflags) == getSurfaceTypeFromFlags(sFlags.surfaceFlags)
                && (sFlags.contentFlags == 0 || flagsMatchExact(sFlags.contentFlags, contentflags)))
            {
                if (surfType == BSP_SURF_TYPE_OPAQUEGLASS && !flagsMatchAny(sFlags.contentFlags, surfaceTypeToFlagMap[BSP_SURF_TYPE_GLASS].contentFlags))
                    result += std::format("{}, ", surfaceTypeToNameMap[surfType]);
            }
        }

        for (size_t surfType = BSP_SURF_TYPE_CLIPMISSILE; surfType < BSP_SURF_TYPE_COUNT; surfType++)
        {
            if (surfType == BSP_SURF_TYPE_ORIGIN || surfType == BSP_SURF_TYPE_PHYSICSGEOM || surfType == BSP_SURF_TYPE_LIGHTPORTAL)
                continue;

            s_SurfaceTypeFlags sFlags = surfaceTypeToFlagMap[surfType];
            if ((sFlags.surfaceFlags == 0 || flagsMatchExact(sFlags.surfaceFlags, surfaceflags))
                && (sFlags.contentFlags == 0 || flagsMatchExact(sFlags.contentFlags, contentflags)))
                result += std::format("{}, ", surfaceTypeToNameMap[surfType]);
        }

        if (result.size() != 0)
            result.resize(result.size() - 2);
        return result;
    }

    // center vertices around (0, 0, 0), return position that original vertices centred around
    vec3_t moveVerticesToOrigin(std::vector<BSPVertex>& inout_vertBuffer)
    {
        if (inout_vertBuffer.empty())
            return {0.0f, 0.0f, 0.0f};

        vec3_t mins{};
        vec3_t maxs{};
        bool first = true;
        for (auto& vert : inout_vertBuffer)
        {
            if (first == true)
            {
                first = false;
                mins = vert.pos;
                maxs = vert.pos;
            }
            else
                BSPUtil::updateAABBWithPoint(vert.pos, mins, maxs);
        }

        vec3_t middle = BSPUtil::calcMiddleOfAABB(mins, maxs);

        for (auto& vert : inout_vertBuffer)
        {
            vert.pos.x = vert.pos.x - middle.x;
            vert.pos.y = vert.pos.y - middle.y;
            vert.pos.z = vert.pos.z - middle.z;
        }

        return middle;
    }

    struct OutSurface
    {
        size_t surfaceStart;
        size_t surfaceCount;
    };

    struct PartitionData
    {
        size_t partitionIdx;
        size_t materialIdx;

        bool operator==(const PartitionData& other) const
        {
            return partitionIdx == other.partitionIdx && materialIdx == other.materialIdx;
        }
    };

    void getPartitionsFromAABBTree(const clipMap_t* clipmap, size_t aabbStartIndex, size_t aabbCount, std::vector<PartitionData>& partitionList)
    {
        std::deque<size_t> aabbQueue;
        for (size_t aabbIdx = 0; aabbIdx < aabbCount; aabbIdx++)
            aabbQueue.emplace_back(aabbStartIndex + aabbIdx);

        while (!aabbQueue.empty())
        {
            size_t aabbIdx = aabbQueue.front();
            aabbQueue.pop_front();

            CollisionAabbTree* aabb = &clipmap->aabbTrees[aabbIdx];
            if (aabb->childCount == 0)
            {
                PartitionData data{};
                data.partitionIdx = static_cast<size_t>(aabb->u.partitionIndex);
                data.materialIdx = static_cast<size_t>(aabb->materialIndex);
                partitionList.emplace_back(data);
            }
            else
            {
                for (uint16_t childIdx = 0; childIdx < aabb->childCount; childIdx++)
                    aabbQueue.emplace_back(static_cast<size_t>(aabb->u.firstChildIndex) + childIdx);
            }
        }
    }

    void createSurfacesFromPartitions(
        const clipMap_t* clipmap, BSPData& dumpData, std::vector<PartitionData>& partitionList, bool useWorldCoordinates, OutSurface& result)
    {
        std::vector<std::pair<size_t, std::vector<size_t>>> materialPartitions;
        for (PartitionData partitionData : partitionList)
        {
            bool found = false;
            for (auto& matSurf : materialPartitions)
            {
                if (matSurf.first == partitionData.materialIdx)
                {
                    bool foundInner = false;
                    for (size_t partIdx : matSurf.second)
                    {
                        if (partitionData.partitionIdx == partIdx)
                        {
                            foundInner = true;
                            break;
                        }
                    }
                    if (!foundInner)
                        matSurf.second.emplace_back(partitionData.partitionIdx);

                    found = true;
                    break;
                }
            }
            if (!found)
                materialPartitions.emplace_back(std::pair(partitionData.materialIdx, std::vector<size_t>({static_cast<size_t>(partitionData.partitionIdx)})));
        }

        result.surfaceCount = materialPartitions.size();
        result.surfaceStart = dumpData.colWorld.surfaces.size();

        for (const auto& matPartition : materialPartitions)
        {
            std::vector<BSPVertex> tempVertices;
            std::vector<size_t> tempIndices;
            size_t tempTriCount = 0;
            for (const auto& partIdx : matPartition.second)
            {
                CollisionPartition* partition = &clipmap->partitions[partIdx];

                size_t partitionStartVertex = tempVertices.size();
                tempTriCount += partition->triCount;
                for (size_t k = 0; k < partition->triCount; k++)
                {
                    uint16_t* vertIndices = clipmap->triIndices[static_cast<size_t>(partition->firstTri) + k];

                    BSPVertex vert0{};
                    BSPVertex vert1{};
                    BSPVertex vert2{};
                    vert0.pos = clipmap->verts[vertIndices[0]];
                    vert1.pos = clipmap->verts[vertIndices[1]];
                    vert2.pos = clipmap->verts[vertIndices[2]];

                    LhcToRhcCoordinates(vert0.pos.v);
                    LhcToRhcCoordinates(vert1.pos.v);
                    LhcToRhcCoordinates(vert2.pos.v);

                    vert0.color = {1.0f, 1.0f, 1.0f, 1.0f};
                    vert1.color = {1.0f, 1.0f, 1.0f, 1.0f};
                    vert2.color = {1.0f, 1.0f, 1.0f, 1.0f};

                    tempVertices.emplace_back(vert0);
                    tempVertices.emplace_back(vert1);
                    tempVertices.emplace_back(vert2);

                    // equivalent of LhcToRhcIndices
                    tempIndices.emplace_back(static_cast<uint16_t>(partitionStartVertex + (k * 3) + 2));
                    tempIndices.emplace_back(static_cast<uint16_t>(partitionStartVertex + (k * 3) + 1));
                    tempIndices.emplace_back(static_cast<uint16_t>(partitionStartVertex + (k * 3) + 0));
                }
            }

            std::vector<BSPVertex> outputVertexBuffer;
            std::shared_ptr<size_t[]> indexMap = std::make_shared<size_t[]>(tempVertices.size());
            for (size_t i = 0; i < tempVertices.size(); i++)
            {
                bool found = false;
                size_t foundIdx = 0;
                const auto& testVertex = tempVertices.at(i);
                for (size_t j = 0; j < outputVertexBuffer.size(); j++)
                {
                    const auto& inVertex = outputVertexBuffer.at(j);
                    if (inVertex.pos.x == testVertex.pos.x && inVertex.pos.y == testVertex.pos.y && inVertex.pos.z == testVertex.pos.z)
                    {
                        found = true;
                        foundIdx = j;
                        break;
                    }
                }
                if (!found)
                {
                    indexMap[i] = outputVertexBuffer.size();
                    outputVertexBuffer.emplace_back(testVertex);
                }
                else
                    indexMap[i] = foundIdx;
            }
            assert(outputVertexBuffer.size() != 0);

            BSPSurface outSurface{};
            outSurface.isLocalCoords = !useWorldCoordinates;
            outSurface.origin = {};
            if (!useWorldCoordinates)
                outSurface.origin = moveVerticesToOrigin(outputVertexBuffer);
            outSurface.materialIndex = matPartition.first;
            outSurface.triCount = tempTriCount;
            outSurface.vertexCount = outputVertexBuffer.size();
            outSurface.indexOfFirstVertex = dumpData.colWorld.vertices.size();
            outSurface.indexOfFirstIndex = dumpData.colWorld.indices.size();
            dumpData.colWorld.surfaces.emplace_back(outSurface);

            dumpData.colWorld.vertices.insert(dumpData.colWorld.vertices.end(), outputVertexBuffer.begin(), outputVertexBuffer.end());
            for (size_t idx : tempIndices)
            {
                assert(indexMap[idx] <= UINT16_MAX);
                dumpData.colWorld.indices.emplace_back(static_cast<uint16_t>(indexMap[idx]));
            }
        }
    }

    void createSurfacesFromBrushes(const clipMap_t* clipmap, BSPData& dumpData, std::vector<size_t>& brushList, bool useWorldCoordinates, OutSurface& result)
    {
        result.surfaceCount = 0;
        result.surfaceStart = dumpData.colWorld.surfaces.size();
        size_t totalVertexCount = dumpData.colWorld.vertices.size();
        size_t totalIndexCount = dumpData.colWorld.indices.size();
        std::unordered_set<size_t> uniqueBrushes;
        for (size_t brushIdx : brushList)
        {
            if (uniqueBrushes.contains(brushIdx))
                continue;

            cbrush_t* brush = &clipmap->info.brushes[brushIdx];

            std::vector<vec3_t> hullVerts;
            if (brush->numverts != 0)
            {
                for (unsigned int vertIdx = 0; vertIdx < brush->numverts; vertIdx++)
                    hullVerts.emplace_back(brush->verts[vertIdx]);
            }
            else
            {
                vec3_t mins = brush->mins;
                vec3_t maxs = brush->maxs;

                float xDiff = maxs.x - mins.x;
                float yDiff = maxs.y - mins.y;
                float zDiff = maxs.z - mins.z;

                vec3_t minsUp = mins;
                vec3_t minsLeft = mins;
                vec3_t minsRight = mins;
                minsUp.x += xDiff;
                minsLeft.y += yDiff;
                minsRight.z += zDiff;

                vec3_t maxsUp = maxs;
                vec3_t maxsLeft = maxs;
                vec3_t maxsRight = maxs;
                maxsUp.x -= xDiff;
                maxsLeft.y -= yDiff;
                maxsRight.z -= zDiff;

                hullVerts.emplace_back(mins);
                hullVerts.emplace_back(minsUp);
                hullVerts.emplace_back(minsLeft);
                hullVerts.emplace_back(minsRight);
                hullVerts.emplace_back(maxs);
                hullVerts.emplace_back(maxsUp);
                hullVerts.emplace_back(maxsLeft);
                hullVerts.emplace_back(maxsRight);
            }

            if (hullVerts.size() == 0)
            {
                con::info("Brush with 0 verts, skipping");
                continue;
            }

            for (size_t vertIdx = 0; vertIdx < hullVerts.size(); vertIdx++)
                LhcToRhcCoordinates(hullVerts[vertIdx].v);

            QuickHull<float> qh;
            auto hull = qh.getConvexHull(&hullVerts[0].x, hullVerts.size(), false, true);
            std::vector<size_t> indexBuffer = hull.getIndexBuffer();
            VertexDataSource<float> vertexBuffer = hull.getVertexBuffer();
            assert(indexBuffer.size() % 3 == 0);

            std::vector<BSPVertex> outVertexBuffer;
            for (const auto& vertex : vertexBuffer)
            {
                BSPVertex vert{};
                vert.pos.x = vertex.x;
                vert.pos.y = vertex.y;
                vert.pos.z = vertex.z;
                outVertexBuffer.emplace_back(vert);
            }

            BSPSurface surface;
            surface.isLocalCoords = !useWorldCoordinates;
            surface.origin = {};
            if (!useWorldCoordinates)
                surface.origin = moveVerticesToOrigin(outVertexBuffer);
            surface.indexOfFirstVertex = totalVertexCount;
            surface.indexOfFirstIndex = totalIndexCount;
            surface.triCount = indexBuffer.size() / 3;
            surface.vertexCount = outVertexBuffer.size();

            size_t matIndex = -1;
            for (unsigned i = 0; i < clipmap->info.numMaterials; i++)
            {
                // this is how brushes with no data selects the flags during a trace
                if (clipmap->info.materials[i].contentFlags == brush->axial_cflags[1][2]
                    && clipmap->info.materials[i].surfaceFlags == brush->axial_sflags[1][2])
                {
                    matIndex = i;
                    break;
                }
            }

            if (matIndex == -1)
            {
                con::warn("Coulndn't determine material idx from flags");
                BSPMaterial bspMaterial;
                bspMaterial.materialName = std::format("unknown_material_{}", dumpData.colWorld.materials.size());
                bspMaterial.materialType = MATERIAL_TYPE_TEXTURE;
                bspMaterial.materialColour = whiteColour;
                bspMaterial.surfaceFlags = brush->axial_sflags[1][2];
                bspMaterial.contentFlags = brush->axial_cflags[1][2];
                matIndex = dumpData.colWorld.materials.size();
                dumpData.colWorld.materials.emplace_back(bspMaterial);
            }
            surface.materialIndex = matIndex;
            dumpData.colWorld.surfaces.emplace_back(surface);

            dumpData.colWorld.vertices.insert(dumpData.colWorld.vertices.end(), outVertexBuffer.begin(), outVertexBuffer.end());

            for (const auto& index : indexBuffer)
            {
                assert(index <= UINT16_MAX);
                dumpData.colWorld.indices.emplace_back(static_cast<uint16_t>(index));
            }

            totalVertexCount += surface.vertexCount;
            totalIndexCount += surface.triCount * 3;

            uniqueBrushes.emplace(brushIdx);
        }

        result.surfaceCount = uniqueBrushes.size();
    }

    void getBrushesFromLeafBrushNode(const clipMap_t* clipmap, size_t leafBrushNodeIdx, std::vector<size_t>& brushList)
    {
        if (leafBrushNodeIdx == 0)
        {
            con::warn("getBrushesFromLeafBrushNode: leafBrushNodeIdx == 0");
            return;
        }

        std::deque<size_t> leafBrushNodeQueue;
        leafBrushNodeQueue.emplace_back(leafBrushNodeIdx);

        while (!leafBrushNodeQueue.empty())
        {
            size_t lbnIdx = leafBrushNodeQueue.front();
            leafBrushNodeQueue.pop_front();
            auto leafBrushNode = &clipmap->info.leafbrushNodes[lbnIdx];

            if (leafBrushNode->leafBrushCount > 0)
            {
                for (int16_t i = 0; i < leafBrushNode->leafBrushCount; i++)
                    brushList.emplace_back(static_cast<size_t>(leafBrushNode->data.leaf.brushes[i]));
                continue;
            }

            if (leafBrushNode->leafBrushCount < 0)
                leafBrushNodeQueue.emplace_back(lbnIdx + 1);
            leafBrushNodeQueue.emplace_back(lbnIdx + static_cast<size_t>(leafBrushNode->data.children.childOffset[0]));
            leafBrushNodeQueue.emplace_back(lbnIdx + static_cast<size_t>(leafBrushNode->data.children.childOffset[1]));
        }
    }

    void getStaticCollisionList(const clipMap_t* clipmap, std::vector<PartitionData>& partitionList, std::vector<size_t>& brushList)
    {
        std::deque<int16_t> nodeQueue;
        nodeQueue.emplace_back(0);
        while (!nodeQueue.empty())
        {
            int16_t nodeIdx = nodeQueue.front();
            nodeQueue.pop_front();

            if (nodeIdx < 0)
            {
                int leafIndex = -1 - nodeIdx;
                cLeaf_s* leaf = &clipmap->leafs[leafIndex];

                if (leaf->collAabbCount != 0)
                    getPartitionsFromAABBTree(clipmap, leaf->firstCollAabbIndex, leaf->collAabbCount, partitionList);
                if (leaf->leafBrushNode != 0)
                    getBrushesFromLeafBrushNode(clipmap, leaf->leafBrushNode, brushList);
            }
            else
            {

                nodeQueue.emplace_back(clipmap->nodes[nodeIdx].children[0]);
                nodeQueue.emplace_back(clipmap->nodes[nodeIdx].children[1]);
            }
        }
    }

    float lengthOfVector(float x, float y, float z)
    {
        return sqrtf(x * x + y * y + z * z);
    }

    void dumpClipmapXModels(BSPData& dumpData, const clipMap_t* clipmap)
    {
        for (size_t modelIdx = 0; modelIdx < clipmap->numStaticModels; modelIdx++)
        {
            cStaticModel_s* colModel = &clipmap->staticModelList[modelIdx];

            // axis vectors should be normalised (length 1), and when scaling is applied it's length will increase with scale
            float length1 = lengthOfVector(colModel->invScaledAxis[0].x, colModel->invScaledAxis[0].y, colModel->invScaledAxis[0].z);
            float length2 = lengthOfVector(colModel->invScaledAxis[1].x, colModel->invScaledAxis[1].y, colModel->invScaledAxis[1].z);
            float length3 = lengthOfVector(colModel->invScaledAxis[2].x, colModel->invScaledAxis[2].y, colModel->invScaledAxis[2].z);
            float forcedScale1 = 1.0f / length1;
            float forcedScale2 = 1.0f / length2;
            float forcedScale3 = 1.0f / length3;

            vec3_t origAxis[3]{};
            origAxis[0].x = colModel->invScaledAxis[0].x * forcedScale1;
            origAxis[0].y = colModel->invScaledAxis[0].y * forcedScale1;
            origAxis[0].z = colModel->invScaledAxis[0].z * forcedScale1;
            origAxis[1].x = colModel->invScaledAxis[1].x * forcedScale2;
            origAxis[1].y = colModel->invScaledAxis[1].y * forcedScale2;
            origAxis[1].z = colModel->invScaledAxis[1].z * forcedScale2;
            origAxis[2].x = colModel->invScaledAxis[2].x * forcedScale3;
            origAxis[2].y = colModel->invScaledAxis[2].y * forcedScale3;
            origAxis[2].z = colModel->invScaledAxis[2].z * forcedScale3;

            BSPXModel model{};
            assert(colModel->xmodel != nullptr);
            model.name = colModel->xmodel->name[0] == ',' ? &colModel->xmodel->name[1] : colModel->xmodel->name;
            model.origin = colModel->origin;
            LhcToRhcCoordinates(model.origin.v);
            model.rotationQuaternion = BSPUtil::convertAxisToQuat(origAxis);
            LhcToRhcQuaternion(model.rotationQuaternion.v);
            model.scale = {forcedScale1, forcedScale2, forcedScale3};
            dumpData.colWorld.xmodels.emplace_back(model);
        }
    }

    void dumpClipmap(BSPData& dumpData, const clipMap_t* clipmap)
    {
        for (unsigned int i = 0; i < clipmap->info.numMaterials; i++)
        {
            auto colMaterial = &clipmap->info.materials[i];
            BSPMaterial bspMaterial;
            bspMaterial.materialName = colMaterial->name;
            bspMaterial.materialType = MATERIAL_TYPE_TEXTURE;
            bspMaterial.materialColour = whiteColour;
            bspMaterial.surfaceFlags = colMaterial->surfaceFlags;
            bspMaterial.contentFlags = colMaterial->contentFlags;
            dumpData.colWorld.materials.emplace_back(bspMaterial);
        }

        dumpClipmapXModels(dumpData, clipmap);

        std::vector<PartitionData> partitionList;
        std::vector<size_t> brushList;
        getStaticCollisionList(clipmap, partitionList, brushList);

        OutSurface result{};
        createSurfacesFromPartitions(clipmap, dumpData, partitionList, true, result); // use world coords as entire mesh is dumped as one node
        dumpData.staticTerrainSurfaceCount = result.surfaceCount;
        dumpData.staticTerrainSurfaceStart = result.surfaceStart;

        createSurfacesFromBrushes(clipmap, dumpData, brushList, false, result); // use local coords as each brush has it's own node
        dumpData.staticBrushSurfaceCount = result.surfaceCount;
        dumpData.staticBrushSurfaceStart = result.surfaceStart;
    }

    size_t createModelFromIndex(size_t modelIndex, BSPData& dumpData, const GfxWorld* gfxworld, const clipMap_t* clipmap)
    {
        assert(modelIndex != 0);

        BSPModel model{};

        // all model verts are in local coordinates (already cnetred around origin)
        if (gfxworld->models[modelIndex].surfaceCount != 0)
        {
            model.surfaceSide = MSS_GFX;
            model.gfxSurfaceCount = static_cast<size_t>(gfxworld->models[modelIndex].surfaceCount);

            // static surfas are always first, followed by ent surfs
            size_t updatedSurfIndex = static_cast<size_t>(gfxworld->models[modelIndex].startSurfIndex) - gfxworld->dpvs.staticSurfaceCount;
            updatedSurfIndex += dumpData.staticSurfaceStart + dumpData.staticSurfaceCount;
            model.gfxSurfaceIndex = updatedSurfIndex;
        }

        cLeaf_s* leaf = &clipmap->cmodels[modelIndex].leaf;
        if (leaf->collAabbCount != 0)
        {
            if (gfxworld->models[modelIndex].surfaceCount != 0)
                model.surfaceSide = MSS_BOTH;
            else
                model.surfaceSide = MSS_COL;
            model.surfaceType = MST_TERRAIN;
            std::vector<PartitionData> partitionList;
            getPartitionsFromAABBTree(clipmap, leaf->firstCollAabbIndex, leaf->collAabbCount, partitionList);
            OutSurface result;
            createSurfacesFromPartitions(clipmap, dumpData, partitionList, true, result);
            model.colTerrainSurfaceCount = result.surfaceCount;
            model.colTerrainSurfaceIndex = result.surfaceStart;
        }
        if (leaf->leafBrushNode != 0)
        {
            if (gfxworld->models[modelIndex].surfaceCount != 0)
                model.surfaceSide = MSS_BOTH;
            else
                model.surfaceSide = MSS_COL;
            if (leaf->collAabbCount != 0)
                model.surfaceType = MST_BOTH;
            else
                model.surfaceType = MST_BRUSH;
            std::vector<size_t> brushList;
            getBrushesFromLeafBrushNode(clipmap, static_cast<size_t>(leaf->leafBrushNode), brushList);
            OutSurface result;
            createSurfacesFromBrushes(clipmap, dumpData, brushList, true, result);
            model.colBrushSurfaceCount = result.surfaceCount;
            model.colBrushSurfaceIndex = result.surfaceStart;
        }

        size_t modelIdx = dumpData.models.size();
        dumpData.models.emplace_back(model);
        return modelIdx;
    }

    bool starts_with(const char* str, const char* pre)
    {
        return strncmp(pre, str, strlen(pre)) == 0;
    }

    void dumpMapEnts(BSPData& dumpData, const MapEnts* mapEnts, const GfxWorld* gfxworld, const clipMap_t* clipmap)
    {
        std::unique_ptr<char[]> origEntStrPtr = std::make_unique<char[]>(strlen(mapEnts->entityString) + 1);
        strcpy(origEntStrPtr.get(), mapEnts->entityString);
        char* entStrPtr = origEntStrPtr.get();
        size_t entIdx = 1;
        while (true)
        {
            while (*entStrPtr != '{' && *entStrPtr != '\0')
                entStrPtr++;
            if (*(entStrPtr++) == '\0')
                break;

            BSPEntity entity{};
            entity.rotationQuaternion = {0.0f, 0.0f, 0.0f, 1.0f};
            entity.type = ET_OTHER;
            entity.hasModel = false;
            entity.classname = "unknown";
            entity.uniqueEntityNumber = entIdx++;
            while (true)
            {
                while (*entStrPtr != '"' && *entStrPtr != '}')
                    entStrPtr++;
                if (*(entStrPtr++) == '}')
                    break;
                char* keyStrPtr = entStrPtr;

                while (*entStrPtr != '"')
                    entStrPtr++;
                *entStrPtr = '\0';
                entStrPtr++;

                while (*entStrPtr != '"')
                    entStrPtr++;
                entStrPtr++;
                char* valueStrPtr = entStrPtr;
                while (*entStrPtr != '"')
                    entStrPtr++;
                *entStrPtr = '\0';
                entStrPtr++;

                if (!strcmp(keyStrPtr, "classname"))
                {
                    entity.classname = valueStrPtr;
                    if (starts_with(valueStrPtr, "weapon_"))
                        entity.type = ET_WEAPON;
                    else if (!strcmp(valueStrPtr, "info_notnull"))
                        entity.type = ET_POINT;
                    else if (!strcmp(valueStrPtr, "info_notnull_big"))
                        entity.type = ET_POINT;
                    else if (!strcmp(valueStrPtr, "script_origin"))
                        entity.type = ET_POINT;
                    else if (!strcmp(valueStrPtr, "info_volume"))
                        entity.type = ET_VOLUME;
                    else if (starts_with(valueStrPtr, "trigger_"))
                        entity.type = ET_TRIGGER;
                    else if (!strcmp(valueStrPtr, "light"))
                        entity.type = ET_LIGHT;
                    else if (!strcmp(valueStrPtr, "script_brushmodel"))
                        entity.type = ET_BRUSHMODEL;
                    else if (!strcmp(valueStrPtr, "script_model"))
                        entity.type = ET_MODEL;
                    else if (!strcmp(valueStrPtr, "script_struct"))
                        entity.type = ET_STRUCT;
                    else if (!strcmp(valueStrPtr, "script_vehicle"))
                        entity.type = ET_VEHICLE;
                    else if (!strcmp(valueStrPtr, "info_vehicle_node"))
                        entity.type = ET_VEHICLE;
                    else if (!strcmp(valueStrPtr, "info_vehicle_node_rotate"))
                        entity.type = ET_VEHICLE;
                    else if (starts_with(valueStrPtr, "zbarrier_"))
                        entity.type = ET_ZBARRIER;
                    else if (starts_with(valueStrPtr, "node_"))
                        entity.type = ET_PATHNODE;
                    else if (starts_with(valueStrPtr, "actor_"))
                        entity.type = ET_ACTOR;
                    else if (!strcmp(valueStrPtr, "glass"))
                        entity.type = ET_GLASS;
                    else if (!strcmp(valueStrPtr, "rope"))
                        entity.type = ET_ROPE;
                    else
                        entity.type = ET_OTHER;
                }

                if (!strcmp(keyStrPtr, "origin"))
                {
                    entity.origin = BSPUtil::convertStringToVec3(valueStrPtr);
                    LhcToRhcCoordinates(entity.origin.v);
                }
                else if (!strcmp(keyStrPtr, "angles"))
                {
                    vec3_t angles = BSPUtil::convertStringToVec3(valueStrPtr);
                    entity.rotationQuaternion = BSPUtil::convertAnglesToQuat(angles);
                    LhcToRhcQuaternion(entity.rotationQuaternion.v);
                }
                else if (!strcmp(keyStrPtr, "model") && *valueStrPtr == '*')
                {
                    entity.hasModel = true;
                    entity.modelIndex = createModelFromIndex(atol(valueStrPtr + 1), dumpData, gfxworld, clipmap);
                }
                else
                {
                    BSPEntityEntry entry = {keyStrPtr, valueStrPtr};
                    entity.entries.emplace_back(entry);
                }
            }
            dumpData.entities.emplace_back(entity);
        }
    }

    size_t createBspMaterial(BSPData& dumpData, Material* material, GfxSurfaceFlags surfFlags)
    {
        for (size_t matIdx = 0; matIdx < dumpData.gfxWorld.materials.size(); matIdx++)
        {
            auto& mat = dumpData.gfxWorld.materials.at(matIdx);
            if (!mat.materialName.compare(material->info.name) && (mat.surfaceFlags == surfFlags))
                return matIdx;
        }

        BSPMaterial bspMaterial;
        bspMaterial.materialName = material->info.name;
        bspMaterial.materialType = MATERIAL_TYPE_TEXTURE;
        bspMaterial.materialColour = whiteColour;
        bspMaterial.surfaceFlags = surfFlags;
        bspMaterial.contentFlags = surfFlags;
        size_t matIndex = dumpData.gfxWorld.materials.size();
        dumpData.gfxWorld.materials.emplace_back(bspMaterial);
        return matIndex;
    }

    std::shared_ptr<size_t[]>
        simplifyVertexBuffer(std::vector<BSPVertex>& in_vertexBuffer, std::unique_ptr<bool[]>& in_isVertUsedMap, std::vector<BSPVertex>& out_vertexBuffer)
    {
        assert(out_vertexBuffer.size() == 0);
        std::shared_ptr<size_t[]> indexMap = std::make_shared<size_t[]>(in_vertexBuffer.size());
        for (size_t i = 0; i < in_vertexBuffer.size(); i++)
        {
            if (!in_isVertUsedMap[i])
                continue;
            bool found = false;
            size_t foundIdx = 0;
            const auto& testVertex = in_vertexBuffer.at(i);
            for (size_t j = 0; j < out_vertexBuffer.size(); j++)
            {
                const auto& inVertex = out_vertexBuffer.at(j);
                if (inVertex.pos.x == testVertex.pos.x && inVertex.pos.y == testVertex.pos.y && inVertex.pos.z == testVertex.pos.z
                    && inVertex.normal.x == testVertex.normal.x && inVertex.normal.y == testVertex.normal.y && inVertex.normal.z == testVertex.normal.z
                    && inVertex.texCoord.x == testVertex.texCoord.x && inVertex.texCoord.y == testVertex.texCoord.y && inVertex.color.x == testVertex.color.x
                    && inVertex.color.y == testVertex.color.y && inVertex.color.z == testVertex.color.z && inVertex.color.w == testVertex.color.w)
                {
                    found = true;
                    foundIdx = j;
                    break;
                }
            }
            if (!found)
            {
                indexMap[i] = out_vertexBuffer.size();
                out_vertexBuffer.emplace_back(testVertex);
            }
            else
                indexMap[i] = foundIdx;
        }

        return indexMap;
    }

    void dumpGfxWorldSurfaces(BSPData& dumpData, const GfxWorld* gfxWorld)
    {
        std::map<size_t, size_t> vd0Offsets; // maps unique vd0 offsets to their maximum index
        for (int surfIdx = 0; surfIdx < gfxWorld->surfaceCount; surfIdx++)
        {
            GfxSurface* inSurface = &gfxWorld->dpvs.surfaces[surfIdx];
            size_t vd0Offset = inSurface->tris.vertexDataOffset0;

            uint16_t largestIndex = 0;
            uint16_t* surfTriIndicies = &gfxWorld->draw.indices[inSurface->tris.baseIndex];
            for (size_t vertIdx = 0; vertIdx < static_cast<size_t>(inSurface->tris.triCount) * 3; vertIdx++)
            {
                if (surfTriIndicies[vertIdx] > largestIndex)
                    largestIndex = surfTriIndicies[vertIdx];
            }

            if (vd0Offsets.contains(vd0Offset))
            {
                if (largestIndex > vd0Offsets.at(vd0Offset))
                    vd0Offsets.at(vd0Offset) = largestIndex;
            }
            else
                vd0Offsets[vd0Offset] = largestIndex;
        }

        size_t totalVertexCount = 0;
        std::map<size_t, std::tuple<size_t, size_t>> vd0OffsetToGltf; // maps unique vd0 offsets to their gltfvertex index equivalent, and size

        std::vector<BSPVertex> allVertices;
        for (const auto& pair : vd0Offsets)
        {
            size_t vd0Offset = pair.first;
            size_t vertexCount = pair.second + 1;

            for (size_t idx = 0; idx < vertexCount; idx++)
            {
                GfxPackedWorldVertex* inVertex = (GfxPackedWorldVertex*)&gfxWorld->draw.vd0.data[vd0Offset + sizeof(GfxPackedWorldVertex) * idx];
                BSPVertex outVertex{};
                outVertex.pos = inVertex->xyz;
                LhcToRhcCoordinates(outVertex.pos.v);
                Common::Vec3UnpackUnitVec(inVertex->normal, outVertex.normal.v);
                LhcToRhcCoordinates(outVertex.normal.v);
                Common::Vec2UnpackTexCoords(inVertex->texCoord, outVertex.texCoord.v);
                Common::Vec4UnpackGfxColor(inVertex->color, outVertex.color.v);
                allVertices.emplace_back(outVertex);
            }

            vd0OffsetToGltf[vd0Offset] = {totalVertexCount, vertexCount};
            totalVertexCount += vertexCount;
        }

        std::vector<std::pair<size_t, std::vector<size_t>>> materialSurfs;
        for (unsigned int surfIdx = 0; surfIdx < gfxWorld->dpvs.staticSurfaceCount; surfIdx++)
        {
            GfxSurface* inSurface = &gfxWorld->dpvs.surfaces[surfIdx];
            size_t materialIndex = createBspMaterial(dumpData, inSurface->material, (GfxSurfaceFlags)inSurface->flags);
            bool found = false;
            for (auto& matSurf : materialSurfs)
            {
                if (matSurf.first == materialIndex)
                {
                    matSurf.second.emplace_back(surfIdx);
                    found = true;
                    break;
                }
            }
            if (!found)
                materialSurfs.emplace_back(std::pair(materialIndex, std::vector<size_t>({static_cast<size_t>(surfIdx)})));
        }

        assert(dumpData.gfxWorld.surfaces.size() == 0);
        dumpData.staticSurfaceStart = dumpData.gfxWorld.surfaces.size();
        dumpData.staticSurfaceCount = 0;
        dumpData.staticSurfaceCount += materialSurfs.size();

        // script surfaces need to stay unmerged or it will mess with entities that use them
        for (int surfIdx = gfxWorld->dpvs.staticSurfaceCount; surfIdx < gfxWorld->surfaceCount; surfIdx++)
        {
            GfxSurface* inSurface = &gfxWorld->dpvs.surfaces[surfIdx];
            size_t materialIndex = createBspMaterial(dumpData, inSurface->material, (GfxSurfaceFlags)inSurface->flags);
            materialSurfs.emplace_back(std::pair(materialIndex, std::vector<size_t>({static_cast<size_t>(surfIdx)})));
        }

        for (const auto& matSurf : materialSurfs)
        {
            std::vector<BSPVertex> tempVertices;
            std::vector<size_t> tempIndices;
            size_t tempTriCount = 0;
            size_t tempVertCount = 0;
            for (const auto& surfIdx : matSurf.second)
            {
                GfxSurface* inSurface = &gfxWorld->dpvs.surfaces[surfIdx];

                assert(vd0OffsetToGltf.contains(inSurface->tris.vertexDataOffset0));
                auto data = vd0OffsetToGltf.at(inSurface->tris.vertexDataOffset0);

                size_t firstVertex = std::get<0>(data);
                size_t vertexCount = std::get<1>(data);

                tempTriCount += inSurface->tris.triCount;
                tempVertCount += vertexCount;

                uint16_t* surfTriIndicies = &gfxWorld->draw.indices[inSurface->tris.baseIndex];
                for (uint16_t triIdx = 0; triIdx < inSurface->tris.triCount; triIdx++)
                {
                    // equivalent to LhcToRhcIndices
                    tempIndices.emplace_back(tempVertices.size() + (size_t)surfTriIndicies[triIdx * 3 + 2]);
                    tempIndices.emplace_back(tempVertices.size() + (size_t)surfTriIndicies[triIdx * 3 + 1]);
                    tempIndices.emplace_back(tempVertices.size() + (size_t)surfTriIndicies[triIdx * 3]);
                }

                tempVertices.insert(tempVertices.end(), allVertices.begin() + firstVertex, allVertices.begin() + firstVertex + vertexCount);
            }

            std::unique_ptr<bool[]> isVertUsedMap = std::make_unique<bool[]>(tempVertCount);
            for (size_t idx : tempIndices)
                isVertUsedMap[idx] = true;

            std::vector<BSPVertex> outputVertexBuffer;
            auto indexMap = simplifyVertexBuffer(tempVertices, isVertUsedMap, outputVertexBuffer);
            assert(outputVertexBuffer.size() != 0);

            BSPSurface outSurface{};
            outSurface.materialIndex = matSurf.first;
            outSurface.triCount = tempTriCount;
            outSurface.vertexCount = outputVertexBuffer.size();
            outSurface.indexOfFirstVertex = dumpData.gfxWorld.vertices.size();
            outSurface.indexOfFirstIndex = dumpData.gfxWorld.indices.size();
            dumpData.gfxWorld.surfaces.emplace_back(outSurface);

            dumpData.gfxWorld.vertices.insert(dumpData.gfxWorld.vertices.end(), outputVertexBuffer.begin(), outputVertexBuffer.end());
            for (size_t idx : tempIndices)
            {
                assert(indexMap[idx] <= UINT16_MAX);
                dumpData.gfxWorld.indices.emplace_back(static_cast<uint16_t>(indexMap[idx]));
            }
        }
    }

    void dumpGfxWorldXModels(BSPData& dumpData, const GfxWorld* gfxWorld)
    {
        for (size_t modelIdx = 0; modelIdx < gfxWorld->dpvs.smodelCount; modelIdx++)
        {
            auto gfxModel = &gfxWorld->dpvs.smodelDrawInsts[modelIdx];
            BSPXModel model{};

            assert(gfxModel->model != nullptr);
            model.name = gfxModel->model->name[0] == ',' ? &gfxModel->model->name[1] : gfxModel->model->name;
            model.origin = gfxModel->placement.origin;
            LhcToRhcCoordinates(model.origin.v);
            model.rotationQuaternion = BSPUtil::convertAxisToQuat(gfxModel->placement.axis);
            LhcToRhcQuaternion(model.rotationQuaternion.v);
            model.scale.x = gfxModel->placement.scale;
            model.scale.y = gfxModel->placement.scale;
            model.scale.z = gfxModel->placement.scale;
            model.doesCastShadow = gfxWorld->dpvs.smodelCastsShadow[modelIdx];
            dumpData.gfxWorld.xmodels.emplace_back(model);
        }
    }

    void dumpGfxWorld(BSPData& dumpData, const GfxWorld* gfxWorld)
    {
        dumpGfxWorldSurfaces(dumpData, gfxWorld);
        dumpGfxWorldXModels(dumpData, gfxWorld);
    }

    struct BSPAssetPtrs
    {
        const MapEnts* mapEnts;
        const GameWorldMp* gameWorldMp;
        const ComWorld* comworld;
        const GfxWorld* gfxworld;
        const clipMap_t* clipmap;
        const SkinnedVertsDef* skinnedverts;
    };

    void dumpBSPData(BSPData& dumpData, std::string zoneName, BSPAssetPtrs& assetPtrs)
    {
        dumpData.name = zoneName;

        dumpGfxWorld(dumpData, assetPtrs.gfxworld);
        dumpClipmap(dumpData, assetPtrs.clipmap);
        dumpMapEnts(dumpData, assetPtrs.mapEnts, assetPtrs.gfxworld, assetPtrs.clipmap); // requires colworld materials
    }

    void writeGltf(JsonRoot& root, std::vector<uint8_t>& bufferData, std::ostream* stream)
    {
        const auto output = std::make_unique<gltf::BinOutput>(*stream);

        root.buffers.emplace();
        JsonBuffer jsonBuffer;
        jsonBuffer.byteLength = static_cast<unsigned>(bufferData.size());
        if (!bufferData.empty())
            jsonBuffer.uri = output->CreateBufferUri(bufferData.data(), bufferData.size());
        root.buffers->emplace_back(std::move(jsonBuffer));

        output->EmitJson(root);
        if (!bufferData.empty())
            output->EmitBuffer(bufferData.data(), bufferData.size());
        output->Finalize();
    }

    void CreateBufferViews(JsonRoot& gltf, BSPData& dumpData, std::vector<uint8_t>& bufferData, bool isGfxWorld)
    {
        gltf.bufferViews.emplace();

        if (isGfxWorld)
        {
            unsigned bufferOffset = 0u;
            m_vertex_buffer_view = static_cast<unsigned>(gltf.bufferViews->size());
            JsonBufferView vertexBufferView;
            vertexBufferView.buffer = 0u;
            vertexBufferView.byteOffset = bufferOffset;
            vertexBufferView.byteStride = static_cast<unsigned>(sizeof(BSPVertex));
            vertexBufferView.byteLength = static_cast<unsigned>(sizeof(BSPVertex) * dumpData.gfxWorld.vertices.size());
            vertexBufferView.target = JsonBufferViewTarget::ARRAY_BUFFER;
            bufferOffset += vertexBufferView.byteLength;
            gltf.bufferViews->emplace_back(vertexBufferView);

            m_index_buffer_view = static_cast<unsigned>(gltf.bufferViews->size());
            JsonBufferView indicesBufferView;
            indicesBufferView.buffer = 0u;
            indicesBufferView.byteOffset = bufferOffset;
            indicesBufferView.byteLength = static_cast<unsigned>(sizeof(uint16_t) * dumpData.gfxWorld.indices.size());
            indicesBufferView.target = JsonBufferViewTarget::ELEMENT_ARRAY_BUFFER;
            bufferOffset += indicesBufferView.byteLength;
            gltf.bufferViews->emplace_back(indicesBufferView);

            size_t vertexBufferSize = dumpData.gfxWorld.vertices.size() * sizeof(BSPVertex);
            size_t indexBufferSize = dumpData.gfxWorld.indices.size() * sizeof(uint16_t);
            bufferData.resize(vertexBufferSize + indexBufferSize);

            size_t currentBufferOffset = 0;
            if (vertexBufferSize != 0)
            {
                memcpy(&bufferData.at(currentBufferOffset), dumpData.gfxWorld.vertices.data(), vertexBufferSize);
                currentBufferOffset += vertexBufferSize;
            }
            if (indexBufferSize != 0)
            {
                memcpy(&bufferData.at(currentBufferOffset), dumpData.gfxWorld.indices.data(), indexBufferSize);
                currentBufferOffset += indexBufferSize;
            }
        }
        else
        {
            unsigned bufferOffset = 0u;
            m_vertex_buffer_view = static_cast<unsigned>(gltf.bufferViews->size());
            JsonBufferView vertexBufferView;
            vertexBufferView.buffer = 0u;
            vertexBufferView.byteOffset = bufferOffset;
            vertexBufferView.byteStride = static_cast<unsigned>(sizeof(BSPVertex));
            vertexBufferView.byteLength = static_cast<unsigned>(sizeof(BSPVertex) * dumpData.colWorld.vertices.size());
            vertexBufferView.target = JsonBufferViewTarget::ARRAY_BUFFER;
            bufferOffset += vertexBufferView.byteLength;
            gltf.bufferViews->emplace_back(vertexBufferView);

            m_index_buffer_view = static_cast<unsigned>(gltf.bufferViews->size());
            JsonBufferView indicesBufferView;
            indicesBufferView.buffer = 0u;
            indicesBufferView.byteOffset = bufferOffset;
            indicesBufferView.byteLength = static_cast<unsigned>(sizeof(unsigned short) * dumpData.colWorld.indices.size());
            indicesBufferView.target = JsonBufferViewTarget::ELEMENT_ARRAY_BUFFER;
            bufferOffset += indicesBufferView.byteLength;
            gltf.bufferViews->emplace_back(indicesBufferView);

            size_t vertexBufferSize = dumpData.colWorld.vertices.size() * sizeof(BSPVertex);
            size_t indexBufferSize = dumpData.colWorld.indices.size() * sizeof(uint16_t);
            bufferData.resize(vertexBufferSize + indexBufferSize);

            size_t currentBufferOffset = 0;
            if (vertexBufferSize != 0)
            {
                memcpy(&bufferData.at(currentBufferOffset), dumpData.colWorld.vertices.data(), vertexBufferSize);
                currentBufferOffset += vertexBufferSize;
            }
            if (indexBufferSize != 0)
            {
                memcpy(&bufferData.at(currentBufferOffset), dumpData.colWorld.indices.data(), indexBufferSize);
                currentBufferOffset += indexBufferSize;
            }
        }
    }

    constexpr size_t ROOT_NODE_IDX = 0;

    size_t addNodeToGltf(JsonRoot& root, JsonNode& node, std::optional<size_t> parentIdx)
    {
        size_t nodeIdx = root.nodes->size();
        if (parentIdx)
            root.nodes->at(*parentIdx).children->emplace_back((unsigned)nodeIdx);
        root.nodes->emplace_back(node);
        return nodeIdx;
    }

    JsonMeshPrimitives createPrimitiveFromSurfaces(JsonRoot& gltf, BSPData& dumpData, BSPSurface& surface, bool isGfxWorld)
    {
        if (!gltf.accessors)
            gltf.accessors.emplace();

        JsonMeshPrimitives primitive;
        primitive.material = (unsigned)surface.materialIndex;
        primitive.mode = JsonMeshPrimitivesMode::TRIANGLES;

        JsonAccessor positionAccessor;
        positionAccessor.bufferView = m_vertex_buffer_view;
        positionAccessor.byteOffset = (unsigned)surface.indexOfFirstVertex * (unsigned)sizeof(BSPVertex) + static_cast<unsigned>(offsetof(BSPVertex, pos));
        positionAccessor.componentType = JsonAccessorComponentType::FLOAT;
        positionAccessor.count = (unsigned int)surface.vertexCount;
        positionAccessor.type = JsonAccessorType::VEC3;
        primitive.attributes.POSITION = (unsigned)gltf.accessors->size();
        gltf.accessors->emplace_back(positionAccessor);

        JsonAccessor normalAccessor;
        normalAccessor.bufferView = m_vertex_buffer_view;
        normalAccessor.byteOffset = (unsigned)surface.indexOfFirstVertex * (unsigned)sizeof(BSPVertex) + static_cast<unsigned>(offsetof(BSPVertex, normal));
        normalAccessor.componentType = JsonAccessorComponentType::FLOAT;
        normalAccessor.count = (unsigned int)surface.vertexCount;
        normalAccessor.type = JsonAccessorType::VEC3;
        primitive.attributes.NORMAL = (unsigned)gltf.accessors->size();
        gltf.accessors->emplace_back(normalAccessor);

        JsonAccessor uvAccessor;
        uvAccessor.bufferView = m_vertex_buffer_view;
        uvAccessor.byteOffset = (unsigned)surface.indexOfFirstVertex * (unsigned)sizeof(BSPVertex) + static_cast<unsigned>(offsetof(BSPVertex, texCoord));
        uvAccessor.componentType = JsonAccessorComponentType::FLOAT;
        uvAccessor.count = (unsigned int)surface.vertexCount;
        uvAccessor.type = JsonAccessorType::VEC2;
        primitive.attributes.TEXCOORD_0 = (unsigned)gltf.accessors->size();
        gltf.accessors->emplace_back(uvAccessor);

        JsonAccessor colorAccessor;
        colorAccessor.bufferView = m_vertex_buffer_view;
        colorAccessor.byteOffset = (unsigned)surface.indexOfFirstVertex * (unsigned)sizeof(BSPVertex) + static_cast<unsigned>(offsetof(BSPVertex, color));
        colorAccessor.componentType = JsonAccessorComponentType::FLOAT;
        colorAccessor.count = (unsigned int)surface.vertexCount;
        colorAccessor.type = JsonAccessorType::VEC4;
        primitive.attributes.COLOR_0 = (unsigned)gltf.accessors->size();
        gltf.accessors->emplace_back(colorAccessor);

        JsonAccessor indicesAccessor;
        indicesAccessor.bufferView = m_index_buffer_view;
        indicesAccessor.byteOffset = (unsigned)surface.indexOfFirstIndex * (unsigned)sizeof(uint16_t);
        indicesAccessor.componentType = JsonAccessorComponentType::UNSIGNED_SHORT;
        indicesAccessor.count = (unsigned int)surface.triCount * 3;
        indicesAccessor.type = JsonAccessorType::SCALAR;
        primitive.indices = (unsigned)gltf.accessors->size();
        gltf.accessors->emplace_back(indicesAccessor);

        return primitive;
    }

    size_t addMeshFromSurface(JsonRoot& root, BSPData& dumpData, size_t startSurf, size_t count, bool isGfxWorld)
    {
        JsonMesh mesh;
        for (size_t surfIdx = startSurf; surfIdx < startSurf + count; surfIdx++)
        {
            BSPSurface surface;
            if (isGfxWorld)
                surface = dumpData.gfxWorld.surfaces[surfIdx];
            else
                surface = dumpData.colWorld.surfaces[surfIdx];

            mesh.primitives.emplace_back(createPrimitiveFromSurfaces(root, dumpData, surface, isGfxWorld));
        }
        size_t meshIdx = root.meshes->size();
        root.meshes->emplace_back(mesh);
        return meshIdx;
    }

    size_t totalBrushes = 0;

    JsonNode createNodeFromParent(
        JsonRoot& root, size_t parentNodeIdx, std::optional<vec3_t> translation, std::optional<vec4_t> rotation, std::optional<vec3_t> scale)
    {
        JsonNode& rootNode = root.nodes->at(parentNodeIdx);
        JsonNode outNode{};

        if (rootNode.translation && translation)
        {
            float x = std::get<0>(*rootNode.translation);
            float y = std::get<1>(*rootNode.translation);
            float z = std::get<2>(*rootNode.translation);
            outNode.translation = {
                {(*translation).x - x, (*translation).y - y, (*translation).z - z}
            };
        }
        else if (!rootNode.translation && translation)
        {
            outNode.translation = {
                {(*translation).x, (*translation).y, (*translation).z}
            };
        }
        else if (rootNode.translation && !translation)
        {
            outNode.translation = {
                {0.0f, 0.0f, 0.0f}
            };
        }

        if (rootNode.rotation && rotation)
        {
            float x = std::get<0>(*rootNode.rotation);
            float y = std::get<1>(*rootNode.rotation);
            float z = std::get<2>(*rootNode.rotation);
            float w = std::get<3>(*rootNode.rotation);

            // GLTF is XYZW, Eigen is WXYZ
            Eigen::Quaternionf rootRotation(w, x, y, z);
            Eigen::Quaternionf nodeRotation((*rotation).w, (*rotation).x, (*rotation).y, (*rotation).z);
            Eigen::Quaternionf difference = rootRotation.inverse() * nodeRotation;
            outNode.rotation = {
                {difference.x(), difference.y(), difference.z(), difference.w()}
            };
        }

        else if (!rootNode.rotation && rotation)
        {
            outNode.rotation = {
                {(*rotation).x, (*rotation).y, (*rotation).z, (*rotation).w}
            };
        }
        else if (rootNode.rotation && !rotation)
        {
            outNode.rotation = {
                {0.0f, 0.0f, 0.0f, 1.0f}
            };
        }

        if (rootNode.scale && scale)
        {
            float x = std::get<0>(*rootNode.scale);
            float y = std::get<1>(*rootNode.scale);
            float z = std::get<2>(*rootNode.scale);
            outNode.scale = {
                {(*scale).x - x, (*scale).y - y, (*scale).z - z}
            };
        }
        else if (!rootNode.scale && scale)
        {
            outNode.scale = {
                {(*scale).x, (*scale).y, (*scale).z}
            };
        }
        else if (rootNode.scale && !scale)
        {
            outNode.scale = {
                {1.0f, 1.0f, 1.0f}
            };
        }

        return outNode;
    }

    void addNodesFromBrushSurfaces(JsonRoot& root, BSPData& dumpData, size_t startSurf, size_t count, size_t rootNodeIdx, bool isGfxWorld)
    {
        for (size_t i = 0; i < count; i++)
        {
            vec3_t origin{};
            if (isGfxWorld)
                origin = dumpData.gfxWorld.surfaces.at(startSurf + i).origin;
            else
                origin = dumpData.colWorld.surfaces.at(startSurf + i).origin;

            JsonNode node{};
            node.translation = {
                {(origin).x, (origin).y, (origin).z}
            };
            node.name = std::format("brush_{}", totalBrushes++);
            node.mesh = (unsigned)addMeshFromSurface(root, dumpData, startSurf + i, 1, isGfxWorld);
            nlohmann::json js;
            js["model"] = "brush";
            node.extras = js;
            addNodeToGltf(root, node, rootNodeIdx);
        }
    }

    size_t totalTerrain = 0;

    void addNodesFromTerrainSurfaces(JsonRoot& root, BSPData& dumpData, size_t startSurf, size_t count, size_t parentNodeIdx, bool isGfxWorld)
    {
        for (size_t i = 0; i < count; i++)
        {
            vec3_t origin{};
            if (isGfxWorld)
                origin = dumpData.gfxWorld.surfaces.at(startSurf + i).origin;
            else
                origin = dumpData.colWorld.surfaces.at(startSurf + i).origin;

            JsonNode node{};
            node.translation = {
                {(origin).x, (origin).y, (origin).z}
            };
            node.name = std::format("terrain_{}", totalTerrain++);
            node.mesh = (unsigned)addMeshFromSurface(root, dumpData, startSurf + i, 1, isGfxWorld);
            nlohmann::json js;
            js["model"] = "terrain";
            node.extras = js;
            addNodeToGltf(root, node, parentNodeIdx);
        }
    }

    void createMapEnts(JsonRoot& root, BSPData& dumpData, bool isGfxWorld)
    {
        JsonNode entNode;
        entNode.name = "Entities";
        entNode.children.emplace();
        size_t entNodeIdx = addNodeToGltf(root, entNode, ROOT_NODE_IDX);
        if (isGfxWorld)
        {
            JsonNode node;
            node.name = "Brushmodels";
            node.children.emplace();
            addNodeToGltf(root, node, entNodeIdx);
        }
        else
        {
            for (size_t i = 0; i < ET_COUNT; i++)
            {
                JsonNode node;
                node.name = bspEntityTypeNames[i];
                node.children.emplace();
                addNodeToGltf(root, node, entNodeIdx);
            }
        }

        int entIdx = 0;
        for (BSPEntity& entity : dumpData.entities)
        {
            BSPModel* model;
            if (entity.hasModel)
                model = &dumpData.models.at(entity.modelIndex);
            else
            {
                if (isGfxWorld)
                    continue;
                model = nullptr;
            }

            if (model != nullptr)
            {
                if (isGfxWorld && model->surfaceSide == MSS_COL)
                    continue;
                if (!isGfxWorld && model->surfaceSide == MSS_GFX)
                    continue;
            }

            JsonNode node;
            node.name = std::format("entity_{}_{}", entity.classname, entIdx++);
            node.children.emplace();
            node.translation.emplace();
            node.rotation.emplace();
            (*node.translation)[0] = entity.origin.x;
            (*node.translation)[1] = entity.origin.y;
            (*node.translation)[2] = entity.origin.z;
            (*node.rotation)[0] = entity.rotationQuaternion.x;
            (*node.rotation)[1] = entity.rotationQuaternion.y;
            (*node.rotation)[2] = entity.rotationQuaternion.z;
            (*node.rotation)[3] = entity.rotationQuaternion.w;
            nlohmann::json js;
            for (const auto& entityEntry : entity.entries)
                js[entityEntry.key] = entityEntry.value;
            if (model != nullptr)
            {
                js["model"] = "*"; // special character to say that the ent uses it's children as a list of models
                if (model->surfaceSide == MSS_BOTH)
                    js["GfxAndColLinkNumber"] = entity.uniqueEntityNumber;
            }
            node.extras = js;

            size_t nodeIdx;
            if (isGfxWorld)
                nodeIdx = addNodeToGltf(root, node, entNodeIdx + 1);
            else
                nodeIdx = addNodeToGltf(root, node, (entNodeIdx + 1) + entity.type);

            if (model != nullptr && model->surfaceSide != MSS_NONE)
            {
                if (model->surfaceSide == MSS_BOTH)
                {
                    if (isGfxWorld)
                        addNodesFromTerrainSurfaces(root, dumpData, model->gfxSurfaceIndex, model->gfxSurfaceCount, nodeIdx, isGfxWorld);
                    else
                    {
                        if (model->surfaceType == MST_TERRAIN)
                            addNodesFromTerrainSurfaces(root, dumpData, model->colTerrainSurfaceIndex, model->colTerrainSurfaceCount, nodeIdx, isGfxWorld);
                        else if (model->surfaceType == MST_BRUSH)
                            addNodesFromBrushSurfaces(root, dumpData, model->colBrushSurfaceIndex, model->colBrushSurfaceCount, nodeIdx, isGfxWorld);
                        else if (model->surfaceType == MST_BOTH)
                        {
                            addNodesFromTerrainSurfaces(root, dumpData, model->colTerrainSurfaceIndex, model->colTerrainSurfaceCount, nodeIdx, isGfxWorld);
                            addNodesFromBrushSurfaces(root, dumpData, model->colBrushSurfaceIndex, model->colBrushSurfaceCount, nodeIdx, isGfxWorld);
                        }
                    }
                }
                else if (model->surfaceSide == MSS_GFX)
                    addNodesFromTerrainSurfaces(root, dumpData, model->gfxSurfaceIndex, model->gfxSurfaceCount, nodeIdx, isGfxWorld);
                else if (model->surfaceSide == MSS_COL)
                {
                    if (model->surfaceType == MST_TERRAIN)
                        addNodesFromTerrainSurfaces(root, dumpData, model->colTerrainSurfaceIndex, model->colTerrainSurfaceCount, nodeIdx, isGfxWorld);
                    else if (model->surfaceType == MST_BRUSH)
                        addNodesFromBrushSurfaces(root, dumpData, model->colBrushSurfaceIndex, model->colBrushSurfaceCount, nodeIdx, isGfxWorld);
                    else if (model->surfaceType == MST_BOTH)
                    {
                        addNodesFromTerrainSurfaces(root, dumpData, model->colTerrainSurfaceIndex, model->colTerrainSurfaceCount, nodeIdx, isGfxWorld);
                        addNodesFromBrushSurfaces(root, dumpData, model->colBrushSurfaceIndex, model->colBrushSurfaceCount, nodeIdx, isGfxWorld);
                    }
                }
            }
        }
    }

    void addXmodelToJson(JsonRoot& root, BSPData& dumpData, const BSPXModel& xmodel, size_t parentNodeIdx, bool isGfxWorld)
    {
        JsonNode node;
        node.name = xmodel.name;
        node.translation.emplace();
        (*node.translation)[0] = xmodel.origin.x;
        (*node.translation)[1] = xmodel.origin.y;
        (*node.translation)[2] = xmodel.origin.z;
        node.rotation.emplace();
        (*node.rotation)[0] = xmodel.rotationQuaternion.x;
        (*node.rotation)[1] = xmodel.rotationQuaternion.y;
        (*node.rotation)[2] = xmodel.rotationQuaternion.z;
        (*node.rotation)[3] = xmodel.rotationQuaternion.w;
        node.scale.emplace();
        (*node.scale)[0] = xmodel.scale.x;
        (*node.scale)[1] = xmodel.scale.y;
        (*node.scale)[2] = xmodel.scale.z;

        nlohmann::json extrasJs;
        extrasJs["xmodel"] = xmodel.name;
        if (xmodel.doesCastShadow && isGfxWorld)
            extrasJs["flags"] = "nocastshadow";
        node.extras = extrasJs;

        addNodeToGltf(root, node, parentNodeIdx);
    }

    void createGfxWorld(JsonRoot& root, BSPData& dumpData, bool isGfxWorld)
    {
        if (!isGfxWorld)
            return;

        JsonNode node;
        node.name = "Surfaces";
        node.mesh = (unsigned)addMeshFromSurface(root, dumpData, dumpData.staticSurfaceStart, dumpData.staticSurfaceCount, true);
        addNodeToGltf(root, node, ROOT_NODE_IDX);

        JsonNode xnode;
        xnode.name = "XModels";
        xnode.children.emplace();
        size_t xmodelNodeIdx = addNodeToGltf(root, xnode, ROOT_NODE_IDX);

        for (const auto& xmodel : dumpData.gfxWorld.xmodels)
            addXmodelToJson(root, dumpData, xmodel, xmodelNodeIdx, true);
    }

    void createColWorld(JsonRoot& root, BSPData& dumpData, bool isGfxWorld)
    {
        if (isGfxWorld)
            return;

        JsonNode tNode;
        tNode.name = "Terrain";
        tNode.mesh = (unsigned)addMeshFromSurface(root, dumpData, dumpData.staticTerrainSurfaceStart, dumpData.staticTerrainSurfaceCount, false);
        size_t terrainNodeIdx = addNodeToGltf(root, tNode, ROOT_NODE_IDX);

        JsonNode xnode;
        xnode.name = "XModels";
        xnode.children.emplace();
        size_t xmodelNodeIdx = addNodeToGltf(root, xnode, ROOT_NODE_IDX);
        for (const auto& xmodel : dumpData.colWorld.xmodels)
            addXmodelToJson(root, dumpData, xmodel, xmodelNodeIdx, false);

        JsonNode bNode;
        bNode.name = "Brushes";
        bNode.children.emplace();
        size_t brushNodeIdx = addNodeToGltf(root, bNode, ROOT_NODE_IDX);

        bNode.name = "solid";
        size_t solidBrushNodeIdx = addNodeToGltf(root, bNode, brushNodeIdx);
        bNode.name = "nonsolid";
        size_t nonSolidBrushNodeIdx = addNodeToGltf(root, bNode, brushNodeIdx);

        std::map<std::string, std::pair<bool, std::vector<size_t>>> uniqueMaterials;
        for (size_t brushIdx = dumpData.staticBrushSurfaceStart; brushIdx < dumpData.staticBrushSurfaceStart + dumpData.staticBrushSurfaceCount; brushIdx++)
        {
            BSPMaterial& material = dumpData.colWorld.materials.at(dumpData.colWorld.surfaces.at(brushIdx).materialIndex);
            if (uniqueMaterials.contains(material.materialName))
            {
                uniqueMaterials.at(material.materialName).second.emplace_back(brushIdx);
            }
            else
            {
                uniqueMaterials[material.materialName] = {true, std::vector<size_t>({brushIdx})};
            }
        }

        for (const auto& material : uniqueMaterials)
        {
            JsonNode mNode;
            mNode.name = material.first;
            mNode.children.emplace();
            size_t parentIdx = material.second.first ? solidBrushNodeIdx : nonSolidBrushNodeIdx;
            size_t matNodeIdx = addNodeToGltf(root, mNode, parentIdx);
            for (const auto& brushIdx : material.second.second)
                addNodesFromBrushSurfaces(root, dumpData, brushIdx, 1, matNodeIdx, false);
        }
    }

    void CreateMaterials(JsonRoot& root, BSPData& dumpData, bool isGfxWorld)
    {
        root.materials.emplace();

        std::vector<BSPMaterial>* matVec;
        if (isGfxWorld)
            matVec = &dumpData.gfxWorld.materials;
        else
            matVec = &dumpData.colWorld.materials;
        for (BSPMaterial& mat : *matVec)
        {
            JsonMaterial material;
            material.name = mat.materialName;
            material.pbrMetallicRoughness.emplace();
            material.pbrMetallicRoughness->baseColorFactor = {mat.materialColour.x, mat.materialColour.y, mat.materialColour.z, mat.materialColour.w};
            nlohmann::json extrasJs;
            extrasJs["sf"] = mat.surfaceFlags;
            extrasJs["cf"] = mat.contentFlags;
            extrasJs["name"] = mat.materialName; // duplicate name incase editor changes the mat name
            material.extras = extrasJs;
            root.materials->emplace_back(material);
        }
    }

    void createJsonHeader(JsonRoot& root, std::string& bspName, bool isGfxWorld)
    {
        root.asset.version = "2.0";
        root.asset.generator = "T6-BSP-Decompiler-v0.1";

        JsonScene scene;
        if (isGfxWorld)
            scene.name = bspName + "_graphics";
        else
            scene.name = bspName + "_collision";
        scene.nodes.emplace_back(0);
        root.scenes.emplace();
        root.scenes->emplace_back(scene);
        root.scene = 0;

        root.nodes.emplace();
        root.meshes.emplace();

        JsonNode rootNode;
        rootNode.name = bspName;
        rootNode.children.emplace();
        addNodeToGltf(root, rootNode, std::nullopt);
    }

    void createJson(JsonRoot& root, BSPData& dumpData, std::vector<uint8_t>& bufferData, bool isGfxWorld)
    {
        createJsonHeader(root, dumpData.name, isGfxWorld);
        CreateBufferViews(root, dumpData, bufferData, isGfxWorld);

        CreateMaterials(root, dumpData, isGfxWorld);

        createGfxWorld(root, dumpData, isGfxWorld);
        createColWorld(root, dumpData, isGfxWorld);
        createMapEnts(root, dumpData, isGfxWorld);
    }
} // namespace

[[nodiscard]] std::optional<asset_type_t> DumperT6::GetHandlingAssetType() const
{
    return std::nullopt;
}

[[nodiscard]] size_t DumperT6::GetProgressTotalCount(AssetDumpingContext& context) const
{
    return 0;
}

void DumperT6::Dump(AssetDumpingContext& context)
{
    const auto& gfxWorldPool = context.m_zone.m_pools.PoolAssets<T6::AssetGfxWorld>();
    const auto& colWorldPool = context.m_zone.m_pools.PoolAssets<T6::AssetClipMapPvs>();
    const auto& mapEntsPool = context.m_zone.m_pools.PoolAssets<T6::AssetMapEnts>();
    const auto& comWorldPool = context.m_zone.m_pools.PoolAssets<T6::AssetComWorld>();
    const auto& gameWorldMpPool = context.m_zone.m_pools.PoolAssets<T6::AssetGameWorldMp>();
    const auto& skinnedvertsPool = context.m_zone.m_pools.PoolAssets<T6::AssetSkinnedVerts>();
    if (gfxWorldPool.size() != 1 || colWorldPool.size() != 1 || mapEntsPool.size() != 1 || comWorldPool.size() != 1 || gameWorldMpPool.size() != 1
        || skinnedvertsPool.size() != 1)
    {
        con::error("0 or multiple BSPs found, skipping BSP decompilation.");
        return;
    }
    const auto* mapEntsInfo = *mapEntsPool.begin();
    const auto* colWorldInfo = *colWorldPool.begin();
    const auto* comWorldInfo = *comWorldPool.begin();
    const auto* gfxWorldInfo = *gfxWorldPool.begin();
    const auto* gameWorldMpInfo = *gameWorldMpPool.begin();
    const auto* skinnedvertsInfo = *skinnedvertsPool.begin();

    BSPData dumpData;
    BSPAssetPtrs assetPtrs;
    assetPtrs.mapEnts = mapEntsInfo->Asset();
    assetPtrs.clipmap = colWorldInfo->Asset();
    assetPtrs.comworld = comWorldInfo->Asset();
    assetPtrs.gfxworld = gfxWorldInfo->Asset();
    assetPtrs.gameWorldMp = gameWorldMpInfo->Asset();
    assetPtrs.skinnedverts = skinnedvertsInfo->Asset();
    dumpBSPData(dumpData, context.m_zone.m_name, assetPtrs);

    { // gfx
        JsonRoot root;
        std::vector<uint8_t> bufferData;
        createJson(root, dumpData, bufferData, true);

        const auto assetFile = context.OpenAssetFile("bsp/map_gfx.glb");
        if (!assetFile)
        {
            con::error("Unable to open bsp output file.");
            return;
        }
        writeGltf(root, bufferData, assetFile.get());
    }
    { // collision
        JsonRoot root;
        std::vector<uint8_t> bufferData;
        createJson(root, dumpData, bufferData, false);

        const auto assetFile = context.OpenAssetFile("bsp/map_col.glb");
        if (!assetFile)
        {
            con::error("Unable to open bsp output file.");
            return;
        }
        writeGltf(root, bufferData, assetFile.get());
    }
    context.IncrementProgress();
}
