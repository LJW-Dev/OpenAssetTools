#pragma once

#include "BSPDumperT6.h"

#include "Game/T6/T6.h"
#include "Utils/Logging/Log.h"
#include "Utils/Pack.h"
#include "XModel/Gltf/GltfBinOutput.h"
#include "XModel/Gltf/GltfTextOutput.h"
#include "XModel/Gltf/GltfWriter.h"
#include "XModel/Gltf/JsonGltf.h"

using namespace gltf;
using namespace T6;

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

    struct BSPEntityEntry
    {
        std::string key;
        std::string value;
    };

    struct BSPEntity
    {
        vec3_t origin;
        vec4_t rotationQuaternion;
        size_t modelIndex;

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
        int flags;
        size_t materialIndex;
        uint16_t triCount;
        int indexOfFirstVertex;
        int indexOfFirstIndex;
        int vertexCount;
    };

    struct bspDumpData
    {
        std::string BSPName;
        std::vector<BSPEntity> entities;

        std::vector<BSPSurface> surfaces;
        std::vector<GltfVertex> vertices;
        std::vector<uint16_t> indices;
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

    void dumpMapEnts(bspDumpData& dumpData, const MapEnts* mapEnts)
    {
        char* origEntStrPtr = _strdup(mapEnts->entityString);
        char* entStrPtr = origEntStrPtr;
        while (true)
        {
            while (*entStrPtr != '{' && *entStrPtr != '\0')
                entStrPtr++;
            if (*(entStrPtr++) == '\0')
                break;

            BSPEntity entity;
            entity.origin = {};
            entity.rotationQuaternion = {};
            entity.modelIndex = 0;
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

                if (!strcmp(keyStrPtr, "origin"))
                {
                    entity.origin = convertStringToVec3(valueStrPtr);
                    LhcToRhcCoordinates(entity.origin.v);
                }
                else if (!strcmp(keyStrPtr, "angles"))
                {
                    vec3_t angles = convertStringToVec3(valueStrPtr);
                    entity.rotationQuaternion = convertAnglesToQuat(angles);
                }
                else if (!strcmp(keyStrPtr, "model") && *valueStrPtr == '*')
                    entity.modelIndex = atol(valueStrPtr + 1);
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

    void dumpGfxWorld(bspDumpData& dumpData, const GfxWorld* gfxWorld)
    {
        std::vector<std::pair<int, uint16_t>> vd0Offsets; // maps unique vd0 offsets to their maximum index
        for (unsigned int surfIdx = 0; surfIdx < gfxWorld->dpvs.staticSurfaceCount; surfIdx++)
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
                dumpData.vertices.emplace_back(outVertex);
            }
        }

        int indexBufferSize = 0;
        for (unsigned int surfIdx = 0; surfIdx < gfxWorld->dpvs.staticSurfaceCount; surfIdx++)
        {
            GfxSurface* inSurface = &gfxWorld->dpvs.surfaces[surfIdx];
            BSPSurface outSurface;

            outSurface.materialIndex = 0;
            outSurface.triCount = inSurface->tris.triCount;
            outSurface.flags = inSurface->flags;

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
                dumpData.indices.emplace_back(surfTriIndicies[triIdx * 3 + 2]);
                dumpData.indices.emplace_back(surfTriIndicies[triIdx * 3 + 1]);
                dumpData.indices.emplace_back(surfTriIndicies[triIdx * 3]);
            }

            dumpData.surfaces.emplace_back(outSurface);
        }
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
        dumpMapEnts(dumpData, assetPtrs.mapEnts);
        dumpGfxWorld(dumpData, assetPtrs.gfxworld);
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

    void CreateBufferViews(JsonRoot& gltf, bspDumpData& dumpData, std::vector<uint8_t>& bufferData)
    {
        gltf.bufferViews.emplace();

        unsigned bufferOffset = 0u;
        m_vertex_buffer_view = static_cast<unsigned>(gltf.bufferViews->size());
        JsonBufferView vertexBufferView;
        vertexBufferView.buffer = 0u;
        vertexBufferView.byteOffset = bufferOffset;
        vertexBufferView.byteStride = static_cast<unsigned>(sizeof(GltfVertex));
        vertexBufferView.byteLength = static_cast<unsigned>(sizeof(GltfVertex) * dumpData.vertices.size());
        vertexBufferView.target = JsonBufferViewTarget::ARRAY_BUFFER;
        bufferOffset += vertexBufferView.byteLength;
        gltf.bufferViews->emplace_back(vertexBufferView);

        m_index_buffer_view = static_cast<unsigned>(gltf.bufferViews->size());
        JsonBufferView indicesBufferView;
        indicesBufferView.buffer = 0u;
        indicesBufferView.byteOffset = bufferOffset;
        indicesBufferView.byteLength = static_cast<unsigned>(sizeof(unsigned short) * dumpData.indices.size());
        indicesBufferView.target = JsonBufferViewTarget::ELEMENT_ARRAY_BUFFER;
        bufferOffset += indicesBufferView.byteLength;
        gltf.bufferViews->emplace_back(indicesBufferView);

        size_t vertexBufferSize = dumpData.vertices.size() * sizeof(GltfVertex);
        size_t indexBufferSize = dumpData.indices.size() * sizeof(uint16_t);
        bufferData.resize(vertexBufferSize + indexBufferSize);

        size_t currentBufferOffset = 0;
        memcpy(&bufferData.at(currentBufferOffset), dumpData.vertices.data(), vertexBufferSize);
        currentBufferOffset += vertexBufferSize;
        memcpy(&bufferData.at(currentBufferOffset), dumpData.indices.data(), indexBufferSize);
        currentBufferOffset += indexBufferSize;
    }

    void CreateAccessors(JsonRoot& gltf, bspDumpData& dumpData)
    {
        m_position_accessor_start = 0;
        m_normal_accessor_start = 1;
        m_uv_accessor_start = 2;
        m_color_accessor_start = 3;
        m_index_accessor_start = 4;

        gltf.accessors.emplace();
        for (size_t i = 0; i < dumpData.surfaces.size(); i++)
        {
            BSPSurface& surf = dumpData.surfaces.at(i);
            JsonAccessor positionAccessor;
            positionAccessor.bufferView = m_vertex_buffer_view;
            positionAccessor.byteOffset = surf.indexOfFirstVertex * sizeof(GltfVertex) + static_cast<unsigned>(offsetof(GltfVertex, coordinates));
            positionAccessor.componentType = JsonAccessorComponentType::FLOAT;
            positionAccessor.count = surf.vertexCount;
            positionAccessor.type = JsonAccessorType::VEC3;
            gltf.accessors->emplace_back(positionAccessor);

            JsonAccessor normalAccessor;
            normalAccessor.bufferView = m_vertex_buffer_view;
            normalAccessor.byteOffset = surf.indexOfFirstVertex * sizeof(GltfVertex) + static_cast<unsigned>(offsetof(GltfVertex, normal));
            normalAccessor.componentType = JsonAccessorComponentType::FLOAT;
            normalAccessor.count = surf.vertexCount;
            normalAccessor.type = JsonAccessorType::VEC3;
            gltf.accessors->emplace_back(normalAccessor);

            JsonAccessor uvAccessor;
            uvAccessor.bufferView = m_vertex_buffer_view;
            uvAccessor.byteOffset = surf.indexOfFirstVertex * sizeof(GltfVertex) + static_cast<unsigned>(offsetof(GltfVertex, uv));
            uvAccessor.componentType = JsonAccessorComponentType::FLOAT;
            uvAccessor.count = surf.vertexCount;
            uvAccessor.type = JsonAccessorType::VEC2;
            gltf.accessors->emplace_back(uvAccessor);

            JsonAccessor colorAccessor;
            colorAccessor.bufferView = m_vertex_buffer_view;
            colorAccessor.byteOffset = surf.indexOfFirstVertex * sizeof(GltfVertex) + static_cast<unsigned>(offsetof(GltfVertex, color));
            colorAccessor.componentType = JsonAccessorComponentType::FLOAT;
            colorAccessor.count = surf.vertexCount;
            colorAccessor.type = JsonAccessorType::VEC4;
            gltf.accessors->emplace_back(colorAccessor);

            JsonAccessor indicesAccessor;
            indicesAccessor.bufferView = m_index_buffer_view;
            indicesAccessor.byteOffset = surf.indexOfFirstIndex * sizeof(uint16_t);
            indicesAccessor.componentType = JsonAccessorComponentType::UNSIGNED_SHORT;
            indicesAccessor.count = surf.triCount * 3;
            indicesAccessor.type = JsonAccessorType::SCALAR;
            gltf.accessors->emplace_back(indicesAccessor);
        }
    }

    void createMapEnts(JsonRoot& root, bspDumpData& dumpData)
    {
        JsonNode entNode;
        entNode.name = "Map Entities";
        entNode.children.emplace();
        size_t entNodeIdx = root.nodes->size();
        root.nodes->at(0).children->emplace_back(entNodeIdx);
        root.nodes->emplace_back(entNode);

        int entIdx = 0;
        for (BSPEntity& entity : dumpData.entities)
        {
            JsonNode node;
            node.name.emplace();
            node.translation.emplace();
            node.rotation.emplace();
            node.extras.emplace();
            node.name = std::format("entity_{}", entIdx++);

            (*node.translation)[0] = entity.origin.x;
            (*node.translation)[1] = entity.origin.y;
            (*node.translation)[2] = entity.origin.z;
            (*node.rotation)[0] = entity.rotationQuaternion.x;
            (*node.rotation)[1] = entity.rotationQuaternion.y;
            (*node.rotation)[2] = entity.rotationQuaternion.z;
            (*node.rotation)[3] = entity.rotationQuaternion.w;
            for (const auto& entityEntry : entity.entries)
                (*node.extras)[entityEntry.key] = entityEntry.value;
            root.nodes->at(entNodeIdx).children->emplace_back(root.nodes->size());
            root.nodes->emplace_back(node);
        }
    }

    void createGfxWorld(JsonRoot& root, bspDumpData& dumpData)
    {
        JsonNode node;
        node.name = "GFX Surfaces";
        node.mesh = 0;
        root.nodes->at(0).children->emplace_back(root.nodes->size());
        root.nodes->emplace_back(node);
    }

    void createMeshes(JsonRoot& root, bspDumpData& dumpData)
    {
        root.meshes.emplace();
        JsonMesh mesh;
        for (unsigned surfIdx = 0; surfIdx < dumpData.surfaces.size(); surfIdx++)
        {
            BSPSurface& surface = dumpData.surfaces[surfIdx];

            JsonMeshPrimitives primitive;
            // primitive.material = surface.materialIndex;
            primitive.mode = JsonMeshPrimitivesMode::TRIANGLES;
            primitive.indices = (surfIdx * m_total_accessor_types) + m_index_accessor_start;
            primitive.attributes.COLOR_0 = (surfIdx * m_total_accessor_types) + m_color_accessor_start;
            primitive.attributes.NORMAL = (surfIdx * m_total_accessor_types) + m_normal_accessor_start;
            primitive.attributes.POSITION = (surfIdx * m_total_accessor_types) + m_position_accessor_start;
            primitive.attributes.TEXCOORD_0 = (surfIdx * m_total_accessor_types) + m_uv_accessor_start;
            mesh.primitives.emplace_back(primitive);
        }
        root.meshes->emplace_back(mesh);
    }

    void CreateMaterials(JsonRoot& root, bspDumpData& dumpData)
    {
        // root.materials.emplace();
        // JsonMaterial material;
        // material.name =  "default";
        // material.pbrMetallicRoughness.
        // root.materials
    }

    void createJsonHeader(JsonRoot& root)
    {
        root.asset.version = "2.0";
        root.asset.generator = "T6-BSP-Decompiler-v0.1";

        JsonScene scene;
        scene.name = "Scene";
        scene.nodes.emplace_back(0);
        root.scenes.emplace();
        root.scenes->emplace_back(scene);
        root.scene = 0;
    }

    void createJson(JsonRoot& root, bspDumpData& dumpData, std::vector<uint8_t>& bufferData)
    {
        createJsonHeader(root);
        CreateBufferViews(root, dumpData, bufferData);
        CreateAccessors(root, dumpData); // requires buffer views
        createMeshes(root, dumpData);    // requires accessors
        CreateMaterials(root, dumpData);

        JsonNode rootNode;
        rootNode.name = dumpData.BSPName;
        rootNode.children.emplace();
        root.nodes.emplace();
        root.nodes->emplace_back(rootNode);

        createGfxWorld(root, dumpData);
        createMapEnts(root, dumpData);
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

        JsonRoot root;
        std::vector<uint8_t> bufferData;
        createJson(root, dumpData, bufferData);

        const auto assetFile = context.OpenAssetFile("bsp/zm_transits.glb");
        if (!assetFile)
        {
            con::error("Unable to open bsp output file.");
            return;
        }
        writeGltf(root, bufferData, assetFile.get());

        context.IncrementProgress();
    }
} // namespace bsp
