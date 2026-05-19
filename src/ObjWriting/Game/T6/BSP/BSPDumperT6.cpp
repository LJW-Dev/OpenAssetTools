#pragma once

#include "BSPDumperT6.h"

#include "BSP/BSPFlags.h"
#include "Game/T6/T6.h"
#include "Gltf/JsonGltf.h"
#include "Utils/Logging/Log.h"
#include "Utils/Pack.h"
#include "XModel/Gltf/GltfBinOutput.h"
#include "XModel/Gltf/GltfTextOutput.h"
#include "XModel/Gltf/GltfWriter.h"
#include "quickhull/quickhull.hpp"

#include <deque>
#include <unordered_set>

using namespace gltf;
using namespace T6;
using namespace quickhull;
using namespace BSPFlags;

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

    constexpr const char* bspEntityTypeNames[] = {"Weapons",
                                                  "Volumes",
                                                  "Triggers",
                                                  "Pathnodes",
                                                  "Lights",
                                                  "Structs",
                                                  "Vehicles",
                                                  "Models",
                                                  "Brushmodels",
                                                  "ZBarriers",
                                                  "Points",
                                                  "Actors",
                                                  "Glass",
                                                  "Ropes",
                                                  "Other"};

    enum bspEntityType
    {
        ET_WEAPON,
        ET_VOLUME,
        ET_TRIGGER,
        ET_PATHNODE,
        ET_LIGHT,
        ET_STRUCT,
        ET_VEHICLE,
        ET_MODEL,
        ET_BRUSHMODEL,
        ET_ZBARRIER,
        ET_POINT,
        ET_ACTOR,
        ET_GLASS,
        ET_ROPE,
        ET_OTHER,
        ET_COUNT
    };

    struct BSPEntityEntry
    {
        std::string key;
        std::string value;
    };

    enum bspEntitySurfType
    {
        EST_BRUSH,
        EST_TERRAIN,
        EST_BOTH
    };

    enum bspEntitySurfSide
    {
        ESS_NONE,
        ESS_GFX,
        ESS_COL,
        ESS_BOTH
    };

    struct BspEntitySurfaces
    {
        bspEntitySurfSide surfaceSide;
        bspEntitySurfType surfaceType;
        size_t gfxSurfaceIndex;
        size_t gfxSurfaceCount;
        size_t colBrushSurfaceIndex;
        size_t colBrushSurfaceCount;
        size_t colTerrainSurfaceIndex;
        size_t colTerrainSurfaceCount;
    };

    struct BSPEntity
    {
        vec3_t origin;
        vec4_t rotationQuaternion;

        size_t uniqueEntityNumber;
        BspEntitySurfaces surface;

        bspEntityType type;
        std::string classname;
        std::vector<BSPEntityEntry> entries;
    };

    struct GltfVertex
    {
        float coordinates[3];
        float normal[3];
        float uv[2];
        float color[4];
    };

    struct BSPSurface
    {
        size_t materialIndex;
        size_t triCount;
        size_t indexOfFirstVertex;
        size_t indexOfFirstIndex;
        size_t vertexCount;
    };

    struct bspMaterial
    {
        std::string name;
        std::string flags;
    };

    struct BSPWorldDump
    {
        std::vector<bspMaterial> materials;
        std::vector<BSPSurface> surfaces;
        std::vector<GltfVertex> vertices;
        std::vector<uint16_t> indices;
    };

    struct bspDumpData
    {
        std::string BSPName;
        std::vector<BSPEntity> entities;

        size_t staticSurfaceStart;
        size_t staticSurfaceCount;
        BSPWorldDump gfxWorld;

        size_t terrainSurfaceStart;
        size_t terrainSurfaceCount;
        size_t brushSurfaceStart;
        size_t brushSurfaceCount;
        BSPWorldDump colWorld;
    };

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

    vec3_t convertStringToVec3(char* str)
    {
        char* v1Str = str;

        int nextValIndex = 0;
        while (v1Str[nextValIndex] != ' ')
            nextValIndex++;
        nextValIndex++; // skip past space
        char* v2Str = &v1Str[nextValIndex];

        nextValIndex = 0;
        while (v2Str[nextValIndex] != ' ')
            nextValIndex++;
        nextValIndex++; // skip past space
        char* v3Str = &v2Str[nextValIndex];

        vec3_t result;
        result.x = static_cast<float>(atof(v1Str));
        result.y = static_cast<float>(atof(v2Str));
        result.z = static_cast<float>(atof(v3Str));
        return result;
    }

    void convertAnglesToAxis(vec3_t& angles, vec3_t axis[3])
    {
        float cosX = cos(angles.x);
        float sinX = sin(angles.x);
        float cosY = cos(angles.y);
        float sinY = sin(angles.y);
        float cosZ = cos(angles.z);
        float sinZ = sin(angles.z);

        axis[0].x = cosX * cosY;
        axis[0].y = cosX * sinY;
        axis[0].z = -sinX;
        axis[1].x = ((sinZ * sinX) * cosY) - (cosZ * sinY);
        axis[1].y = ((sinZ * sinX) * sinY) + (cosZ * cosY);
        axis[1].z = sinZ * cosX;
        axis[2].x = ((cosZ * sinX) * cosY) + (sinZ * sinY);
        axis[2].y = ((cosZ * sinX) * sinY) - (sinZ * cosY);
        axis[2].z = cosZ * cosX;
    }

    inline float lengthSquaredOfQuat(float quat[4])
    {
        return quat[0] * quat[0] + quat[1] * quat[1] + quat[2] * quat[2] + quat[3] * quat[3];
    }

    vec4_t convertAxisToQuat(vec3_t axis[3])
    {
        float possibleQuats[4][4];

        float maxXX = axis[0].x;
        float matXY = axis[0].y;
        float matXZ = axis[0].z;
        float matYX = axis[1].x;
        float matYY = axis[1].y;
        float matYZ = axis[1].z;
        float matZX = axis[2].x;
        float matZY = axis[2].y;
        float matZZ = axis[2].z;

        float YZminZY = matYZ - matZY;
        float ZXminXZ = matZX - matXZ;
        float XYminYX = matXY - matYX;
        float ZYplusYZ = matZY + matYZ;
        float XZplusZX = matXZ + matZX;
        float YXplusXY = matYX + matXY;

        char axisToUse = 0;
        possibleQuats[0][0] = YZminZY;
        possibleQuats[0][1] = ZXminXZ;
        possibleQuats[0][2] = XYminYX;
        possibleQuats[0][3] = (maxXX + matYY + matZZ) + 1.0f;
        float lengthSquared = lengthSquaredOfQuat(possibleQuats[0]);
        if (lengthSquared < 1.0f)
        {
            axisToUse = 1;
            possibleQuats[1][0] = XZplusZX;
            possibleQuats[1][1] = ZYplusYZ;
            possibleQuats[1][2] = (matZZ - matYY - maxXX) + 1.0f;
            possibleQuats[1][3] = XYminYX;
            lengthSquared = lengthSquaredOfQuat(possibleQuats[1]);
            if (lengthSquared < 1.0f)
            {
                axisToUse = 2;
                possibleQuats[2][0] = (maxXX - matYY - matZZ) + 1.0f;
                possibleQuats[2][1] = YXplusXY;
                possibleQuats[2][2] = XZplusZX;
                possibleQuats[2][3] = YZminZY;
                lengthSquared = lengthSquaredOfQuat(possibleQuats[2]);
                if (lengthSquared < 1.0f)
                {
                    axisToUse = 3;
                    possibleQuats[3][0] = YXplusXY;
                    possibleQuats[3][1] = (matYY - maxXX - matZZ) + 1.0f;
                    possibleQuats[3][2] = ZYplusYZ;
                    possibleQuats[3][3] = ZXminXZ;
                    lengthSquared = lengthSquaredOfQuat(possibleQuats[3]);
                    if (lengthSquared < 1.0f)
                        con::warn("Axis to quatrnion: bad axis.");
                }
            }
        }

        if (lengthSquared == 0.0f)
        {
            con::warn("Axis to quatrnion: bad length.");
            lengthSquared = 1.0f;
        }

        vec4_t quaternion;
        float inverseLength = 1.0f / sqrtf(lengthSquared);
        quaternion.x = possibleQuats[axisToUse][0] * inverseLength;
        quaternion.y = possibleQuats[axisToUse][1] * inverseLength;
        quaternion.z = possibleQuats[axisToUse][2] * inverseLength;
        quaternion.w = possibleQuats[axisToUse][3] * inverseLength;
        return quaternion;
    }

    vec4_t convertAnglesToQuat(vec3_t& angles)
    {
        vec3_t axis[3];
        convertAnglesToAxis(angles, axis);
        return convertAxisToQuat(axis);
    }

    struct outSurface
    {
        size_t surfaceStart;
        size_t surfaceCount;
    };

    struct partitionData
    {
        size_t partitionIdx;
        size_t materialIdx;
    };

    void getPartitionsFromAABBTree(const clipMap_t* clipmap, int aabbStartIndex, int aabbCount, std::vector<partitionData>& partitionList)
    {
        std::deque<int> aabbQueue;
        for (int i = 0; i < aabbCount; i++)
            aabbQueue.emplace_back(aabbStartIndex + i);

        while (!aabbQueue.empty())
        {
            int aabbIdx = aabbQueue.front();
            aabbQueue.pop_front();

            CollisionAabbTree* aabb = &clipmap->aabbTrees[aabbIdx];
            if (aabb->childCount == 0)
            {
                partitionData data;
                data.partitionIdx = aabb->u.partitionIndex;
                data.materialIdx = aabb->materialIndex;
                partitionList.emplace_back(data);
            }
            else
            {
                for (uint16_t i = 0; i < aabb->childCount; i++)
                    aabbQueue.emplace_back(aabb->u.firstChildIndex + i);
            }
        }
    }

    void createSurfacesFromPartitions(const clipMap_t* clipmap, bspDumpData& dumpData, std::vector<partitionData>& partitionList, outSurface& result)
    {
        result.surfaceStart = dumpData.colWorld.surfaces.size();
        size_t totalVertexCount = (int)dumpData.colWorld.vertices.size();
        size_t totalIndexCount = (int)dumpData.colWorld.indices.size();
        std::unordered_set<size_t> uniquePartitions;
        for (partitionData partitionIdx : partitionList)
        {
            if (uniquePartitions.contains(partitionIdx.partitionIdx))
                continue;
            uniquePartitions.emplace(partitionIdx.partitionIdx);

            BSPSurface surface;
            CollisionPartition* partition = &clipmap->partitions[partitionIdx.partitionIdx];

            surface.indexOfFirstVertex = totalVertexCount;
            surface.triCount = partition->triCount;
            surface.vertexCount = partition->triCount * 3;
            surface.materialIndex = partitionIdx.materialIdx;
            surface.indexOfFirstIndex = totalIndexCount;
            dumpData.colWorld.surfaces.emplace_back(surface);

            for (char k = 0; k < partition->triCount; k++)
            {
                int triIndex = partition->firstTri + k;
                uint16_t* vertIndices = clipmap->triIndices[triIndex];

                GltfVertex vert0{};
                GltfVertex vert1{};
                GltfVertex vert2{};
                vert0.coordinates[0] = clipmap->verts[vertIndices[0]].x;
                vert0.coordinates[1] = clipmap->verts[vertIndices[0]].y;
                vert0.coordinates[2] = clipmap->verts[vertIndices[0]].z;
                vert1.coordinates[0] = clipmap->verts[vertIndices[1]].x;
                vert1.coordinates[1] = clipmap->verts[vertIndices[1]].y;
                vert1.coordinates[2] = clipmap->verts[vertIndices[1]].z;
                vert2.coordinates[0] = clipmap->verts[vertIndices[2]].x;
                vert2.coordinates[1] = clipmap->verts[vertIndices[2]].y;
                vert2.coordinates[2] = clipmap->verts[vertIndices[2]].z;

                LhcToRhcCoordinates(vert0.coordinates);
                LhcToRhcCoordinates(vert1.coordinates);
                LhcToRhcCoordinates(vert2.coordinates);

                dumpData.colWorld.vertices.emplace_back(vert0);
                dumpData.colWorld.vertices.emplace_back(vert1);
                dumpData.colWorld.vertices.emplace_back(vert2);

                dumpData.colWorld.indices.emplace_back((k * 3) + 2);
                dumpData.colWorld.indices.emplace_back((k * 3) + 1);
                dumpData.colWorld.indices.emplace_back((k * 3) + 0);
            }

            totalVertexCount += surface.vertexCount;
            totalIndexCount += surface.vertexCount;
        }

        result.surfaceCount = uniquePartitions.size();
    }

    void createSurfacesFromBrushes(const clipMap_t* clipmap, bspDumpData& dumpData, std::vector<size_t>& brushList, outSurface& result)
    {
        result.surfaceStart = dumpData.colWorld.surfaces.size();
        size_t totalVertexCount = (int)dumpData.colWorld.vertices.size();
        size_t totalIndexCount = (int)dumpData.colWorld.indices.size();
        std::unordered_set<size_t> uniqueBrushes;
        for (size_t brushIdx : brushList)
        {
            if (uniqueBrushes.contains(brushIdx))
                continue;
            uniqueBrushes.emplace(brushIdx);

            cbrush_t* brush = &clipmap->info.brushes[brushIdx];

            std::vector<vec3_t> hullVerts;
            if (brush->numverts != 0)
            {
                for (unsigned int i = 0; i < brush->numverts; i++)
                    hullVerts.emplace_back(brush->verts[i]);
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
            for (size_t i = 0; i < hullVerts.size(); i++)
                LhcToRhcCoordinates(hullVerts[i].v);

            QuickHull<float> qh;
            auto hull = qh.getConvexHull(&hullVerts[0].x, hullVerts.size(), false, true);
            const auto& indexBuffer = hull.getIndexBuffer();
            const auto& vertexBuffer = hull.getVertexBuffer();
            assert(indexBuffer.size() % 3 == 0);

            BSPSurface surface;
            surface.indexOfFirstVertex = totalVertexCount;
            surface.indexOfFirstIndex = totalIndexCount;
            surface.triCount = (uint16_t)(indexBuffer.size() / 3);
            surface.vertexCount = (int)vertexBuffer.size();

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
                con::warn("Coulndn't determine material idx from flags");
            surface.materialIndex = matIndex;

            dumpData.colWorld.surfaces.emplace_back(surface);

            for (const auto& vertex : vertexBuffer)
            {
                GltfVertex vert{};
                vert.coordinates[0] = vertex.x;
                vert.coordinates[1] = vertex.y;
                vert.coordinates[2] = vertex.z;
                dumpData.colWorld.vertices.emplace_back(vert);
            }

            for (const auto& index : indexBuffer)
            {
                dumpData.colWorld.indices.emplace_back((uint16_t)index);
            }

            totalVertexCount += surface.vertexCount;
            totalIndexCount += surface.triCount * 3;
        }

        result.surfaceCount = uniqueBrushes.size();
    }

    void getBrushesFromLeafBrushNode(const clipMap_t* clipmap, int leafBrushNodeIdx, std::vector<size_t>& brushList)
    {
        if (leafBrushNodeIdx == 0)
        {
            con::warn("getBrushesFromLeafBrushNode: leafBrushNodeIdx == 0");
            return;
        }

        std::deque<int> leafBrushNodeQueue;
        leafBrushNodeQueue.emplace_back(leafBrushNodeIdx);

        while (!leafBrushNodeQueue.empty())
        {
            int lbnIdx = leafBrushNodeQueue.front();
            leafBrushNodeQueue.pop_front();
            auto leafBrushNode = &clipmap->info.leafbrushNodes[lbnIdx];

            if (leafBrushNode->leafBrushCount > 0)
            {
                for (int16_t i = 0; i < leafBrushNode->leafBrushCount; i++)
                    brushList.emplace_back(leafBrushNode->data.leaf.brushes[i]);
                continue;
            }

            if (leafBrushNode->leafBrushCount < 0)
                leafBrushNodeQueue.emplace_back(lbnIdx + 1);
            leafBrushNodeQueue.emplace_back(lbnIdx + leafBrushNode->data.children.childOffset[0]);
            leafBrushNodeQueue.emplace_back(lbnIdx + leafBrushNode->data.children.childOffset[1]);
        }
    }

    void createSurfacesFromEntity(size_t modelIndex, BSPEntity& entity, bspDumpData& dumpData, const GfxWorld* gfxworld, const clipMap_t* clipmap)
    {
        if (modelIndex == 0)
            return;

        if (gfxworld->models[modelIndex].surfaceCount != 0)
        {
            entity.surface.surfaceSide = ESS_GFX;
            entity.surface.gfxSurfaceCount = gfxworld->models[modelIndex].surfaceCount;
            entity.surface.gfxSurfaceIndex = gfxworld->models[modelIndex].startSurfIndex;
        }

        cLeaf_s* leaf = &clipmap->cmodels[modelIndex].leaf;
        if (leaf->collAabbCount != 0)
        {
            if (gfxworld->models[modelIndex].surfaceCount != 0)
                entity.surface.surfaceSide = ESS_BOTH;
            else
                entity.surface.surfaceSide = ESS_COL;
            entity.surface.surfaceType = EST_TERRAIN;
            std::vector<partitionData> partitionList;
            getPartitionsFromAABBTree(clipmap, leaf->firstCollAabbIndex, leaf->collAabbCount, partitionList);
            outSurface result;
            createSurfacesFromPartitions(clipmap, dumpData, partitionList, result);
            entity.surface.colTerrainSurfaceCount = result.surfaceCount;
            entity.surface.colTerrainSurfaceIndex = result.surfaceStart;
        }
        if (leaf->leafBrushNode != 0)
        {
            if (gfxworld->models[modelIndex].surfaceCount != 0)
                entity.surface.surfaceSide = ESS_BOTH;
            else
                entity.surface.surfaceSide = ESS_COL;
            if (leaf->collAabbCount != 0)
                entity.surface.surfaceType = EST_BOTH;
            else
                entity.surface.surfaceType = EST_BRUSH;
            std::vector<size_t> brushList;
            getBrushesFromLeafBrushNode(clipmap, leaf->leafBrushNode, brushList);
            outSurface result;
            createSurfacesFromBrushes(clipmap, dumpData, brushList, result);
            entity.surface.colBrushSurfaceCount = result.surfaceCount;
            entity.surface.colBrushSurfaceIndex = result.surfaceStart;
        }
    }

    std::string convertFlagsToString(int surfaceflags, int contentflags)
    {
        std::string result;

        for (int i = 0; i < SURF_TYPE_CLIPMISSILE; i++)
        {
            s_SurfaceTypeFlags sFlags = surfaceTypeToFlagMap[i];
            if (((surfaceflags >> 20) & 0x3F) == ((sFlags.surfaceFlags >> 20) & 0x3F)
                && (sFlags.contentFlags == 0 || (sFlags.contentFlags & contentflags) != 0))
                result += std::format("{}, ", surfaceTypeToNameMap[i]);
        }

        for (int i = SURF_TYPE_CLIPMISSILE; i < SURF_TYPE_COUNT; i++)
        {
            if (i == SURF_TYPE_ORIGIN || i == SURF_TYPE_PHYSICSGEOM || i == SURF_TYPE_LIGHTPORTAL || i == SURF_TYPE_NONSOLID)
                continue;

            s_SurfaceTypeFlags sFlags = surfaceTypeToFlagMap[i];
            if ((sFlags.surfaceFlags == 0 || (sFlags.surfaceFlags & surfaceflags) == sFlags.surfaceFlags)
                && (sFlags.contentFlags == 0 || (sFlags.contentFlags & contentflags) == sFlags.contentFlags))
                result += std::format("{}, ", surfaceTypeToNameMap[i]);
        }
        if ((contentflags & 1) == 0)
            result += "nonsolid, ";

        if (result.size() != 0)
            result.resize(result.size() - 2);
        return result;
    }

    bool starts_with(const char* str, const char* pre)
    {
        return strncmp(pre, str, strlen(pre)) == 0;
    }

    void dumpMapEnts(bspDumpData& dumpData, const MapEnts* mapEnts, const GfxWorld* gfxworld, const clipMap_t* clipmap)
    {
        char* origEntStrPtr = _strdup(mapEnts->entityString);
        char* entStrPtr = origEntStrPtr;
        size_t entIdx = 0;
        while (true)
        {
            while (*entStrPtr != '{' && *entStrPtr != '\0')
                entStrPtr++;
            if (*(entStrPtr++) == '\0')
                break;

            BSPEntity entity{};
            entity.origin = {};
            entity.rotationQuaternion = {0.0f, 0.0f, 0.0f, 1.0f};
            entity.surface = {};
            entity.type = ET_OTHER;
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
                    entity.origin = convertStringToVec3(valueStrPtr);
                    LhcToRhcCoordinates(entity.origin.v);
                }
                else if (!strcmp(keyStrPtr, "angles"))
                {
                    vec3_t angles = convertStringToVec3(valueStrPtr);
                    entity.rotationQuaternion = convertAnglesToQuat(angles);
                    LhcToRhcQuaternion(entity.rotationQuaternion.v);
                }
                else if (!strcmp(keyStrPtr, "model") && *valueStrPtr == '*')
                    createSurfacesFromEntity(atol(valueStrPtr + 1), entity, dumpData, gfxworld, clipmap);
                else
                {
                    BSPEntityEntry entry = {keyStrPtr, valueStrPtr};
                    entity.entries.emplace_back(entry);
                }
            }
            dumpData.entities.emplace_back(entity);
        }
        free(origEntStrPtr);
    }

    union PackedLmapCoords
    {
        unsigned int packed;
    };

    enum GfxSurfaceFlags
    {
        GFX_SURFACE_CASTS_SUN_SHADOW = 0x1,
        GFX_SURFACE_HAS_PRIMARY_LIGHT_CONFLICT = 0x2,
        GFX_SURFACE_IS_SKY = 0x4,
        GFX_SURFACE_NO_DRAW = 0x8,
        GFX_SURFACE_CASTS_SHADOW = 0x10,
        GFX_SURFACE_QUANTIZED = 0x20,
        GFX_SURFACE_NO_COLOR = 0x40
    };

    struct GfxPackedWorldVertex
    {
        vec3_t xyz;
        float binormalSign;
        GfxColor color;
        PackedTexCoords texCoord;
        PackedUnitVec normal;
        PackedUnitVec tangent;
        PackedLmapCoords lmapCoord;
    };

    size_t createBspMaterial(bspDumpData& dumpData, Material* material, GfxSurfaceFlags surfFlags)
    {
        int sflags = 0;
        int cflags = 1;
        if ((surfFlags & GFX_SURFACE_IS_SKY) != 0)
        {
            sflags |= surfaceTypeToFlagMap[SURF_TYPE_SKY].surfaceFlags;
            cflags |= surfaceTypeToFlagMap[SURF_TYPE_SKY].contentFlags;
        }
        if ((surfFlags & GFX_SURFACE_CASTS_SUN_SHADOW) != 0)
        {
            sflags |= surfaceTypeToFlagMap[SURF_TYPE_ONLYCASTSHADOW].surfaceFlags;
            cflags |= surfaceTypeToFlagMap[SURF_TYPE_ONLYCASTSHADOW].contentFlags;
        }
        if ((surfFlags & GFX_SURFACE_NO_DRAW) != 0)
        {
            sflags |= surfaceTypeToFlagMap[SURF_TYPE_NODRAW].surfaceFlags;
            cflags |= surfaceTypeToFlagMap[SURF_TYPE_NODRAW].contentFlags;
        }
        std::string flagStr = convertFlagsToString(sflags, cflags);
        for (size_t i = 0; i < dumpData.gfxWorld.materials.size(); i++)
        {
            auto& mat = dumpData.gfxWorld.materials.at(i);
            if (!mat.name.compare(material->info.name) && !mat.flags.compare(flagStr))
                return i;
        }

        bspMaterial bspMaterial;
        bspMaterial.name = material->info.name;
        bspMaterial.flags = flagStr;
        size_t matIdx = dumpData.gfxWorld.materials.size();
        dumpData.gfxWorld.materials.emplace_back(bspMaterial);

        return matIdx;
    }

    void dumpGfxWorld(bspDumpData& dumpData, const GfxWorld* gfxWorld)
    {
        dumpData.staticSurfaceStart = dumpData.gfxWorld.surfaces.size();
        dumpData.staticSurfaceCount = gfxWorld->dpvs.staticSurfaceCount;
        std::vector<std::pair<int, uint16_t>> vd0Offsets; // maps unique vd0 offsets to their maximum index
        for (int surfIdx = 0; surfIdx < gfxWorld->surfaceCount; surfIdx++)
        {
            GfxSurface* inSurface = &gfxWorld->dpvs.surfaces[surfIdx];
            int vd0Offset = inSurface->tris.vertexDataOffset0;

            uint16_t largestIndex = 0;
            uint16_t* surfTriIndicies = &gfxWorld->draw.indices[inSurface->tris.baseIndex];
            for (int vertIdx = 0; vertIdx < inSurface->tris.triCount * 3; vertIdx++)
            {
                if (surfTriIndicies[vertIdx] > largestIndex)
                    largestIndex = surfTriIndicies[vertIdx];
            }

            bool foundMatch = false;
            for (auto& eee : vd0Offsets)
            {
                if (eee.first == vd0Offset)
                {
                    foundMatch = true;
                    if (largestIndex > eee.second)
                        eee.second = largestIndex;
                }
            }
            if (!foundMatch)
            {
                vd0Offsets.emplace_back(vd0Offset, largestIndex);
            }
        }

        std::vector<std::array<int, 3>> vd0OffsetToGltf; // maps unique vd0 offsets to their gltfvertex index equivalent and size
        int totalVertexCount = 0;
        for (size_t offIdx = 0; offIdx < vd0Offsets.size(); offIdx++)
        {
            int vd0Offset = vd0Offsets.at(offIdx).first;
            uint16_t vertexCount = vd0Offsets.at(offIdx).second + 1;

            vd0OffsetToGltf.emplace_back(std::array<int, 3>({vd0Offset, totalVertexCount, vertexCount}));

            totalVertexCount += vertexCount;
            for (uint16_t idx = 0; idx < vertexCount; idx++)
            {
                GfxPackedWorldVertex* inVertex = (GfxPackedWorldVertex*)&gfxWorld->draw.vd0.data[vd0Offset + sizeof(GfxPackedWorldVertex) * idx];
                GltfVertex outVertex;
                outVertex.coordinates[0] = inVertex->xyz.x;
                outVertex.coordinates[1] = inVertex->xyz.y;
                outVertex.coordinates[2] = inVertex->xyz.z;
                LhcToRhcCoordinates(outVertex.coordinates);
                outVertex.normal[0] = inVertex->xyz.x;
                outVertex.normal[1] = inVertex->xyz.y;
                outVertex.normal[2] = inVertex->xyz.z;
                LhcToRhcCoordinates(outVertex.normal);
                pack32::Vec2UnpackTexCoordsUV(inVertex->texCoord.packed, outVertex.uv);
                pack32::Vec4UnpackGfxColor(inVertex->color.packed, outVertex.color);
                dumpData.gfxWorld.vertices.emplace_back(outVertex);
            }
        }

        int indexBufferSize = 0;
        for (int surfIdx = 0; surfIdx < gfxWorld->surfaceCount; surfIdx++)
        {
            GfxSurface* inSurface = &gfxWorld->dpvs.surfaces[surfIdx];
            BSPSurface outSurface;

            outSurface.materialIndex = createBspMaterial(dumpData, inSurface->material, (GfxSurfaceFlags)inSurface->flags);
            outSurface.triCount = inSurface->tris.triCount;

            outSurface.indexOfFirstVertex = -1;
            for (auto mapArr : vd0OffsetToGltf)
            {
                if (inSurface->tris.vertexDataOffset0 == mapArr[0])
                {
                    outSurface.indexOfFirstVertex = mapArr[1];
                    outSurface.vertexCount = mapArr[2];
                }
            }
            assert(outSurface.indexOfFirstVertex != -1);

            outSurface.indexOfFirstIndex = indexBufferSize;
            indexBufferSize += inSurface->tris.triCount * 3;
            uint16_t* surfTriIndicies = &gfxWorld->draw.indices[inSurface->tris.baseIndex];
            for (int triIdx = 0; triIdx < inSurface->tris.triCount; triIdx++)
            {
                dumpData.gfxWorld.indices.emplace_back(surfTriIndicies[triIdx * 3 + 2]);
                dumpData.gfxWorld.indices.emplace_back(surfTriIndicies[triIdx * 3 + 1]);
                dumpData.gfxWorld.indices.emplace_back(surfTriIndicies[triIdx * 3]);
            }

            dumpData.gfxWorld.surfaces.emplace_back(outSurface);
        }
    }

    void getStaticCollisionList(const clipMap_t* clipmap, std::vector<partitionData>& partitionList, std::vector<size_t>& brushList)
    {
        std::deque<int16_t> nodeQueue;
        nodeQueue.emplace_back(0);
        while (!nodeQueue.empty())
        {
            int nodeIdx = nodeQueue.front();
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

    void dumpClipmap(bspDumpData& dumpData, const clipMap_t* clipmap)
    {
        for (unsigned int i = 0; i < clipmap->info.numMaterials; i++)
        {
            bspMaterial material;
            auto colMaterial = &clipmap->info.materials[i];
            material.name = colMaterial->name;
            material.flags = convertFlagsToString(colMaterial->surfaceFlags, colMaterial->contentFlags);
            dumpData.colWorld.materials.emplace_back(material);
        }

        std::vector<partitionData> partitionList;
        std::vector<size_t> brushList;
        getStaticCollisionList(clipmap, partitionList, brushList);

        outSurface result;
        createSurfacesFromPartitions(clipmap, dumpData, partitionList, result);
        dumpData.terrainSurfaceCount = result.surfaceCount;
        dumpData.terrainSurfaceStart = result.surfaceStart;

        createSurfacesFromBrushes(clipmap, dumpData, brushList, result);
        dumpData.brushSurfaceCount = result.surfaceCount;
        dumpData.brushSurfaceStart = result.surfaceStart;
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

    void dumpBSPData(bspDumpData& dumpData, std::string zoneName, BSPAssetPtrs& assetPtrs)
    {
        dumpData.BSPName = zoneName;

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

    void CreateBufferViews(JsonRoot& gltf, bspDumpData& dumpData, std::vector<uint8_t>& bufferData, bool isGfxWorld)
    {
        gltf.bufferViews.emplace();

        if (isGfxWorld)
        {
            unsigned bufferOffset = 0u;
            m_vertex_buffer_view = static_cast<unsigned>(gltf.bufferViews->size());
            JsonBufferView vertexBufferView;
            vertexBufferView.buffer = 0u;
            vertexBufferView.byteOffset = bufferOffset;
            vertexBufferView.byteStride = static_cast<unsigned>(sizeof(GltfVertex));
            vertexBufferView.byteLength = static_cast<unsigned>(sizeof(GltfVertex) * dumpData.gfxWorld.vertices.size());
            vertexBufferView.target = JsonBufferViewTarget::ARRAY_BUFFER;
            bufferOffset += vertexBufferView.byteLength;
            gltf.bufferViews->emplace_back(vertexBufferView);

            m_index_buffer_view = static_cast<unsigned>(gltf.bufferViews->size());
            JsonBufferView indicesBufferView;
            indicesBufferView.buffer = 0u;
            indicesBufferView.byteOffset = bufferOffset;
            indicesBufferView.byteLength = static_cast<unsigned>(sizeof(unsigned short) * dumpData.gfxWorld.indices.size());
            indicesBufferView.target = JsonBufferViewTarget::ELEMENT_ARRAY_BUFFER;
            bufferOffset += indicesBufferView.byteLength;
            gltf.bufferViews->emplace_back(indicesBufferView);

            size_t vertexBufferSize = dumpData.gfxWorld.vertices.size() * sizeof(GltfVertex);
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
            vertexBufferView.byteStride = static_cast<unsigned>(sizeof(GltfVertex));
            vertexBufferView.byteLength = static_cast<unsigned>(sizeof(GltfVertex) * dumpData.colWorld.vertices.size());
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

            size_t vertexBufferSize = dumpData.colWorld.vertices.size() * sizeof(GltfVertex);
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

    void CreateAccessors(JsonRoot& gltf, bspDumpData& dumpData, bool isGfxWorld)
    {
        m_position_accessor_start = 0;
        m_normal_accessor_start = 1;
        m_uv_accessor_start = 2;
        m_color_accessor_start = 3;
        m_index_accessor_start = 4;

        gltf.accessors.emplace();

        size_t surfCount;
        if (isGfxWorld)
            surfCount = dumpData.gfxWorld.surfaces.size();
        else
            surfCount = dumpData.colWorld.surfaces.size();
        for (size_t i = 0; i < surfCount; i++)
        {
            BSPSurface surf;
            if (isGfxWorld)
                surf = dumpData.gfxWorld.surfaces.at(i);
            else
                surf = dumpData.colWorld.surfaces.at(i);
            JsonAccessor positionAccessor;
            positionAccessor.bufferView = m_vertex_buffer_view;
            positionAccessor.byteOffset =
                (unsigned)surf.indexOfFirstVertex * (unsigned)sizeof(GltfVertex) + static_cast<unsigned>(offsetof(GltfVertex, coordinates));
            positionAccessor.componentType = JsonAccessorComponentType::FLOAT;
            positionAccessor.count = (unsigned int)surf.vertexCount;
            positionAccessor.type = JsonAccessorType::VEC3;
            gltf.accessors->emplace_back(positionAccessor);

            JsonAccessor normalAccessor;
            normalAccessor.bufferView = m_vertex_buffer_view;
            normalAccessor.byteOffset = (unsigned)surf.indexOfFirstVertex * (unsigned)sizeof(GltfVertex) + static_cast<unsigned>(offsetof(GltfVertex, normal));
            normalAccessor.componentType = JsonAccessorComponentType::FLOAT;
            normalAccessor.count = (unsigned int)surf.vertexCount;
            normalAccessor.type = JsonAccessorType::VEC3;
            gltf.accessors->emplace_back(normalAccessor);

            JsonAccessor uvAccessor;
            uvAccessor.bufferView = m_vertex_buffer_view;
            uvAccessor.byteOffset = (unsigned)surf.indexOfFirstVertex * (unsigned)sizeof(GltfVertex) + static_cast<unsigned>(offsetof(GltfVertex, uv));
            uvAccessor.componentType = JsonAccessorComponentType::FLOAT;
            uvAccessor.count = (unsigned int)surf.vertexCount;
            uvAccessor.type = JsonAccessorType::VEC2;
            gltf.accessors->emplace_back(uvAccessor);

            JsonAccessor colorAccessor;
            colorAccessor.bufferView = m_vertex_buffer_view;
            colorAccessor.byteOffset = (unsigned)surf.indexOfFirstVertex * (unsigned)sizeof(GltfVertex) + static_cast<unsigned>(offsetof(GltfVertex, color));
            colorAccessor.componentType = JsonAccessorComponentType::FLOAT;
            colorAccessor.count = (unsigned int)surf.vertexCount;
            colorAccessor.type = JsonAccessorType::VEC4;
            gltf.accessors->emplace_back(colorAccessor);

            JsonAccessor indicesAccessor;
            indicesAccessor.bufferView = m_index_buffer_view;
            indicesAccessor.byteOffset = (unsigned)surf.indexOfFirstIndex * (unsigned)sizeof(uint16_t);
            indicesAccessor.componentType = JsonAccessorComponentType::UNSIGNED_SHORT;
            indicesAccessor.count = (unsigned int)surf.triCount * 3;
            indicesAccessor.type = JsonAccessorType::SCALAR;
            gltf.accessors->emplace_back(indicesAccessor);
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

    size_t addMeshFromSurface(JsonRoot& root, bspDumpData& dumpData, size_t startSurf, size_t count, bool isGfxWorld)
    {
        JsonMesh mesh;
        for (size_t surfIdx = startSurf; surfIdx < startSurf + count; surfIdx++)
        {
            BSPSurface surface;
            if (isGfxWorld)
                surface = dumpData.gfxWorld.surfaces[surfIdx];
            else
                surface = dumpData.colWorld.surfaces[surfIdx];

            JsonMeshPrimitives primitive;
            if (surface.materialIndex != -1)
                primitive.material = (unsigned)surface.materialIndex;
            primitive.mode = JsonMeshPrimitivesMode::TRIANGLES;
            primitive.indices = ((unsigned)surfIdx * m_total_accessor_types) + m_index_accessor_start;
            primitive.attributes.COLOR_0 = ((unsigned)surfIdx * m_total_accessor_types) + m_color_accessor_start;
            primitive.attributes.NORMAL = ((unsigned)surfIdx * m_total_accessor_types) + m_normal_accessor_start;
            primitive.attributes.POSITION = ((unsigned)surfIdx * m_total_accessor_types) + m_position_accessor_start;
            primitive.attributes.TEXCOORD_0 = ((unsigned)surfIdx * m_total_accessor_types) + m_uv_accessor_start;
            mesh.primitives.emplace_back(primitive);
        }
        size_t meshIdx = root.meshes->size();
        root.meshes->emplace_back(mesh);
        return meshIdx;
    }

    size_t totalBrushes = 0;

    void addNodesFromBrushSurfaces(JsonRoot& root, bspDumpData& dumpData, size_t startSurf, size_t count, size_t rootNodeIdx, bool isGfxWorld)
    {
        for (size_t i = 0; i < count; i++)
        {
            JsonNode node;
            node.name = std::format("brush_{}", totalBrushes++);
            node.mesh = (unsigned)addMeshFromSurface(root, dumpData, startSurf + i, 1, isGfxWorld);
            nlohmann::json js;
            js["model"] = "brush";
            node.extras = js;
            addNodeToGltf(root, node, rootNodeIdx);
        }
    }

    size_t totalTerrain = 0;

    void addNodesFromTerrainSurfaces(JsonRoot& root, bspDumpData& dumpData, size_t startSurf, size_t count, size_t parentNodeIdx, bool isGfxWorld)
    {
        for (size_t i = 0; i < count; i++)
        {
            JsonNode node;
            node.name = std::format("terrain_{}", totalTerrain++);
            node.mesh = (unsigned)addMeshFromSurface(root, dumpData, startSurf + i, 1, isGfxWorld);
            nlohmann::json js;
            js["model"] = "terrain";
            node.extras = js;
            addNodeToGltf(root, node, parentNodeIdx);
        }
    }

    void createMapEnts(JsonRoot& root, bspDumpData& dumpData, bool isGfxWorld)
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
            if (isGfxWorld && (entity.surface.surfaceSide != ESS_GFX && entity.surface.surfaceSide != ESS_BOTH))
                continue;

            JsonNode node;
            node.name = std::format("entity_{}_{}", entity.classname, entIdx++);
            node.children.emplace();
            node.translation.emplace();
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
            if (entity.surface.surfaceSide == ESS_BOTH)
                js["GfxAndColLinkNumber"] = entity.uniqueEntityNumber;
            node.extras = js;

            size_t nodeIdx;
            if (isGfxWorld)
                nodeIdx = addNodeToGltf(root, node, entNodeIdx + 1);
            else
                nodeIdx = addNodeToGltf(root, node, (entNodeIdx + 1) + entity.type);

            if (entity.surface.surfaceSide != ESS_NONE)
            {
                if (entity.surface.surfaceSide == ESS_BOTH || (entity.surface.surfaceSide == ESS_GFX && isGfxWorld))
                    addNodesFromTerrainSurfaces(root, dumpData, entity.surface.gfxSurfaceIndex, entity.surface.gfxSurfaceCount, nodeIdx, isGfxWorld);
                else if (entity.surface.surfaceSide == ESS_BOTH || (entity.surface.surfaceSide == ESS_COL && !isGfxWorld))
                {
                    if (entity.surface.surfaceType == EST_TERRAIN)
                        addNodesFromTerrainSurfaces(
                            root, dumpData, entity.surface.colTerrainSurfaceIndex, entity.surface.colTerrainSurfaceCount, nodeIdx, isGfxWorld);
                    else if (entity.surface.surfaceType == EST_BRUSH)
                        addNodesFromBrushSurfaces(
                            root, dumpData, entity.surface.colBrushSurfaceIndex, entity.surface.colBrushSurfaceCount, nodeIdx, isGfxWorld);
                    else // EST_BOTH
                    {
                        addNodesFromTerrainSurfaces(
                            root, dumpData, entity.surface.colTerrainSurfaceIndex, entity.surface.colTerrainSurfaceCount, nodeIdx, isGfxWorld);
                        addNodesFromBrushSurfaces(
                            root, dumpData, entity.surface.colBrushSurfaceIndex, entity.surface.colBrushSurfaceCount, nodeIdx, isGfxWorld);
                    }
                }
            }
        }
    }

    void createGfxWorld(JsonRoot& root, bspDumpData& dumpData, bool isGfxWorld)
    {
        if (!isGfxWorld)
            return;

        JsonNode node;
        node.name = "Surfaces";
        node.mesh = (unsigned)addMeshFromSurface(root, dumpData, dumpData.staticSurfaceStart, dumpData.staticSurfaceCount, true);
        addNodeToGltf(root, node, ROOT_NODE_IDX);
    }

    void createColWorld(JsonRoot& root, bspDumpData& dumpData, bool isGfxWorld)
    {
        if (isGfxWorld)
            return;

        JsonNode tNode;
        tNode.name = "Terrain";
        tNode.mesh = (unsigned)addMeshFromSurface(root, dumpData, dumpData.terrainSurfaceStart, dumpData.terrainSurfaceCount, false);
        size_t terrainNodeIdx = addNodeToGltf(root, tNode, ROOT_NODE_IDX);

        JsonNode bNode;
        bNode.name = "Brushes";
        bNode.children.emplace();
        size_t brushNodeIdx = addNodeToGltf(root, bNode, ROOT_NODE_IDX);
        addNodesFromBrushSurfaces(root, dumpData, dumpData.brushSurfaceStart, dumpData.brushSurfaceCount, brushNodeIdx, false);
    }

    void CreateMaterials(JsonRoot& root, bspDumpData& dumpData, bool isGfxWorld)
    {
        root.materials.emplace();

        std::vector<bspMaterial>* matVec;
        if (isGfxWorld)
            matVec = &dumpData.gfxWorld.materials;
        else
            matVec = &dumpData.colWorld.materials;
        for (bspMaterial& mat : *matVec)
        {
            JsonMaterial material;
            material.name = mat.name;
            nlohmann::json extrasJs;
            extrasJs["flags"] = mat.flags;
            extrasJs["name"] = mat.name; // duplicate name incase editor changes the mat name
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

    void createJson(JsonRoot& root, bspDumpData& dumpData, std::vector<uint8_t>& bufferData, bool isGfxWorld)
    {
        createJsonHeader(root, dumpData.BSPName, isGfxWorld);
        CreateBufferViews(root, dumpData, bufferData, isGfxWorld);
        CreateAccessors(root, dumpData, isGfxWorld); // requires buffer views

        CreateMaterials(root, dumpData, isGfxWorld);

        createGfxWorld(root, dumpData, isGfxWorld);
        createColWorld(root, dumpData, isGfxWorld);
        createMapEnts(root, dumpData, isGfxWorld);
    }
} // namespace

namespace bsp
{
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

        bspDumpData dumpData;
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
} // namespace bsp
