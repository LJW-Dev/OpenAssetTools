#include "BSPWriter.h"

#include "BSP/BSP.h"
#include "BSP/BSPUtil.h"
#include "BSPUnlinker.h"
#include "Game/T6/CommonT6.h"
#include "Gltf/JsonGltf.h"
#include "XModel/Gltf/GltfBinOutput.h"

#pragma warning(push, 0)
#include <Eigen>
#pragma warning(pop)

using namespace T6;
using namespace BSP;
using namespace BSPFlags;
using namespace gltf;

namespace
{
    constexpr vec4_t whiteColour = {1.0f, 1.0f, 1.0f, 1.0f};

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

    void CreateBufferViews(JsonRoot& root, BSPData& dumpData, std::vector<uint8_t>& bufferData, bool isGfxWorld)
    {
        root.bufferViews.emplace();

        if (isGfxWorld)
        {
            unsigned bufferOffset = 0u;
            JsonBufferView vertexBufferView;
            vertexBufferView.buffer = 0u;
            vertexBufferView.byteOffset = bufferOffset;
            vertexBufferView.byteStride = static_cast<unsigned>(sizeof(BSPVertex));
            vertexBufferView.byteLength = static_cast<unsigned>(sizeof(BSPVertex) * dumpData.gfxWorld.vertices.size());
            vertexBufferView.target = JsonBufferViewTarget::ARRAY_BUFFER;
            bufferOffset += vertexBufferView.byteLength;
            root.bufferViews->emplace_back(vertexBufferView);

            JsonBufferView indicesBufferView;
            indicesBufferView.buffer = 0u;
            indicesBufferView.byteOffset = bufferOffset;
            indicesBufferView.byteLength = static_cast<unsigned>(sizeof(uint16_t) * dumpData.gfxWorld.indices.size());
            indicesBufferView.target = JsonBufferViewTarget::ELEMENT_ARRAY_BUFFER;
            bufferOffset += indicesBufferView.byteLength;
            root.bufferViews->emplace_back(indicesBufferView);

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
            JsonBufferView vertexBufferView;
            vertexBufferView.buffer = 0u;
            vertexBufferView.byteOffset = bufferOffset;
            vertexBufferView.byteStride = static_cast<unsigned>(sizeof(BSPVertex));
            vertexBufferView.byteLength = static_cast<unsigned>(sizeof(BSPVertex) * dumpData.colWorld.vertices.size());
            vertexBufferView.target = JsonBufferViewTarget::ARRAY_BUFFER;
            bufferOffset += vertexBufferView.byteLength;
            root.bufferViews->emplace_back(vertexBufferView);

            JsonBufferView indicesBufferView;
            indicesBufferView.buffer = 0u;
            indicesBufferView.byteOffset = bufferOffset;
            indicesBufferView.byteLength = static_cast<unsigned>(sizeof(unsigned short) * dumpData.colWorld.indices.size());
            indicesBufferView.target = JsonBufferViewTarget::ELEMENT_ARRAY_BUFFER;
            bufferOffset += indicesBufferView.byteLength;
            root.bufferViews->emplace_back(indicesBufferView);

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
        positionAccessor.bufferView = 0;
        positionAccessor.byteOffset = (unsigned)surface.indexOfFirstVertex * (unsigned)sizeof(BSPVertex) + static_cast<unsigned>(offsetof(BSPVertex, pos));
        positionAccessor.componentType = JsonAccessorComponentType::FLOAT;
        positionAccessor.count = (unsigned int)surface.vertexCount;
        positionAccessor.type = JsonAccessorType::VEC3;
        primitive.attributes.POSITION = (unsigned)gltf.accessors->size();
        gltf.accessors->emplace_back(positionAccessor);

        JsonAccessor normalAccessor;
        normalAccessor.bufferView = 0;
        normalAccessor.byteOffset = (unsigned)surface.indexOfFirstVertex * (unsigned)sizeof(BSPVertex) + static_cast<unsigned>(offsetof(BSPVertex, normal));
        normalAccessor.componentType = JsonAccessorComponentType::FLOAT;
        normalAccessor.count = (unsigned int)surface.vertexCount;
        normalAccessor.type = JsonAccessorType::VEC3;
        primitive.attributes.NORMAL = (unsigned)gltf.accessors->size();
        gltf.accessors->emplace_back(normalAccessor);

        JsonAccessor uvAccessor;
        uvAccessor.bufferView = 0;
        uvAccessor.byteOffset = (unsigned)surface.indexOfFirstVertex * (unsigned)sizeof(BSPVertex) + static_cast<unsigned>(offsetof(BSPVertex, texCoord));
        uvAccessor.componentType = JsonAccessorComponentType::FLOAT;
        uvAccessor.count = (unsigned int)surface.vertexCount;
        uvAccessor.type = JsonAccessorType::VEC2;
        primitive.attributes.TEXCOORD_0 = (unsigned)gltf.accessors->size();
        gltf.accessors->emplace_back(uvAccessor);

        JsonAccessor colorAccessor;
        colorAccessor.bufferView = 0;
        colorAccessor.byteOffset = (unsigned)surface.indexOfFirstVertex * (unsigned)sizeof(BSPVertex) + static_cast<unsigned>(offsetof(BSPVertex, color));
        colorAccessor.componentType = JsonAccessorComponentType::FLOAT;
        colorAccessor.count = (unsigned int)surface.vertexCount;
        colorAccessor.type = JsonAccessorType::VEC4;
        primitive.attributes.COLOR_0 = (unsigned)gltf.accessors->size();
        gltf.accessors->emplace_back(colorAccessor);

        JsonAccessor indicesAccessor;
        indicesAccessor.bufferView = 1;
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

            JsonNode node{};
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
            if (entity.surfaceCount != 0)
                node.mesh = (unsigned)addMeshFromSurface(root, dumpData, entity.surfaceStart, entity.surfaceCount, false);
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

        if (xmodel.surfaceCount != 0)
            node.mesh = (unsigned)addMeshFromSurface(root, dumpData, xmodel.surfaceStart, xmodel.surfaceCount, isGfxWorld);

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
        node.children.emplace();
        size_t surfNodeIdx = addNodeToGltf(root, node, ROOT_NODE_IDX);

        nlohmann::json surfJs;

        JsonNode surfNode{};
        surfNode.name = "Lit Opaque";
        surfNode.mesh = (unsigned)addMeshFromSurface(root, dumpData, dumpData.litOpaqueSurfaceStart, dumpData.litOpaqueSurfaceCount, true);
        surfJs["type"] = "lit_opaque";
        surfNode.extras = surfJs;
        addNodeToGltf(root, surfNode, surfNodeIdx);
        surfNode.name = "Lit Transparent";
        surfNode.mesh = (unsigned)addMeshFromSurface(root, dumpData, dumpData.litTransparentSurfaceStart, dumpData.litTransparentSurfaceCount, true);
        surfJs["type"] = "lit_transparent";
        surfNode.extras = surfJs;
        addNodeToGltf(root, surfNode, surfNodeIdx);
        surfNode.name = "Emissive Opaque";
        surfNode.mesh = (unsigned)addMeshFromSurface(root, dumpData, dumpData.emissiveOpaqueSurfaceStart, dumpData.emissiveOpaqueSurfaceCount, true);
        surfJs["type"] = "emissive_opaque";
        surfNode.extras = surfJs;
        addNodeToGltf(root, surfNode, surfNodeIdx);
        surfNode.name = "Emissive Transparent";
        surfNode.mesh = (unsigned)addMeshFromSurface(root, dumpData, dumpData.emissiveTransparentSurfaceStart, dumpData.emissiveTransparentSurfaceCount, true);
        surfJs["type"] = "emissive_transparent";
        surfNode.extras = surfJs;
        addNodeToGltf(root, surfNode, surfNodeIdx);

        JsonNode xnode;
        xnode.name = "XModels";
        xnode.children.emplace();
        size_t xmodelNodeIdx = addNodeToGltf(root, xnode, ROOT_NODE_IDX);

        for (const auto& xmodel : dumpData.gfxWorld.xmodels)
            addXmodelToJson(root, dumpData, xmodel, xmodelNodeIdx, true);

        JsonNode probeParent;
        probeParent.name = "Reflection Probes";
        probeParent.children.emplace();
        size_t probeNodeIdx = addNodeToGltf(root, probeParent, ROOT_NODE_IDX);
        size_t probeidx = 0;
        for (const auto& probe : dumpData.rProbes)
        {
            nlohmann::json probeJs;
            JsonNode probeNode{};
            probeNode.name = std::format("rprobe_{}", probeidx++);
            std::array<float, 3> posArr({probe.origin.x, probe.origin.y, probe.origin.z});
            probeNode.translation = posArr;
            probeJs["rprobe"] = probe.image;
            probeNode.extras = probeJs;
            addNodeToGltf(root, probeNode, probeNodeIdx);
        }
        
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

        JsonNode ropeRoot;
        ropeRoot.name = "Ropes";
        ropeRoot.children.emplace();
        size_t ropeRootIdx = addNodeToGltf(root, ropeRoot, ROOT_NODE_IDX);
        size_t ropeIdx = 0;
        for (const auto& rope : dumpData.ropes)
        {
            vec3_t middle{};
            middle.x = (rope.endPoint.x - rope.startPoint.x) / 2.0f + rope.startPoint.x;
            middle.y = (rope.endPoint.y - rope.startPoint.y) / 2.0f + rope.startPoint.y;
            middle.z = (rope.endPoint.z - rope.startPoint.z) / 2.0f + rope.startPoint.z;

            JsonNode ropeParent{};
            ropeParent.name = std::format("rope_{}", ropeIdx++);
            ropeParent.children.emplace();
            ropeParent.translation.emplace();
            (*ropeParent.translation)[0] = middle.x;
            (*ropeParent.translation)[1] = middle.y;
            (*ropeParent.translation)[2] = middle.z;
            nlohmann::json rpJs;
            rpJs["rope"] = rope.material;
            rpJs["length"] = rope.length;
            rpJs["width"] = rope.width;
            rpJs["scale"] = rope.scale;
            ropeParent.extras = rpJs;
            size_t ropeParentIdx = addNodeToGltf(root, ropeParent, ropeRootIdx);

            JsonNode ropeChild{};
            nlohmann::json rJs;
            ropeChild.translation.emplace();
            (*ropeChild.translation)[0] = rope.startPoint.x - middle.x;
            (*ropeChild.translation)[1] = rope.startPoint.y - middle.y;
            (*ropeChild.translation)[2] = rope.startPoint.z - middle.z;
            rJs["rope"] = "start";
            ropeChild.extras = rJs;
            addNodeToGltf(root, ropeChild, ropeParentIdx);

            (*ropeChild.translation)[0] = rope.endPoint.x - middle.x;
            (*ropeChild.translation)[1] = rope.endPoint.y - middle.y;
            (*ropeChild.translation)[2] = rope.endPoint.z - middle.z;
            rJs["rope"] = "end";
            ropeChild.extras = rJs;
            addNodeToGltf(root, ropeChild, ropeParentIdx);
        }

        JsonNode pNodePar;
        pNodePar.name = "Physmodels";
        pNodePar.children.emplace();
        size_t physNodeIdx = addNodeToGltf(root, pNodePar, ROOT_NODE_IDX);
        for (const auto& BSPPhysModel : dumpData.physModels)
        {
            JsonNode pNode;
            pNode.name = "phys_" + BSPPhysModel.xmodel;
            pNode.translation.emplace();
            pNode.rotation.emplace();
            (*pNode.translation)[0] = BSPPhysModel.origin.x;
            (*pNode.translation)[1] = BSPPhysModel.origin.y;
            (*pNode.translation)[2] = BSPPhysModel.origin.z;
            (*pNode.rotation)[0] = BSPPhysModel.rotationQuat.x;
            (*pNode.rotation)[1] = BSPPhysModel.rotationQuat.y;
            (*pNode.rotation)[2] = BSPPhysModel.rotationQuat.z;
            (*pNode.rotation)[3] = BSPPhysModel.rotationQuat.w;
            nlohmann::json pJs;
            pJs["physmodel"] = BSPPhysModel.xmodel;
            pJs["physpreset"] = BSPPhysModel.physPreset;
            pJs["type"] = BSPPhysModel.type == DYNENT_TYPE_CLUTTER ? "clutter" : "destruct";
            pJs["flags"] = BSPPhysModel.flags;
            std::string contentFlags;
            for (const auto& flagToStr : BSPFlags::contentFlags_FlagToName)
            {
                if (BSPUtil::flagsMatchExact(flagToStr.first, BSPPhysModel.contentFlags))
                    contentFlags.append(std::format("{}, ", flagToStr.second));
            }
            pJs["contentflags"] = contentFlags.substr(0, contentFlags.empty() ? 0 : contentFlags.size() - 2);
            pNode.extras = pJs;
            addNodeToGltf(root, pNode, physNodeIdx);
        }

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
                bool isSolid = false;
                if (BSPUtil::flagsMatchExact(BSPFlags::contentFlags_NameToFlag.at("solid"), material.contentFlags))
                    isSolid = true;
                uniqueMaterials[material.materialName] = {isSolid, std::vector<size_t>({brushIdx})};
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

    void addLightToGltf(BSPLight* inLight, JsonPunctualLight& outLight, bool isSunlight)
    {
        std::array<float, 3> colourArr({inLight->colour.x, inLight->colour.y, inLight->colour.z});
        outLight.color = colourArr;

        if (inLight->type == LIGHT_TYPE_DIRECTIONAL)
            outLight.type = JsonPunctualLightType::DIRECTIONAL;
        else if (inLight->type == LIGHT_TYPE_POINT)
            outLight.type = JsonPunctualLightType::POINT;
        else if (inLight->type == LIGHT_TYPE_SPOT)
        {
            outLight.type = JsonPunctualLightType::SPOT;
            JsonPunctualSpotLightProperties properties;
            properties.innerConeAngle = inLight->innerConeAngle;
            properties.outerConeAngle = inLight->outerConeAngle;
            outLight.spot = properties;
        }
        else
            assert(false);

        nlohmann::json extras;
        if (!isSunlight)
        {
            outLight.intensity = inLight->intensity;
            std::array<float, 4> superEllipseArr({inLight->superEllipse.x, inLight->superEllipse.y, inLight->superEllipse.z, inLight->superEllipse.w});
            extras["superellipse"] = superEllipseArr;
            extras["culldistance"] = inLight->cullDistance;
            extras["roundness"] = inLight->roundness;
            extras["image"] = inLight->image;
            extras["range"] = inLight->range;
        }
        else
        {
            outLight.intensity = inLight->intensity * 1000.0f;
            extras["intensity"] = inLight->intensity;
        }
        outLight.extras = extras;
    }

    void createComWorld(JsonRoot& root, BSPData& dumpData, bool isGfxWorld)
    {
        if (!isGfxWorld)
            return;

        JsonExtension extension;
        JsonPunctualLightsExt punctualLightsExtension;
        punctualLightsExtension.lights = std::vector<JsonPunctualLight>();
        for (size_t lightIdx = 0; lightIdx < dumpData.lights.size() + 1; lightIdx++)
        {
            BSPLight* inLight;
            JsonPunctualLight outLight{};
            if (lightIdx == dumpData.lights.size())
                inLight = &dumpData.sunlight;
            else
                inLight = &dumpData.lights.at(lightIdx);
            addLightToGltf(inLight, outLight, lightIdx == dumpData.lights.size());
            punctualLightsExtension.lights->emplace_back(outLight);
        }
        size_t heroLightStartIdx = punctualLightsExtension.lights->size();
        for (size_t lightIdx = 0; lightIdx < dumpData.heroLights.size(); lightIdx++)
        {
            JsonPunctualLight outLight{};
            BSPLight* inLight = &dumpData.heroLights.at(lightIdx);
            addLightToGltf(inLight, outLight, true);
            punctualLightsExtension.lights->emplace_back(outLight);
        }
        extension.KHR_lights_punctual = punctualLightsExtension;
        root.extensions = extension;

        JsonNode lightNode;
        lightNode.name = "Lights";
        lightNode.children.emplace();
        size_t lightNodeIdx = addNodeToGltf(root, lightNode, ROOT_NODE_IDX);
        for (size_t lightIdx = 0; lightIdx < dumpData.lights.size() + 1; lightIdx++)
        {
            JsonNode node{};
            BSPLight* inLight;
            if (lightIdx == dumpData.lights.size())
            {
                node.name = std::format("sunlight");
                nlohmann::json extras;
                extras["sunlight"] = true;
                node.extras = extras;
                inLight = &dumpData.sunlight;
            }
            else
            {
                node.name = std::format("light_{}", lightIdx);
                inLight = &dumpData.lights.at(lightIdx);
            }
            if (inLight->isLinkedToEntity == true)
                continue;

            JsonPunctualLightIndex jsLightIndex{};
            jsLightIndex.light = static_cast<int>(lightIdx);
            JsonNodeExtension extension{};
            extension.KHR_lights_punctual = jsLightIndex;
            node.extensions = extension;

            std::array<float, 3> posArr({inLight->pos.x, inLight->pos.y, inLight->pos.z});
            node.translation = posArr;

            Eigen::Vector3f defaultDirection(0.0f, 0.0f, 1.0f);
            Eigen::Vector3f lightDirection(inLight->forwardVector.x, inLight->forwardVector.y, inLight->forwardVector.z);
            Eigen::Quaternionf forwardQuat = Eigen::Quaternionf::FromTwoVectors(defaultDirection, lightDirection);
            Eigen::AngleAxisf rollAxis(inLight->rollAngle, Eigen::Vector3f::UnitZ());
            Eigen::Quaternionf quat = forwardQuat * rollAxis;
            node.rotation = {quat.x(), quat.y(), quat.z(), quat.w()};
            addNodeToGltf(root, node, lightNodeIdx);
        }

        JsonNode hlightNode;
        hlightNode.name = "Hero Lights";
        hlightNode.children.emplace();
        size_t hlightNodeIdx = addNodeToGltf(root, hlightNode, ROOT_NODE_IDX);
        for (size_t lightIdx = 0; lightIdx < dumpData.heroLights.size(); lightIdx++)
        {
            JsonNode node{};
            BSPLight* inLight = &dumpData.heroLights.at(lightIdx);
        
            node.name = std::format("herolight_{}", lightIdx);
        
            nlohmann::json extras;
            extras["herolight"] = true;
            node.extras = extras;
        
            JsonPunctualLightIndex jsLightIndex{};
            jsLightIndex.light = static_cast<int>(lightIdx + heroLightStartIdx);
            JsonNodeExtension extension{};
            extension.KHR_lights_punctual = jsLightIndex;
            node.extensions = extension;
        
            std::array<float, 3> posArr({inLight->pos.x, inLight->pos.y, inLight->pos.z});
            node.translation = posArr;
        
            Eigen::Vector3f defaultDirection(0.0f, 0.0f, 1.0f);
            Eigen::Vector3f lightDirection(inLight->forwardVector.x, inLight->forwardVector.y, inLight->forwardVector.z);
            Eigen::Quaternionf quat = Eigen::Quaternionf::FromTwoVectors(defaultDirection, lightDirection);
            node.rotation = {quat.x(), quat.y(), quat.z(), quat.w()};
            addNodeToGltf(root, node, hlightNodeIdx);
        }
    }

    static unsigned CreateTexture(JsonRoot& gltf, const std::string& textureName)
    {
        if (!gltf.textures.has_value())
            gltf.textures.emplace();
        if (!gltf.images.has_value())
            gltf.images.emplace();

        auto uri = std::format("../images/{}.dds", textureName);

        auto existingTexIndex = 0u;
        for (const auto& existingTex : gltf.textures.value())
        {
            const auto& existingImage = gltf.images.value()[existingTex.source];

            if (existingImage.uri == uri)
                return existingTexIndex;

            existingTexIndex++;
        }

        JsonImage image;
        image.uri = std::move(uri);
        image.mimeType = "image/dds";
        const auto imageIndex = static_cast<unsigned>(gltf.images->size());
        gltf.images->emplace_back(std::move(image));

        JsonTexture texture;
        texture.source = imageIndex;
        const auto textureIndex = static_cast<unsigned>(gltf.textures->size());
        gltf.textures->emplace_back(texture);

        return textureIndex;
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

            if (!mat.imageName.empty())
            {
                material.pbrMetallicRoughness->baseColorTexture.emplace();
                material.pbrMetallicRoughness->baseColorTexture->index = CreateTexture(root, mat.imageName);
            }

            if (!mat.normalName.empty())
                material.normalTexture.emplace().index = CreateTexture(root, mat.normalName);

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

    void createJsonHeader(JsonRoot& root, std::string& bspName, bool isGfxWorld)
    {
        root.asset.version = "2.0";
        root.asset.generator = "OAT-T6-BSP-Decompiler-v2.0";

        if (isGfxWorld)
        {
            root.extensionsUsed = std::vector<std::string>({"KHR_lights_punctual"});
            root.extensionsRequired = std::vector<std::string>({"KHR_lights_punctual"});
        }

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
        rootNode.name = scene.name;
        rootNode.children.emplace();
        addNodeToGltf(root, rootNode, std::nullopt);
    }

    void createJson(JsonRoot& root, BSPData& dumpData, std::vector<uint8_t>& bufferData, bool isGfxWorld)
    {
        createJsonHeader(root, dumpData.name, isGfxWorld);
        CreateBufferViews(root, dumpData, bufferData, isGfxWorld);

        CreateMaterials(root, dumpData, isGfxWorld);

        createComWorld(root, dumpData, isGfxWorld);
        createGfxWorld(root, dumpData, isGfxWorld);
        createColWorld(root, dumpData, isGfxWorld);
        createMapEnts(root, dumpData, isGfxWorld);
    }
} // namespace

void T6::BSP::writeDumpDataToGltf(BSPData dumpData, const std::unique_ptr<std::ostream>& gfxFile, const std::unique_ptr<std::ostream>& colFile)
{
    { // gfx
        JsonRoot root;
        std::vector<uint8_t> bufferData;
        createJson(root, dumpData, bufferData, true);
        writeGltf(root, bufferData, gfxFile.get());
    }
    { // collision
        JsonRoot root;
        std::vector<uint8_t> bufferData;
        createJson(root, dumpData, bufferData, false);
        writeGltf(root, bufferData, colFile.get());
    }
}
