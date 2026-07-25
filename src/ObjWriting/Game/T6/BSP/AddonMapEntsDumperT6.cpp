#include "AddonMapEntsDumperT6.h"

#include "BSP/BSP.h"
#include "BSP/BSPUtil.h"
#include "Gltf/JsonGltf.h"
#include "XModel/Gltf/GltfBinOutput.h"

#include <QuickHull.hpp>
#include <queue>
#include <string>
#include <unordered_set>

#pragma warning(push, 0)
#include <Eigen>
#pragma warning(pop)

using namespace T6;
using namespace BSP;
using namespace gltf;
using namespace quickhull;

namespace
{
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

    int getSurfaceTypeFromFlags(int surfaceFlags)
    {
        return ((surfaceFlags >> 20) & 0x3F);
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

            std::string surfaceFlags;
            std::string contentFlags;
            if (isGfxWorld)
            {
                if (BSPUtil::flagsMatchExact(GFX_SURFACE_CASTS_SUN_SHADOW, mat.surfaceFlags))
                    surfaceFlags.append("onlycastshadow, ");
                if (BSPUtil::flagsMatchExact(GFX_SURFACE_IS_SKY, mat.surfaceFlags))
                    surfaceFlags.append("sky, ");
                if (BSPUtil::flagsMatchExact(GFX_SURFACE_NO_DRAW, mat.surfaceFlags))
                    surfaceFlags.append("nodraw, ");
            }
            else
            {
                if (BSPFlags::surfaceFlags_TypeToName.contains(getSurfaceTypeFromFlags(mat.surfaceFlags)))
                    surfaceFlags.append(std::format("{}, ", BSPFlags::surfaceFlags_TypeToName.at(getSurfaceTypeFromFlags(mat.surfaceFlags))));
                for (const auto& flagToStr : BSPFlags::surfaceFlags_FlagToName)
                {
                    if (BSPUtil::flagsMatchExact(flagToStr.first, mat.surfaceFlags))
                        surfaceFlags.append(std::format("{}, ", flagToStr.second));
                }
                for (const auto& flagToStr : BSPFlags::contentFlags_FlagToName)
                {
                    if (BSPUtil::flagsMatchExact(flagToStr.first, mat.contentFlags))
                        contentFlags.append(std::format("{}, ", flagToStr.second));
                }
            }

            nlohmann::json extrasJs;
            extrasJs["surfaceflags"] = surfaceFlags.substr(0, surfaceFlags.empty() ? 0 : surfaceFlags.size() - 2);
            extrasJs["contentflags"] = contentFlags.substr(0, contentFlags.empty() ? 0 : contentFlags.size() - 2);
            extrasJs["name"] = mat.materialName; // duplicate name incase editor changes the mat name
            material.extras = extrasJs;
            root.materials->emplace_back(material);
        }
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

    size_t totalTerrain = 0;
    size_t totalBrushes = 0;

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

    void createMapEnts(JsonRoot& root, BSPData& dumpData, bool isGfxWorld)
    {
        JsonNode entNode;
        entNode.name = "Entities";
        entNode.children.emplace();
        size_t entNodeIdx = addNodeToGltf(root, entNode, ROOT_NODE_IDX);

        std::vector<unsigned int> entityIndexes[ET_COUNT];
        int entIdx = 0;
        for (BSPEntity& entity : dumpData.entities)
        {
            BSPModel* model = nullptr;
            if (entity.hasModel)
                model = &dumpData.models.at(entity.modelIndex);

            if (!isGfxWorld && entity.type == ET_LIGHT)
                continue;

            if (model == nullptr)
            {
                if (isGfxWorld && entity.type != ET_LIGHT)
                    continue;
            }
            else
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
            {
                if (!entity.classname.compare("worldspawn"))
                {
                    // only keep values that do anything
                    if (!entityEntry.key.compare("classname") || !entityEntry.key.compare("skyboxmodel") || !entityEntry.key.compare("guid")
                        || !entityEntry.key.compare("gravity"))
                        js[entityEntry.key] = entityEntry.value;
                    else
                        continue;
                }
                else if (entity.type == ET_LIGHT)
                {
                    if (!entityEntry.key.compare("lightToEntLinkNumber"))
                    {
                        JsonPunctualLightIndex jsLightIndex{};
                        jsLightIndex.light = atoi(entityEntry.value.c_str());
                        JsonNodeExtension extension{};
                        extension.KHR_lights_punctual = jsLightIndex;
                        node.extensions = extension;

                        // overwrite entity rotation with light rotation
                        BSPLight* inLight = &dumpData.lights.at(jsLightIndex.light);
                        Eigen::Vector3f defaultDirection(0.0f, 0.0f, 1.0f);
                        Eigen::Vector3f lightDirection(inLight->forwardVector.x, inLight->forwardVector.y, inLight->forwardVector.z);
                        Eigen::Quaternionf forwardQuat = Eigen::Quaternionf::FromTwoVectors(defaultDirection, lightDirection);
                        Eigen::AngleAxisf rollAxis(inLight->rollAngle, Eigen::Vector3f::UnitZ());
                        Eigen::Quaternionf quat = forwardQuat * rollAxis;
                        node.rotation = {quat.x(), quat.y(), quat.z(), quat.w()};
                        continue;
                    }
                    // remove unsed data that the user might think effects the light's properties
                    if (!entityEntry.key.compare("_bakecolor") || !entityEntry.key.compare("bakecolor") || !entityEntry.key.compare("_color")
                        || !entityEntry.key.compare("angle") || !entityEntry.key.compare("attenuation") || !entityEntry.key.compare("bounceintensity")
                        || !entityEntry.key.compare("culldist") || !entityEntry.key.compare("cut_on") || !entityEntry.key.compare("def")
                        || !entityEntry.key.compare("def_rotation") || !entityEntry.key.compare("defcube") || !entityEntry.key.compare("falloffdistance")
                        || !entityEntry.key.compare("far_edge") || !entityEntry.key.compare("fov_inner") || !entityEntry.key.compare("fov_outer")
                        || !entityEntry.key.compare("intensity") || !entityEntry.key.compare("near_edge") || !entityEntry.key.compare("pl#")
                        || !entityEntry.key.compare("priority") || !entityEntry.key.compare("radius") || !entityEntry.key.compare("roundness")
                        || !entityEntry.key.compare("shadowmap_volume") || !entityEntry.key.compare("superellipse"))
                        continue;
                }

                js[entityEntry.key] = entityEntry.value;
            }
            if (model != nullptr)
            {
                js["model"] = "*"; // special character to say that the ent uses it's children as a list of models
                if (model->surfaceSide == MSS_BOTH)
                    js["GfxAndColLinkNumber"] = entity.uniqueEntityNumber;
            }
            node.extras = js;

            size_t nodeIdx = addNodeToGltf(root, node, std::nullopt);
            entityIndexes[entity.type].emplace_back(static_cast<unsigned int>(nodeIdx));

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

        for (size_t i = 0; i < ET_COUNT; i++)
        {
            if (entityIndexes[i].empty())
                continue;

            JsonNode node;
            node.name = bspEntityTypeNames[i];
            node.children = entityIndexes[i];
            addNodeToGltf(root, node, entNodeIdx);
        }
    }

    void createAddonJson(JsonRoot& root, BSPData& dumpData, std::vector<uint8_t>& bufferData)
    {
        root.asset.version = "2.0";
        root.asset.generator = "T6-BSP-Decompiler-v0.1";

        JsonScene scene;
        scene.name = dumpData.name;
        scene.nodes.emplace_back(0);
        root.scenes.emplace();
        root.scenes->emplace_back(scene);
        root.scene = 0;

        root.nodes.emplace();
        root.meshes.emplace();

        JsonNode rootNode;
        rootNode.name = dumpData.name;
        rootNode.children.emplace();
        addNodeToGltf(root, rootNode, std::nullopt);

        CreateBufferViews(root, dumpData, bufferData, false);
        CreateMaterials(root, dumpData, false);
        createMapEnts(root, dumpData, false);
    }

    size_t createAddonModelFromIndex(size_t modelIndex, BSPData& dumpData, const AddonMapEnts* addonMapEnts)
    {
        assert(modelIndex != 0);

        BSPModel model{};

        // all model verts are in local coordinates (already cnetred around origin)
        if (addonMapEnts->models[modelIndex].surfaceCount != 0)
        {
            con::warn("Igonring addon model GFX data: index {} as it has an invalid GFX surface count (must be 0, is : {})",
                      modelIndex,
                      addonMapEnts->models[modelIndex].surfaceCount);
        }

        cLeaf_s* leaf = &addonMapEnts->cmodels[modelIndex].leaf;
        if (leaf->collAabbCount != 0)
        {
            con::warn(
                "Igonring addon model COL terrain data: index {} as it has an invalid COL terrain count (must be 0, is : {})", modelIndex, leaf->collAabbCount);
        }
        if (leaf->leafBrushNode != 0) // addons only use leafBrushNodes
        {
            model.surfaceSide = MSS_COL;
            model.surfaceType = MST_BRUSH;
            std::vector<size_t> brushList;
            getBrushesFromLeafBrushNode(addonMapEnts->info, static_cast<size_t>(leaf->leafBrushNode), brushList);
            OutSurface result;
            createSurfacesFromBrushes(addonMapEnts->info, dumpData, brushList, true, result);
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

    void dumpAddonMapEnts(BSPData& dumpData, const AddonMapEnts* addonMapEnts)
    {
        std::unique_ptr<char[]> origEntStrPtr = std::make_unique<char[]>(strlen(addonMapEnts->entityString) + 1);
        strcpy(origEntStrPtr.get(), addonMapEnts->entityString);
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
            bool isWorldspawnEnt = false;
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
                    if (!strcmp(valueStrPtr, "worldspawn"))
                        isWorldspawnEnt = true;
                    else if (starts_with(valueStrPtr, "weapon_"))
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
                    entity.modelIndex = createAddonModelFromIndex(atol(valueStrPtr + 1), dumpData, addonMapEnts);
                }
                else
                {
                    BSPEntityEntry entry = {keyStrPtr, valueStrPtr};
                    entity.entries.emplace_back(entry);
                }
            }
            if (isWorldspawnEnt)
                continue;
            dumpData.entities.emplace_back(entity);
        }
    }

    void dumpAddonMaterials(BSPData& dumpData, const AddonMapEnts* addonMapEnts)
    {
        for (unsigned int i = 0; i < addonMapEnts->info->numMaterials; i++)
        {
            auto colMaterial = &addonMapEnts->info->materials[i];
            BSPMaterial bspMaterial;
            bspMaterial.materialName = colMaterial->name;
            bspMaterial.materialType = MATERIAL_TYPE_TEXTURE;
            bspMaterial.materialColour = whiteColour;
            bspMaterial.surfaceFlags = colMaterial->surfaceFlags;
            bspMaterial.contentFlags = colMaterial->contentFlags;
            dumpData.colWorld.materials.emplace_back(bspMaterial);
        }
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
} // namespace

[[nodiscard]] std::optional<asset_type_t> AddonMapEntsDumper::DumperT6::GetHandlingAssetType() const
{
    return ASSET_TYPE_ADDON_MAP_ENTS;
}

[[nodiscard]] size_t AddonMapEntsDumper::DumperT6::GetProgressTotalCount(AssetDumpingContext& context) const
{
    return 0;
}

void AddonMapEntsDumper::DumperT6::DumpAsset(AssetDumpingContext& context, const XAssetInfo<AssetAddonMapEnts::Type>& asset)
{
    con::info("------ Addon Map Ents Dumping Started ------");

    const auto addonMapEnts = asset.Asset();

    std::vector<cmodel_t2> colModels;
    std::vector<GfxBrushModel> gfxModels;
    std::vector<cLeafBrushNode_s> leafBrushNodes;

    BSPData addonDumpData{};

    char* namePtr = _strdup(context.m_zone.m_name.c_str());
    namePtr += 3; // skip so_ part of fastfile
    char* gameModeName = namePtr;
    while (*namePtr != '_')
        namePtr++;
    *namePtr = '\0';
    namePtr++;

    addonDumpData.name = std::format("{}_{}_addons", namePtr, gameModeName);
    dumpAddonMaterials(addonDumpData, addonMapEnts);
    dumpAddonMapEnts(addonDumpData, addonMapEnts);

    con::info("------ Addon Map Ents Writing Started ------");

    JsonRoot root;
    std::vector<uint8_t> bufferData;
    createAddonJson(root, addonDumpData, bufferData);

    const auto assetFile = context.OpenAssetFile("bsp/map_col_addons.glb");
    if (!assetFile)
    {
        con::error("Unable to open addon map ents bsp output file.");
        return;
    }
    writeGltf(root, bufferData, assetFile.get());

    context.IncrementProgress();
}
