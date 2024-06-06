#include "AssetDumperXModel.h"

#include "Game/T6/CommonT6.h"
#include "ObjWriting.h"
#include "Utils/DistinctMapper.h"
#include "Utils/QuatInt16.h"
#include "XModel/Export/XModelExportWriter.h"
#include "XModel/Gltf/GltfBinOutput.h"
#include "XModel/Gltf/GltfTextOutput.h"
#include "XModel/Gltf/GltfWriter.h"
#include "XModel/Obj/ObjWriter.h"
#include "XModel/XModelWriter.h"

#include <cassert>
#include <format>

using namespace T6;

namespace
{
    std::string GetFileNameForLod(const std::string& modelName, const unsigned lod, const std::string& extension)
    {
        return std::format("model_export/{}_lod{}{}", modelName, lod, extension);
    }

    GfxImage* GetMaterialColorMap(const Material* material)
    {
        std::vector<MaterialTextureDef*> potentialTextureDefs;

        for (auto textureIndex = 0u; textureIndex < material->textureCount; textureIndex++)
        {
            MaterialTextureDef* def = &material->textureTable[textureIndex];

            if (def->semantic == TS_COLOR_MAP || def->semantic >= TS_COLOR0_MAP && def->semantic <= TS_COLOR15_MAP)
                potentialTextureDefs.push_back(def);
        }

        if (potentialTextureDefs.empty())
            return nullptr;
        if (potentialTextureDefs.size() == 1)
            return potentialTextureDefs[0]->image;

        for (const auto* def : potentialTextureDefs)
        {
            if (tolower(def->nameStart) == 'c' && tolower(def->nameEnd) == 'p')
                return def->image;
        }

        for (const auto* def : potentialTextureDefs)
        {
            if (tolower(def->nameStart) == 'r' && tolower(def->nameEnd) == 'k')
                return def->image;
        }

        for (const auto* def : potentialTextureDefs)
        {
            if (tolower(def->nameStart) == 'd' && tolower(def->nameEnd) == 'p')
                return def->image;
        }

        return potentialTextureDefs[0]->image;
    }

    GfxImage* GetMaterialNormalMap(const Material* material)
    {
        std::vector<MaterialTextureDef*> potentialTextureDefs;

        for (auto textureIndex = 0u; textureIndex < material->textureCount; textureIndex++)
        {
            MaterialTextureDef* def = &material->textureTable[textureIndex];

            if (def->semantic == TS_NORMAL_MAP)
                potentialTextureDefs.push_back(def);
        }

        if (potentialTextureDefs.empty())
            return nullptr;
        if (potentialTextureDefs.size() == 1)
            return potentialTextureDefs[0]->image;

        for (const auto* def : potentialTextureDefs)
        {
            if (def->nameStart == 'n' && def->nameEnd == 'p')
                return def->image;
        }

        return potentialTextureDefs[0]->image;
    }

    GfxImage* GetMaterialSpecularMap(const Material* material)
    {
        std::vector<MaterialTextureDef*> potentialTextureDefs;

        for (auto textureIndex = 0u; textureIndex < material->textureCount; textureIndex++)
        {
            MaterialTextureDef* def = &material->textureTable[textureIndex];

            if (def->semantic == TS_SPECULAR_MAP)
                potentialTextureDefs.push_back(def);
        }

        if (potentialTextureDefs.empty())
            return nullptr;
        if (potentialTextureDefs.size() == 1)
            return potentialTextureDefs[0]->image;

        for (const auto* def : potentialTextureDefs)
        {
            if (def->nameStart == 's' && def->nameEnd == 'p')
                return def->image;
        }

        return potentialTextureDefs[0]->image;
    }

    void AddXModelBones(XModelCommon& out, const AssetDumpingContext& context, const XModel* model)
    {
        for (auto boneNum = 0u; boneNum < model->numBones; boneNum++)
        {
            XModelBone bone;
            if (model->boneNames[boneNum] < context.m_zone->m_script_strings.Count())
                bone.name = context.m_zone->m_script_strings[model->boneNames[boneNum]];
            else
                bone.name = "INVALID_BONE_NAME";

            if (boneNum >= model->numRootBones)
                bone.parentIndex = static_cast<int>(boneNum - static_cast<unsigned int>(model->parentList[boneNum - model->numRootBones]));
            else
                bone.parentIndex = std::nullopt;

            bone.scale[0] = 1.0f;
            bone.scale[1] = 1.0f;
            bone.scale[2] = 1.0f;

            bone.globalOffset[0] = model->baseMat[boneNum].trans.x;
            bone.globalOffset[1] = model->baseMat[boneNum].trans.y;
            bone.globalOffset[2] = model->baseMat[boneNum].trans.z;
            bone.globalRotation = {
                model->baseMat[boneNum].quat.x,
                model->baseMat[boneNum].quat.y,
                model->baseMat[boneNum].quat.z,
                model->baseMat[boneNum].quat.w,
            };

            if (boneNum < model->numRootBones)
            {
                bone.localOffset[0] = 0;
                bone.localOffset[1] = 0;
                bone.localOffset[2] = 0;
                bone.localRotation = {0, 0, 0, 1};
            }
            else
            {
                bone.localOffset[0] = model->trans[boneNum - model->numRootBones][0];
                bone.localOffset[1] = model->trans[boneNum - model->numRootBones][1];
                bone.localOffset[2] = model->trans[boneNum - model->numRootBones][2];
                bone.localRotation = {
                    QuatInt16::ToFloat(model->quats[boneNum - model->numRootBones][0]),
                    QuatInt16::ToFloat(model->quats[boneNum - model->numRootBones][1]),
                    QuatInt16::ToFloat(model->quats[boneNum - model->numRootBones][2]),
                    QuatInt16::ToFloat(model->quats[boneNum - model->numRootBones][3]),
                };
            }

            out.m_bones.emplace_back(std::move(bone));
        }
    }

    const char* AssetName(const char* input)
    {
        if (input && input[0] == ',')
            return &input[1];

        return input;
    }

    void AddXModelMaterials(XModelCommon& out, DistinctMapper<Material*>& materialMapper, const XModel* model)
    {
        for (auto surfaceMaterialNum = 0; surfaceMaterialNum < model->numsurfs; surfaceMaterialNum++)
        {
            Material* material = model->materialHandles[surfaceMaterialNum];
            if (materialMapper.Add(material))
            {
                XModelMaterial xMaterial;
                xMaterial.ApplyDefaults();

                xMaterial.name = AssetName(material->info.name);
                const auto* colorMap = GetMaterialColorMap(material);
                if (colorMap)
                    xMaterial.colorMapName = AssetName(colorMap->name);

                const auto* normalMap = GetMaterialNormalMap(material);
                if (normalMap)
                    xMaterial.normalMapName = AssetName(normalMap->name);

                const auto* specularMap = GetMaterialSpecularMap(material);
                if (specularMap)
                    xMaterial.specularMapName = AssetName(specularMap->name);

                out.m_materials.emplace_back(std::move(xMaterial));
            }
        }
    }

    void AddXModelObjects(XModelCommon& out, const XModel* model, const unsigned lod, const DistinctMapper<Material*>& materialMapper)
    {
        const auto surfCount = model->lodInfo[lod].numsurfs;
        const auto baseSurfaceIndex = model->lodInfo[lod].surfIndex;

        for (auto surfIndex = 0u; surfIndex < surfCount; surfIndex++)
        {
            XModelObject object;
            object.name = std::format("surf{}", surfIndex);
            object.materialIndex = static_cast<int>(materialMapper.GetDistinctPositionByInputPosition(surfIndex + baseSurfaceIndex));

            out.m_objects.emplace_back(std::move(object));
        }
    }

    void AddXModelVertices(XModelCommon& out, const XModel* model, const unsigned lod)
    {
        const auto* surfs = &model->surfs[model->lodInfo[lod].surfIndex];
        const auto surfCount = model->lodInfo[lod].numsurfs;

        if (!surfs)
            return;

        for (auto surfIndex = 0u; surfIndex < surfCount; surfIndex++)
        {
            const auto& surface = surfs[surfIndex];

            for (auto vertexIndex = 0u; vertexIndex < surface.vertCount; vertexIndex++)
            {
                const auto& v = surface.verts0[vertexIndex];
                vec2_t uv{};
                vec3_t normalVec{};
                vec4_t color{};

                Common::Vec2UnpackTexCoords(v.texCoord, &uv);
                Common::Vec3UnpackUnitVec(v.normal, &normalVec);
                Common::Vec4UnpackGfxColor(v.color, &color);

                XModelVertex vertex{};
                vertex.coordinates[0] = v.xyz.x;
                vertex.coordinates[1] = v.xyz.y;
                vertex.coordinates[2] = v.xyz.z;
                vertex.normal[0] = normalVec.x;
                vertex.normal[1] = normalVec.y;
                vertex.normal[2] = normalVec.z;
                vertex.color[0] = color.x;
                vertex.color[1] = color.y;
                vertex.color[2] = color.z;
                vertex.color[3] = color.w;
                vertex.uv[0] = uv.x;
                vertex.uv[1] = uv.y;

                out.m_vertices.emplace_back(vertex);
            }
        }
    }

    void AllocateXModelBoneWeights(const XModel* model, const unsigned lod, XModelVertexBoneWeightCollection& weightCollection)
    {
        const auto* surfs = &model->surfs[model->lodInfo[lod].surfIndex];
        const auto surfCount = model->lodInfo[lod].numsurfs;

        if (!surfs)
            return;

        auto totalWeightCount = 0u;
        for (auto surfIndex = 0u; surfIndex < surfCount; surfIndex++)
        {
            const auto& surface = surfs[surfIndex];

            if (surface.vertList)
            {
                totalWeightCount += surface.vertListCount;
            }

            if (surface.vertInfo.vertsBlend)
            {
                totalWeightCount += surface.vertInfo.vertCount[0] * 1;
                totalWeightCount += surface.vertInfo.vertCount[1] * 2;
                totalWeightCount += surface.vertInfo.vertCount[2] * 3;
                totalWeightCount += surface.vertInfo.vertCount[3] * 4;
            }
        }

        weightCollection.weights.resize(totalWeightCount);
    }

    float BoneWeight16(const uint16_t value)
    {
        return static_cast<float>(value) / static_cast<float>(std::numeric_limits<uint16_t>::max());
    }

    void AddXModelVertexBoneWeights(XModelCommon& out, const XModel* model, const unsigned lod)
    {
        const auto* surfs = &model->surfs[model->lodInfo[lod].surfIndex];
        const auto surfCount = model->lodInfo[lod].numsurfs;
        auto& weightCollection = out.m_bone_weight_data;

        if (!surfs)
            return;

        size_t weightOffset = 0u;

        for (auto surfIndex = 0u; surfIndex < surfCount; surfIndex++)
        {
            const auto& surface = surfs[surfIndex];
            auto handledVertices = 0u;

            if (surface.vertList)
            {
                for (auto vertListIndex = 0u; vertListIndex < surface.vertListCount; vertListIndex++)
                {
                    const auto& vertList = surface.vertList[vertListIndex];
                    const auto* boneWeightOffset = &weightCollection.weights[weightOffset];

                    weightCollection.weights[weightOffset++] = XModelBoneWeight{vertList.boneOffset / sizeof(DObjSkelMat), 1.0f};

                    for (auto vertListVertexOffset = 0u; vertListVertexOffset < vertList.vertCount; vertListVertexOffset++)
                    {
                        out.m_vertex_bone_weights.emplace_back(boneWeightOffset, 1);
                    }
                    handledVertices += vertList.vertCount;
                }
            }

            auto vertsBlendOffset = 0u;
            if (surface.vertInfo.vertsBlend)
            {
                // 1 bone weight
                for (auto vertIndex = 0; vertIndex < surface.vertInfo.vertCount[0]; vertIndex++)
                {
                    const auto* boneWeightOffset = &weightCollection.weights[weightOffset];
                    const auto boneIndex0 = surface.vertInfo.vertsBlend[vertsBlendOffset + 0] / sizeof(DObjSkelMat);
                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex0, 1.0f};

                    vertsBlendOffset += 1;

                    out.m_vertex_bone_weights.emplace_back(boneWeightOffset, 1);
                }

                // 2 bone weights
                for (auto vertIndex = 0; vertIndex < surface.vertInfo.vertCount[1]; vertIndex++)
                {
                    const auto* boneWeightOffset = &weightCollection.weights[weightOffset];
                    const auto boneIndex0 = surface.vertInfo.vertsBlend[vertsBlendOffset + 0] / sizeof(DObjSkelMat);
                    const auto boneIndex1 = surface.vertInfo.vertsBlend[vertsBlendOffset + 1] / sizeof(DObjSkelMat);
                    const auto boneWeight1 = BoneWeight16(surface.vertInfo.vertsBlend[vertsBlendOffset + 2]);
                    const auto boneWeight0 = 1.0f - boneWeight1;

                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex0, boneWeight0};
                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex1, boneWeight1};

                    vertsBlendOffset += 3;

                    out.m_vertex_bone_weights.emplace_back(boneWeightOffset, 2);
                }

                // 3 bone weights
                for (auto vertIndex = 0; vertIndex < surface.vertInfo.vertCount[2]; vertIndex++)
                {
                    const auto* boneWeightOffset = &weightCollection.weights[weightOffset];
                    const auto boneIndex0 = surface.vertInfo.vertsBlend[vertsBlendOffset + 0] / sizeof(DObjSkelMat);
                    const auto boneIndex1 = surface.vertInfo.vertsBlend[vertsBlendOffset + 1] / sizeof(DObjSkelMat);
                    const auto boneWeight1 = BoneWeight16(surface.vertInfo.vertsBlend[vertsBlendOffset + 2]);
                    const auto boneIndex2 = surface.vertInfo.vertsBlend[vertsBlendOffset + 3] / sizeof(DObjSkelMat);
                    const auto boneWeight2 = BoneWeight16(surface.vertInfo.vertsBlend[vertsBlendOffset + 4]);
                    const auto boneWeight0 = 1.0f - boneWeight1 - boneWeight2;

                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex0, boneWeight0};
                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex1, boneWeight1};
                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex2, boneWeight2};

                    vertsBlendOffset += 5;

                    out.m_vertex_bone_weights.emplace_back(boneWeightOffset, 3);
                }

                // 4 bone weights
                for (auto vertIndex = 0; vertIndex < surface.vertInfo.vertCount[3]; vertIndex++)
                {
                    const auto* boneWeightOffset = &weightCollection.weights[weightOffset];
                    const auto boneIndex0 = surface.vertInfo.vertsBlend[vertsBlendOffset + 0] / sizeof(DObjSkelMat);
                    const auto boneIndex1 = surface.vertInfo.vertsBlend[vertsBlendOffset + 1] / sizeof(DObjSkelMat);
                    const auto boneWeight1 = BoneWeight16(surface.vertInfo.vertsBlend[vertsBlendOffset + 2]);
                    const auto boneIndex2 = surface.vertInfo.vertsBlend[vertsBlendOffset + 3] / sizeof(DObjSkelMat);
                    const auto boneWeight2 = BoneWeight16(surface.vertInfo.vertsBlend[vertsBlendOffset + 4]);
                    const auto boneIndex3 = surface.vertInfo.vertsBlend[vertsBlendOffset + 5] / sizeof(DObjSkelMat);
                    const auto boneWeight3 = BoneWeight16(surface.vertInfo.vertsBlend[vertsBlendOffset + 6]);
                    const auto boneWeight0 = 1.0f - boneWeight1 - boneWeight2 - boneWeight3;

                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex0, boneWeight0};
                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex1, boneWeight1};
                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex2, boneWeight2};
                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex3, boneWeight3};

                    vertsBlendOffset += 7;

                    out.m_vertex_bone_weights.emplace_back(boneWeightOffset, 4);
                }

                handledVertices +=
                    surface.vertInfo.vertCount[0] + surface.vertInfo.vertCount[1] + surface.vertInfo.vertCount[2] + surface.vertInfo.vertCount[3];
            }

            for (; handledVertices < surface.vertCount; handledVertices++)
            {
                out.m_vertex_bone_weights.emplace_back(nullptr, 0);
            }
        }
    }

    void AddXModelFaces(XModelCommon& out, const XModel* model, const unsigned lod)
    {
        const auto* surfs = &model->surfs[model->lodInfo[lod].surfIndex];
        const auto surfCount = model->lodInfo[lod].numsurfs;

        if (!surfs)
            return;

        for (auto surfIndex = 0u; surfIndex < surfCount; surfIndex++)
        {
            const auto& surface = surfs[surfIndex];
            auto& object = out.m_objects[surfIndex];
            object.m_faces.reserve(surface.triCount);

            for (auto triIndex = 0u; triIndex < surface.triCount; triIndex++)
            {
                const auto& tri = surface.triIndices[triIndex];

                XModelFace face{};
                face.vertexIndex[0] = tri[0] + surface.baseVertIndex;
                face.vertexIndex[1] = tri[1] + surface.baseVertIndex;
                face.vertexIndex[2] = tri[2] + surface.baseVertIndex;
                object.m_faces.emplace_back(face);
            }
        }
    }

    void PopulateXModelWriter(XModelCommon& out, const AssetDumpingContext& context, const unsigned lod, const XModel* model)
    {
        DistinctMapper<Material*> materialMapper(model->numsurfs);
        AllocateXModelBoneWeights(model, lod, out.m_bone_weight_data);

        out.m_name = std::format("{}_lod{}", model->name, lod);
        AddXModelBones(out, context, model);
        AddXModelMaterials(out, materialMapper, model);
        AddXModelObjects(out, model, lod, materialMapper);
        AddXModelVertices(out, model, lod);
        AddXModelVertexBoneWeights(out, model, lod);
        AddXModelFaces(out, model, lod);
    }

    void DumpObjMtl(const XModelCommon& common, const AssetDumpingContext& context, const XAssetInfo<XModel>* asset)
    {
        const auto* model = asset->Asset();
        const auto mtlFile = context.OpenAssetFile(std::format("model_export/{}.mtl", model->name));

        if (!mtlFile)
            return;

        const auto writer = obj::CreateMtlWriter(*mtlFile, context.m_zone->m_game->GetShortName(), context.m_zone->m_name);
        DistinctMapper<Material*> materialMapper(model->numsurfs);

        writer->Write(common);
    }

    void DumpObjLod(const XModelCommon& common, const AssetDumpingContext& context, const XAssetInfo<XModel>* asset, const unsigned lod)
    {
        const auto* model = asset->Asset();
        const auto assetFile = context.OpenAssetFile(GetFileNameForLod(model->name, lod, ".obj"));

        if (!assetFile)
            return;

        const auto writer =
            obj::CreateObjWriter(*assetFile, std::format("{}.mtl", model->name), context.m_zone->m_game->GetShortName(), context.m_zone->m_name);
        DistinctMapper<Material*> materialMapper(model->numsurfs);

        writer->Write(common);
    }

    void DumpXModelExportLod(const XModelCommon& common, const AssetDumpingContext& context, const XAssetInfo<XModel>* asset, const unsigned lod)
    {
        const auto* model = asset->Asset();
        const auto assetFile = context.OpenAssetFile(GetFileNameForLod(model->name, lod, ".XMODEL_EXPORT"));

        if (!assetFile)
            return;

        const auto writer = xmodel_export::CreateWriterForVersion6(*assetFile, context.m_zone->m_game->GetShortName(), context.m_zone->m_name);
        writer->Write(common);
    }

    template<typename T>
    void DumpGltfLod(
        const XModelCommon& common, const AssetDumpingContext& context, const XAssetInfo<XModel>* asset, const unsigned lod, const std::string& extension)
    {
        const auto* model = asset->Asset();
        const auto assetFile = context.OpenAssetFile(GetFileNameForLod(model->name, lod, extension));

        if (!assetFile)
            return;

        const auto output = std::make_unique<T>(*assetFile);
        const auto writer = gltf::Writer::CreateWriter(output.get(), context.m_zone->m_game->GetShortName(), context.m_zone->m_name);

        writer->Write(common);
    }

    void DumpXModelSurfs(const AssetDumpingContext& context, const XAssetInfo<XModel>* asset)
    {
        const auto* model = asset->Asset();

        for (auto currentLod = 0u; currentLod < model->numLods; currentLod++)
        {
            XModelCommon common;
            PopulateXModelWriter(common, context, currentLod, asset->Asset());

            switch (ObjWriting::Configuration.ModelOutputFormat)
            {
            case ObjWriting::Configuration_t::ModelOutputFormat_e::OBJ:
                DumpObjLod(common, context, asset, currentLod);
                if (currentLod == 0u)
                    DumpObjMtl(common, context, asset);
                break;

            case ObjWriting::Configuration_t::ModelOutputFormat_e::XMODEL_EXPORT:
                DumpXModelExportLod(common, context, asset, currentLod);
                break;

            case ObjWriting::Configuration_t::ModelOutputFormat_e::GLTF:
                DumpGltfLod<gltf::TextOutput>(common, context, asset, currentLod, ".gltf");
                break;

            case ObjWriting::Configuration_t::ModelOutputFormat_e::GLB:
                DumpGltfLod<gltf::BinOutput>(common, context, asset, currentLod, ".glb");
                break;

            default:
                assert(false);
                break;
            }
        }
    }
} // namespace

bool AssetDumperXModel::ShouldDump(XAssetInfo<XModel>* asset)
{
    return !asset->m_name.empty() && asset->m_name[0] != ',';
}

void AssetDumperXModel::DumpAsset(AssetDumpingContext& context, XAssetInfo<XModel>* asset)
{
    DumpXModelSurfs(context, asset);
}
