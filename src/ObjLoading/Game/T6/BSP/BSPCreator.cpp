#include "BSPCreator.h"

#include "BSP/BSPUtil.h"
#include "Gltf/JsonGltf.h"
#include "Utils/StringUtils.h"
#include "XModel/Gltf/GltfBinInput.h"
#include "XModel/Gltf/GltfTextInput.h"
#include "XModel/Gltf/Internal/GltfAccessor.h"
#include "XModel/Gltf/Internal/GltfBuffer.h"
#include "XModel/Gltf/Internal/GltfBufferView.h"
#include "XModel/Tangentspace.h"

#pragma warning(push, 0)
#include <Eigen>
#pragma warning(pop)

#include <deque>
#include <exception>
#include <format>
#include <iostream>
#include <limits>
#include <numbers>
#include <string>

using namespace T6;
using namespace BSP;
using namespace BSPFlags;
using namespace gltf;

namespace
{
    struct AccessorsForVertex
    {
        unsigned m_position_accessor;
        unsigned m_normal_accessor;
        std::optional<unsigned> m_color_accessor;
        std::optional<unsigned> m_uv_accessor;
        unsigned m_index_accessor;
    };

    void RhcToLhcQuaternion(float (&coords)[4])
    {
        const float two[4]{coords[0], coords[1], coords[2], coords[3]};

        coords[0] = two[0];
        coords[1] = -two[2];
        coords[2] = two[1];
        coords[3] = two[3];
    }

    void RhcToLhcCoordinates(float (&coords)[3])
    {
        const float two[3]{coords[0], coords[1], coords[2]};

        coords[0] = two[0];
        coords[1] = -two[2];
        coords[2] = two[1];
    }

    void RhcToLhcIndices(unsigned (&indices)[3])
    {
        const unsigned two[3]{indices[0], indices[1], indices[2]};

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

    bool convertStringToFlags(const std::string& flagStr, int& surfaceFlags, int& contentFlags)
    {
        surfaceFlags = 0;
        contentFlags = 1;
        bool matchedAnyFlag = false;
        std::vector<std::string> flagStrVec = utils::StringSplit(flagStr, ',');
        for (std::string& flag : flagStrVec)
        {
            utils::MakeStringLowerCase(flag);
            utils::StringTrim(flag);
            for (size_t typeIdx = 0; typeIdx < BSP_SURF_TYPE_COUNT; typeIdx++)
            {
                if (!flag.compare(surfaceTypeToNameMap[typeIdx]))
                {
                    s_SurfaceTypeFlags flags = surfaceTypeToFlagMap[typeIdx];
                    surfaceFlags |= flags.surfaceFlags;
                    contentFlags |= flags.contentFlags;
                    matchedAnyFlag = true;

                    if (typeIdx == BSP_SURF_TYPE_NONSOLID)
                        contentFlags &= 0xFFFFFFFE;

                    break;
                }
            }
        }

        return matchedAnyFlag;
    }

    class GltfLoadException final : std::exception
    {
    public:
        explicit GltfLoadException(std::string message)
            : m_message(std::move(message))
        {
        }

        [[nodiscard]] const std::string& Str() const
        {
            return m_message;
        }

        [[nodiscard]] const char* what() const noexcept override
        {
            return m_message.c_str();
        }

    private:
        std::string m_message;
    };

    class BSPLoader
    {
    private:
        BSPData* m_bsp;
        BSPWorld* m_curr_bsp_world;
        bool m_is_world_gfx;
        size_t m_emptyMaterialIndex;
        std::map<size_t, size_t> gfxToColModelLinkMap; // key: unique entity number, value: model index

        std::vector<std::unique_ptr<Accessor>> m_accessors;
        std::vector<std::unique_ptr<BufferView>> m_buffer_views;
        std::vector<std::unique_ptr<Buffer>> m_buffers;

        std::string getWorldTypeName()
        {
            return m_is_world_gfx ? "gfx" : "col";
        }

        std::optional<Accessor*> GetAccessorForIndex(const char* attributeName,
                                                     const std::optional<unsigned> index,
                                                     std::initializer_list<JsonAccessorType> allowedAccessorTypes,
                                                     std::initializer_list<JsonAccessorComponentType> allowedAccessorComponentTypes) const
        {
            if (!index)
                return std::nullopt;

            if (*index > m_accessors.size())
                throw GltfLoadException(std::format("Index for {} accessor out of bounds", attributeName));

            auto* accessor = m_accessors[*index].get();

            const auto maybeType = accessor->GetType();
            if (maybeType)
            {
                if (std::ranges::find(allowedAccessorTypes, *maybeType) == allowedAccessorTypes.end())
                    throw GltfLoadException(std::format("Accessor for {} has unsupported type {}", attributeName, static_cast<unsigned>(*maybeType)));
            }

            const auto maybeComponentType = accessor->GetComponentType();
            if (maybeComponentType)
            {
                if (std::ranges::find(allowedAccessorComponentTypes, *maybeComponentType) == allowedAccessorComponentTypes.end())
                    throw GltfLoadException(
                        std::format("Accessor for {} has unsupported component type {}", attributeName, static_cast<unsigned>(*maybeComponentType)));
            }

            return accessor;
        }

        static void VerifyAccessorVertexCount(const char* accessorType, const Accessor* accessor, const size_t vertexCount)
        {
            if (accessor->GetCount() != vertexCount)
                throw GltfLoadException(std::format("Element count of {} accessor does not match expected vertex count of {}", accessorType, vertexCount));
        }

        Eigen::Matrix4f createNodeMatrix(const gltf::JsonNode& node)
        {
            if (node.matrix)
                return Eigen::Matrix4f({
                    {(*node.matrix)[0], (*node.matrix)[4], (*node.matrix)[8],  (*node.matrix)[12]},
                    {(*node.matrix)[1], (*node.matrix)[5], (*node.matrix)[9],  (*node.matrix)[13]},
                    {(*node.matrix)[2], (*node.matrix)[6], (*node.matrix)[10], (*node.matrix)[14]},
                    {(*node.matrix)[3], (*node.matrix)[7], (*node.matrix)[11], (*node.matrix)[15]}
                });

            float localTranslation[3];
            float localRotation[4];
            float localScale[3];
            if (node.translation)
            {
                localTranslation[0] = (*node.translation)[0];
                localTranslation[1] = (*node.translation)[1];
                localTranslation[2] = (*node.translation)[2];
            }
            else
            {
                localTranslation[0] = 0.0f;
                localTranslation[1] = 0.0f;
                localTranslation[2] = 0.0f;
            }

            if (node.rotation)
            {
                localRotation[0] = (*node.rotation)[0];
                localRotation[1] = (*node.rotation)[1];
                localRotation[2] = (*node.rotation)[2];
                localRotation[3] = (*node.rotation)[3];
            }
            else
            {
                localRotation[0] = 0.0f;
                localRotation[1] = 0.0f;
                localRotation[2] = 0.0f;
                localRotation[3] = 1.0f;
            }

            if (node.scale)
            {
                localScale[0] = (*node.scale)[0];
                localScale[1] = (*node.scale)[1];
                localScale[2] = (*node.scale)[2];
            }
            else
            {
                localScale[0] = 1.0f;
                localScale[1] = 1.0f;
                localScale[2] = 1.0f;
            }

            Eigen::Vector3f translation(localTranslation[0], localTranslation[1], localTranslation[2]);
            Eigen::Quaternionf rotation(localRotation[3], localRotation[0], localRotation[1], localRotation[2]); // GLTF is XYZW, Eigen constructor is WXYZ
            Eigen::Vector3f scale(localScale[0], localScale[1], localScale[2]);

            Eigen::Transform<float, 3, Eigen::Affine> T;
            T = T.fromPositionOrientationScale(translation, rotation, scale);
            return T.matrix();
        }

        unsigned CreateSurface(const AccessorsForVertex& accessorsForVertex,
                               const Eigen::Matrix4f& nodeMatrix,
                               size_t materialIndex,
                               bool convertWorldToLocalPos,
                               const std::string& nodeName)
        {
            // clang-format off
            const auto* positionAccessor = GetAccessorForIndex(
                "POSITION",
                accessorsForVertex.m_position_accessor,
                { JsonAccessorType::VEC3 },
                { JsonAccessorComponentType::FLOAT }
            ).value_or(nullptr);
            // clang-format on
            assert(positionAccessor != nullptr);

            const auto vertexCount = positionAccessor->GetCount();
            NullAccessor nullAccessor(vertexCount);
            OnesAccessor onesAccessor(vertexCount);

            // clang-format off
            const auto* normalAccessor = GetAccessorForIndex(
                "NORMAL",
                accessorsForVertex.m_normal_accessor,
                { JsonAccessorType::VEC3 },
                { JsonAccessorComponentType::FLOAT }
            ).value_or(nullptr);
            VerifyAccessorVertexCount("NORMAL", normalAccessor, vertexCount);
            assert(normalAccessor != nullptr);

            const auto* uvAccessor = GetAccessorForIndex(
                "TEXCOORD_0",
                accessorsForVertex.m_uv_accessor,
                { JsonAccessorType::VEC2 },
                { JsonAccessorComponentType::FLOAT, JsonAccessorComponentType::UNSIGNED_BYTE, JsonAccessorComponentType::UNSIGNED_SHORT }
            ).value_or(&nullAccessor);
            VerifyAccessorVertexCount("TEXCOORD_0", uvAccessor, vertexCount);

            const auto* colorAccessor = GetAccessorForIndex(
                "COLOR_0",
                accessorsForVertex.m_color_accessor,
                { JsonAccessorType::VEC3, JsonAccessorType::VEC4 },
                { JsonAccessorComponentType::FLOAT, JsonAccessorComponentType::UNSIGNED_BYTE, JsonAccessorComponentType::UNSIGNED_SHORT }
            ).value_or(&onesAccessor);
            VerifyAccessorVertexCount("COLOR_0", colorAccessor, vertexCount);

            const auto* indexAccessor = GetAccessorForIndex(
                "INDICES",
                accessorsForVertex.m_index_accessor,
                { JsonAccessorType::SCALAR },
                { JsonAccessorComponentType::UNSIGNED_BYTE, JsonAccessorComponentType::UNSIGNED_SHORT, JsonAccessorComponentType::UNSIGNED_INT }
            ).value_or(nullptr);
            assert(indexAccessor != nullptr);
            // clang-format on

            const auto indexCount = indexAccessor->GetCount();
            if (indexCount % 3 != 0)
                throw GltfLoadException("Index count must be dividable by 3 for triangles");
            const auto faceCount = indexCount / 3u;
            if (faceCount > UINT16_MAX)
                throw GltfLoadException(std::format("Face count ({}) on node {} exceeded the UINT16_MAX", faceCount, nodeName));
            if (vertexCount > UINT16_MAX)
                throw GltfLoadException(std::format("Vertex count ({}) on node {} exceeded the UINT16_MAX", vertexCount, nodeName));

            Eigen::Vector4f tempPosition(0, 0, 0, 1.0f);
            Eigen::Vector4f transformedPosition = nodeMatrix * tempPosition;
            vec3_t surfaceOrigin;
            surfaceOrigin.x = transformedPosition.x();
            surfaceOrigin.y = transformedPosition.y();
            surfaceOrigin.z = transformedPosition.z();
            RhcToLhcCoordinates(surfaceOrigin.v);

            BSPSurface out_surface;
            out_surface.isLocalCoords = convertWorldToLocalPos;
            out_surface.origin = surfaceOrigin;
            out_surface.vertexCount = static_cast<uint16_t>(vertexCount);
            out_surface.triCount = static_cast<uint16_t>(faceCount);
            out_surface.indexOfFirstIndex = static_cast<int>(m_curr_bsp_world->indices.size());
            out_surface.indexOfFirstVertex = static_cast<int>(m_curr_bsp_world->vertices.size());
            out_surface.materialIndex = materialIndex;
            vec4_t materialColor = m_curr_bsp_world->materials.at(materialIndex).materialColour;
            m_curr_bsp_world->surfaces.emplace_back(out_surface);

            for (auto faceIndex = 0u; faceIndex < faceCount; faceIndex++)
            {
                unsigned indices[3];
                if (!indexAccessor->GetUnsigned(faceIndex * 3u + 0u, indices[0]) || !indexAccessor->GetUnsigned(faceIndex * 3u + 1u, indices[1])
                    || !indexAccessor->GetUnsigned(faceIndex * 3u + 2u, indices[2]))
                {
                    assert(false);
                }
                if (indices[0] > UINT16_MAX || indices[1] > UINT16_MAX || indices[2] > UINT16_MAX)
                    throw GltfLoadException("Index number exceeded the UINT16_MAX");

                RhcToLhcIndices(indices);
                m_curr_bsp_world->indices.emplace_back(static_cast<uint16_t>(indices[0]));
                m_curr_bsp_world->indices.emplace_back(static_cast<uint16_t>(indices[1]));
                m_curr_bsp_world->indices.emplace_back(static_cast<uint16_t>(indices[2]));
            }

            const auto vertexOffset = static_cast<unsigned>(m_curr_bsp_world->vertices.size());
            for (auto vertexIndex = 0u; vertexIndex < vertexCount; vertexIndex++)
            {
                BSPVertex vertex{};

                if (!positionAccessor->GetFloatVec3(vertexIndex, vertex.pos.v))
                    assert(false);
                if (!normalAccessor->GetFloatVec3(vertexIndex, vertex.normal.v))
                    assert(false);
                std::optional<JsonAccessorType> colourType = colorAccessor->GetType();
                if (!colourType.has_value())
                {
                    if (!colorAccessor->GetFloatVec4(vertexIndex, vertex.color.v))
                        assert(false);
                }
                else if (*colourType == JsonAccessorType::VEC4)
                {
                    if (!colorAccessor->GetFloatVec4(vertexIndex, vertex.color.v))
                        assert(false);
                }
                else if (*colourType == JsonAccessorType::VEC3)
                {
                    vec3_t colorout{};
                    if (!colorAccessor->GetFloatVec3(vertexIndex, colorout.v))
                        assert(false);
                    vertex.color.x = colorout.x;
                    vertex.color.y = colorout.y;
                    vertex.color.z = colorout.z;
                    vertex.color.w = 1.0f;
                }
                else
                    assert(false);
                if (!uvAccessor->GetFloatVec2(vertexIndex, vertex.texCoord.v))
                    assert(false);

                vertex.color.x *= materialColor.x;
                vertex.color.y *= materialColor.y;
                vertex.color.z *= materialColor.z;
                vertex.color.w *= materialColor.w;

                Eigen::Vector4f position(vertex.pos.x, vertex.pos.y, vertex.pos.z, 1.0f);
                Eigen::Vector4f transformedPosition = nodeMatrix * position;
                vertex.pos.x = transformedPosition.x();
                vertex.pos.y = transformedPosition.y();
                vertex.pos.z = transformedPosition.z();

                RhcToLhcCoordinates(vertex.pos.v);
                RhcToLhcCoordinates(vertex.normal.v);

                if (convertWorldToLocalPos)
                {
                    vertex.pos.x -= surfaceOrigin.x;
                    vertex.pos.y -= surfaceOrigin.y;
                    vertex.pos.z -= surfaceOrigin.z;
                }

                m_curr_bsp_world->vertices.emplace_back(vertex);
            }

            // generate tangent and binormal vectors
            tangent_space::VertexData vertexData{
                &m_curr_bsp_world->vertices[out_surface.indexOfFirstVertex].pos,
                sizeof(BSPVertex),
                &m_curr_bsp_world->vertices[out_surface.indexOfFirstVertex].normal,
                sizeof(BSPVertex),
                &m_curr_bsp_world->vertices[out_surface.indexOfFirstVertex].texCoord,
                sizeof(BSPVertex),
                &m_curr_bsp_world->vertices[out_surface.indexOfFirstVertex].tangent,
                sizeof(BSPVertex),
                &m_curr_bsp_world->vertices[out_surface.indexOfFirstVertex].binormal,
                sizeof(BSPVertex),
                &m_curr_bsp_world->indices[out_surface.indexOfFirstIndex],
            };
            tangent_space::CalculateTangentSpace(vertexData, faceCount, vertexCount);

            return vertexOffset;
        }

        bool addLightNode(const JsonRoot& jRoot, const gltf::JsonNode& node, const Eigen::Matrix4f& nodeMatrix, bool isEntityLight)
        {
            if (!m_is_world_gfx || !jRoot.extensions || !jRoot.extensions->KHR_lights_punctual || !jRoot.extensions->KHR_lights_punctual->lights)
                return false;

            int lightIndex = node.extensions->KHR_lights_punctual->light;
            const JsonPunctualLight& jsLight = jRoot.extensions->KHR_lights_punctual->lights->at(lightIndex);
            BSPLight light{};

            if (jsLight.type == JsonPunctualLightType::DIRECTIONAL)
            {
                light.type = LIGHT_TYPE_DIRECTIONAL;
            }
            else if (jsLight.type == JsonPunctualLightType::POINT)
            {
                light.type = LIGHT_TYPE_POINT;
            }
            else if (jsLight.type == JsonPunctualLightType::SPOT)
            {
                light.type = LIGHT_TYPE_SPOT;

                assert(jsLight.spot);
                if (jsLight.spot->innerConeAngle)
                    light.innerConeAngle = *jsLight.spot->innerConeAngle;
                else
                    light.innerConeAngle = 0.0f;

                if (jsLight.spot->outerConeAngle)
                    light.outerConeAngle = *jsLight.spot->outerConeAngle;
                else
                    light.outerConeAngle = std::numbers::pi_v<float> / 4.0f; /// spec of 45 degrees
            }
            else
                assert(false);

            if (!jsLight.color)
            {
                light.colour.x = 1.0f;
                light.colour.y = 1.0f;
                light.colour.z = 1.0f;
            }
            else
            {
                light.colour.x = (*jsLight.color)[0];
                light.colour.y = (*jsLight.color)[1];
                light.colour.z = (*jsLight.color)[2];
            }

            Eigen::Vector3f defaultDirection(0.0f, 0.0f, 1.0f); // gltf default light spec is straight down (0, 0, -1) but bo2's is straight up (0, 0, 1)
            Eigen::Affine3f affineTransform(nodeMatrix);
            Eigen::Matrix3f rotationMatrix = affineTransform.rotation();
            Eigen::Vector3f outputDirection = rotationMatrix * defaultDirection;
            outputDirection.normalize();
            light.forwardVector.x = outputDirection.x();
            light.forwardVector.y = outputDirection.y();
            light.forwardVector.z = outputDirection.z();
            RhcToLhcCoordinates(light.forwardVector.v);
            Eigen::Vector3f eigenEulerAngles = rotationMatrix.canonicalEulerAngles(2, 1, 0);
            vec3_t eulerAngles = {eigenEulerAngles.x(), eigenEulerAngles.y(), eigenEulerAngles.z()};
            RhcToLhcCoordinates(eulerAngles.v);
            light.rollAngle = eulerAngles.z;

            if (!jsLight.intensity)
                light.intensity = 10000.0f; // adjusted from spec to better match BO2
            else
                light.intensity = *jsLight.intensity;
            if (light.intensity < 0.0f)
                throw GltfLoadException(std::format("light intensity must be positive"));

            bool isSunlight = false;
            if (node.extras && node.extras->contains("sunlight"))
            {
                nlohmann::json isSunlightJs = node.extras->at("sunlight");
                if (!isSunlightJs.is_boolean())
                    throw GltfLoadException("Sunlight property must be a boolean");
                isSunlight = isSunlightJs;
            }
            if (isSunlight)
            {
                if (isEntityLight)
                    throw GltfLoadException("Sunlight cannot be an entity as well");
                if (light.type != LIGHT_TYPE_DIRECTIONAL)
                    throw GltfLoadException("Sunlight must be a sun/directional light");
                if (m_bsp->hasSunlightBeenSet)
                    throw GltfLoadException("Multiple sunlights found");
                if (!jsLight.intensity)
                    light.intensity = 1000.0f;
                m_bsp->sunlight = light;
                m_bsp->hasSunlightBeenSet = true;
            }
            else
            {
                Eigen::Vector4f position(0, 0, 0, 1.0f);
                Eigen::Vector4f transformedPosition = nodeMatrix * position;
                light.pos = vec3_t{transformedPosition.x(), transformedPosition.y(), transformedPosition.z()};
                RhcToLhcCoordinates(light.pos.v);

                if (jsLight.extras && jsLight.extras->contains("range"))
                {
                    nlohmann::json rangeJs = jsLight.extras->at("range");
                    if (rangeJs.is_string())
                    {
                        std::string rangeStr = rangeJs;
                        light.range = static_cast<float>(atof(rangeStr.c_str()));
                    }
                    else if (rangeJs.is_number())
                        light.range = rangeJs;
                    else
                        assert(false);
                }
                else
                    light.range = sqrtf(light.intensity) / 2.0f; // how most light ranges in BO2 are calculated
                if (light.range < 0.0f)
                    throw GltfLoadException(std::format("light range must be positive"));

                if (jsLight.extras && jsLight.extras->contains("superellipse"))
                {
                    nlohmann::json ellipseJs = jsLight.extras->at("superellipse");
                    if (!ellipseJs.is_array() || ellipseJs.size() != 4)
                        throw GltfLoadException(std::format("light superellipse must be a vec4"));

                    std::array<float, 4> superEllipseArr = ellipseJs;
                    light.superEllipse.x = superEllipseArr[0];
                    light.superEllipse.y = superEllipseArr[1];
                    light.superEllipse.z = superEllipseArr[2];
                    light.superEllipse.w = superEllipseArr[3];
                    if (light.superEllipse.x < 0.0f || light.superEllipse.x > 1.0f || light.superEllipse.y < 0.0f || light.superEllipse.y > 1.0f
                        || light.superEllipse.z < 0.0f || light.superEllipse.z > 1.0f || light.superEllipse.w < 0.0f || light.superEllipse.w > 1.0f)
                        throw GltfLoadException(std::format("light superellipse values must be between 0.0 and 1.0"));
                }
                else
                    light.superEllipse = {0.75f, 1.0f, 0.75f, 1.0f}; // creates a circular light

                if (jsLight.extras && jsLight.extras->contains("culldistance"))
                {
                    nlohmann::json cullDistanceJs = jsLight.extras->at("culldistance");
                    if (cullDistanceJs.is_string())
                    {
                        std::string cullDistanceStr = cullDistanceJs;
                        int cullDist = atoi(cullDistanceStr.c_str());
                        if (cullDist < 0 || cullDist > INT16_MAX)
                            throw GltfLoadException(std::format("light cullDist is less than 0 or greater than {}", INT16_MAX));

                        light.cullDistance = static_cast<size_t>(cullDist);
                    }
                    else if (cullDistanceJs.is_number())
                    {
                        int cullDist = cullDistanceJs;
                        if (cullDist < 0 || cullDist > INT16_MAX)
                            throw GltfLoadException(std::format("light cullDist is less than 0 or greater than {}", INT16_MAX));
                        light.cullDistance = static_cast<size_t>(cullDist);
                    }
                    else
                        assert(false);
                }
                else
                    light.cullDistance = 1000;

                if (jsLight.extras && jsLight.extras->contains("roundness"))
                {
                    nlohmann::json roundnessJs = jsLight.extras->at("roundness");
                    if (roundnessJs.is_string())
                    {
                        std::string roundnessStr = roundnessJs;
                        light.roundness = static_cast<float>(atof(roundnessStr.c_str()));
                    }
                    else if (roundnessJs.is_number())
                        light.roundness = roundnessJs;
                    else
                        assert(false);
                }
                else
                    light.roundness = 1.0f;
                if (light.roundness < 0.0f || light.roundness > 1.0f)
                    throw GltfLoadException(std::format("light roundness must be between 0.0 and 1.0"));

                if (jsLight.extras && jsLight.extras->contains("image"))
                    light.image = jsLight.extras->at("image");
                else
                    light.image = "";

                m_bsp->lights.emplace_back(light);
            }

            return true;
        }

        bool addMeshNode(const JsonRoot& jRoot, const gltf::JsonNode& node, const Eigen::Matrix4f& nodeMatrix, bool convertWorldToLocalPos)
        {
            assert(node.mesh);
            assert(jRoot.meshes);

            const auto& mesh = jRoot.meshes.value()[node.mesh.value()];
            for (const auto& primitive : mesh.primitives)
            {
                if (!primitive.indices)
                    throw GltfLoadException("Requires primitives indices");
                if (primitive.mode.value_or(JsonMeshPrimitivesMode::TRIANGLES) != JsonMeshPrimitivesMode::TRIANGLES)
                    throw GltfLoadException("Only triangles are supported");
                if (!primitive.attributes.POSITION)
                    throw GltfLoadException("Requires primitives attribute POSITION");
                if (!primitive.attributes.NORMAL)
                    throw GltfLoadException("Requires primitives attribute NORMAL");

                const AccessorsForVertex accessorsForVertex{
                    .m_position_accessor = *primitive.attributes.POSITION,
                    .m_normal_accessor = *primitive.attributes.NORMAL,
                    .m_color_accessor = primitive.attributes.COLOR_0,
                    .m_uv_accessor = primitive.attributes.TEXCOORD_0,
                    .m_index_accessor = *primitive.indices,
                };

                size_t materialIndex;
                if (primitive.material)
                    materialIndex = *primitive.material;
                else
                    materialIndex = m_emptyMaterialIndex;

                CreateSurface(accessorsForVertex, nodeMatrix, materialIndex, convertWorldToLocalPos, node.name.value_or("unnamed node"));
            }

            return true;
        }

        void calculateXmodelBounds(BSPXModel& xmodel, std::optional<int> meshIndex, const Eigen::Matrix4f& nodeMatrix, const JsonRoot& jRoot)
        {
            if (meshIndex)
            {
                xmodel.areBoundsValid = true;
                Eigen::Vector4f position(0, 0, 0, 1.0f);
                Eigen::Vector4f transformedPosition = nodeMatrix * position;
                xmodel.mins.x = transformedPosition.x();
                xmodel.mins.y = transformedPosition.y();
                xmodel.mins.z = transformedPosition.z();
                xmodel.maxs.x = transformedPosition.x();
                xmodel.maxs.y = transformedPosition.y();
                xmodel.maxs.z = transformedPosition.z();
                RhcToLhcCoordinates(xmodel.mins.v);
                RhcToLhcCoordinates(xmodel.maxs.v);

                const auto& mesh = jRoot.meshes.value()[*meshIndex];
                for (size_t primIdx = 0; primIdx < mesh.primitives.size(); primIdx++)
                {
                    const auto& primitive = mesh.primitives.at(primIdx);

                    if (!primitive.attributes.POSITION)
                        throw GltfLoadException("Requires primitives attribute POSITION");

                    // clang-format off
                     const auto* positionAccessor = GetAccessorForIndex(
                         "POSITION",
                         primitive.attributes.POSITION,
                         { JsonAccessorType::VEC3 },
                         { JsonAccessorComponentType::FLOAT }
                     ).value_or(nullptr);
                    // clang-format on
                    assert(positionAccessor != nullptr);

                    for (size_t vertexIndex = 0u; vertexIndex < positionAccessor->GetCount(); vertexIndex++)
                    {
                        vec3_t vertex;
                        if (!positionAccessor->GetFloatVec3(vertexIndex, vertex.v))
                            assert(false);

                        Eigen::Vector4f position(vertex.x, vertex.y, vertex.z, 1.0f);
                        Eigen::Vector4f transformedPosition = nodeMatrix * position;
                        vertex.x = transformedPosition.x();
                        vertex.y = transformedPosition.y();
                        vertex.z = transformedPosition.z();
                        RhcToLhcCoordinates(vertex.v);

                        if (vertexIndex == 0 && primIdx == 0)
                        {
                            xmodel.mins = vertex;
                            xmodel.maxs = vertex;
                        }
                        else
                            BSPUtil::updateAABBWithPoint(vertex, xmodel.mins, xmodel.maxs);
                    }
                }
            }
            else
            {
                xmodel.areBoundsValid = false;
                xmodel.mins.x = 0.0f;
                xmodel.mins.y = 0.0f;
                xmodel.mins.z = 0.0f;
                xmodel.maxs.x = 0.0f;
                xmodel.maxs.y = 0.0f;
                xmodel.maxs.z = 0.0f;
            }
        }

        float lengthOfVector(float x, float y, float z)
        {
            return sqrtf(x * x + y * y + z * z);
        }

        vec3_t getScaleFromMatrix(const Eigen::Matrix4f& matrix)
        {
            const auto& col0 = matrix.col(0);
            const auto& col1 = matrix.col(1);
            const auto& col2 = matrix.col(2);

            return {lengthOfVector(col0.x(), col0.y(), col0.z()), lengthOfVector(col1.x(), col1.y(), col1.z()), lengthOfVector(col2.x(), col2.y(), col2.z())};
        }

        bool addXModelNode(const JsonRoot& jRoot, const gltf::JsonNode& node, const Eigen::Matrix4f& nodeMatrix)
        {
            assert(node.extras);
            assert(node.extras->contains("xmodel"));

            BSPXModel xmodel;
            xmodel.name = node.extras->at("xmodel");

            xmodel.doesCastShadow = true;
            if (node.extras && node.extras->contains("flags"))
            {
                std::string flagStr = node.extras->at("flags");
                xmodel.doesCastShadow = !flagStr.contains("nocastshadow");
            }

            Eigen::Vector4f position(0, 0, 0, 1.0f);
            Eigen::Vector4f transformedPosition = nodeMatrix * position;
            xmodel.origin.x = transformedPosition.x();
            xmodel.origin.y = transformedPosition.y();
            xmodel.origin.z = transformedPosition.z();
            RhcToLhcCoordinates(xmodel.origin.v);

            Eigen::Affine3f affineTransform(nodeMatrix);
            Eigen::Quaternionf rotationQuat(affineTransform.rotation());
            rotationQuat.normalize();
            xmodel.rotationQuaternion.x = rotationQuat.x();
            xmodel.rotationQuaternion.y = rotationQuat.y();
            xmodel.rotationQuaternion.z = rotationQuat.z();
            xmodel.rotationQuaternion.w = rotationQuat.w();
            RhcToLhcQuaternion(xmodel.rotationQuaternion.v);

            vec3_t mScale = getScaleFromMatrix(nodeMatrix);
            xmodel.scale.x = std::round(mScale.x * 1000.0f) / 1000.0f;
            xmodel.scale.y = std::round(mScale.y * 1000.0f) / 1000.0f;
            xmodel.scale.z = std::round(mScale.z * 1000.0f) / 1000.0f;

            calculateXmodelBounds(xmodel, node.mesh, nodeMatrix, jRoot);

            m_curr_bsp_world->xmodels.emplace_back(xmodel);

            return true;
        }

        bool addSpawnPointNode(const gltf::JsonNode& node, const Eigen::Matrix4f& nodeMatrix)
        {
            if (m_is_world_gfx)
                return true;

            assert(node.extras);
            assert(node.extras->contains("spawnpoint"));

            Eigen::Vector4f position(0, 0, 0, 1.0f);
            Eigen::Vector4f transformedPosition = nodeMatrix * position;
            vec3_t origin;
            origin.x = transformedPosition.x();
            origin.y = transformedPosition.y();
            origin.z = transformedPosition.z();
            RhcToLhcCoordinates(origin.v);

            // GLTF default direction is +Y up
            Eigen::Vector3f defaultDirection(0.0f, 1.0f, 0.0f);
            Eigen::Affine3f affineTransform(nodeMatrix);
            Eigen::Matrix3f rotationMatrix = affineTransform.rotation();
            Eigen::Vector3f outputDirection = rotationMatrix * defaultDirection;
            outputDirection.normalize();
            vec3_t forward;
            forward.x = outputDirection.x();
            forward.y = outputDirection.y();
            forward.z = outputDirection.z();
            RhcToLhcCoordinates(forward.v);

            std::string team = node.extras->at("spawnpoint");
            if (!m_bsp->isZombiesMap && team.compare("attacker") && team.compare("defender") && team.compare("all"))
            {
                con::warn("Ignoring spawn point with an invalid type (must be attacker, defender or all)");
                return false;
            }

            BSPSpawnPoint spawnPoint;
            spawnPoint.origin = origin;
            spawnPoint.forward = forward;
            spawnPoint.spawnpointGroupName = team;
            m_bsp->spawnpoints.emplace_back(spawnPoint);

            return true;
        }

        size_t addScriptModel(const JsonRoot& jRoot,
                              const std::optional<std::vector<unsigned>>& modelNodes,
                              const Eigen::Matrix4f& parentEntityMatrix,
                              const gltf::JsonNode& parentNode,
                              std::optional<size_t> gfxAndColLinkNum)
        {
            if (!modelNodes && m_is_world_gfx)
                throw GltfLoadException("GFX Script Model was made with no children");
            if (!modelNodes && !gfxAndColLinkNum && !m_is_world_gfx)
                throw GltfLoadException(std::format("COL Script Model (node: {}) was made with no children and has no gfxAndColLinkNumber",
                                                    parentNode.name.value_or("unnamed node")));

            std::vector<std::pair<const JsonNode*, Eigen::Matrix4f>> terrainNodes;
            std::vector<std::pair<const JsonNode*, Eigen::Matrix4f>> brushNodes;
            if (modelNodes)
            {
                for (unsigned nodeIdx : *modelNodes)
                {
                    const JsonNode& node = jRoot.nodes->at(nodeIdx);

                    if (!node.extras || !node.extras->contains("model"))
                        throw GltfLoadException(std::format("Script Model child {} has no model field", *node.name));

                    Eigen::Matrix4f nodeMatrix = createNodeMatrix(node);
                    Eigen::Matrix4f transformedNodeMatrix = parentEntityMatrix * nodeMatrix;

                    std::string modelType = node.extras->at("model");
                    if (!modelType.compare("brush"))
                    {
                        if (m_is_world_gfx)
                            throw GltfLoadException(std::format("Script Model child {} is a brush. Brushes can be used in collision files only.", *node.name));
                        brushNodes.emplace_back(std::pair(&node, transformedNodeMatrix));
                    }
                    else if (!modelType.compare("terrain"))
                        terrainNodes.emplace_back(std::pair(&node, transformedNodeMatrix));
                    else
                        throw GltfLoadException(std::format("Script Model child {} model field value isn't brush or terrain", *node.name));
                }
            }

            BSPModel model{};
            if (m_is_world_gfx)
            {
                model.gfxSurfaceIndex = m_curr_bsp_world->surfaces.size();
                for (const auto& node : terrainNodes)
                    addMeshNode(jRoot, *node.first, node.second, true);
                model.gfxSurfaceCount = m_curr_bsp_world->surfaces.size() - model.gfxSurfaceIndex;

                model.surfaceSide = MSS_GFX;
                model.surfaceType = MST_NONE;
                if (gfxAndColLinkNum)
                {
                    if (gfxToColModelLinkMap.contains(*gfxAndColLinkNum))
                        throw GltfLoadException(std::format("Script Model child GfxAndColLinkNumber {} is used by multiple gfx entities", *gfxAndColLinkNum));
                    else
                        gfxToColModelLinkMap[*gfxAndColLinkNum] = m_bsp->models.size();
                }
            }
            else
            {
                BSPModel* modelPtr = &model;
                if (gfxAndColLinkNum)
                {
                    if (!gfxToColModelLinkMap.contains(*gfxAndColLinkNum))
                        throw GltfLoadException(
                            std::format("Script Model child GfxAndColLinkNumber {} has a collision node but no gfx node", *gfxAndColLinkNum));

                    modelPtr = &m_bsp->models.at(gfxToColModelLinkMap.at(*gfxAndColLinkNum));
                }

                modelPtr->colTerrainSurfaceIndex = m_curr_bsp_world->surfaces.size();
                for (const auto& node : terrainNodes)
                    addMeshNode(jRoot, *node.first, node.second, true);
                modelPtr->colTerrainSurfaceCount = m_curr_bsp_world->surfaces.size() - modelPtr->colTerrainSurfaceIndex;

                modelPtr->colBrushSurfaceIndex = m_curr_bsp_world->surfaces.size();
                for (const auto& node : brushNodes)
                    addMeshNode(jRoot, *node.first, node.second, true);
                modelPtr->colBrushSurfaceCount = m_curr_bsp_world->surfaces.size() - modelPtr->colBrushSurfaceIndex;

                if (modelPtr->colTerrainSurfaceCount != 0 && modelPtr->colBrushSurfaceCount != 0)
                    modelPtr->surfaceType = MST_BOTH;
                else if (modelPtr->colTerrainSurfaceCount != 0)
                    modelPtr->surfaceType = MST_TERRAIN;
                else if (modelPtr->colBrushSurfaceCount != 0)
                    modelPtr->surfaceType = MST_BRUSH;
                else
                    modelPtr->surfaceType = MST_NONE;

                if (modelPtr->surfaceType != MST_NONE && modelPtr->gfxSurfaceCount != 0)
                    modelPtr->surfaceSide = MSS_BOTH;
                else if (modelPtr->surfaceType != MST_NONE)
                    modelPtr->surfaceSide = MSS_COL;
                else if (modelPtr->gfxSurfaceCount != 0)
                    modelPtr->surfaceSide = MSS_GFX;
                else
                    modelPtr->surfaceSide = MSS_NONE;
            }
            if (gfxAndColLinkNum && !m_is_world_gfx)
            {
                return gfxToColModelLinkMap.at(*gfxAndColLinkNum) + 1; // script model index starts at 1
            }
            else
            {
                m_bsp->models.emplace_back(model);
                return m_bsp->models.size(); // script model index starts at 1
            }
        }

        bool addZoneNode(const JsonRoot& jRoot, const gltf::JsonNode& node, const Eigen::Matrix4f& nodeMatrix)
        {
            if (m_is_world_gfx)
                return true;

            assert(node.extras);
            assert(node.extras->contains("zone"));

            if (!node.extras->contains("spawner_group") || !node.extras->contains("spawnpoint_group"))
            {
                con::error("ignoring zone: Zone object must have a valid spawner_group and spawnpoint_group property");
                return false;
            }

            Eigen::Vector4f position(0, 0, 0, 1.0f);
            Eigen::Vector4f transformedPosition = nodeMatrix * position;
            vec3_t origin;
            origin.x = transformedPosition.x();
            origin.y = transformedPosition.y();
            origin.z = transformedPosition.z();
            RhcToLhcCoordinates(origin.v);

            BSPZoneZM zone;
            zone.origin = origin;
            zone.zoneName = node.extras->at("zone");
            zone.spawnerGroupName = node.extras->at("spawner_group");
            zone.spawnpointGroupName = node.extras->at("spawnpoint_group");
            zone.modelIndex = addScriptModel(jRoot, node.children, nodeMatrix, node, std::nullopt);
            m_bsp->zm_zones.emplace_back(zone);

            return true;
        }

        bool addZSpawnerNode(const gltf::JsonNode& node, const Eigen::Matrix4f& nodeMatrix)
        {
            if (m_is_world_gfx)
                return true;

            assert(node.extras);
            assert(node.extras->contains("spawner"));

            Eigen::Vector4f position(0, 0, 0, 1.0f);
            Eigen::Vector4f transformedPosition = nodeMatrix * position;
            vec3_t origin;
            origin.x = transformedPosition.x();
            origin.y = transformedPosition.y();
            origin.z = transformedPosition.z();
            RhcToLhcCoordinates(origin.v);

            // GLTF default direction is +Y up
            Eigen::Vector3f defaultDirection(0.0f, 1.0f, 0.0f);
            Eigen::Affine3f affineTransform(nodeMatrix);
            Eigen::Matrix3f rotationMatrix = affineTransform.rotation();
            Eigen::Vector3f outputDirection = rotationMatrix * defaultDirection;
            outputDirection.normalize();
            vec3_t forward;
            forward.x = outputDirection.x();
            forward.y = outputDirection.y();
            forward.z = outputDirection.z();
            RhcToLhcCoordinates(forward.v);

            BSPZSpawnerZM spawner;
            spawner.origin = origin;
            spawner.forward = forward;
            spawner.spawnerGroupName = node.extras->at("spawner");
            m_bsp->zm_spawners.emplace_back(spawner);

            return true;
        }

        bool addClassNode(const JsonRoot& jRoot, const gltf::JsonNode& node, const Eigen::Matrix4f& nodeMatrix)
        {
            assert(node.extras);
            assert(node.extras->contains("classname"));

            std::string classname = node.extras->at("classname");
            if (m_is_world_gfx && !node.extras->contains("model"))
            {
                if (classname.compare("light"))
                    return false; // skip any gfx node with no model or classname not light
            }

            BSPEntity entity{};
            if (!classname.compare("light"))
                entity.type = ET_LIGHT;
            else
                entity.type = ET_OTHER;

            for (auto& element : node.extras->items())
            {
                std::string key = element.key();
                std::string value;
                if (element.value().is_string())
                    value = element.value();
                else if (element.value().is_number())
                {
                    size_t valNum = element.value();
                    value = std::format("{}", valNum);
                }
                else
                    assert(false);
                if (!key.compare("origin") || !key.compare("angles") || !key.compare("GfxAndColLinkNumber"))
                    continue;

                if (!key.compare("model") && !value.compare("*")) // ignore line if the entity has a terrain/brush model
                    continue;

                BSPEntityEntry entry;
                entry.key = key;
                entry.value = value;
                entity.entries.emplace_back(entry);
            }

            if (entity.type == ET_LIGHT)
            {
                BSPEntityEntry entry;
                entry.key = "pl#";
                entry.value = std::format("{}", m_bsp->lights.size() + 2); // +2 as empty and sunlight are already in the lights array when linking
                entity.entries.emplace_back(entry);
                addLightNode(jRoot, node, nodeMatrix, true);
            }

            if (!node.extras->contains("model"))
                entity.modelIndex = 0;
            else
            {
                std::string modelStr = node.extras->at("model");
                if (!modelStr.compare("*"))
                {
                    if (node.extras->contains("GfxAndColLinkNumber"))
                    {
                        size_t linkNumber = node.extras->at("GfxAndColLinkNumber");
                        entity.modelIndex = addScriptModel(jRoot, node.children, nodeMatrix, node, linkNumber);
                        if (m_is_world_gfx) // we don't want both linked entities to be added, so only the col entity added to mapents
                            return true;
                    }
                    else
                        entity.modelIndex = addScriptModel(jRoot, node.children, nodeMatrix, node, std::nullopt);
                }
            }

            Eigen::Vector4f position(0, 0, 0, 1.0f);
            Eigen::Vector4f transformedPosition = nodeMatrix * position;
            entity.origin.x = transformedPosition.x();
            entity.origin.y = transformedPosition.y();
            entity.origin.z = transformedPosition.z();
            RhcToLhcCoordinates(entity.origin.v);

            Eigen::Affine3f affineTransform(nodeMatrix);
            Eigen::Quaternionf rotationQuat(affineTransform.rotation());
            rotationQuat.normalize();
            entity.rotationQuaternion.x = rotationQuat.x();
            entity.rotationQuaternion.y = rotationQuat.y();
            entity.rotationQuaternion.z = rotationQuat.z();
            entity.rotationQuaternion.w = rotationQuat.w();
            RhcToLhcQuaternion(entity.rotationQuaternion.v);

            if (!classname.compare("worldspawn"))
            {
                if (m_bsp->containsWorldspawn)
                    con::warn("WARNING: multiple worldspawn classes found, only one will be used.");
                m_bsp->worldspawn = entity;
                m_bsp->containsWorldspawn = true;

                if (node.extras->contains("skyboxmodel"))
                {
                    std::string sbModel = node.extras->at("skyboxmodel");
                    m_bsp->skyboxName = sbModel;
                }
                else
                    m_bsp->skyboxName = std::format("skybox_{}", m_bsp->name);

                return true; // worldspawn entity is in m_bsp->worldspawn so it shouldn't be added to the overall entity list
            }
            else if (!classname.compare("mp_global_intermission"))
                m_bsp->containsIntermssion = true;

            m_bsp->entities.emplace_back(entity);

            return true;
        }

        bool addNodeToBSP(const JsonRoot& jRoot, const gltf::JsonNode& node, const Eigen::Matrix4f& nodeMatrix)
        {
            if (node.extras)
            {
                bool hasSpawnpoint = node.extras->contains("spawnpoint");
                bool hasZone = node.extras->contains("zone");
                bool hasSpawner = node.extras->contains("spawner");
                bool hasXmodel = node.extras->contains("xmodel");
                bool hasClassname = node.extras->contains("classname");

                if (hasClassname)
                    return addClassNode(jRoot, node, nodeMatrix);

                if (hasXmodel)
                    return addXModelNode(jRoot, node, nodeMatrix);

                if (hasSpawnpoint)
                    return addSpawnPointNode(node, nodeMatrix);

                if (m_bsp->isZombiesMap)
                {
                    if (hasZone)
                        return addZoneNode(jRoot, node, nodeMatrix);

                    if (hasSpawner)
                        return addZSpawnerNode(node, nodeMatrix);
                }
            }

            if (node.extensions && node.extensions->KHR_lights_punctual)
                return addLightNode(jRoot, node, nodeMatrix, false);

            if (node.mesh)
                return addMeshNode(jRoot, node, nodeMatrix, false);

            return false;
        }

        static std::vector<unsigned> GetRootNodes(const JsonRoot& jRoot)
        {
            if (!jRoot.nodes || jRoot.nodes->empty())
                return {};

            const auto nodeCount = jRoot.nodes->size();
            std::vector<unsigned> rootNodes;
            std::vector<bool> isChild(nodeCount);

            for (const auto& node : jRoot.nodes.value())
            {
                if (!node.children)
                    continue;

                for (const auto childIndex : node.children.value())
                {
                    if (childIndex >= nodeCount)
                        throw GltfLoadException("Illegal child index");

                    if (isChild[childIndex])
                        throw GltfLoadException("Node hierarchy is not a set of disjoint strict trees");

                    isChild[childIndex] = true;
                }
            }

            for (auto nodeIndex = 0u; nodeIndex < nodeCount; nodeIndex++)
            {
                if (!isChild[nodeIndex])
                    rootNodes.emplace_back(nodeIndex);
            }

            return rootNodes;
        }

        void TraverseNodes(const JsonRoot& jRoot)
        {
            // Make sure there are any nodes to traverse
            if (!jRoot.nodes || jRoot.nodes->empty())
                return;

            struct s_nodes
            {
                size_t nodeIndex;
                Eigen::Matrix4f parentNodeMatrix;
            };

            std::deque<s_nodes> nodeQueue;
            for (const auto rootNode : GetRootNodes(jRoot))
                nodeQueue.emplace_back(s_nodes{rootNode, Eigen::Matrix4f::Identity()});

            std::vector<s_nodes> colStaticNodes;
            std::vector<s_nodes> colStaticBrushNodes;
            std::vector<s_nodes> gfxLitOpaqueNodes;
            std::vector<s_nodes> gfxLitTransparentNodes;
            std::vector<s_nodes> gfxEmissiveOpaqueNodes;
            std::vector<s_nodes> gfxEmissiveTransparentnodes;
            std::vector<s_nodes> scriptNodes;
            while (!nodeQueue.empty())
            {
                size_t nodeIndex = nodeQueue.front().nodeIndex;
                const auto& node = jRoot.nodes.value()[nodeIndex];
                Eigen::Matrix4f parentNodeMatrix = nodeQueue.front().parentNodeMatrix;
                nodeQueue.pop_front();

                Eigen::Matrix4f nodeMatrix = createNodeMatrix(node);
                Eigen::Matrix4f transformedNodeMatrix = parentNodeMatrix * nodeMatrix;

                bool shouldAddChildren = true;
                if (node.extras && node.extras->contains("model"))
                {
                    std::string modelStr = node.extras->at("model");
                    if (!modelStr.compare("*"))    // entities with "model: *" use their children as collision data
                        shouldAddChildren = false; // we don't want to add entity collision to static collision
                }

                if (shouldAddChildren && node.children)
                {
                    for (const auto childIndex : *node.children)
                        nodeQueue.emplace_back(s_nodes{childIndex, transformedNodeMatrix});
                }

                if (m_is_world_gfx)
                {
                    if (node.extras && (node.extras->contains("classname")))
                        scriptNodes.emplace_back(s_nodes{nodeIndex, transformedNodeMatrix});
                    else if (node.extras && (node.extras->contains("type")))
                    {
                        std::string typeStr = node.extras->at("type");
                        if (!typeStr.compare("lit_opaque"))
                            gfxLitOpaqueNodes.emplace_back(s_nodes{nodeIndex, transformedNodeMatrix});
                        else if (!typeStr.compare("lit_transparent"))
                            gfxLitTransparentNodes.emplace_back(s_nodes{nodeIndex, transformedNodeMatrix});
                        else if (!typeStr.compare("emissive_opaque"))
                            gfxEmissiveOpaqueNodes.emplace_back(s_nodes{nodeIndex, transformedNodeMatrix});
                        else if (!typeStr.compare("emissive_transparent"))
                            gfxEmissiveTransparentnodes.emplace_back(s_nodes{nodeIndex, transformedNodeMatrix});
                        else
                            throw GltfLoadException(
                                std::format("Node {} has type property but isn't lit_opaque, lit_transparent, emissive_opaque, emissive_transparent",
                                            node.name.value_or("unnamed node")));
                    }
                    else
                        gfxLitOpaqueNodes.emplace_back(s_nodes{nodeIndex, transformedNodeMatrix});
                }
                else
                {
                    if (node.extras && (node.extras->contains("classname") || node.extras->contains("zone")))
                        scriptNodes.emplace_back(s_nodes{nodeIndex, transformedNodeMatrix});
                    else if (node.extras && node.extras->contains("model"))
                    {
                        std::string modelStr = node.extras->at("model");
                        if (!modelStr.compare("brush"))
                            colStaticBrushNodes.emplace_back(s_nodes{nodeIndex, transformedNodeMatrix});
                        else if (!modelStr.compare("terrain"))
                            colStaticNodes.emplace_back(s_nodes{nodeIndex, transformedNodeMatrix});
                        else
                            throw GltfLoadException(std::format("Node {} has model property but isn't brush or terrain ", node.name.value_or("unnamed node")));
                    }
                    else
                        colStaticNodes.emplace_back(s_nodes{nodeIndex, transformedNodeMatrix});
                }
            }

            if (m_is_world_gfx)
            {
                m_bsp->staticSurfaceStart = m_curr_bsp_world->surfaces.size();
                assert(m_bsp->staticSurfaceStart == 0);

                m_bsp->litOpaqueSurfaceStart = m_curr_bsp_world->surfaces.size();
                for (const auto& node : gfxLitOpaqueNodes)
                {
                    if (!addNodeToBSP(jRoot, jRoot.nodes->at(node.nodeIndex), node.parentNodeMatrix))
                        con::warn("({}) Ignoring node: {}", getWorldTypeName(), jRoot.nodes->at(node.nodeIndex).name.value_or("unnamed node"));
                }
                m_bsp->litOpaqueSurfaceCount = m_curr_bsp_world->surfaces.size() - m_bsp->litOpaqueSurfaceStart;

                m_bsp->litTransparentSurfaceStart = m_curr_bsp_world->surfaces.size();
                for (const auto& node : gfxLitTransparentNodes)
                {
                    if (!addNodeToBSP(jRoot, jRoot.nodes->at(node.nodeIndex), node.parentNodeMatrix))
                        con::warn("({}) Ignoring node: {}", getWorldTypeName(), jRoot.nodes->at(node.nodeIndex).name.value_or("unnamed node"));
                }
                m_bsp->litTransparentSurfaceCount = m_curr_bsp_world->surfaces.size() - m_bsp->litTransparentSurfaceStart;

                m_bsp->emissiveOpaqueSurfaceStart = m_curr_bsp_world->surfaces.size();
                for (const auto& node : gfxEmissiveOpaqueNodes)
                {
                    if (!addNodeToBSP(jRoot, jRoot.nodes->at(node.nodeIndex), node.parentNodeMatrix))
                        con::warn("({}) Ignoring node: {}", getWorldTypeName(), jRoot.nodes->at(node.nodeIndex).name.value_or("unnamed node"));
                }
                m_bsp->emissiveOpaqueSurfaceCount = m_curr_bsp_world->surfaces.size() - m_bsp->emissiveOpaqueSurfaceStart;

                m_bsp->emissiveTransparentSurfaceStart = m_curr_bsp_world->surfaces.size();
                for (const auto& node : gfxEmissiveTransparentnodes)
                {
                    if (!addNodeToBSP(jRoot, jRoot.nodes->at(node.nodeIndex), node.parentNodeMatrix))
                        con::warn("({}) Ignoring node: {}", getWorldTypeName(), jRoot.nodes->at(node.nodeIndex).name.value_or("unnamed node"));
                }
                m_bsp->emissiveTransparentSurfaceCount = m_curr_bsp_world->surfaces.size() - m_bsp->emissiveTransparentSurfaceStart;

                m_bsp->staticSurfaceCount = m_curr_bsp_world->surfaces.size() - m_bsp->staticSurfaceStart;
            }
            else
            {
                assert(m_curr_bsp_world->surfaces.size() == 0);
                m_bsp->staticTerrainSurfaceStart = m_curr_bsp_world->surfaces.size();
                for (const auto& node : colStaticNodes)
                {
                    if (!addNodeToBSP(jRoot, jRoot.nodes->at(node.nodeIndex), node.parentNodeMatrix))
                        con::warn("({}) Ignoring node: {}", getWorldTypeName(), jRoot.nodes->at(node.nodeIndex).name.value_or("unnamed node"));
                }
                m_bsp->staticTerrainSurfaceCount = m_curr_bsp_world->surfaces.size() - m_bsp->staticTerrainSurfaceStart;

                m_bsp->staticBrushSurfaceStart = m_curr_bsp_world->surfaces.size();
                for (const auto& node : colStaticBrushNodes)
                {
                    if (!addNodeToBSP(jRoot, jRoot.nodes->at(node.nodeIndex), node.parentNodeMatrix))
                        con::warn("({}) Ignoring node: {}", getWorldTypeName(), jRoot.nodes->at(node.nodeIndex).name.value_or("unnamed node"));
                }
                m_bsp->staticBrushSurfaceCount = m_curr_bsp_world->surfaces.size() - m_bsp->staticBrushSurfaceStart;
            }

            for (const auto& node : scriptNodes)
            {
                if (!addNodeToBSP(jRoot, jRoot.nodes->at(node.nodeIndex), node.parentNodeMatrix))
                    con::warn("({}) Ignoring node: {}", getWorldTypeName(), jRoot.nodes->at(node.nodeIndex).name.value_or("unnamed node"));
            }
        }

        void CreateBuffers(const JsonRoot& jRoot, Input& gltfInput)
        {
            if (!jRoot.buffers)
                return;

            m_buffers.reserve(jRoot.buffers->size());
            for (const auto& jBuffer : *jRoot.buffers)
            {
                if (!jBuffer.uri)
                {
                    const void* embeddedBufferPtr = nullptr;
                    size_t embeddedBufferSize = 0u;
                    if (!gltfInput.GetEmbeddedBuffer(embeddedBufferPtr, embeddedBufferSize) || embeddedBufferSize == 0u)
                        throw GltfLoadException("Buffer tried to access embedded data when there is none");

                    m_buffers.emplace_back(std::make_unique<EmbeddedBuffer>(embeddedBufferPtr, embeddedBufferSize));
                }
                else if (DataUriBuffer::IsDataUri(*jBuffer.uri))
                {
                    auto dataUriBuffer = std::make_unique<DataUriBuffer>();
                    if (!dataUriBuffer->ReadDataFromUri(*jBuffer.uri))
                        throw GltfLoadException("Buffer has invalid data uri");

                    m_buffers.emplace_back(std::move(dataUriBuffer));
                }
                else
                {
                    throw GltfLoadException("File buffers are not supported");
                }
            }
        }

        void CreateBufferViews(const JsonRoot& jRoot)
        {
            if (!jRoot.bufferViews)
                return;

            m_buffer_views.reserve(jRoot.bufferViews->size());
            for (const auto& jBufferView : *jRoot.bufferViews)
            {
                if (jBufferView.buffer >= m_buffers.size())
                    throw GltfLoadException("Buffer view references invalid buffer");

                const auto* buffer = m_buffers[jBufferView.buffer].get();
                const auto offset = jBufferView.byteOffset.value_or(0u);
                const auto length = jBufferView.byteLength;
                const auto stride = jBufferView.byteStride.value_or(0u);

                if (offset + length > buffer->GetSize())
                    throw GltfLoadException("Buffer view is defined larger as underlying buffer");

                m_buffer_views.emplace_back(std::make_unique<BufferView>(buffer, offset, length, stride));
            }
        }

        void CreateAccessors(const JsonRoot& jRoot)
        {
            if (!jRoot.accessors)
                return;

            m_accessors.reserve(jRoot.accessors->size());
            for (const auto& jAccessor : *jRoot.accessors)
            {
                if (!jAccessor.bufferView)
                {
                    m_accessors.emplace_back(std::make_unique<NullAccessor>(jAccessor.count));
                    continue;
                }

                if (*jAccessor.bufferView >= m_buffer_views.size())
                    throw GltfLoadException("Accessor references invalid buffer view");

                const auto* bufferView = m_buffer_views[*jAccessor.bufferView].get();
                const auto byteOffset = jAccessor.byteOffset.value_or(0u);
                if (jAccessor.componentType == JsonAccessorComponentType::FLOAT)
                    m_accessors.emplace_back(std::make_unique<FloatAccessor>(bufferView, jAccessor.type, byteOffset, jAccessor.count));
                else if (jAccessor.componentType == JsonAccessorComponentType::UNSIGNED_BYTE)
                    m_accessors.emplace_back(std::make_unique<UnsignedByteAccessor>(bufferView, jAccessor.type, byteOffset, jAccessor.count));
                else if (jAccessor.componentType == JsonAccessorComponentType::UNSIGNED_SHORT)
                    m_accessors.emplace_back(std::make_unique<UnsignedShortAccessor>(bufferView, jAccessor.type, byteOffset, jAccessor.count));
                else if (jAccessor.componentType == JsonAccessorComponentType::UNSIGNED_INT)
                    m_accessors.emplace_back(std::make_unique<UnsignedIntAccessor>(bufferView, jAccessor.type, byteOffset, jAccessor.count));
                else
                    throw GltfLoadException(std::format("Accessor has unsupported component type {}", static_cast<unsigned>(jAccessor.componentType)));
            }
        }

        void LoadMaterials(const JsonRoot& jRoot)
        {
            if (jRoot.materials)
            {
                m_curr_bsp_world->materials.reserve((*jRoot.materials).size());
                for (auto& jsMaterial : *jRoot.materials)
                {
                    BSPMaterial material;

                    if (jsMaterial.extras && jsMaterial.extras->contains("name"))
                        material.materialName = jsMaterial.extras->at("name");
                    else if (jsMaterial.name && (*jsMaterial.name).length() != 0)
                        material.materialName = *jsMaterial.name;
                    else
                        throw GltfLoadException("Materials must have a name.");

                    material.materialType = MATERIAL_TYPE_TEXTURE;
                    material.materialColour.x = 1.0f;
                    material.materialColour.y = 1.0f;
                    material.materialColour.z = 1.0f;
                    material.materialColour.w = 1.0f;

                    material.surfaceFlags = 0;
                    material.contentFlags = 0;
                    bool hasFlags = false;
                    if (jsMaterial.extras && jsMaterial.extras->contains("sf"))
                    {
                        hasFlags = true;
                        nlohmann::json sf = jsMaterial.extras->at("sf");
                        if (sf.is_number())
                            material.surfaceFlags = sf;
                        else if (sf.is_string())
                        {
                            std::string str = sf;
                            material.surfaceFlags = atoi(str.c_str());
                        }
                        else
                            throw GltfLoadException("Bad surface flags type ");
                    }
                    if (jsMaterial.extras && jsMaterial.extras->contains("cf"))
                    {
                        hasFlags = true;
                        nlohmann::json cf = jsMaterial.extras->at("cf");
                        if (cf.is_number())
                            material.contentFlags = cf;
                        else if (cf.is_string())
                        {
                            std::string str = cf;
                            material.contentFlags = atoi(str.c_str());
                        }
                        else
                            throw GltfLoadException("Bad content flags type ");
                    }
                    if (!hasFlags)
                    {
                        material.surfaceFlags = 0;
                        material.contentFlags = 1;
                    }

                    m_curr_bsp_world->materials.emplace_back(material);
                }
            }

            m_emptyMaterialIndex = m_curr_bsp_world->materials.size();
            BSPMaterial emptyMaterial;
            emptyMaterial.materialType = MATERIAL_TYPE_COLOUR;
            emptyMaterial.surfaceFlags = 0;
            emptyMaterial.contentFlags = 1;
            emptyMaterial.materialName = "";
            emptyMaterial.materialColour.x = 1.0f;
            emptyMaterial.materialColour.y = 1.0f;
            emptyMaterial.materialColour.z = 1.0f;
            emptyMaterial.materialColour.w = 1.0f;
            m_curr_bsp_world->materials.emplace_back(emptyMaterial);
        }

    public:
        bool addGLTFDataToBSP(Input& gltfInput, bool isGfxWorld)
        {
            JsonRoot jRoot;
            try
            {
                jRoot = gltfInput.GetJson().get<JsonRoot>();
            }
            catch (const nlohmann::json::exception& e)
            {
                con::error("Failed to parse GLTF JSON: {}", e.what());
                return false;
            }

            try
            {
                m_is_world_gfx = isGfxWorld;
                if (isGfxWorld)
                    m_curr_bsp_world = &m_bsp->gfxWorld;
                else
                    m_curr_bsp_world = &m_bsp->colWorld;
                m_accessors.clear();
                m_buffer_views.clear();
                m_buffers.clear();

                CreateBuffers(jRoot, gltfInput);
                CreateBufferViews(jRoot);
                CreateAccessors(jRoot);

                LoadMaterials(jRoot);
                TraverseNodes(jRoot); // requires materials and lights
            }
            catch (const GltfLoadException& e)
            {
                con::error("Failed to load GLTF: {}", e.Str());
                return false;
            }

            return true;
        }

        BSPLoader(BSPData* bsp)
            : m_bsp(bsp)
        {
            m_curr_bsp_world = nullptr;
            m_is_world_gfx = false;
        };
    };
} // namespace

std::unique_ptr<BSPData> T6::BSP::createBSPData(std::string& mapName, ISearchPath& searchPath, bool isZombiesMap)
{
    bool seperateColFile = true;
    bool isGfxFileGltf = true;
    bool isColFileGltf = true;

    std::string gfxFilePath = BSPUtil::getFileNameForBSPAsset("map_gfx.gltf");
    auto gfxFile = searchPath.Open(gfxFilePath);
    if (!gfxFile.IsOpen())
    {
        isGfxFileGltf = false;
        gfxFilePath = BSPUtil::getFileNameForBSPAsset("map_gfx.glb");
        gfxFile = searchPath.Open(gfxFilePath);
        if (!gfxFile.IsOpen())
        {
            con::error("BSP Creator: Can't find map_gfx.gltf or map_gfx.glb.");
            return nullptr;
        }
    }

    std::string colFilePath = BSPUtil::getFileNameForBSPAsset("map_col.gltf");
    auto colFile = searchPath.Open(colFilePath);
    if (!colFile.IsOpen())
    {
        isColFileGltf = false;
        colFilePath = BSPUtil::getFileNameForBSPAsset("map_col.glb");
        colFile = searchPath.Open(colFilePath);
        if (!colFile.IsOpen())
        {
            con::info("BSP Creator: generating colision data from GLTF graphics data.");
            seperateColFile = false;
        }
    }

    std::unique_ptr<BSPData> bsp = std::make_unique<BSPData>();
    bsp->name = mapName;
    bsp->bspName = "maps/mp/" + mapName + ".d3dbsp";
    bsp->isZombiesMap = isZombiesMap;
    bsp->hasSunlightBeenSet = false;
    bsp->containsIntermssion = false;
    bsp->containsWorldspawn = false;

    BSPLoader loader(bsp.get());
    if (isGfxFileGltf)
    {
        gltf::TextInput input;
        if (!input.ReadGltfData(*gfxFile.m_stream))
            return nullptr;
        if (!loader.addGLTFDataToBSP(input, true))
            return nullptr;
        if (!seperateColFile)
            if (!loader.addGLTFDataToBSP(input, false))
                return nullptr;
    }
    else
    {
        gltf::BinInput input;
        if (!input.ReadGltfData(*gfxFile.m_stream))
            return nullptr;
        if (!loader.addGLTFDataToBSP(input, true))
            return nullptr;
        if (!seperateColFile)
            if (!loader.addGLTFDataToBSP(input, false))
                return nullptr;
    }

    if (seperateColFile)
    {
        if (isColFileGltf)
        {
            gltf::TextInput input;
            if (!input.ReadGltfData(*colFile.m_stream))
                return nullptr;
            if (!loader.addGLTFDataToBSP(input, false))
                return nullptr;
        }
        else
        {
            gltf::BinInput input;
            if (!input.ReadGltfData(*colFile.m_stream))
                return nullptr;
            if (!loader.addGLTFDataToBSP(input, false))
                return nullptr;
        }
    }

    if (!bsp->hasSunlightBeenSet)
    {
        con::info("Writing default sun values");
        bsp->sunlight.type = LIGHT_TYPE_DIRECTIONAL;
        bsp->sunlight.colour = {1.0f, 1.0f, 1.0f};
        bsp->sunlight.range = 1000.0f;
        bsp->sunlight.intensity = 1000.0f;
        bsp->sunlight.pos = {0.0f, 0.0f, 0.0f};
        bsp->sunlight.forwardVector = {0.0f, 0.0f, 1.0f};
        bsp->sunlight.rollAngle = 0.0f;
        bsp->sunlight.innerConeAngle = 0.0f;
        bsp->sunlight.outerConeAngle = 0.0f;
    }
    if (!bsp->containsIntermssion)
    {
        con::error("Map does not contain a mp_global_intermission class");
        return nullptr;
    }
    if (!bsp->containsWorldspawn)
    {
        con::error("Map does not contain a worldspawn class");
        return nullptr;
    }
    if (bsp->gfxWorld.surfaces.size() == 0 || bsp->gfxWorld.vertices.size() == 0 || bsp->gfxWorld.indices.size() == 0)
    {
        con::error("GFX world has no surfaces, indicies or vertices!");
        return nullptr;
    }
    if (bsp->colWorld.surfaces.size() == 0 || bsp->colWorld.vertices.size() == 0 || bsp->colWorld.indices.size() == 0)
    {
        con::error("Collision world has no surfaces, indicies or vertices!");
        return nullptr;
    }

    return bsp;
}
