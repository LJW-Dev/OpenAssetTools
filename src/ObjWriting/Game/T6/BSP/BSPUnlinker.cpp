#include "BSPUnlinker.h"

#include "BSP/BSPUtil.h"
#include "Game/T6/CommonT6.h"

#include <QuickHull.hpp>
#include <queue>
#include <unordered_set>

using namespace T6;
using namespace BSP;
using namespace quickhull;

namespace
{
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

    void createSurfacesFromBrushes(const ClipInfo* clipInfo, BSPData& dumpData, std::vector<size_t>& brushList, bool useWorldCoordinates, OutSurface& result)
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

            cbrush_t* brush = &clipInfo->brushes[brushIdx];

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
            for (unsigned i = 0; i < clipInfo->numMaterials; i++)
            {
                // this is how brushes with no data selects the flags during a trace
                if (clipInfo->materials[i].contentFlags == brush->axial_cflags[1][2] && clipInfo->materials[i].surfaceFlags == brush->axial_sflags[1][2])
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

    void getBrushesFromLeafBrushNode(const ClipInfo* clipInfo, size_t leafBrushNodeIdx, std::vector<size_t>& brushList)
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
            auto leafBrushNode = &clipInfo->leafbrushNodes[lbnIdx];

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
                    getBrushesFromLeafBrushNode(&clipmap->info, leaf->leafBrushNode, brushList);
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

    bool floatIsClose(float one, float two)
    {
        return fabs(one - two) < 1.0f;
    }

    void dumpClipmapXModels(BSPData& dumpData, const clipMap_t* clipmap)
    {
        // int ii = 0;
        for (size_t modelIdx = 0; modelIdx < clipmap->numStaticModels; modelIdx++)
        {
            cStaticModel_s* colModel = &clipmap->staticModelList[modelIdx];

            vec3_t outOrig[3]{};
            BSPUtil::matrixTranspose3x3(colModel->invScaledAxis, outOrig);

            /*
            This method of finding the original axis seems to have subtle errors, see below
            */
            // axis vectors should be normalised (length 1), and when scaling is applied it's length will increase with scale
            float length1 = lengthOfVector(outOrig[0].x, outOrig[0].y, outOrig[0].z);
            float length2 = lengthOfVector(outOrig[1].x, outOrig[1].y, outOrig[1].z);
            float length3 = lengthOfVector(outOrig[2].x, outOrig[2].y, outOrig[2].z);
            float forcedScale1 = 1.0f / length1;
            float forcedScale2 = 1.0f / length2;
            float forcedScale3 = 1.0f / length3;

            vec3_t origAxis[3]{};
            origAxis[0].x = outOrig[0].x * forcedScale1;
            origAxis[0].y = outOrig[0].y * forcedScale1;
            origAxis[0].z = outOrig[0].z * forcedScale1;
            origAxis[1].x = outOrig[1].x * forcedScale2;
            origAxis[1].y = outOrig[1].y * forcedScale2;
            origAxis[1].z = outOrig[1].z * forcedScale2;
            origAxis[2].x = outOrig[2].x * forcedScale3;
            origAxis[2].y = outOrig[2].y * forcedScale3;
            origAxis[2].z = outOrig[2].z * forcedScale3;

            BSPXModel model{};
            assert(colModel->xmodel != nullptr);
            model.name = colModel->xmodel->name[0] == ',' ? &colModel->xmodel->name[1] : colModel->xmodel->name;
            model.origin = colModel->origin;
            LhcToRhcCoordinates(model.origin.v);
            model.rotationQuaternion = BSPUtil::convertAxisToQuat(origAxis);
            LhcToRhcQuaternion(model.rotationQuaternion.v);
            model.scale = {forcedScale1, forcedScale2, forcedScale3};
            dumpData.colWorld.xmodels.emplace_back(model);

            /*
            vec3_t cminsOrig;
            vec3_t cmaxsOrig;
            vec3_t cmins;
            vec3_t cmaxs;

            // best so far: bad: 1538 total: 2082 - transpose before finding scaling then scale axis with scaling value
            // even with scale found to be 1, results aren't correct
            // scale: 1 1 1
            // origin: -1357.4 1633.4 408.5
            // orig: -1364.2158 1621.7128 408.64072, -1345.5366 1649.0393 439.38885
            // new: -1372.5813 1616.6323 407.65405, -1343.4738 1653.3081 440.66187
            // local: -15.181262 -16.767706 -0.8459563, 13.926324 19.908106 32.16186


            * gfx:
            scale: 1 1 1 origin: -1357.4 1633.4 408.5
            orig: -1364.2158 1621.7128 408.64072, -1345.5366 1649.0393 439.38885
            new: -1365.4347 1620.3545 407.24295, -1344.5284 1648.6028 438.61993
            local: -8.034626 -13.045589 -1.2570589, 12.871585 15.20278 30.119923

            BSPUtil::calculateXmodelColBounds(colModel->xmodel, origAxis, cminsOrig, cmaxsOrig);
            cmins.x = cminsOrig.x + colModel->origin.x;
            cmins.y = cminsOrig.y + colModel->origin.y;
            cmins.z = cminsOrig.z + colModel->origin.z;
            cmaxs.x = cmaxsOrig.x + colModel->origin.x;
            cmaxs.y = cmaxsOrig.y + colModel->origin.y;
            cmaxs.z = cmaxsOrig.z + colModel->origin.z;

            if (!floatIsClose(colModel->absmin.x, cmins.x) || !floatIsClose(colModel->absmin.y, cmins.y) || !floatIsClose(colModel->absmin.z, cmins.z)
                || !floatIsClose(colModel->absmax.x, cmaxs.x) || !floatIsClose(colModel->absmax.y, cmaxs.y) || !floatIsClose(colModel->absmax.z, cmaxs.z))
            {
                ii++;
                con::info("scale: {} {} {} origin: {} {} {}\torig: {} {} {}, {} {} {}\tnew: {} {} {}, {} {} {}\tlocal: {} {} {}, {} {} {}",
                          forcedScale1,
                          forcedScale2,
                          forcedScale3,
                          colModel->origin.x,
                          colModel->origin.y,
                          colModel->origin.z,
                          colModel->absmin.x,
                          colModel->absmin.y,
                          colModel->absmin.z,
                          colModel->absmax.x,
                          colModel->absmax.y,
                          colModel->absmax.z,
                          cmins.x,
                          cmins.y,
                          cmins.z,
                          cmaxs.x,
                          cmaxs.y,
                          cmaxs.z,
                          cminsOrig.x,
                          cminsOrig.y,
                          cminsOrig.z,
                          cmaxsOrig.x,
                          cmaxsOrig.y,
                          cmaxsOrig.z);
            }
            */
        }
        // con::info("bad: {} total: {}", ii, clipmap->numStaticModels);
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

        createSurfacesFromBrushes(&clipmap->info, dumpData, brushList, false, result); // use local coords as each brush has it's own node
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
            getBrushesFromLeafBrushNode(&clipmap->info, static_cast<size_t>(leaf->leafBrushNode), brushList);
            OutSurface result;
            createSurfacesFromBrushes(&clipmap->info, dumpData, brushList, true, result);
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
                    int modelIdx = atol(valueStrPtr + 1);
                    assert(modelIdx > 0);
                    entity.modelIndex = createModelFromIndex(static_cast<size_t>(modelIdx), dumpData, gfxworld, clipmap);
                }
                else
                {
                    BSPEntityEntry entry = {keyStrPtr, valueStrPtr};
                    entity.entries.emplace_back(entry);
                }
            }

            if (entity.type == ET_LIGHT)
            {
                bool foundLight = false;
                for (const auto& entry : entity.entries)
                {
                    if (!entry.key.compare("pl#"))
                    {
                        int lightIdx = atoi(entry.value.c_str());
                        assert(lightIdx > 1); // can't index sunlight or empty light
                        lightIdx -= 2;        // sun and empty light aren't added to output lights
                        dumpData.lights[lightIdx].isLinkedToEntity = true;
                        BSPEntityEntry entry = {"lightToEntLinkNumber", std::format("{}", lightIdx)};
                        entity.entries.emplace_back(entry);
                        foundLight = true;
                        break;
                    }
                }
                if (!foundLight)
                    assert(false);
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

    void addSurfacesFromMaterialMap(BSPData& dumpData,
                                    const GfxWorld* gfxWorld,
                                    std::vector<BSPVertex>& totalVertexBuffer,
                                    std::map<size_t, std::tuple<size_t, size_t>>& vd0OffsetToGltf,
                                    std::vector<std::pair<size_t, std::vector<size_t>>>& materialMap)
    {
        for (const auto& matSurf : materialMap)
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

                tempVertices.insert(tempVertices.end(), totalVertexBuffer.begin() + firstVertex, totalVertexBuffer.begin() + firstVertex + vertexCount);
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

    void collectSurfacesWithSameMaterials(
        BSPData& dumpData, const GfxWorld* gfxWorld, size_t surfStart, size_t surfEnd, std::vector<std::pair<size_t, std::vector<size_t>>>& out_materialMap)
    {
        for (size_t surfIdx = surfStart; surfIdx < surfEnd; surfIdx++)
        {
            GfxSurface* inSurface = &gfxWorld->dpvs.surfaces[surfIdx];
            size_t materialIndex = createBspMaterial(dumpData, inSurface->material, (GfxSurfaceFlags)inSurface->flags);
            bool found = false;
            for (auto& matSurf : out_materialMap)
            {
                if (matSurf.first == materialIndex)
                {
                    matSurf.second.emplace_back(surfIdx);
                    found = true;
                    break;
                }
            }
            if (!found)
                out_materialMap.emplace_back(std::pair(materialIndex, std::vector<size_t>({static_cast<size_t>(surfIdx)})));
        }
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

        std::vector<std::pair<size_t, std::vector<size_t>>> LitOpaqueSurfs;
        std::vector<std::pair<size_t, std::vector<size_t>>> LitTransparentSurfs;
        std::vector<std::pair<size_t, std::vector<size_t>>> EmissiveOpaqueSurfs;
        std::vector<std::pair<size_t, std::vector<size_t>>> EmissiveTransparentSurfs;
        std::vector<std::pair<size_t, std::vector<size_t>>> scriptSurfs;

        collectSurfacesWithSameMaterials(dumpData, gfxWorld, gfxWorld->dpvs.litSurfsBegin, gfxWorld->dpvs.litSurfsEnd, LitOpaqueSurfs);
        collectSurfacesWithSameMaterials(dumpData, gfxWorld, gfxWorld->dpvs.litTransSurfsBegin, gfxWorld->dpvs.litTransSurfsEnd, LitTransparentSurfs);
        collectSurfacesWithSameMaterials(
            dumpData, gfxWorld, gfxWorld->dpvs.emissiveOpaqueSurfsBegin, gfxWorld->dpvs.emissiveOpaqueSurfsEnd, EmissiveOpaqueSurfs);
        collectSurfacesWithSameMaterials(
            dumpData, gfxWorld, gfxWorld->dpvs.emissiveTransSurfsBegin, gfxWorld->dpvs.emissiveTransSurfsEnd, EmissiveTransparentSurfs);
        // script surfaces need to be in order and unmerged since they are indexed by an entity
        for (int surfIdx = gfxWorld->dpvs.staticSurfaceCount; surfIdx < gfxWorld->surfaceCount; surfIdx++)
        {
            GfxSurface* inSurface = &gfxWorld->dpvs.surfaces[surfIdx];
            size_t materialIndex = createBspMaterial(dumpData, inSurface->material, (GfxSurfaceFlags)inSurface->flags);
            scriptSurfs.emplace_back(std::pair(materialIndex, std::vector<size_t>({static_cast<size_t>(surfIdx)})));
        }

        assert(dumpData.gfxWorld.surfaces.size() == 0);
        dumpData.staticSurfaceStart = 0;

        dumpData.litOpaqueSurfaceStart = dumpData.gfxWorld.surfaces.size();
        addSurfacesFromMaterialMap(dumpData, gfxWorld, allVertices, vd0OffsetToGltf, LitOpaqueSurfs);
        dumpData.litOpaqueSurfaceCount = dumpData.gfxWorld.surfaces.size() - dumpData.litOpaqueSurfaceStart;

        dumpData.litTransparentSurfaceStart = dumpData.gfxWorld.surfaces.size();
        addSurfacesFromMaterialMap(dumpData, gfxWorld, allVertices, vd0OffsetToGltf, LitTransparentSurfs);
        dumpData.litTransparentSurfaceCount = dumpData.gfxWorld.surfaces.size() - dumpData.litTransparentSurfaceStart;

        dumpData.emissiveOpaqueSurfaceStart = dumpData.gfxWorld.surfaces.size();
        addSurfacesFromMaterialMap(dumpData, gfxWorld, allVertices, vd0OffsetToGltf, EmissiveOpaqueSurfs);
        dumpData.emissiveOpaqueSurfaceCount = dumpData.gfxWorld.surfaces.size() - dumpData.emissiveOpaqueSurfaceStart;

        dumpData.emissiveTransparentSurfaceStart = dumpData.gfxWorld.surfaces.size();
        addSurfacesFromMaterialMap(dumpData, gfxWorld, allVertices, vd0OffsetToGltf, EmissiveTransparentSurfs);
        dumpData.emissiveTransparentSurfaceCount = dumpData.gfxWorld.surfaces.size() - dumpData.emissiveTransparentSurfaceStart;

        dumpData.staticSurfaceCount = dumpData.gfxWorld.surfaces.size();

        addSurfacesFromMaterialMap(dumpData, gfxWorld, allVertices, vd0OffsetToGltf, scriptSurfs);
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

    void dumpComWorld(BSPData& dumpData, const ComWorld* comWorld, std::vector<GfxLightDef*>& lightDefs)
    {
        for (unsigned int lightIdx = 0; lightIdx < comWorld->primaryLightCount; lightIdx++)
        {
            ComPrimaryLight* inLight = &comWorld->primaryLights[lightIdx];
            BSPLight outLight{};

            switch (inLight->type)
            {
            case GFX_LIGHT_TYPE_NONE:
                assert(lightIdx == 0);
                continue;
            case GFX_LIGHT_TYPE_DIR:
                outLight.type = LIGHT_TYPE_DIRECTIONAL;
                break;
            case GFX_LIGHT_TYPE_OMNI:
                outLight.type = LIGHT_TYPE_POINT;
                break;
            case GFX_LIGHT_TYPE_SPOT:
            case GFX_LIGHT_TYPE_SPOT_SQUARE:
            case GFX_LIGHT_TYPE_SPOT_ROUND:
                outLight.type = LIGHT_TYPE_SPOT;
                outLight.innerConeAngle = acosf(inLight->cosHalfFovInner);
                outLight.outerConeAngle = acosf(inLight->cosHalfFovOuter);
                break;
            default:
                assert(false);
            }

            outLight.forwardVector = inLight->dir;
            LhcToRhcCoordinates(outLight.forwardVector.v);
            outLight.rollAngle = inLight->angle.z;

            outLight.colour.x = inLight->diffuseColor.x;
            outLight.colour.y = inLight->diffuseColor.y;
            outLight.colour.z = inLight->diffuseColor.z;
            if (lightIdx != SUN_LIGHT_INDEX)
            {
                outLight.pos = inLight->origin;
                LhcToRhcCoordinates(outLight.pos.v);
                outLight.range = inLight->radius;
                outLight.intensity = inLight->dAttenuation;
                outLight.superEllipse = inLight->aAbB;
                assert(inLight->cullDist > 0);
                outLight.cullDistance = inLight->cullDist;
                outLight.roundness = inLight->roundness;

                bool foundLightDef = false;
                for (const auto& lightDef : lightDefs)
                {
                    if (!strcmp(inLight->defName, lightDef->name))
                    {
                        outLight.image = lightDef->attenuation.image->name;
                        foundLightDef = true;
                        break;
                    }
                }
                if (!foundLightDef)
                    assert(false);
            }

            if (lightIdx == SUN_LIGHT_INDEX)
                dumpData.sunlight = outLight;
            else
                dumpData.lights.emplace_back(outLight);
        }
    }
} // namespace

void T6::BSP::dumpBSPData(BSPData& dumpData, std::string zoneName, BSPAssetPtrs& assetPtrs)
{
    dumpData.name = zoneName;

    dumpComWorld(dumpData, assetPtrs.comworld, assetPtrs.lightDefs);
    dumpGfxWorld(dumpData, assetPtrs.gfxworld);
    dumpClipmap(dumpData, assetPtrs.clipmap);
    dumpMapEnts(dumpData, assetPtrs.mapEnts, assetPtrs.gfxworld, assetPtrs.clipmap); // requires colworld materials
}
