#include <nlohmann/json.hpp>

#include "LoaderGfxWorldT6.h"

#include "Game/T6/T6.h"

#include <cstring>

using namespace T6;

namespace
{
    class GfxWorldLoader final : public AssetCreator<AssetGfxWorld>
    {
    public:
        GfxWorldLoader(MemoryManager& memory, ISearchPath& searchPath)
            : m_memory(memory),
              m_search_path(searchPath)
        {
        }

        AssetCreationResult CreateAsset(const std::string& assetName, AssetCreationContext& context) override
        {
            const auto file = m_search_path.Open("bsp/gfxworld.json");
            if (!file.IsOpen())
                return AssetCreationResult::NoAction();

            nlohmann::json j = nlohmann::json::parse(*file.m_stream);
            
            auto* gfxWorld = m_memory.Alloc<GfxWorld>();

            gfxWorld->name = m_memory.Dup(assetName.c_str());
            gfxWorld->baseName = m_memory.Dup(static_cast<std::string>(j["baseName"]).c_str());
            gfxWorld->planeCount = j["planeCount"];
            gfxWorld->nodeCount = j["nodeCount"];
            gfxWorld->surfaceCount = j["surfaceCount"];

            
            gfxWorld->streamInfo.aabbTreeCount = j["streamInfo"]["aabbTreeCount"];
            if (gfxWorld->streamInfo.aabbTreeCount > 0)
                gfxWorld->streamInfo.aabbTrees = m_memory.Alloc<GfxStreamingAabbTree>(gfxWorld->streamInfo.aabbTreeCount);
            else
                gfxWorld->streamInfo.aabbTrees = nullptr;

            gfxWorld->streamInfo.leafRefCount = j["streamInfo"]["leafRefCount"];

            for (int i = 0; i < gfxWorld->streamInfo.aabbTreeCount; i++)
            {
                auto currentAabbTree = &gfxWorld->streamInfo.aabbTrees[i];
                auto currentAabbTreeData = j["streamInfo"]["aabbTrees"][i];

                currentAabbTree->mins.x = currentAabbTreeData["mins"]["x"];
                currentAabbTree->mins.y = currentAabbTreeData["mins"]["y"];
                currentAabbTree->mins.z = currentAabbTreeData["mins"]["z"];
                currentAabbTree->maxs.x = currentAabbTreeData["maxs"]["x"];
                currentAabbTree->maxs.y = currentAabbTreeData["maxs"]["y"];
                currentAabbTree->maxs.z = currentAabbTreeData["maxs"]["z"];
                currentAabbTree->maxStreamingDistance = currentAabbTreeData["maxStreamingDistance"];
                currentAabbTree->firstItem            = currentAabbTreeData["firstItem"          ];
                currentAabbTree->itemCount            = currentAabbTreeData["itemCount"          ];
                currentAabbTree->firstChild           = currentAabbTreeData["firstChild"         ];
                currentAabbTree->childCount           = currentAabbTreeData["childCount"         ];
                currentAabbTree->smodelCount          = currentAabbTreeData["smodelCount"        ];
                currentAabbTree->surfaceCount         = currentAabbTreeData["surfaceCount"       ];
            }

            if (gfxWorld->streamInfo.leafRefCount > 0)
                gfxWorld->streamInfo.leafRefs = m_memory.Alloc<int>(gfxWorld->streamInfo.leafRefCount);
            else
                gfxWorld->streamInfo.leafRefs = nullptr;
            for (int i = 0; i < gfxWorld->streamInfo.leafRefCount; i++)
            {
                gfxWorld->streamInfo.leafRefs[i] = j["streamInfo"]["leafRefs"][i];
            }

            gfxWorld->skyBoxModel = m_memory.Dup(static_cast<std::string>(j["skyBoxModel"]).c_str());

            const char* sunParseName = m_memory.Dup(static_cast<std::string>(j["sunParse"]["name"]).c_str());
            size_t sunParseNameLen = strlen(sunParseName);
            assert(sunParseNameLen < 63);
            memcpy(gfxWorld->sunParse.name, sunParseName, sunParseNameLen + 1);
            gfxWorld->sunParse.fogTransitionTime = j["sunParse"]["fogTransitionTime"];

            gfxWorld->sunParse.initWorldSun->control = j["sunParse"]["initWorldSun"]["control"];
            gfxWorld->sunParse.initWorldSun->exposure = j["sunParse"]["initWorldSun"]["exposure"];
            gfxWorld->sunParse.initWorldSun->angles.x = j["sunParse"]["initWorldSun"]["angles"]["x"];
            gfxWorld->sunParse.initWorldSun->angles.y = j["sunParse"]["initWorldSun"]["angles"]["y"];
            gfxWorld->sunParse.initWorldSun->angles.z = j["sunParse"]["initWorldSun"]["angles"]["z"];
            gfxWorld->sunParse.initWorldSun->ambientColor.x = j["sunParse"]["initWorldSun"]["ambientColor"]["x"];
            gfxWorld->sunParse.initWorldSun->ambientColor.y = j["sunParse"]["initWorldSun"]["ambientColor"]["y"];
            gfxWorld->sunParse.initWorldSun->ambientColor.z = j["sunParse"]["initWorldSun"]["ambientColor"]["z"];
            gfxWorld->sunParse.initWorldSun->ambientColor.w = j["sunParse"]["initWorldSun"]["ambientColor"]["w"];
            gfxWorld->sunParse.initWorldSun->sunCd.x = j["sunParse"]["initWorldSun"]["sunCd"]["x"];
            gfxWorld->sunParse.initWorldSun->sunCd.y = j["sunParse"]["initWorldSun"]["sunCd"]["y"];
            gfxWorld->sunParse.initWorldSun->sunCd.z = j["sunParse"]["initWorldSun"]["sunCd"]["z"];
            gfxWorld->sunParse.initWorldSun->sunCd.w = j["sunParse"]["initWorldSun"]["sunCd"]["w"];
            gfxWorld->sunParse.initWorldSun->sunCs.x = j["sunParse"]["initWorldSun"]["sunCs"]["x"];
            gfxWorld->sunParse.initWorldSun->sunCs.y = j["sunParse"]["initWorldSun"]["sunCs"]["y"];
            gfxWorld->sunParse.initWorldSun->sunCs.z = j["sunParse"]["initWorldSun"]["sunCs"]["z"];
            gfxWorld->sunParse.initWorldSun->sunCs.w = j["sunParse"]["initWorldSun"]["sunCs"]["w"];
            gfxWorld->sunParse.initWorldSun->skyColor.x = j["sunParse"]["initWorldSun"]["skyColor"]["x"];
            gfxWorld->sunParse.initWorldSun->skyColor.y = j["sunParse"]["initWorldSun"]["skyColor"]["y"];
            gfxWorld->sunParse.initWorldSun->skyColor.z = j["sunParse"]["initWorldSun"]["skyColor"]["z"];
            gfxWorld->sunParse.initWorldSun->skyColor.w = j["sunParse"]["initWorldSun"]["skyColor"]["w"];


            gfxWorld->sunParse.initWorldFog->baseDist = j["sunParse"]["initWorldFog"]["baseDist"];
            gfxWorld->sunParse.initWorldFog->halfDist = j["sunParse"]["initWorldFog"]["halfDist"];
            gfxWorld->sunParse.initWorldFog->baseHeight = j["sunParse"]["initWorldFog"]["baseHeight"];
            gfxWorld->sunParse.initWorldFog->halfHeight = j["sunParse"]["initWorldFog"]["halfHeight"];
            gfxWorld->sunParse.initWorldFog->sunFogPitch = j["sunParse"]["initWorldFog"]["sunFogPitch"];
            gfxWorld->sunParse.initWorldFog->sunFogYaw = j["sunParse"]["initWorldFog"]["sunFogYaw"];
            gfxWorld->sunParse.initWorldFog->sunFogInner = j["sunParse"]["initWorldFog"]["sunFogInner"];
            gfxWorld->sunParse.initWorldFog->sunFogOuter = j["sunParse"]["initWorldFog"]["sunFogOuter"];
            gfxWorld->sunParse.initWorldFog->fogOpacity = j["sunParse"]["initWorldFog"]["fogOpacity"];
            gfxWorld->sunParse.initWorldFog->sunFogOpacity = j["sunParse"]["initWorldFog"]["sunFogOpacity"];
            gfxWorld->sunParse.initWorldFog->fogColor.x = j["sunParse"]["initWorldFog"]["fogColor"]["x"];
            gfxWorld->sunParse.initWorldFog->fogColor.y = j["sunParse"]["initWorldFog"]["fogColor"]["y"];
            gfxWorld->sunParse.initWorldFog->fogColor.z = j["sunParse"]["initWorldFog"]["fogColor"]["z"];
            gfxWorld->sunParse.initWorldFog->sunFogColor.x = j["sunParse"]["initWorldFog"]["sunFogColor"]["x"];
            gfxWorld->sunParse.initWorldFog->sunFogColor.y = j["sunParse"]["initWorldFog"]["sunFogColor"]["y"];
            gfxWorld->sunParse.initWorldFog->sunFogColor.z = j["sunParse"]["initWorldFog"]["sunFogColor"]["z"];

            if (j["sunLight"].size() > 0)
            {
                gfxWorld->sunLight = m_memory.Alloc<GfxLight>();

                gfxWorld->sunLight->type = static_cast<unsigned char>(j["sunLight"]["type"]);
                gfxWorld->sunLight->canUseShadowMap = static_cast<unsigned char>(j["sunLight"]["canUseShadowMap"]);
                gfxWorld->sunLight->shadowmapVolume = static_cast<unsigned char>(j["sunLight"]["shadowmapVolume"]);
                gfxWorld->sunLight->cullDist = j["sunLight"]["cullDist"];
                
                gfxWorld->sunLight->color.x = j["sunLight"]["color"]["x"];
                gfxWorld->sunLight->color.y = j["sunLight"]["color"]["y"];
                gfxWorld->sunLight->color.z = j["sunLight"]["color"]["z"];
                
                gfxWorld->sunLight->dir.x = j["sunLight"]["dir"]["x"];
                gfxWorld->sunLight->dir.y = j["sunLight"]["dir"]["y"];
                gfxWorld->sunLight->dir.z = j["sunLight"]["dir"]["z"];
                
                gfxWorld->sunLight->origin.x = j["sunLight"]["origin"]["x"];
                gfxWorld->sunLight->origin.y = j["sunLight"]["origin"]["y"];
                gfxWorld->sunLight->origin.z = j["sunLight"]["origin"]["z"];
                
                gfxWorld->sunLight->radius = j["sunLight"]["radius"];
                gfxWorld->sunLight->cosHalfFovOuter = j["sunLight"]["cosHalfFovOuter"];
                gfxWorld->sunLight->cosHalfFovInner = j["sunLight"]["cosHalfFovInner"];
                gfxWorld->sunLight->exponent = j["sunLight"]["exponent"];
                gfxWorld->sunLight->spotShadowIndex = j["sunLight"]["spotShadowIndex"];
                gfxWorld->sunLight->dAttenuation = j["sunLight"]["dAttenuation"];
                gfxWorld->sunLight->roundness = j["sunLight"]["roundness"];
                
                gfxWorld->sunLight->angles.x = j["sunLight"]["angles"]["x"];
                gfxWorld->sunLight->angles.y = j["sunLight"]["angles"]["y"];
                gfxWorld->sunLight->angles.z = j["sunLight"]["angles"]["z"];
                
                gfxWorld->sunLight->spotShadowHiDistance = j["sunLight"]["spotShadowHiDistance"];
                
                gfxWorld->sunLight->diffuseColor.x = j["sunLight"]["diffuseColor"]["x"];
                gfxWorld->sunLight->diffuseColor.y = j["sunLight"]["diffuseColor"]["y"];
                gfxWorld->sunLight->diffuseColor.z = j["sunLight"]["diffuseColor"]["z"];
                gfxWorld->sunLight->diffuseColor.w = j["sunLight"]["diffuseColor"]["w"];
                
                gfxWorld->sunLight->shadowColor.x = j["sunLight"]["shadowColor"]["x"];
                gfxWorld->sunLight->shadowColor.y = j["sunLight"]["shadowColor"]["y"];
                gfxWorld->sunLight->shadowColor.z = j["sunLight"]["shadowColor"]["z"];
                gfxWorld->sunLight->shadowColor.w = j["sunLight"]["shadowColor"]["w"];
                
                gfxWorld->sunLight->falloff.x = j["sunLight"]["falloff"]["x"];
                gfxWorld->sunLight->falloff.y = j["sunLight"]["falloff"]["y"];
                gfxWorld->sunLight->falloff.z = j["sunLight"]["falloff"]["z"];
                gfxWorld->sunLight->falloff.w = j["sunLight"]["falloff"]["w"];
                
                gfxWorld->sunLight->aAbB.x = j["sunLight"]["aAbB"]["x"];
                gfxWorld->sunLight->aAbB.y = j["sunLight"]["aAbB"]["y"];
                gfxWorld->sunLight->aAbB.z = j["sunLight"]["aAbB"]["z"];
                gfxWorld->sunLight->aAbB.w = j["sunLight"]["aAbB"]["w"];
                
                gfxWorld->sunLight->cookieControl0.x = j["sunLight"]["cookieControl0"]["x"];
                gfxWorld->sunLight->cookieControl0.y = j["sunLight"]["cookieControl0"]["y"];
                gfxWorld->sunLight->cookieControl0.z = j["sunLight"]["cookieControl0"]["z"];
                gfxWorld->sunLight->cookieControl0.w = j["sunLight"]["cookieControl0"]["w"];
                
                gfxWorld->sunLight->cookieControl1.x = j["sunLight"]["cookieControl1"]["x"];
                gfxWorld->sunLight->cookieControl1.y = j["sunLight"]["cookieControl1"]["y"];
                gfxWorld->sunLight->cookieControl1.z = j["sunLight"]["cookieControl1"]["z"];
                gfxWorld->sunLight->cookieControl1.w = j["sunLight"]["cookieControl1"]["w"];
                
                gfxWorld->sunLight->cookieControl2.x = j["sunLight"]["cookieControl2"]["x"];
                gfxWorld->sunLight->cookieControl2.y = j["sunLight"]["cookieControl2"]["y"];
                gfxWorld->sunLight->cookieControl2.z = j["sunLight"]["cookieControl2"]["z"];
                gfxWorld->sunLight->cookieControl2.w = j["sunLight"]["cookieControl2"]["w"];
                
                gfxWorld->sunLight->viewMatrix.m[0].x = j["sunLight"]["viewMatrix"]["0"]["x"];
                gfxWorld->sunLight->viewMatrix.m[0].y = j["sunLight"]["viewMatrix"]["0"]["y"];
                gfxWorld->sunLight->viewMatrix.m[0].z = j["sunLight"]["viewMatrix"]["0"]["z"];
                gfxWorld->sunLight->viewMatrix.m[0].w = j["sunLight"]["viewMatrix"]["0"]["w"];
                
                gfxWorld->sunLight->viewMatrix.m[1].x = j["sunLight"]["viewMatrix"]["1"]["x"];
                gfxWorld->sunLight->viewMatrix.m[1].y = j["sunLight"]["viewMatrix"]["1"]["y"];
                gfxWorld->sunLight->viewMatrix.m[1].z = j["sunLight"]["viewMatrix"]["1"]["z"];
                gfxWorld->sunLight->viewMatrix.m[1].w = j["sunLight"]["viewMatrix"]["1"]["w"];
                
                gfxWorld->sunLight->viewMatrix.m[2].x = j["sunLight"]["viewMatrix"]["2"]["x"];
                gfxWorld->sunLight->viewMatrix.m[2].y = j["sunLight"]["viewMatrix"]["2"]["y"];
                gfxWorld->sunLight->viewMatrix.m[2].z = j["sunLight"]["viewMatrix"]["2"]["z"];
                gfxWorld->sunLight->viewMatrix.m[2].w = j["sunLight"]["viewMatrix"]["2"]["w"];
                
                gfxWorld->sunLight->viewMatrix.m[3].x = j["sunLight"]["viewMatrix"]["3"]["x"];
                gfxWorld->sunLight->viewMatrix.m[3].y = j["sunLight"]["viewMatrix"]["3"]["y"];
                gfxWorld->sunLight->viewMatrix.m[3].z = j["sunLight"]["viewMatrix"]["3"]["z"];
                gfxWorld->sunLight->viewMatrix.m[3].w = j["sunLight"]["viewMatrix"]["3"]["w"];
                
                gfxWorld->sunLight->projMatrix.m[0].x = j["sunLight"]["projMatrix"]["0"]["x"];
                gfxWorld->sunLight->projMatrix.m[0].y = j["sunLight"]["projMatrix"]["0"]["y"];
                gfxWorld->sunLight->projMatrix.m[0].z = j["sunLight"]["projMatrix"]["0"]["z"];
                gfxWorld->sunLight->projMatrix.m[0].w = j["sunLight"]["projMatrix"]["0"]["w"];
                
                gfxWorld->sunLight->projMatrix.m[1].x = j["sunLight"]["projMatrix"]["1"]["x"];
                gfxWorld->sunLight->projMatrix.m[1].y = j["sunLight"]["projMatrix"]["1"]["y"];
                gfxWorld->sunLight->projMatrix.m[1].z = j["sunLight"]["projMatrix"]["1"]["z"];
                gfxWorld->sunLight->projMatrix.m[1].w = j["sunLight"]["projMatrix"]["1"]["w"];
                
                gfxWorld->sunLight->projMatrix.m[2].x = j["sunLight"]["projMatrix"]["2"]["x"];
                gfxWorld->sunLight->projMatrix.m[2].y = j["sunLight"]["projMatrix"]["2"]["y"];
                gfxWorld->sunLight->projMatrix.m[2].z = j["sunLight"]["projMatrix"]["2"]["z"];
                gfxWorld->sunLight->projMatrix.m[2].w = j["sunLight"]["projMatrix"]["2"]["w"];

                gfxWorld->sunLight->projMatrix.m[3].x = j["sunLight"]["projMatrix"]["3"]["x"];
                gfxWorld->sunLight->projMatrix.m[3].y = j["sunLight"]["projMatrix"]["3"]["y"];
                gfxWorld->sunLight->projMatrix.m[3].z = j["sunLight"]["projMatrix"]["3"]["z"];
                gfxWorld->sunLight->projMatrix.m[3].w = j["sunLight"]["projMatrix"]["3"]["w"];

                std::string defName = j["sunLight"]["defName"];
                if (defName.length() > 0)
                {
                    auto* assetInfo = context.LoadDependency<AssetLightDef>(defName);
                    if (!assetInfo)
                    {
                        printf("can't find sunLight def\n");
                        return AssetCreationResult::Failure();
                    }
                    gfxWorld->sunLight->def = assetInfo->Asset();
                }
                else
                    gfxWorld->sunLight->def = nullptr;
                
            }
            else
                gfxWorld->sunLight = nullptr;

            gfxWorld->sunPrimaryLightIndex = j["sunPrimaryLightIndex"];
            gfxWorld->primaryLightCount = j["primaryLightCount"];

            gfxWorld->coronaCount = j["coronaCount"];
            if (gfxWorld->coronaCount > 0)
                gfxWorld->coronas = m_memory.Alloc<GfxLightCorona>(gfxWorld->coronaCount);
            else
                gfxWorld->coronas = nullptr;
            for (unsigned int i = 0; i < gfxWorld->coronaCount; i++)
            {
                auto currentCorona = &gfxWorld->coronas[i];
                auto currentCoronaData = j["coronas"][i];

                currentCorona->origin.x = currentCoronaData["origin"]["x"];
                currentCorona->origin.y = currentCoronaData["origin"]["y"];
                currentCorona->origin.z = currentCoronaData["origin"]["z"];
                currentCorona->color.x = currentCoronaData["color"]["x"];
                currentCorona->color.y = currentCoronaData["color"]["y"];
                currentCorona->color.z = currentCoronaData["color"]["z"];
                currentCorona->radius = currentCoronaData["radius"];
                currentCorona->intensity = currentCoronaData["intensity"];
            }

            gfxWorld->shadowMapVolumeCount = j["shadowMapVolumeCount"];
            if (gfxWorld->shadowMapVolumeCount > 0)
                gfxWorld->shadowMapVolumes = m_memory.Alloc<GfxShadowMapVolume>(gfxWorld->shadowMapVolumeCount);
            else
                gfxWorld->shadowMapVolumes = nullptr;
            for (unsigned int i = 0; i < gfxWorld->shadowMapVolumeCount; i++)
            {
                auto currentShadowMapVolume = &gfxWorld->shadowMapVolumes[i];
                auto currentShadowMapVolumeData = j["shadowMapVolumes"][i];

                currentShadowMapVolume->control = currentShadowMapVolumeData["control"];
                currentShadowMapVolume->padding1 = currentShadowMapVolumeData["padding1"];
                currentShadowMapVolume->padding2 = currentShadowMapVolumeData["padding2"];
                currentShadowMapVolume->padding3 = currentShadowMapVolumeData["padding3"];
            }

            gfxWorld->shadowMapVolumePlaneCount = j["shadowMapVolumePlaneCount"];
            if (gfxWorld->shadowMapVolumePlaneCount > 0)
                gfxWorld->shadowMapVolumePlanes = m_memory.Alloc<GfxVolumePlane>(gfxWorld->shadowMapVolumePlaneCount);
            else
                gfxWorld->shadowMapVolumePlanes = nullptr;
            for (unsigned int i = 0; i < gfxWorld->shadowMapVolumePlaneCount; i++)
            {
                auto currentShadowMapVolumePlane = &gfxWorld->shadowMapVolumePlanes[i];
                auto currentShadowMapVolumePlaneData = j["shadowMapVolumePlanes"][i];

                currentShadowMapVolumePlane->plane.x = currentShadowMapVolumePlaneData["plane"]["x"];
                currentShadowMapVolumePlane->plane.y = currentShadowMapVolumePlaneData["plane"]["y"];
                currentShadowMapVolumePlane->plane.z = currentShadowMapVolumePlaneData["plane"]["z"];
                currentShadowMapVolumePlane->plane.w = currentShadowMapVolumePlaneData["plane"]["w"];
            }

            gfxWorld->exposureVolumeCount = j["exposureVolumeCount"];
            if (gfxWorld->exposureVolumeCount > 0)
                gfxWorld->exposureVolumes = m_memory.Alloc<GfxExposureVolume>(gfxWorld->exposureVolumeCount);
            else
                gfxWorld->exposureVolumes = nullptr;
            for (unsigned int i = 0; i < gfxWorld->exposureVolumeCount; i++)
            {
                auto currentExposureVolume = &gfxWorld->exposureVolumes[i];
                auto currentExposureVolumeData = j["exposureVolumes"][i];

               currentExposureVolume->control = currentExposureVolumeData["control"];
                currentExposureVolume->exposure = currentExposureVolumeData["exposure"];
                currentExposureVolume->luminanceIncreaseScale = currentExposureVolumeData["luminanceIncreaseScale"];
                currentExposureVolume->luminanceDecreaseScale = currentExposureVolumeData["luminanceDecreaseScale"];
                currentExposureVolume->featherRange = currentExposureVolumeData["featherRange"];
                currentExposureVolume->featherAdjust = currentExposureVolumeData["featherAdjust"];
            }

            gfxWorld->exposureVolumePlaneCount = j["exposureVolumePlaneCount"];
            if (gfxWorld->exposureVolumePlaneCount > 0)
                gfxWorld->exposureVolumePlanes = m_memory.Alloc<GfxVolumePlane>(gfxWorld->exposureVolumePlaneCount);
            else
                gfxWorld->exposureVolumePlanes = nullptr;
            for (unsigned int i = 0; i < gfxWorld->exposureVolumePlaneCount; i++)
            {
                auto currentExposureVolumePlane = &gfxWorld->exposureVolumePlanes[i];
                auto currentExposureVolumePlaneData = j["exposureVolumePlanes"][i];

                currentExposureVolumePlane->plane.x = currentExposureVolumePlaneData["plane"]["x"];
                currentExposureVolumePlane->plane.y = currentExposureVolumePlaneData["plane"]["y"];
                currentExposureVolumePlane->plane.z = currentExposureVolumePlaneData["plane"]["z"];
                currentExposureVolumePlane->plane.w = currentExposureVolumePlaneData["plane"]["w"];
            }

            gfxWorld->worldFogVolumeCount = j["worldFogVolumeCount"];
            if (gfxWorld->worldFogVolumeCount > 0)
                gfxWorld->worldFogVolumes = m_memory.Alloc<GfxWorldFogVolume>(gfxWorld->worldFogVolumeCount);
            else
                gfxWorld->worldFogVolumes = nullptr;
            for (unsigned int i = 0; i < gfxWorld->worldFogVolumeCount; i++)
            {
                auto currentWorldFogVolume = &gfxWorld->worldFogVolumes[i];
                auto currentWorldFogVolumeData = j["worldFogVolumes"][i];

                currentWorldFogVolume->control = currentWorldFogVolumeData["control"];
                currentWorldFogVolume->mins.x = currentWorldFogVolumeData["mins"]["x"];
                currentWorldFogVolume->mins.y = currentWorldFogVolumeData["mins"]["y"];
                currentWorldFogVolume->mins.z = currentWorldFogVolumeData["mins"]["z"];
                currentWorldFogVolume->maxs.x = currentWorldFogVolumeData["maxs"]["x"];
                currentWorldFogVolume->maxs.y = currentWorldFogVolumeData["maxs"]["y"];
                currentWorldFogVolume->maxs.z = currentWorldFogVolumeData["maxs"]["z"];
                currentWorldFogVolume->fogTransitionTime = currentWorldFogVolumeData["fogTransitionTime"];
                currentWorldFogVolume->controlEx = currentWorldFogVolumeData["controlEx"];
                currentWorldFogVolume->volumeWorldFog->baseDist = currentWorldFogVolumeData["volumeWorldFog"]["baseDist"];
                currentWorldFogVolume->volumeWorldFog->halfDist = currentWorldFogVolumeData["volumeWorldFog"]["halfDist"];
                currentWorldFogVolume->volumeWorldFog->baseHeight = currentWorldFogVolumeData["volumeWorldFog"]["baseHeight"];
                currentWorldFogVolume->volumeWorldFog->halfHeight = currentWorldFogVolumeData["volumeWorldFog"]["halfHeight"];
                currentWorldFogVolume->volumeWorldFog->sunFogPitch = currentWorldFogVolumeData["volumeWorldFog"]["sunFogPitch"];
                currentWorldFogVolume->volumeWorldFog->sunFogYaw = currentWorldFogVolumeData["volumeWorldFog"]["sunFogYaw"];
                currentWorldFogVolume->volumeWorldFog->sunFogInner = currentWorldFogVolumeData["volumeWorldFog"]["sunFogInner"];
                currentWorldFogVolume->volumeWorldFog->sunFogOuter = currentWorldFogVolumeData["volumeWorldFog"]["sunFogOuter"];
                currentWorldFogVolume->volumeWorldFog->fogOpacity = currentWorldFogVolumeData["volumeWorldFog"]["fogOpacity"];
                currentWorldFogVolume->volumeWorldFog->sunFogOpacity = currentWorldFogVolumeData["volumeWorldFog"]["sunFogOpacity"];
                currentWorldFogVolume->volumeWorldFog->fogColor.x = currentWorldFogVolumeData["volumeWorldFog"]["fogColor"]["x"];
                currentWorldFogVolume->volumeWorldFog->fogColor.y = currentWorldFogVolumeData["volumeWorldFog"]["fogColor"]["y"];
                currentWorldFogVolume->volumeWorldFog->fogColor.z = currentWorldFogVolumeData["volumeWorldFog"]["fogColor"]["z"];
                currentWorldFogVolume->volumeWorldFog->sunFogColor.x = currentWorldFogVolumeData["volumeWorldFog"]["sunFogColor"]["x"];
                currentWorldFogVolume->volumeWorldFog->sunFogColor.y = currentWorldFogVolumeData["volumeWorldFog"]["sunFogColor"]["y"];
                currentWorldFogVolume->volumeWorldFog->sunFogColor.z = currentWorldFogVolumeData["volumeWorldFog"]["sunFogColor"]["z"];
            }

            gfxWorld->worldFogVolumePlaneCount = j["worldFogVolumePlaneCount"];
            if (gfxWorld->worldFogVolumePlaneCount > 0)
                gfxWorld->worldFogVolumePlanes = m_memory.Alloc<GfxVolumePlane>(gfxWorld->worldFogVolumePlaneCount);
            else
                gfxWorld->worldFogVolumePlanes = nullptr;
            for (unsigned int i = 0; i < gfxWorld->worldFogVolumePlaneCount; i++)
            {
                auto currentWorldFogVolumePlane = &gfxWorld->worldFogVolumePlanes[i];
                auto currentWorldFogVolumePlaneData = j["worldFogVolumePlanes"][i];

                currentWorldFogVolumePlane->plane.x = currentWorldFogVolumePlaneData["plane"]["x"];
                currentWorldFogVolumePlane->plane.y = currentWorldFogVolumePlaneData["plane"]["y"];
                currentWorldFogVolumePlane->plane.z = currentWorldFogVolumePlaneData["plane"]["z"];
                currentWorldFogVolumePlane->plane.w = currentWorldFogVolumePlaneData["plane"]["w"];
            }

            gfxWorld->worldFogModifierVolumeCount = j["worldFogModifierVolumeCount"];
            if (gfxWorld->worldFogModifierVolumeCount > 0)
                gfxWorld->worldFogModifierVolumes = m_memory.Alloc<GfxWorldFogModifierVolume>(gfxWorld->worldFogModifierVolumeCount);
            else
                gfxWorld->worldFogModifierVolumes = nullptr;
            for (unsigned int i = 0; i < gfxWorld->worldFogModifierVolumeCount; i++)
            {
                auto currentWorldFogModifierVolume = &gfxWorld->worldFogModifierVolumes[i];
                auto currentWorldFogModifierVolumeData = j["worldFogModifierVolumes"][i];

                currentWorldFogModifierVolume->control         = currentWorldFogModifierVolumeData["control"];
                currentWorldFogModifierVolume->minX            = currentWorldFogModifierVolumeData["minX"];
                currentWorldFogModifierVolume->minY            = currentWorldFogModifierVolumeData["minY"];
                currentWorldFogModifierVolume->minZ            = currentWorldFogModifierVolumeData["minZ"];
                currentWorldFogModifierVolume->maxX            = currentWorldFogModifierVolumeData["maxX"];
                currentWorldFogModifierVolume->maxY            = currentWorldFogModifierVolumeData["maxY"];
                currentWorldFogModifierVolume->maxZ            = currentWorldFogModifierVolumeData["maxZ"];
                currentWorldFogModifierVolume->controlEx       = currentWorldFogModifierVolumeData["controlEx"];
                currentWorldFogModifierVolume->transitionTime  = currentWorldFogModifierVolumeData["transitionTime"];
                currentWorldFogModifierVolume->depthScale      = currentWorldFogModifierVolumeData["depthScale"];
                currentWorldFogModifierVolume->heightScale     = currentWorldFogModifierVolumeData["heightScale"];
                currentWorldFogModifierVolume->colorAdjust.x   = currentWorldFogModifierVolumeData["colorAdjust"]["x"];
                currentWorldFogModifierVolume->colorAdjust.y   = currentWorldFogModifierVolumeData["colorAdjust"]["y"];
                currentWorldFogModifierVolume->colorAdjust.z   = currentWorldFogModifierVolumeData["colorAdjust"]["z"];
                currentWorldFogModifierVolume->colorAdjust.w   = currentWorldFogModifierVolumeData["colorAdjust"]["w"];
            }

            gfxWorld->worldFogModifierVolumePlaneCount = j["worldFogModifierVolumePlaneCount"];
            if (gfxWorld->worldFogModifierVolumePlaneCount > 0)
                gfxWorld->worldFogModifierVolumePlanes = m_memory.Alloc<GfxVolumePlane>(gfxWorld->worldFogModifierVolumePlaneCount);
            else
                gfxWorld->worldFogModifierVolumePlanes = nullptr;
            for (unsigned int i = 0; i < gfxWorld->worldFogModifierVolumePlaneCount; i++)
            {
                auto currentWorldFogModifierVolumePlane = &gfxWorld->worldFogModifierVolumePlanes[i];
                auto currentWorldFogModifierVolumePlaneData = j["worldFogModifierVolumePlanes"][i];

                currentWorldFogModifierVolumePlane->plane.x = currentWorldFogModifierVolumePlaneData["plane"]["x"];
                currentWorldFogModifierVolumePlane->plane.y = currentWorldFogModifierVolumePlaneData["plane"]["y"];
                currentWorldFogModifierVolumePlane->plane.z = currentWorldFogModifierVolumePlaneData["plane"]["z"];
                currentWorldFogModifierVolumePlane->plane.w = currentWorldFogModifierVolumePlaneData["plane"]["w"];
            }

            gfxWorld->lutVolumeCount = j["lutVolumeCount"];
            if (gfxWorld->lutVolumeCount > 0)
                gfxWorld->lutVolumes = m_memory.Alloc<GfxLutVolume>(gfxWorld->lutVolumeCount);
            else
                gfxWorld->lutVolumes = nullptr;
            for (unsigned int i = 0; i < gfxWorld->lutVolumeCount; i++)
            {
                auto currentLutVolume = &gfxWorld->lutVolumes[i];
                auto currentLutVolumeData = j["lutVolumes"][i];

                currentLutVolume->mins.x            = currentLutVolumeData["mins"]["x"];
                currentLutVolume->mins.y            = currentLutVolumeData["mins"]["y"];
                currentLutVolume->mins.z            = currentLutVolumeData["mins"]["z"];
                currentLutVolume->control           = currentLutVolumeData["control"];
                currentLutVolume->maxs.x            = currentLutVolumeData["maxs"]["x"];
                currentLutVolume->maxs.y            = currentLutVolumeData["maxs"]["y"];
                currentLutVolume->maxs.z            = currentLutVolumeData["maxs"]["z"];
                currentLutVolume->lutTransitionTime = currentLutVolumeData["lutTransitionTime"];
                currentLutVolume->lutIndex          = currentLutVolumeData["lutIndex"];
            }

            gfxWorld->lutVolumePlaneCount = j["lutVolumePlaneCount"];
            if (gfxWorld->lutVolumePlaneCount > 0)
                gfxWorld->lutVolumePlanes = m_memory.Alloc<GfxVolumePlane>(gfxWorld->lutVolumePlaneCount);
            else
                gfxWorld->lutVolumePlanes = nullptr;
            for (unsigned int i = 0; i < gfxWorld->lutVolumePlaneCount; i++)
            {
                auto currentLutVolumePlane = &gfxWorld->lutVolumePlanes[i];
                auto currentLutVolumePlaneData = j["lutVolumePlanes"][i];

                currentLutVolumePlane->plane.x = currentLutVolumePlaneData["plane"]["x"];
                currentLutVolumePlane->plane.y = currentLutVolumePlaneData["plane"]["y"];
                currentLutVolumePlane->plane.z = currentLutVolumePlaneData["plane"]["z"];
                currentLutVolumePlane->plane.w = currentLutVolumePlaneData["plane"]["w"];
            }

            ////////*

            gfxWorld->skyDynIntensity.angle0 = j["skyDynIntensity"]["angle0"];
            gfxWorld->skyDynIntensity.angle1 = j["skyDynIntensity"]["angle1"];
            gfxWorld->skyDynIntensity.factor0 = j["skyDynIntensity"]["factor0"];
            gfxWorld->skyDynIntensity.factor1 = j["skyDynIntensity"]["factor1"];


            gfxWorld->dpvsPlanes.cellCount = j["dpvsPlanes"]["cellCount"];

            if (gfxWorld->planeCount > 0)
                gfxWorld->dpvsPlanes.planes = m_memory.Alloc<cplane_s>(gfxWorld->planeCount);
            else
                gfxWorld->dpvsPlanes.planes = nullptr;
            for (int i = 0; i < gfxWorld->planeCount; i++)
            {
                auto currentDpvsPlane = &gfxWorld->dpvsPlanes.planes[i];
                auto currentDpvsPlaneData = j["dpvsPlanes"]["planes"][i];

                currentDpvsPlane->normal.x  = currentDpvsPlaneData["normal"]["x"];
                currentDpvsPlane->normal.y  = currentDpvsPlaneData["normal"]["y"];
                currentDpvsPlane->normal.z  = currentDpvsPlaneData["normal"]["z"];
                currentDpvsPlane->dist      = currentDpvsPlaneData["dist"];
                currentDpvsPlane->type      = static_cast<unsigned char>(currentDpvsPlaneData["type"]);
                currentDpvsPlane->signbits  = static_cast<unsigned char>(currentDpvsPlaneData["signbits"]);
                currentDpvsPlane->pad[0]    = static_cast<unsigned char>(currentDpvsPlaneData["pad"]["0"]);
                currentDpvsPlane->pad[1]    = static_cast<unsigned char>(currentDpvsPlaneData["pad"]["1"]);
            }
            
            if (gfxWorld->nodeCount > 0)
                gfxWorld->dpvsPlanes.nodes = m_memory.Alloc<uint16_t>(gfxWorld->nodeCount);
            else
                gfxWorld->dpvsPlanes.nodes = nullptr;
            for (int i = 0; i < gfxWorld->nodeCount; i++)
            {
                gfxWorld->dpvsPlanes.nodes[i] = j["dpvsPlanes"]["nodes"][i];
            }

            if (gfxWorld->dpvsPlanes.cellCount * 512 > 0)
            {
                gfxWorld->dpvsPlanes.sceneEntCellBits = m_memory.Alloc<unsigned int>(gfxWorld->dpvsPlanes.cellCount * 512);
                std::string sceneEntCellBitsFileName = j["dpvsPlanes"]["sceneEntCellBits_Filepath"];
                auto sceneEntCellBitsFile = m_search_path.Open(sceneEntCellBitsFileName);
                if (sceneEntCellBitsFile.IsOpen())
                {
                    auto& sceneEntCellBitsStream = *sceneEntCellBitsFile.m_stream;
                    sceneEntCellBitsStream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvsPlanes.sceneEntCellBits), 
                        gfxWorld->dpvsPlanes.cellCount * 512 * sizeof(unsigned int)
                    );
                }
            }
            else
                gfxWorld->dpvsPlanes.sceneEntCellBits = nullptr;

            gfxWorld->cellBitsCount = j["cellBitsCount"];

            if (gfxWorld->dpvsPlanes.cellCount > 0)
                gfxWorld->cells = m_memory.Alloc<GfxCell>(gfxWorld->dpvsPlanes.cellCount);
            else
                gfxWorld->cells = nullptr;
            for (int i = 0; i < gfxWorld->dpvsPlanes.cellCount; i++)
            {
                auto currentCell = &gfxWorld->cells[i];
                auto currentCellData = j["cells"][i];

                currentCell->mins.x = currentCellData["mins"]["x"];
                currentCell->mins.y = currentCellData["mins"]["y"];
                currentCell->mins.z = currentCellData["mins"]["z"];
                currentCell->maxs.x = currentCellData["maxs"]["x"];
                currentCell->maxs.y = currentCellData["maxs"]["y"];
                currentCell->maxs.z = currentCellData["maxs"]["z"];
                currentCell->aabbTreeCount = currentCellData["aabbTreeCount"];
                currentCell->portalCount = currentCellData["portalCount"];
                currentCell->reflectionProbeCount = static_cast<unsigned char>(currentCellData["reflectionProbeCount"]);

                if (currentCell->aabbTreeCount > 0)
                    currentCell->aabbTree = m_memory.Alloc<GfxAabbTree>(currentCell->aabbTreeCount);
                else
                    currentCell->aabbTree = nullptr;
                for (int k = 0; k < currentCell->aabbTreeCount; k++)
                {
                    auto currentAabbTree = &currentCell->aabbTree[k];
                    auto currentAabbTreeData = currentCellData["aabbTree"][k];

                    currentAabbTree->mins.x               = currentAabbTreeData["mins"]["x"];
                    currentAabbTree->mins.y               = currentAabbTreeData["mins"]["y"];
                    currentAabbTree->mins.z               = currentAabbTreeData["mins"]["z"];
                    currentAabbTree->maxs.x               = currentAabbTreeData["maxs"]["x"];
                    currentAabbTree->maxs.y               = currentAabbTreeData["maxs"]["y"];
                    currentAabbTree->maxs.z               = currentAabbTreeData["maxs"]["z"];
                    currentAabbTree->childCount         = currentAabbTreeData["childCount"];
                    currentAabbTree->surfaceCount       = currentAabbTreeData["surfaceCount"];
                    currentAabbTree->startSurfIndex     = currentAabbTreeData["startSurfIndex"];
                    currentAabbTree->smodelIndexCount   = currentAabbTreeData["smodelIndexCount"];
                    currentAabbTree->childrenOffset     = currentAabbTreeData["childrenOffset"];

                    if (currentAabbTree->smodelIndexCount > 0)
                        currentAabbTree->smodelIndexes = m_memory.Alloc<uint16_t>(currentAabbTree->smodelIndexCount);
                    else
                        currentAabbTree->smodelIndexes = nullptr;
                    for (unsigned int l = 0; l < currentAabbTree->smodelIndexCount; l++)
                    {
                        currentAabbTree->smodelIndexes[l] = currentAabbTreeData["smodelIndexes"][l];
                    }
                }

                if (currentCell->portalCount > 0)
                    currentCell->portals = m_memory.Alloc<GfxPortal>(currentCell->portalCount);
                else
                    currentCell->portals = nullptr;
                for (int k = 0; k < currentCell->portalCount; k++)
                {
                    auto currentPortal = &currentCell->portals[k];
                    auto currentPortalData = currentCellData["portals"][k];

                    currentPortal->writable.isQueued = false;
                    currentPortal->writable.isAncestor = false;
                    currentPortal->writable.recursionDepth = 0;
                    currentPortal->writable.hullPointCount = 0;
                    currentPortal->writable.hullPoints = nullptr;
                    currentPortal->writable.queuedParent = nullptr;
                    
                    currentPortal->plane.coeffs.x = currentPortalData["plane"]["coeffs"]["x"];
                    currentPortal->plane.coeffs.y = currentPortalData["plane"]["coeffs"]["y"];
                    currentPortal->plane.coeffs.z = currentPortalData["plane"]["coeffs"]["z"];
                    currentPortal->plane.coeffs.w = currentPortalData["plane"]["coeffs"]["w"];
                    currentPortal->plane.side[0] = static_cast<unsigned char>(currentPortalData["plane"]["side"]["0"]);
                    currentPortal->plane.side[1] = static_cast<unsigned char>(currentPortalData["plane"]["side"]["1"]);
                    currentPortal->plane.side[2] = static_cast<unsigned char>(currentPortalData["plane"]["side"]["2"]);
                    currentPortal->plane.pad = static_cast<unsigned char>(currentPortalData["plane"]["pad"]);

                    currentPortal->hullAxis[0].x = currentPortalData["hullAxis"]["0"]["x"];
                    currentPortal->hullAxis[0].y = currentPortalData["hullAxis"]["0"]["y"];
                    currentPortal->hullAxis[0].z = currentPortalData["hullAxis"]["0"]["z"];
                    currentPortal->hullAxis[1].x = currentPortalData["hullAxis"]["1"]["x"];
                    currentPortal->hullAxis[1].y = currentPortalData["hullAxis"]["1"]["y"];
                    currentPortal->hullAxis[1].z = currentPortalData["hullAxis"]["1"]["z"];

                    currentPortal->bounds[0].x = currentPortalData["bounds"]["0"]["x"];
                    currentPortal->bounds[0].y = currentPortalData["bounds"]["0"]["y"];
                    currentPortal->bounds[0].z = currentPortalData["bounds"]["0"]["z"];
                    currentPortal->bounds[1].x = currentPortalData["bounds"]["1"]["x"];
                    currentPortal->bounds[1].y = currentPortalData["bounds"]["1"]["y"];
                    currentPortal->bounds[1].z = currentPortalData["bounds"]["1"]["z"];

                    size_t cellReferenceIndex = currentPortalData["cellReferenceIndex"];
                    currentPortal->cell = &gfxWorld->cells[cellReferenceIndex];

                    currentPortal->vertexCount = static_cast<unsigned char>(currentPortalData["vertexCount"]);
                    if (currentPortal->vertexCount > 0)
                        currentPortal->vertices = m_memory.Alloc<vec3_t>(currentPortal->vertexCount);
                    else
                        currentPortal->vertices = nullptr;
                    for (int l = 0; l < currentPortal->vertexCount; l++)
                    {
                        currentPortal->vertices[l].x = currentPortalData["vertices"][l]["x"];
                        currentPortal->vertices[l].y = currentPortalData["vertices"][l]["y"];
                        currentPortal->vertices[l].z = currentPortalData["vertices"][l]["z"];
                    }
                }

                if (currentCell->reflectionProbeCount > 0)
                    currentCell->reflectionProbes = m_memory.Alloc<char>(currentCell->reflectionProbeCount);
                else
                    currentCell->reflectionProbes = nullptr;
                for (int k = 0; k < currentCell->reflectionProbeCount; k++)
                {
                    currentCell->reflectionProbes[k] = static_cast<unsigned char>(currentCellData["reflectionProbes"][k]);
                }
            }

            gfxWorld->draw.reflectionProbeCount = j["draw"]["reflectionProbeCount"];
            gfxWorld->draw.reflectionProbes = m_memory.Alloc<GfxReflectionProbe>(gfxWorld->draw.reflectionProbeCount);
            for (unsigned int i = 0; i < gfxWorld->draw.reflectionProbeCount; i++)
            {
                auto currentRProbe = &gfxWorld->draw.reflectionProbes[i];
                auto currentRProbeData = j["draw"]["reflectionProbes"][i];

                currentRProbe->origin.x = currentRProbeData["origin"]["x"];
                currentRProbe->origin.y = currentRProbeData["origin"]["y"];
                currentRProbe->origin.z = currentRProbeData["origin"]["z"];

                currentRProbe->lightingSH.V0.x = currentRProbeData["lightingSH"]["V0"]["x"];
                currentRProbe->lightingSH.V0.y = currentRProbeData["lightingSH"]["V0"]["y"];
                currentRProbe->lightingSH.V0.z = currentRProbeData["lightingSH"]["V0"]["z"];
                currentRProbe->lightingSH.V0.w = currentRProbeData["lightingSH"]["V0"]["w"];

                currentRProbe->lightingSH.V1.x = currentRProbeData["lightingSH"]["V1"]["x"];
                currentRProbe->lightingSH.V1.y = currentRProbeData["lightingSH"]["V1"]["y"];
                currentRProbe->lightingSH.V1.z = currentRProbeData["lightingSH"]["V1"]["z"];
                currentRProbe->lightingSH.V1.w = currentRProbeData["lightingSH"]["V1"]["w"];

                currentRProbe->lightingSH.V2.x = currentRProbeData["lightingSH"]["V2"]["x"];
                currentRProbe->lightingSH.V2.y = currentRProbeData["lightingSH"]["V2"]["y"];
                currentRProbe->lightingSH.V2.z = currentRProbeData["lightingSH"]["V2"]["z"];
                currentRProbe->lightingSH.V2.w = currentRProbeData["lightingSH"]["V2"]["w"];

                std::string reflectionName = currentRProbeData["reflectionImage"];
                if (reflectionName.length() > 0)
                {
                    auto* assetInfo = context.LoadDependency<AssetImage>(reflectionName);
                    if (!assetInfo)
                    {
                        printf("can't find reflection image\n");
                        return AssetCreationResult::Failure();
                    }
                    currentRProbe->reflectionImage = assetInfo->Asset();
                }
                else
                    currentRProbe->reflectionImage = nullptr;
                
                currentRProbe->probeVolumeCount = currentRProbeData["probeVolumeCount"];
                currentRProbe->mipLodBias = currentRProbeData["mipLodBias"];

                if (currentRProbe->probeVolumeCount > 0)
                    currentRProbe->probeVolumes = m_memory.Alloc<GfxReflectionProbeVolumeData>(currentRProbe->probeVolumeCount);
                else
                    currentRProbe->probeVolumes = nullptr;
                for (unsigned int k = 0; k < currentRProbe->probeVolumeCount; k++)
                {
                    auto currentRProbeVolume = &currentRProbe->probeVolumes[k];
                    auto currentRProbeVolumeData = currentRProbeData["probeVolumes"][k];

                    currentRProbeVolume->volumePlanes[0].x = currentRProbeVolumeData["volumePlanes0"]["x"];
                    currentRProbeVolume->volumePlanes[0].y = currentRProbeVolumeData["volumePlanes0"]["y"];
                    currentRProbeVolume->volumePlanes[0].z = currentRProbeVolumeData["volumePlanes0"]["z"];
                    currentRProbeVolume->volumePlanes[0].w = currentRProbeVolumeData["volumePlanes0"]["w"];

                    currentRProbeVolume->volumePlanes[1].x = currentRProbeVolumeData["volumePlanes1"]["x"];
                    currentRProbeVolume->volumePlanes[1].y = currentRProbeVolumeData["volumePlanes1"]["y"];
                    currentRProbeVolume->volumePlanes[1].z = currentRProbeVolumeData["volumePlanes1"]["z"];
                    currentRProbeVolume->volumePlanes[1].w = currentRProbeVolumeData["volumePlanes1"]["w"];

                    currentRProbeVolume->volumePlanes[2].x = currentRProbeVolumeData["volumePlanes2"]["x"];
                    currentRProbeVolume->volumePlanes[2].y = currentRProbeVolumeData["volumePlanes2"]["y"];
                    currentRProbeVolume->volumePlanes[2].z = currentRProbeVolumeData["volumePlanes2"]["z"];
                    currentRProbeVolume->volumePlanes[2].w = currentRProbeVolumeData["volumePlanes2"]["w"];

                    currentRProbeVolume->volumePlanes[3].x = currentRProbeVolumeData["volumePlanes3"]["x"];
                    currentRProbeVolume->volumePlanes[3].y = currentRProbeVolumeData["volumePlanes3"]["y"];
                    currentRProbeVolume->volumePlanes[3].z = currentRProbeVolumeData["volumePlanes3"]["z"];
                    currentRProbeVolume->volumePlanes[3].w = currentRProbeVolumeData["volumePlanes3"]["w"];

                    currentRProbeVolume->volumePlanes[4].x = currentRProbeVolumeData["volumePlanes4"]["x"];
                    currentRProbeVolume->volumePlanes[4].y = currentRProbeVolumeData["volumePlanes4"]["y"];
                    currentRProbeVolume->volumePlanes[4].z = currentRProbeVolumeData["volumePlanes4"]["z"];
                    currentRProbeVolume->volumePlanes[4].w = currentRProbeVolumeData["volumePlanes4"]["w"];

                    currentRProbeVolume->volumePlanes[5].x = currentRProbeVolumeData["volumePlanes5"]["x"];
                    currentRProbeVolume->volumePlanes[5].y = currentRProbeVolumeData["volumePlanes5"]["y"];
                    currentRProbeVolume->volumePlanes[5].z = currentRProbeVolumeData["volumePlanes5"]["z"];
                    currentRProbeVolume->volumePlanes[5].w = currentRProbeVolumeData["volumePlanes5"]["w"];
                }
            }
            
            gfxWorld->draw.reflectionProbeTextures = m_memory.Alloc<GfxTexture>(gfxWorld->draw.reflectionProbeCount + 1);
            for (unsigned int i = 0; i < gfxWorld->draw.reflectionProbeCount + 1; i++)
            {
                auto currentRProbe = &gfxWorld->draw.reflectionProbeTextures[i];
                auto currentRProbeData = j["draw"]["reflectionProbeTextures"][i];

                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                currentRProbe->basemap = 0;
            }
            
            gfxWorld->draw.lightmapCount = j["draw"]["lightmapCount"];
            if (gfxWorld->draw.lightmapCount > 0)
            {
                gfxWorld->draw.lightmaps = m_memory.Alloc<GfxLightmapArray>(gfxWorld->draw.lightmapCount);
                gfxWorld->draw.lightmapPrimaryTextures = m_memory.Alloc<GfxTexture>(gfxWorld->draw.lightmapCount);
                gfxWorld->draw.lightmapSecondaryTextures = m_memory.Alloc<GfxTexture>(gfxWorld->draw.lightmapCount);
            }
            else
            {
                gfxWorld->draw.lightmaps = nullptr;
                gfxWorld->draw.lightmapPrimaryTextures = nullptr;
                gfxWorld->draw.lightmapSecondaryTextures = nullptr;
            }
            for (int i = 0; i < gfxWorld->draw.lightmapCount; i++)
            {
                gfxWorld->draw.lightmapPrimaryTextures[i].basemap = 0;
                gfxWorld->draw.lightmapSecondaryTextures[i].basemap = 0;

                std::string primaryName = j["draw"]["lightmaps"][i]["primary"];
                if (primaryName.length() > 0)
                {
                    auto* primaryAssetInfo = context.LoadDependency<AssetImage>(primaryName);
                    if (!primaryAssetInfo)
                    {
                        printf("can't find lightmaps primary image\n");
                        return AssetCreationResult::Failure();
                    }
                    gfxWorld->draw.lightmaps[i].primary = primaryAssetInfo->Asset();
                }
                else
                    gfxWorld->draw.lightmaps[i].primary = nullptr;

                std::string secondaryName = j["draw"]["lightmaps"][i]["secondary"];
                if (secondaryName.length() > 0)
                {
                    auto* secondaryAssetInfo = context.LoadDependency<AssetImage>(secondaryName);
                    if (!secondaryAssetInfo)
                    {
                        printf("can't find lightmaps secondary image\n");
                        return AssetCreationResult::Failure();
                    }
                    gfxWorld->draw.lightmaps[i].secondary = secondaryAssetInfo->Asset();
                }
                else
                    gfxWorld->draw.lightmaps[i].secondary = nullptr;
            }

            gfxWorld->draw.vertexCount = j["draw"]["vertexCount"];

            gfxWorld->draw.vertexDataSize0 = j["draw"]["vertexDataSize0"];
            auto vertexData0_Filename = j["draw"]["vertexData0_Filename"];
            if (gfxWorld->draw.vertexDataSize0 > 0)
            {
                gfxWorld->draw.vd0.data = m_memory.Alloc<byte128>(gfxWorld->draw.vertexDataSize0);
                auto vertexData0File = m_search_path.Open(vertexData0_Filename);
                if (vertexData0File.IsOpen())
                {
                    auto& vertexData0Stream = *vertexData0File.m_stream;
                    vertexData0Stream.read(gfxWorld->draw.vd0.data, gfxWorld->draw.vertexDataSize0);
                }
            }
            else
                gfxWorld->draw.vd0.data = nullptr;

            gfxWorld->draw.vertexDataSize1 = j["draw"]["vertexDataSize1"];
            auto vertexData1_Filename = j["draw"]["vertexData1_Filename"];
            if (gfxWorld->draw.vertexDataSize1 > 0)
            {
                gfxWorld->draw.vd1.data = m_memory.Alloc<byte128>(gfxWorld->draw.vertexDataSize1);
                auto vertexData1File = m_search_path.Open(vertexData1_Filename);
                if (vertexData1File.IsOpen())
                {
                    auto& vertexData1Stream = *vertexData1File.m_stream;
                    vertexData1Stream.read(gfxWorld->draw.vd1.data, gfxWorld->draw.vertexDataSize1);
                }
            }
            else
                gfxWorld->draw.vd1.data = nullptr;

            gfxWorld->draw.indexCount = j["draw"]["indexCount"];
            auto indices_Filename = j["draw"]["indices_Filename"];
            if (gfxWorld->draw.indexCount > 0)
            {
                gfxWorld->draw.indices = m_memory.Alloc<uint16_t>(gfxWorld->draw.indexCount);
                auto indicesFile = m_search_path.Open(indices_Filename);
                if (indicesFile.IsOpen())
                {
                    auto& indicesStream = *indicesFile.m_stream;
                    indicesStream.read(reinterpret_cast<char*>(gfxWorld->draw.indices), gfxWorld->draw.indexCount * sizeof(uint16_t));
                }
            }
            else
                gfxWorld->draw.indices = nullptr;

            gfxWorld->lightGrid.sunPrimaryLightIndex = j["lightGrid"]["sunPrimaryLightIndex"];
            gfxWorld->lightGrid.mins[0] = j["lightGrid"]["mins"]["0"];
            gfxWorld->lightGrid.mins[1] = j["lightGrid"]["mins"]["1"];
            gfxWorld->lightGrid.mins[2] = j["lightGrid"]["mins"]["2"];
            gfxWorld->lightGrid.maxs[0] = j["lightGrid"]["maxs"]["0"];
            gfxWorld->lightGrid.maxs[1] = j["lightGrid"]["maxs"]["1"];
            gfxWorld->lightGrid.maxs[2] = j["lightGrid"]["maxs"]["2"];
            gfxWorld->lightGrid.offset = j["lightGrid"]["offset"];
            gfxWorld->lightGrid.rowAxis = j["lightGrid"]["rowAxis"];
            gfxWorld->lightGrid.colAxis = j["lightGrid"]["colAxis"];

            
            
            auto rowDataReadCount = gfxWorld->lightGrid.maxs[gfxWorld->lightGrid.rowAxis] - gfxWorld->lightGrid.mins[gfxWorld->lightGrid.rowAxis] + 1;
            if (rowDataReadCount > 0)
            {
                gfxWorld->lightGrid.rowDataStart = m_memory.Alloc<uint16_t>(rowDataReadCount);
                auto rowDataStartFile = m_search_path.Open(j["lightGrid"]["rowDataStart_Filename"]);
                if (rowDataStartFile.IsOpen())
                {
                    auto& rowDataStartStream = *rowDataStartFile.m_stream;
                    rowDataStartStream.read(
                        reinterpret_cast<char*>(gfxWorld->lightGrid.rowDataStart), 
                        rowDataReadCount * sizeof(uint16_t));
                }
            }
            else
                gfxWorld->lightGrid.rowDataStart = nullptr;

            gfxWorld->lightGrid.rawRowDataSize = j["lightGrid"]["rawRowDataSize"];
            if (gfxWorld->lightGrid.rawRowDataSize > 0)
            {
                gfxWorld->lightGrid.rawRowData = m_memory.Alloc<char>(gfxWorld->lightGrid.rawRowDataSize);
                auto rawRowDataFile = m_search_path.Open(j["lightGrid"]["rawRowData_Filename"]);
                if (rawRowDataFile.IsOpen())
                {
                    auto& rawRowDataStream = *rawRowDataFile.m_stream;
                    rawRowDataStream.read(
                        reinterpret_cast<char*>(gfxWorld->lightGrid.rawRowData), 
                        gfxWorld->lightGrid.rawRowDataSize);
                }
            }
            else
                gfxWorld->lightGrid.rawRowData = nullptr;

            gfxWorld->lightGrid.entryCount = j["lightGrid"]["entryCount"];
            if (gfxWorld->lightGrid.entryCount > 0)
            {
                gfxWorld->lightGrid.entries = m_memory.Alloc<GfxLightGridEntry>(gfxWorld->lightGrid.entryCount);
                auto entriesFile = m_search_path.Open(j["lightGrid"]["entries_Filename"]);
                if (entriesFile.IsOpen())
                {
                    auto& entriesStream = *entriesFile.m_stream;
                    entriesStream.read(
                        reinterpret_cast<char*>(gfxWorld->lightGrid.entries), 
                        gfxWorld->lightGrid.entryCount * sizeof(GfxLightGridEntry));
                }
            }
            else
                gfxWorld->lightGrid.entries = nullptr;
            

            gfxWorld->lightGrid.colorCount = j["lightGrid"]["colorCount"];
            if (gfxWorld->lightGrid.colorCount > 0)
            {
                gfxWorld->lightGrid.colors = m_memory.Alloc<GfxCompressedLightGridColors>(gfxWorld->lightGrid.colorCount);
                auto colorsFile = m_search_path.Open(j["lightGrid"]["colors_Filename"]);
                if (colorsFile.IsOpen())
                {
                    auto& colorsStream = *colorsFile.m_stream;
                    colorsStream.read(
                        reinterpret_cast<char*>(gfxWorld->lightGrid.colors),
                        gfxWorld->lightGrid.colorCount * sizeof(GfxCompressedLightGridColors)
                    );
                }
            }
            else
                gfxWorld->lightGrid.colors = nullptr;
            

            gfxWorld->lightGrid.coeffCount = j["lightGrid"]["coeffCount"];
            auto coeffs_Filename = j["lightGrid"]["coeffs_Filename"];
            if (gfxWorld->lightGrid.coeffCount > 0)
            {
                gfxWorld->lightGrid.coeffs = m_memory.Alloc<GfxCompressedLightGridCoeffs_align4>(gfxWorld->lightGrid.coeffCount);
                auto coeffsFile = m_search_path.Open(coeffs_Filename);
                if (coeffsFile.IsOpen())
                {
                    auto& coeffsStream = *coeffsFile.m_stream;
                    coeffsStream.read(reinterpret_cast<char*>(gfxWorld->lightGrid.coeffs),
                                      gfxWorld->lightGrid.coeffCount * sizeof(GfxCompressedLightGridCoeffs_align4));
                }
            }
            else
                gfxWorld->lightGrid.coeffs = nullptr;

            gfxWorld->lightGrid.skyGridVolumeCount = j["lightGrid"]["skyGridVolumeCount"];
            if (gfxWorld->lightGrid.skyGridVolumeCount > 0)
                gfxWorld->lightGrid.skyGridVolumes = m_memory.Alloc<GfxSkyGridVolume>(gfxWorld->lightGrid.skyGridVolumeCount);
            else
                gfxWorld->lightGrid.skyGridVolumes = nullptr;
            for (unsigned int i = 0; i < gfxWorld->lightGrid.skyGridVolumeCount; i++)
            {
                auto currentSkyGrid = &gfxWorld->lightGrid.skyGridVolumes[i];
                auto currentSkyGridData = j["lightGrid"]["skyGridVolumes"][i];

                currentSkyGrid->mins.x = currentSkyGridData["mins"]["x"];
                currentSkyGrid->mins.y = currentSkyGridData["mins"]["y"];
                currentSkyGrid->mins.z = currentSkyGridData["mins"]["z"];
                currentSkyGrid->maxs.x = currentSkyGridData["maxs"]["x"];
                currentSkyGrid->maxs.y = currentSkyGridData["maxs"]["y"];
                currentSkyGrid->maxs.z = currentSkyGridData["maxs"]["z"];
                currentSkyGrid->lightingOrigin.x = currentSkyGridData["lightingOrigin"]["x"];
                currentSkyGrid->lightingOrigin.y = currentSkyGridData["lightingOrigin"]["y"];
                currentSkyGrid->lightingOrigin.z = currentSkyGridData["lightingOrigin"]["z"];
                currentSkyGrid->colorsIndex = currentSkyGridData["colorsIndex"];
                currentSkyGrid->primaryLightIndex = static_cast<unsigned char>(currentSkyGridData["primaryLightIndex"]);
                currentSkyGrid->visibility = static_cast<unsigned char>(currentSkyGridData["visibility"]);
            }
            
            gfxWorld->modelCount = j["modelCount"];
            if (gfxWorld->modelCount > 0)
                gfxWorld->models = m_memory.Alloc<GfxBrushModel>(gfxWorld->modelCount);
            else
                gfxWorld->models = nullptr;
            for (int i = 0; i < gfxWorld->modelCount; i++)
            {
                auto currentModel = &gfxWorld->models[i];
                auto currentModelData = j["models"][i];

                currentModel->surfaceCount = currentModelData["surfaceCount"];
                currentModel->startSurfIndex = currentModelData["startSurfIndex"];

                currentModel->bounds[0].x = currentModelData["bounds"]["0"]["x"];
                currentModel->bounds[0].y = currentModelData["bounds"]["0"]["y"];
                currentModel->bounds[0].z = currentModelData["bounds"]["0"]["z"];
                currentModel->bounds[1].x = currentModelData["bounds"]["1"]["x"];
                currentModel->bounds[1].y = currentModelData["bounds"]["1"]["y"];
                currentModel->bounds[1].z = currentModelData["bounds"]["1"]["z"];

                currentModel->writable.mins.x = currentModelData["writable"]["mins"]["x"];
                currentModel->writable.mins.y = currentModelData["writable"]["mins"]["y"];
                currentModel->writable.mins.z = currentModelData["writable"]["mins"]["z"];
                currentModel->writable.maxs.x = currentModelData["writable"]["maxs"]["x"];
                currentModel->writable.maxs.y = currentModelData["writable"]["maxs"]["y"];
                currentModel->writable.maxs.z = currentModelData["writable"]["maxs"]["z"];
                currentModel->writable.padding1 = currentModelData["writable"]["padding1"];
                currentModel->writable.padding2 = currentModelData["writable"]["padding2"];
            }

            gfxWorld->mins.x = j["mins"]["x"];
            gfxWorld->mins.y = j["mins"]["y"];
            gfxWorld->mins.z = j["mins"]["z"];
            gfxWorld->maxs.x = j["maxs"]["x"];
            gfxWorld->maxs.y = j["maxs"]["y"];
            gfxWorld->maxs.z = j["maxs"]["z"];

            gfxWorld->checksum = j["checksum"];

            gfxWorld->materialMemoryCount = j["materialMemoryCount"];
            if (gfxWorld->materialMemoryCount > 0)
                gfxWorld->materialMemory = m_memory.Alloc<MaterialMemory>(gfxWorld->materialMemoryCount);
            else
                gfxWorld->materialMemory = nullptr;
            for (int i = 0; i < gfxWorld->materialMemoryCount; i++)
            {
                gfxWorld->materialMemory[i].memory = j["materialMemory"][i]["memory"];

                std::string matName = j["materialMemory"][i]["material"];
                if (matName.length() > 0)
                {
                    auto* matInfo = context.LoadDependency<AssetMaterial>(matName);
                    if (!matInfo)
                    {
                        printf("can't find materialMemory material\n");
                        return AssetCreationResult::Failure();
                    }
                    gfxWorld->materialMemory[i].material = matInfo->Asset();
                }
                else
                    gfxWorld->materialMemory[i].material = nullptr;
            }

            gfxWorld->sun.hasValidData = j["sun"]["hasValidData"];
            gfxWorld->sun.spriteSize = j["sun"]["spriteSize"];
            gfxWorld->sun.flareMinSize = j["sun"]["flareMinSize"];
            gfxWorld->sun.flareMinDot = j["sun"]["flareMinDot"];
            gfxWorld->sun.flareMaxSize = j["sun"]["flareMaxSize"];
            gfxWorld->sun.flareMaxDot = j["sun"]["flareMaxDot"];
            gfxWorld->sun.flareMaxAlpha = j["sun"]["flareMaxAlpha"];
            gfxWorld->sun.flareFadeInTime = j["sun"]["flareFadeInTime"];
            gfxWorld->sun.flareFadeOutTime = j["sun"]["flareFadeOutTime"];
            gfxWorld->sun.blindMinDot = j["sun"]["blindMinDot"];
            gfxWorld->sun.blindMaxDot = j["sun"]["blindMaxDot"];
            gfxWorld->sun.blindMaxDarken = j["sun"]["blindMaxDarken"];
            gfxWorld->sun.blindFadeInTime = j["sun"]["blindFadeInTime"];
            gfxWorld->sun.blindFadeOutTime = j["sun"]["blindFadeOutTime"];
            gfxWorld->sun.glareMinDot = j["sun"]["glareMinDot"];
            gfxWorld->sun.glareMaxDot = j["sun"]["glareMaxDot"];
            gfxWorld->sun.glareMaxLighten = j["sun"]["glareMaxLighten"];
            gfxWorld->sun.glareFadeInTime = j["sun"]["glareFadeInTime"];
            gfxWorld->sun.glareFadeOutTime = j["sun"]["glareFadeOutTime"];
            gfxWorld->sun.sunFxPosition.x = j["sun"]["sunFxPosition"]["x"];
            gfxWorld->sun.sunFxPosition.y = j["sun"]["sunFxPosition"]["y"];
            gfxWorld->sun.sunFxPosition.z = j["sun"]["sunFxPosition"]["z"];

            std::string spriteMaterial = j["sun"]["spriteMaterial"];
            if (spriteMaterial.length() > 0)
            {
                auto* spriteInfo = context.LoadDependency<AssetMaterial>(spriteMaterial);
                if (!spriteInfo)
                {
                    printf("can't find sun spriteMaterial\n");
                    return AssetCreationResult::Failure();
                }

                gfxWorld->sun.spriteMaterial = spriteInfo->Asset();
            }
            else
                gfxWorld->sun.spriteMaterial = nullptr;
            
            std::string flareMaterial = j["sun"]["spriteMaterial"];
            if (flareMaterial.length() > 0)
            {
                auto* flareInfo = context.LoadDependency<AssetMaterial>(flareMaterial);
                if (!flareInfo)
                {
                    printf("can't find sun flareMaterial\n");
                    return AssetCreationResult::Failure();
                }

                gfxWorld->sun.flareMaterial = flareInfo->Asset();
            }
            else
                gfxWorld->sun.flareMaterial = nullptr;

            gfxWorld->outdoorLookupMatrix[0].x = j["outdoorLookupMatrix"]["0"]["x"];
            gfxWorld->outdoorLookupMatrix[0].y = j["outdoorLookupMatrix"]["0"]["y"];
            gfxWorld->outdoorLookupMatrix[0].z = j["outdoorLookupMatrix"]["0"]["z"];
            gfxWorld->outdoorLookupMatrix[0].w = j["outdoorLookupMatrix"]["0"]["w"];

            gfxWorld->outdoorLookupMatrix[1].x = j["outdoorLookupMatrix"]["1"]["x"];
            gfxWorld->outdoorLookupMatrix[1].y = j["outdoorLookupMatrix"]["1"]["y"];
            gfxWorld->outdoorLookupMatrix[1].z = j["outdoorLookupMatrix"]["1"]["z"];
            gfxWorld->outdoorLookupMatrix[1].w = j["outdoorLookupMatrix"]["1"]["w"];

            gfxWorld->outdoorLookupMatrix[2].x = j["outdoorLookupMatrix"]["2"]["x"];
            gfxWorld->outdoorLookupMatrix[2].y = j["outdoorLookupMatrix"]["2"]["y"];
            gfxWorld->outdoorLookupMatrix[2].z = j["outdoorLookupMatrix"]["2"]["z"];
            gfxWorld->outdoorLookupMatrix[2].w = j["outdoorLookupMatrix"]["2"]["w"];

            gfxWorld->outdoorLookupMatrix[3].x = j["outdoorLookupMatrix"]["3"]["x"];
            gfxWorld->outdoorLookupMatrix[3].y = j["outdoorLookupMatrix"]["3"]["y"];
            gfxWorld->outdoorLookupMatrix[3].z = j["outdoorLookupMatrix"]["3"]["z"];
            gfxWorld->outdoorLookupMatrix[3].w = j["outdoorLookupMatrix"]["3"]["w"];

            std::string outdoorName = j["outdoorImage"];
            if (outdoorName.length() > 0)
            {
                auto* assetInfo = context.LoadDependency<AssetImage>(outdoorName);
                if (!assetInfo)
                {
                    printf("can't find outdoorImage\n");
                    return AssetCreationResult::Failure();
                }
                gfxWorld->outdoorImage = assetInfo->Asset();
            }
            else
                gfxWorld->outdoorImage = nullptr;
            

            if (gfxWorld->dpvsPlanes.cellCount * ((gfxWorld->dpvsPlanes.cellCount + 31) / 32) > 0)
                gfxWorld->cellCasterBits = m_memory.Alloc<unsigned int>(gfxWorld->dpvsPlanes.cellCount * ((gfxWorld->dpvsPlanes.cellCount + 31) / 32));
            else
                gfxWorld->cellCasterBits = nullptr;
            for (int i = 0; i < gfxWorld->dpvsPlanes.cellCount * ((gfxWorld->dpvsPlanes.cellCount + 31) / 32); i++)
            {
                gfxWorld->cellCasterBits[i] = j["cellCasterBits"][i];
            }

            if ((gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1) * 8192 > 0)
            {
                gfxWorld->primaryLightEntityShadowVis = m_memory.Alloc<unsigned int>((gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1) * 8192);
                auto primaryLightEntityShadowVisFile = m_search_path.Open(j["primaryLightEntityShadowVis_Filename"]);
                if (primaryLightEntityShadowVisFile.IsOpen())
                {
                    auto& primaryLightEntityShadowVisStream = *primaryLightEntityShadowVisFile.m_stream;
                    primaryLightEntityShadowVisStream.read(
                        reinterpret_cast<char*>(gfxWorld->primaryLightEntityShadowVis),
                        ((gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1) * 8192) * sizeof(unsigned int)
                    );
                }
            }
            else
                gfxWorld->primaryLightEntityShadowVis = nullptr;

            //gfxWorld->numSiegeSkinInsts = j["numSiegeSkinInsts"];
            gfxWorld->numSiegeSkinInsts = 0;
            gfxWorld->siegeSkinInsts = nullptr;
             /////////////////////////////////////////////////////////

            if (gfxWorld->primaryLightCount > 0)
                gfxWorld->shadowGeom = m_memory.Alloc<GfxShadowGeometry>(gfxWorld->primaryLightCount);
            else
                gfxWorld->shadowGeom = nullptr;
            for (unsigned int i = 0; i < gfxWorld->primaryLightCount; i++)
            {
                auto currentShadow = &gfxWorld->shadowGeom[i];
                auto currentShadowData = j["shadowGeom"][i];

                currentShadow->surfaceCount = currentShadowData["surfaceCount"];
                currentShadow->smodelCount = currentShadowData["smodelCount"];

                if (currentShadow->surfaceCount > 0)
                    currentShadow->sortedSurfIndex = m_memory.Alloc<uint16_t>(currentShadow->surfaceCount);
                else
                    currentShadow->sortedSurfIndex = nullptr;
                for (unsigned int k = 0; k < currentShadow->surfaceCount; k++)
                {
                    currentShadow->sortedSurfIndex[k] = currentShadowData["sortedSurfIndex"][k];
                }

                if (currentShadow->smodelCount > 0)
                    currentShadow->smodelIndex = m_memory.Alloc<uint16_t>(currentShadow->smodelCount);
                else
                    currentShadow->smodelIndex = nullptr;
                for (unsigned int k = 0; k < currentShadow->smodelCount; k++)
                {
                    currentShadow->smodelIndex[k] = currentShadowData["smodelIndex"][k];
                }
            }

            if (gfxWorld->primaryLightCount > 0)
                gfxWorld->lightRegion = m_memory.Alloc<GfxLightRegion>(gfxWorld->primaryLightCount);
            else
                gfxWorld->lightRegion = nullptr;
            for (unsigned int i = 0; i < gfxWorld->primaryLightCount; i++)
            {
                gfxWorld->lightRegion[i].hullCount = j["lightRegion"][i]["hullCount"];
                if (gfxWorld->lightRegion[i].hullCount > 0)
                    gfxWorld->lightRegion[i].hulls = m_memory.Alloc<GfxLightRegionHull>(gfxWorld->lightRegion[i].hullCount);
                else
                    gfxWorld->lightRegion[i].hulls = nullptr;
                for (unsigned int k = 0; k < gfxWorld->lightRegion[i].hullCount; k++)
                {
                    auto currentHull = &gfxWorld->lightRegion[i].hulls[k];
                    auto currentHullData = j["lightRegion"][i]["hulls"][k];

                    currentHull->kdopMidPoint[0] = currentHullData["kdopMidPoint"]["0"];
                    currentHull->kdopMidPoint[1] = currentHullData["kdopMidPoint"]["1"];
                    currentHull->kdopMidPoint[2] = currentHullData["kdopMidPoint"]["2"];
                    currentHull->kdopMidPoint[3] = currentHullData["kdopMidPoint"]["3"];
                    currentHull->kdopMidPoint[4] = currentHullData["kdopMidPoint"]["4"];
                    currentHull->kdopMidPoint[5] = currentHullData["kdopMidPoint"]["5"];
                    currentHull->kdopMidPoint[6] = currentHullData["kdopMidPoint"]["6"];
                    currentHull->kdopMidPoint[7] = currentHullData["kdopMidPoint"]["7"];
                    currentHull->kdopMidPoint[8] = currentHullData["kdopMidPoint"]["8"];

                    currentHull->kdopHalfSize[0] = currentHullData["kdopHalfSize"]["0"];
                    currentHull->kdopHalfSize[1] = currentHullData["kdopHalfSize"]["1"];
                    currentHull->kdopHalfSize[2] = currentHullData["kdopHalfSize"]["2"];
                    currentHull->kdopHalfSize[3] = currentHullData["kdopHalfSize"]["3"];
                    currentHull->kdopHalfSize[4] = currentHullData["kdopHalfSize"]["4"];
                    currentHull->kdopHalfSize[5] = currentHullData["kdopHalfSize"]["5"];
                    currentHull->kdopHalfSize[6] = currentHullData["kdopHalfSize"]["6"];
                    currentHull->kdopHalfSize[7] = currentHullData["kdopHalfSize"]["7"];
                    currentHull->kdopHalfSize[8] = currentHullData["kdopHalfSize"]["8"];

                    currentHull->axisCount = currentHullData["axisCount"];
                    if (currentHull->axisCount > 0)
                        currentHull->axis = m_memory.Alloc<GfxLightRegionAxis>(currentHull->axisCount);
                    else
                        currentHull->axis = nullptr;
                    for (unsigned int l = 0; l < currentHull->axisCount; l++)
                    {
                        currentHull->axis[l].dir.x = currentHullData["axis"][l]["dir"]["x"];
                        currentHull->axis[l].dir.y = currentHullData["axis"][l]["dir"]["y"];
                        currentHull->axis[l].dir.z = currentHullData["axis"][l]["dir"]["z"];
                        currentHull->axis[l].midPoint = currentHullData["axis"][l]["midPoint"];
                        currentHull->axis[l].halfSize = currentHullData["axis"][l]["halfSize"];
                    }
                }
            }

            gfxWorld->dpvs.smodelCount = j["dpvs"]["smodelCount"];
            gfxWorld->dpvs.staticSurfaceCount = j["dpvs"]["staticSurfaceCount"];
            gfxWorld->dpvs.litSurfsBegin = j["dpvs"]["litSurfsBegin"];
            gfxWorld->dpvs.litSurfsEnd = j["dpvs"]["litSurfsEnd"];
            gfxWorld->dpvs.litTransSurfsBegin = j["dpvs"]["litTransSurfsBegin"];
            gfxWorld->dpvs.litTransSurfsEnd = j["dpvs"]["litTransSurfsEnd"];
            gfxWorld->dpvs.emissiveOpaqueSurfsBegin = j["dpvs"]["emissiveOpaqueSurfsBegin"];
            gfxWorld->dpvs.emissiveOpaqueSurfsEnd = j["dpvs"]["emissiveOpaqueSurfsEnd"];
            gfxWorld->dpvs.emissiveTransSurfsBegin = j["dpvs"]["emissiveTransSurfsBegin"];
            gfxWorld->dpvs.emissiveTransSurfsEnd = j["dpvs"]["emissiveTransSurfsEnd"];
            gfxWorld->dpvs.smodelVisDataCount = j["dpvs"]["smodelVisDataCount"];
            gfxWorld->dpvs.surfaceVisDataCount = j["dpvs"]["surfaceVisDataCount"];
            gfxWorld->dpvs.usageCount = j["dpvs"]["usageCount"];

            if (gfxWorld->dpvs.smodelVisDataCount > 0)
            {
                auto smodelVisData0File = m_search_path.Open(j["dpvs"]["smodelVisData0_Filename"]);
                if (smodelVisData0File.IsOpen())
                {
                    gfxWorld->dpvs.smodelVisData[0] = m_memory.Alloc<char>(gfxWorld->dpvs.smodelVisDataCount);
                    auto& smodelVisData0Stream = *smodelVisData0File.m_stream;
                    smodelVisData0Stream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvs.smodelVisData[0]), 
                        gfxWorld->dpvs.smodelVisDataCount
                    );
                }

                auto smodelVisData1File = m_search_path.Open(j["dpvs"]["smodelVisData1_Filename"]);
                if (smodelVisData1File.IsOpen())
                {
                    gfxWorld->dpvs.smodelVisData[1] = m_memory.Alloc<char>(gfxWorld->dpvs.smodelVisDataCount);
                    auto& smodelVisData1Stream = *smodelVisData1File.m_stream;
                    smodelVisData1Stream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvs.smodelVisData[1]), 
                        gfxWorld->dpvs.smodelVisDataCount
                    );
                }

                auto smodelVisData2File = m_search_path.Open(j["dpvs"]["smodelVisData2_Filename"]);
                if (smodelVisData2File.IsOpen())
                {
                    gfxWorld->dpvs.smodelVisData[2] = m_memory.Alloc<char>(gfxWorld->dpvs.smodelVisDataCount);
                    auto& smodelVisData2Stream = *smodelVisData2File.m_stream;
                    smodelVisData2Stream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvs.smodelVisData[2]), 
                        gfxWorld->dpvs.smodelVisDataCount
                    );
                }

                auto cameraSavedFile = m_search_path.Open(j["dpvs"]["smodelVisDataCameraSaved_Filename"]);
                if (cameraSavedFile.IsOpen())
                {
                    gfxWorld->dpvs.smodelVisDataCameraSaved = m_memory.Alloc<char>(gfxWorld->dpvs.smodelVisDataCount);
                    auto& cameraSavedStream = *cameraSavedFile.m_stream;
                    cameraSavedStream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvs.smodelVisDataCameraSaved), 
                        gfxWorld->dpvs.smodelVisDataCount
                    );
                }

                auto smodelCastsShadowFile = m_search_path.Open(j["dpvs"]["smodelCastsShadow_Filename"]);
                if (smodelCastsShadowFile.IsOpen())
                {
                    gfxWorld->dpvs.smodelCastsShadow = m_memory.Alloc<char>(gfxWorld->dpvs.smodelVisDataCount);
                    auto& smodelCastsShadowStream = *smodelCastsShadowFile.m_stream;
                    smodelCastsShadowStream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvs.smodelCastsShadow), 
                        gfxWorld->dpvs.smodelVisDataCount
                    );
                }
            }
            else
            {
                gfxWorld->dpvs.smodelVisData[0] = nullptr;
                gfxWorld->dpvs.smodelVisData[1] = nullptr;
                gfxWorld->dpvs.smodelVisData[2] = nullptr;
                gfxWorld->dpvs.smodelVisDataCameraSaved = nullptr;
                gfxWorld->dpvs.smodelCastsShadow = nullptr;
            }
            
            if (gfxWorld->dpvs.surfaceVisDataCount > 0)
            {
                auto surfaceVisData0File = m_search_path.Open(j["dpvs"]["surfaceVisData0_Filename"]);
                if (surfaceVisData0File.IsOpen())
                {
                    gfxWorld->dpvs.surfaceVisData[0] = m_memory.Alloc<char>(gfxWorld->dpvs.surfaceVisDataCount);
                    auto& surfaceVisData0Stream = *surfaceVisData0File.m_stream;
                    surfaceVisData0Stream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvs.surfaceVisData[0]), 
                        gfxWorld->dpvs.surfaceVisDataCount
                    );
                }

                auto surfaceVisData1File = m_search_path.Open(j["dpvs"]["surfaceVisData1_Filename"]);
                if (surfaceVisData1File.IsOpen())
                {
                    gfxWorld->dpvs.surfaceVisData[1] = m_memory.Alloc<char>(gfxWorld->dpvs.surfaceVisDataCount);
                    auto& surfaceVisData1Stream = *surfaceVisData1File.m_stream;
                    surfaceVisData1Stream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvs.surfaceVisData[1]), 
                        gfxWorld->dpvs.surfaceVisDataCount
                    );
                }

                auto surfaceVisData2File = m_search_path.Open(j["dpvs"]["surfaceVisData2_Filename"]);
                if (surfaceVisData2File.IsOpen())
                {
                    gfxWorld->dpvs.surfaceVisData[2] = m_memory.Alloc<char>(gfxWorld->dpvs.surfaceVisDataCount);
                    auto& surfaceVisData2Stream = *surfaceVisData2File.m_stream;
                    surfaceVisData2Stream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvs.surfaceVisData[2]), 
                        gfxWorld->dpvs.surfaceVisDataCount
                    );
                }

                auto surfaceVisDataCameraSavedFile = m_search_path.Open(j["dpvs"]["surfaceVisDataCameraSaved_Filename"]);
                if (surfaceVisDataCameraSavedFile.IsOpen())
                {
                    gfxWorld->dpvs.surfaceVisDataCameraSaved = m_memory.Alloc<char>(gfxWorld->dpvs.surfaceVisDataCount);
                    auto& surfaceVisDataCameraSavedStream = *surfaceVisDataCameraSavedFile.m_stream;
                    surfaceVisDataCameraSavedStream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvs.surfaceVisDataCameraSaved), 
                        gfxWorld->dpvs.surfaceVisDataCount
                    );
                }

                auto surfaceCastsSunShadowFile = m_search_path.Open(j["dpvs"]["surfaceCastsSunShadow_Filename"]);
                if (surfaceCastsSunShadowFile.IsOpen())
                {
                    gfxWorld->dpvs.surfaceCastsSunShadow = m_memory.Alloc<char>(gfxWorld->dpvs.surfaceVisDataCount);
                    auto& surfaceCastsSunShadowStream = *surfaceCastsSunShadowFile.m_stream;
                    surfaceCastsSunShadowStream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvs.surfaceCastsSunShadow), 
                        gfxWorld->dpvs.surfaceVisDataCount
                    );
                }

                auto surfaceCastsShadowFile = m_search_path.Open(j["dpvs"]["surfaceCastsShadow_Filename"]);
                if (surfaceCastsShadowFile.IsOpen())
                {
                    gfxWorld->dpvs.surfaceCastsShadow = m_memory.Alloc<char>(gfxWorld->dpvs.surfaceVisDataCount);
                    auto& surfaceCastsShadowStream = *surfaceCastsShadowFile.m_stream;
                    surfaceCastsShadowStream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvs.surfaceCastsShadow), 
                        gfxWorld->dpvs.surfaceVisDataCount
                    );
                }
            }
            else
            {
                gfxWorld->dpvs.surfaceVisData[0] = nullptr;
                gfxWorld->dpvs.surfaceVisData[1] = nullptr;
                gfxWorld->dpvs.surfaceVisData[2] = nullptr;
                gfxWorld->dpvs.surfaceVisDataCameraSaved = nullptr;
                gfxWorld->dpvs.surfaceCastsSunShadow = nullptr;
                gfxWorld->dpvs.surfaceCastsShadow = nullptr;
            }
            
            if (gfxWorld->dpvs.staticSurfaceCount > 0)
            {
                auto sortedSurfIndexFile = m_search_path.Open(j["dpvs"]["sortedSurfIndex_Filename"]);
                if (sortedSurfIndexFile.IsOpen())
                {
                    gfxWorld->dpvs.sortedSurfIndex = m_memory.Alloc<uint16_t>(gfxWorld->dpvs.staticSurfaceCount);
                    auto& sortedSurfIndexStream = *sortedSurfIndexFile.m_stream;
                    sortedSurfIndexStream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvs.sortedSurfIndex), 
                        gfxWorld->dpvs.staticSurfaceCount * sizeof(uint16_t)
                    );
                }
            }
            else
                gfxWorld->dpvs.sortedSurfIndex = nullptr;

            if (gfxWorld->dpvs.smodelCount > 0)
                gfxWorld->dpvs.smodelInsts = m_memory.Alloc<GfxStaticModelInst>(gfxWorld->dpvs.smodelCount);
            else
                gfxWorld->dpvs.smodelInsts = nullptr;
            for (unsigned int i = 0; i < gfxWorld->dpvs.smodelCount; i++)
            {
                auto currentSmodelInst = &gfxWorld->dpvs.smodelInsts[i];
                auto currentSmodelInstData = j["dpvs"]["smodelInsts"][i];

                currentSmodelInst->mins.x = currentSmodelInstData["mins"]["x"];
                currentSmodelInst->mins.y = currentSmodelInstData["mins"]["y"];
                currentSmodelInst->mins.z = currentSmodelInstData["mins"]["z"];
                currentSmodelInst->maxs.x = currentSmodelInstData["maxs"]["x"];
                currentSmodelInst->maxs.y = currentSmodelInstData["maxs"]["y"];
                currentSmodelInst->maxs.z = currentSmodelInstData["maxs"]["z"];
                currentSmodelInst->lightingOrigin.x = currentSmodelInstData["lightingOrigin"]["x"];
                currentSmodelInst->lightingOrigin.y = currentSmodelInstData["lightingOrigin"]["y"];
                currentSmodelInst->lightingOrigin.z = currentSmodelInstData["lightingOrigin"]["z"];
            }
            
            if (gfxWorld->surfaceCount > 0)
                gfxWorld->dpvs.surfaces = m_memory.Alloc<GfxSurface>(gfxWorld->surfaceCount);
            else
                gfxWorld->dpvs.surfaces = nullptr;
            for (int i = 0; i < gfxWorld->surfaceCount; i++)
            {
                auto currentSurface = &gfxWorld->dpvs.surfaces[i];
                auto currentSurfaceData = j["dpvs"]["surfaces"][i];

                std::string materialName = currentSurfaceData["material"];
                if (materialName.length() > 0)
                {
                    auto* materialInfo = context.LoadDependency<AssetMaterial>(materialName);
                    if (!materialInfo)
                    {
                        printf("can't find material surface\n");
                        return AssetCreationResult::Failure();
                    }

                    currentSurface->material = materialInfo->Asset();
                }
                else
                    currentSurface->material = nullptr;
                currentSurface->lightmapIndex = static_cast<unsigned char>(currentSurfaceData["lightmapIndex"]);
                currentSurface->reflectionProbeIndex = static_cast<unsigned char>(currentSurfaceData["reflectionProbeIndex"]);
                currentSurface->primaryLightIndex = static_cast<unsigned char>(currentSurfaceData["primaryLightIndex"]);
                currentSurface->flags = static_cast<unsigned char>(currentSurfaceData["flags"]);
                currentSurface->bounds[0].x = currentSurfaceData["bounds0"]["x"];
                currentSurface->bounds[0].y = currentSurfaceData["bounds0"]["y"];
                currentSurface->bounds[0].z = currentSurfaceData["bounds0"]["z"];
                currentSurface->bounds[1].x = currentSurfaceData["bounds1"]["x"];
                currentSurface->bounds[1].y = currentSurfaceData["bounds1"]["y"];
                currentSurface->bounds[1].z = currentSurfaceData["bounds1"]["z"];

                currentSurface->tris.mins.x = currentSurfaceData["tris"]["mins"]["x"];
                currentSurface->tris.mins.y = currentSurfaceData["tris"]["mins"]["y"];
                currentSurface->tris.mins.z = currentSurfaceData["tris"]["mins"]["z"];
                currentSurface->tris.maxs.x = currentSurfaceData["tris"]["maxs"]["x"];
                currentSurface->tris.maxs.y = currentSurfaceData["tris"]["maxs"]["y"];
                currentSurface->tris.maxs.z = currentSurfaceData["tris"]["maxs"]["z"];
                currentSurface->tris.vertexDataOffset0 = currentSurfaceData["tris"]["vertexDataOffset0"];
                currentSurface->tris.vertexDataOffset1 = currentSurfaceData["tris"]["vertexDataOffset1"];
                currentSurface->tris.firstVertex = currentSurfaceData["tris"]["firstVertex"];
                currentSurface->tris.himipRadiusInvSq = currentSurfaceData["tris"]["himipRadiusInvSq"];
                currentSurface->tris.vertexCount = currentSurfaceData["tris"]["vertexCount"];
                currentSurface->tris.triCount = currentSurfaceData["tris"]["triCount"];
                currentSurface->tris.baseIndex = currentSurfaceData["tris"]["baseIndex"];
            }

            if (gfxWorld->dpvs.staticSurfaceCount > 0)
                gfxWorld->dpvs.surfaceMaterials = m_memory.Alloc<GfxDrawSurf_align4>(gfxWorld->dpvs.staticSurfaceCount);
            else
                gfxWorld->dpvs.surfaceMaterials = nullptr;
            for (unsigned int i = 0; i < gfxWorld->dpvs.staticSurfaceCount; i++)
            {
                gfxWorld->dpvs.surfaceMaterials[i].packed = j["dpvs"]["surfaceMaterials"][i];
            }

            if (gfxWorld->dpvs.smodelCount > 0)
                gfxWorld->dpvs.smodelDrawInsts = m_memory.Alloc<GfxStaticModelDrawInst>(gfxWorld->dpvs.smodelCount);
            else
                gfxWorld->dpvs.smodelDrawInsts = nullptr;
            for (unsigned int i = 0; i < gfxWorld->dpvs.smodelCount; i++)
            {
                auto currentSModel = &gfxWorld->dpvs.smodelDrawInsts[i];
                auto currentSModelData = j["dpvs"]["smodelDrawInsts"][i];

                currentSModel->cullDist = currentSModelData["cullDist"];

                currentSModel->placement.scale = currentSModelData["placement"]["scale"];
                currentSModel->placement.origin.x = currentSModelData["placement"]["origin"]["x"];
                currentSModel->placement.origin.y = currentSModelData["placement"]["origin"]["y"];
                currentSModel->placement.origin.z = currentSModelData["placement"]["origin"]["z"];
                currentSModel->placement.axis[0].x = currentSModelData["placement"]["axis0"]["x"];
                currentSModel->placement.axis[0].y = currentSModelData["placement"]["axis0"]["y"];
                currentSModel->placement.axis[0].z = currentSModelData["placement"]["axis0"]["z"];
                currentSModel->placement.axis[1].x = currentSModelData["placement"]["axis1"]["x"];
                currentSModel->placement.axis[1].y = currentSModelData["placement"]["axis1"]["y"];
                currentSModel->placement.axis[1].z = currentSModelData["placement"]["axis1"]["z"];
                currentSModel->placement.axis[2].x = currentSModelData["placement"]["axis2"]["x"];
                currentSModel->placement.axis[2].y = currentSModelData["placement"]["axis2"]["y"];
                currentSModel->placement.axis[2].z = currentSModelData["placement"]["axis2"]["z"];

                std::string modelName = currentSModelData["model"];
                if (modelName.length() > 0)
                {
                    auto* modelInfo = context.LoadDependency<AssetXModel>(modelName);
                    if (!modelInfo)
                    {
                        printf("can't find model smodel\n");
                        return AssetCreationResult::Failure();
                    }

                    currentSModel->model = modelInfo->Asset();
                }
                else
                    currentSModel->model = nullptr;

                currentSModel->flags = currentSModelData["flags"];
                currentSModel->invScaleSq = currentSModelData["invScaleSq"];
                currentSModel->lightingHandle = currentSModelData["lightingHandle"];
                currentSModel->colorsIndex = currentSModelData["colorsIndex"];

                currentSModel->lightingSH.V0[0] = currentSModelData["lightingSH"]["V00"];
                currentSModel->lightingSH.V0[1] = currentSModelData["lightingSH"]["V01"];
                currentSModel->lightingSH.V0[2] = currentSModelData["lightingSH"]["V02"];
                currentSModel->lightingSH.V0[3] = currentSModelData["lightingSH"]["V03"];
                currentSModel->lightingSH.V1[0] = currentSModelData["lightingSH"]["V10"];
                currentSModel->lightingSH.V1[1] = currentSModelData["lightingSH"]["V11"];
                currentSModel->lightingSH.V1[2] = currentSModelData["lightingSH"]["V12"];
                currentSModel->lightingSH.V1[3] = currentSModelData["lightingSH"]["V13"];
                currentSModel->lightingSH.V2[0] = currentSModelData["lightingSH"]["V20"];
                currentSModel->lightingSH.V2[1] = currentSModelData["lightingSH"]["V21"];
                currentSModel->lightingSH.V2[2] = currentSModelData["lightingSH"]["V22"];
                currentSModel->lightingSH.V2[3] = currentSModelData["lightingSH"]["V23"];

                currentSModel->primaryLightIndex = static_cast<unsigned char>(currentSModelData["primaryLightIndex"]);
                currentSModel->visibility = static_cast<unsigned char>(currentSModelData["visibility"]);
                currentSModel->reflectionProbeIndex = static_cast<unsigned char>(currentSModelData["reflectionProbeIndex"]);
                currentSModel->smid = currentSModelData["smid"];

                currentSModel->lmapVertexInfo[0].numLmapVertexColors = currentSModelData["lmapVertexInfo0"]["numLmapVertexColors"];
                currentSModel->lmapVertexInfo[0].lmapVertexColorsVB = nullptr;
                if (currentSModel->lmapVertexInfo[0].numLmapVertexColors > 0)
                    currentSModel->lmapVertexInfo[0].lmapVertexColors = m_memory.Alloc<unsigned int>(currentSModel->lmapVertexInfo[0].numLmapVertexColors);
                else
                    currentSModel->lmapVertexInfo[0].lmapVertexColors = nullptr;
                for (int k = 0; k < currentSModel->lmapVertexInfo[0].numLmapVertexColors; k++)
                {
                    currentSModel->lmapVertexInfo[0].lmapVertexColors[k] = currentSModelData["lmapVertexInfo0"]["lmapVertexColors"][k];
                }

                currentSModel->lmapVertexInfo[1].numLmapVertexColors = currentSModelData["lmapVertexInfo1"]["numLmapVertexColors"];
                currentSModel->lmapVertexInfo[1].lmapVertexColorsVB = nullptr;
                if (currentSModel->lmapVertexInfo[1].numLmapVertexColors > 0)
                    currentSModel->lmapVertexInfo[1].lmapVertexColors = m_memory.Alloc<unsigned int>(currentSModel->lmapVertexInfo[1].numLmapVertexColors);
                else
                    currentSModel->lmapVertexInfo[1].lmapVertexColors = nullptr;
                for (int k = 0; k < currentSModel->lmapVertexInfo[1].numLmapVertexColors; k++)
                {
                    currentSModel->lmapVertexInfo[1].lmapVertexColors[k] = currentSModelData["lmapVertexInfo1"]["lmapVertexColors"][k];
                }

                currentSModel->lmapVertexInfo[2].numLmapVertexColors = currentSModelData["lmapVertexInfo2"]["numLmapVertexColors"];
                currentSModel->lmapVertexInfo[2].lmapVertexColorsVB = nullptr;
                if (currentSModel->lmapVertexInfo[2].numLmapVertexColors > 0)
                    currentSModel->lmapVertexInfo[2].lmapVertexColors = m_memory.Alloc<unsigned int>(currentSModel->lmapVertexInfo[2].numLmapVertexColors);
                else
                    currentSModel->lmapVertexInfo[2].lmapVertexColors = nullptr;
                for (int k = 0; k < currentSModel->lmapVertexInfo[2].numLmapVertexColors; k++)
                {
                    currentSModel->lmapVertexInfo[2].lmapVertexColors[k] = currentSModelData["lmapVertexInfo2"]["lmapVertexColors"][k];
                }

                currentSModel->lmapVertexInfo[3].numLmapVertexColors = currentSModelData["lmapVertexInfo3"]["numLmapVertexColors"];
                currentSModel->lmapVertexInfo[3].lmapVertexColorsVB = nullptr;
                if (currentSModel->lmapVertexInfo[3].numLmapVertexColors > 0)
                    currentSModel->lmapVertexInfo[3].lmapVertexColors = m_memory.Alloc<unsigned int>(currentSModel->lmapVertexInfo[3].numLmapVertexColors);
                else
                    currentSModel->lmapVertexInfo[3].lmapVertexColors = nullptr;
                for (int k = 0; k < currentSModel->lmapVertexInfo[3].numLmapVertexColors; k++)
                {
                    currentSModel->lmapVertexInfo[3].lmapVertexColors[k] = currentSModelData["lmapVertexInfo3"]["lmapVertexColors"][k];
                }
            }

            gfxWorld->dpvsDyn.dynEntClientWordCount[0] = j["dpvsDyn"]["dynEntClientWordCount0"];
            gfxWorld->dpvsDyn.dynEntClientWordCount[1] = j["dpvsDyn"]["dynEntClientWordCount1"];
            gfxWorld->dpvsDyn.dynEntClientCount[0] = j["dpvsDyn"]["dynEntClientCount0"];
            gfxWorld->dpvsDyn.dynEntClientCount[1] = j["dpvsDyn"]["dynEntClientCount1"];
            gfxWorld->dpvsDyn.usageCount = j["dpvsDyn"]["usageCount"];

            if (gfxWorld->dpvsDyn.dynEntClientWordCount[0] * gfxWorld->dpvsPlanes.cellCount > 0)
            {
                gfxWorld->dpvsDyn.dynEntCellBits[0] = m_memory.Alloc<unsigned int>(gfxWorld->dpvsDyn.dynEntClientWordCount[0] * gfxWorld->dpvsPlanes.cellCount);
                auto dynEntCellBits0File = m_search_path.Open(j["dpvsDyn"]["dynEntCellBits0_Filename"]);
                if (dynEntCellBits0File.IsOpen())
                {
                    auto& dynEntCellBits0Stream = *dynEntCellBits0File.m_stream;
                    dynEntCellBits0Stream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvsDyn.dynEntCellBits[0]), 
                         gfxWorld->dpvsDyn.dynEntClientWordCount[0] * gfxWorld->dpvsPlanes.cellCount * sizeof(unsigned int)
                    );
                }
            }
            else
                gfxWorld->dpvsDyn.dynEntCellBits[0] = nullptr;

            if (gfxWorld->dpvsDyn.dynEntClientWordCount[1] * gfxWorld->dpvsPlanes.cellCount > 0)
            {
                gfxWorld->dpvsDyn.dynEntCellBits[1] = m_memory.Alloc<unsigned int>(gfxWorld->dpvsDyn.dynEntClientWordCount[1] * gfxWorld->dpvsPlanes.cellCount);
                auto dynEntCellBits1File = m_search_path.Open(j["dpvsDyn"]["dynEntCellBits1_Filename"]);
                if (dynEntCellBits1File.IsOpen())
                {
                    auto& dynEntCellBits1Stream = *dynEntCellBits1File.m_stream;
                    dynEntCellBits1Stream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvsDyn.dynEntCellBits[1]),
                        gfxWorld->dpvsDyn.dynEntClientWordCount[1] * gfxWorld->dpvsPlanes.cellCount * sizeof(unsigned int)
                    );
                }
            }
            else
                gfxWorld->dpvsDyn.dynEntCellBits[1] = nullptr;

            if (gfxWorld->dpvsDyn.dynEntClientWordCount[0] * 32 > 0)
            {
                gfxWorld->dpvsDyn.dynEntVisData[0][0] = m_memory.Alloc<char>( gfxWorld->dpvsDyn.dynEntClientWordCount[0] * 32);
                gfxWorld->dpvsDyn.dynEntVisData[0][1] = m_memory.Alloc<char>( gfxWorld->dpvsDyn.dynEntClientWordCount[0] * 32);
                gfxWorld->dpvsDyn.dynEntVisData[0][2] = m_memory.Alloc<char>( gfxWorld->dpvsDyn.dynEntClientWordCount[0] * 32);

                auto visData00File = m_search_path.Open(j["dpvsDyn"]["dynEntVisData00_Filename"]);
                if (visData00File.IsOpen())
                {
                    auto& visData00Stream = *visData00File.m_stream;
                    visData00Stream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvsDyn.dynEntVisData[0][0]), 
                        gfxWorld->dpvsDyn.dynEntClientWordCount[0] * 32);
                }

                auto visData01File = m_search_path.Open(j["dpvsDyn"]["dynEntVisData01_Filename"]);
                if (visData01File.IsOpen())
                {
                    auto& visData01Stream = *visData01File.m_stream;
                    visData01Stream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvsDyn.dynEntVisData[0][1]), 
                        gfxWorld->dpvsDyn.dynEntClientWordCount[0] * 32);
                }

                auto visData02File = m_search_path.Open(j["dpvsDyn"]["dynEntVisData02_Filename"]);
                if (visData02File.IsOpen())
                {
                    auto& visData02Stream = *visData02File.m_stream;
                    visData02Stream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvsDyn.dynEntVisData[0][2]), 
                        gfxWorld->dpvsDyn.dynEntClientWordCount[0] * 32);
                }
            }
            else
            {
                gfxWorld->dpvsDyn.dynEntVisData[0][0] = nullptr;
                gfxWorld->dpvsDyn.dynEntVisData[0][1] = nullptr;
                gfxWorld->dpvsDyn.dynEntVisData[0][2] = nullptr;
            }

            if (gfxWorld->dpvsDyn.dynEntClientWordCount[1] * 32 > 0)
            {
                gfxWorld->dpvsDyn.dynEntVisData[1][0] = m_memory.Alloc<char>(gfxWorld->dpvsDyn.dynEntClientWordCount[1] * 32);
                gfxWorld->dpvsDyn.dynEntVisData[1][1] = m_memory.Alloc<char>(gfxWorld->dpvsDyn.dynEntClientWordCount[1] * 32);
                gfxWorld->dpvsDyn.dynEntVisData[1][2] = m_memory.Alloc<char>(gfxWorld->dpvsDyn.dynEntClientWordCount[1] * 32);

                auto visData10File = m_search_path.Open(j["dpvsDyn"]["dynEntVisData10_Filename"]);
                if (visData10File.IsOpen())
                {
                    auto& visData10Stream = *visData10File.m_stream;
                    visData10Stream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvsDyn.dynEntVisData[1][0]),
                        gfxWorld->dpvsDyn.dynEntClientWordCount[1] * 32
                    );
                }

                auto visData11File = m_search_path.Open(j["dpvsDyn"]["dynEntVisData11_Filename"]);
                if (visData11File.IsOpen())
                {
                    auto& visData11Stream = *visData11File.m_stream;
                    visData11Stream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvsDyn.dynEntVisData[1][1]), 
                        gfxWorld->dpvsDyn.dynEntClientWordCount[1] * 32
                    );
                }

                auto visData12File = m_search_path.Open(j["dpvsDyn"]["dynEntVisData12_Filename"]);
                if (visData12File.IsOpen())
                {
                    auto& visData12Stream = *visData12File.m_stream;
                    visData12Stream.read(
                        reinterpret_cast<char*>(gfxWorld->dpvsDyn.dynEntVisData[1][2]), 
                        gfxWorld->dpvsDyn.dynEntClientWordCount[1] * 32
                    );
                }
            }
            else
            {
                gfxWorld->dpvsDyn.dynEntVisData[1][0] = nullptr;
                gfxWorld->dpvsDyn.dynEntVisData[1][1] = nullptr;
                gfxWorld->dpvsDyn.dynEntVisData[1][2] = nullptr;
            }

            // put here not above, as it requires dvpsdyn values to be initalised
            if (gfxWorld->dpvsDyn.dynEntClientCount[0] > 0)
                gfxWorld->sceneDynModel = m_memory.Alloc<GfxSceneDynModel>(gfxWorld->dpvsDyn.dynEntClientCount[0]);
            else
                gfxWorld->sceneDynModel = nullptr;
            for (unsigned int i = 0; i < gfxWorld->dpvsDyn.dynEntClientCount[0]; i++)
            {
                gfxWorld->sceneDynModel[i].info.packed = j["sceneDynModel"][i]["info_packed"];
                gfxWorld->sceneDynModel[i].dynEntId = j["sceneDynModel"][i]["dynEntId"];
                gfxWorld->sceneDynModel[i].primaryLightIndex = static_cast<unsigned char>(j["sceneDynModel"][i]["primaryLightIndex"]);
                gfxWorld->sceneDynModel[i].reflectionProbeIndex = static_cast<unsigned char>(j["sceneDynModel"][i]["reflectionProbeIndex"]);
            }

            if (gfxWorld->dpvsDyn.dynEntClientCount[1] > 0)
                gfxWorld->sceneDynBrush = m_memory.Alloc<GfxSceneDynBrush>(gfxWorld->dpvsDyn.dynEntClientCount[1]);
            else
                gfxWorld->sceneDynBrush = nullptr;
            for (unsigned int i = 0; i < gfxWorld->dpvsDyn.dynEntClientCount[1]; i++)
            {
                gfxWorld->sceneDynBrush[i].info.surfId = j["sceneDynBrush"][i]["info_surfId"];
                gfxWorld->sceneDynBrush[i].dynEntId = j["sceneDynBrush"][i]["dynEntId"];
            }
            
            if (gfxWorld->dpvsDyn.dynEntClientCount[0] * (gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1) > 0)
            {
                gfxWorld->primaryLightDynEntShadowVis[0] =
                    m_memory.Alloc<unsigned int>(gfxWorld->dpvsDyn.dynEntClientCount[0] * (gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1));
                auto primaryLightDynEntShadowVis0File = m_search_path.Open(j["primaryLightDynEntShadowVis0_Filename"]);
                if (primaryLightDynEntShadowVis0File.IsOpen())
                {
                    auto& primaryLightDynEntShadowVis0Stream = *primaryLightDynEntShadowVis0File.m_stream;
                    primaryLightDynEntShadowVis0Stream.read(
                        reinterpret_cast<char*>(gfxWorld->primaryLightDynEntShadowVis[0]),
                        (gfxWorld->dpvsDyn.dynEntClientCount[0] * (gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1)) * sizeof(unsigned int));
                }
            }
            else
                gfxWorld->primaryLightDynEntShadowVis[0] = nullptr;

            if (gfxWorld->dpvsDyn.dynEntClientCount[1] * (gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1) > 0)
            {
                gfxWorld->primaryLightDynEntShadowVis[1] =
                    m_memory.Alloc<unsigned int>(gfxWorld->dpvsDyn.dynEntClientCount[1] * (gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1));
                auto primaryLightDynEntShadowVis0File = m_search_path.Open(j["primaryLightDynEntShadowVis1_Filename"]);
                if (primaryLightDynEntShadowVis0File.IsOpen())
                {
                    auto& primaryLightDynEntShadowVis0Stream = *primaryLightDynEntShadowVis0File.m_stream;
                    primaryLightDynEntShadowVis0Stream.read(
                        reinterpret_cast<char*>(gfxWorld->primaryLightDynEntShadowVis[1]),
                        (gfxWorld->dpvsDyn.dynEntClientCount[1] * (gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1)) * sizeof(unsigned int));
                }
            }
            else
                gfxWorld->primaryLightDynEntShadowVis[1] = nullptr;

            gfxWorld->waterDirection = j["waterDirection"];
            gfxWorld->waterBuffers[0].bufferSize = j["waterBuffers"]["0"]["bufferSize"];
            if (gfxWorld->waterBuffers[0].bufferSize > 0)
                gfxWorld->waterBuffers[0].buffer = m_memory.Alloc<vec4_t>(gfxWorld->waterBuffers[0].bufferSize / 16);
            else
                gfxWorld->waterBuffers[0].buffer = nullptr;
            for (unsigned int i = 0; i < gfxWorld->waterBuffers[0].bufferSize / 16; i++)
            {
                gfxWorld->waterBuffers[0].buffer[i].x = j["waterBuffers"]["0"]["buffer"][i]["x"];
                gfxWorld->waterBuffers[0].buffer[i].y = j["waterBuffers"]["0"]["buffer"][i]["y"];
                gfxWorld->waterBuffers[0].buffer[i].z = j["waterBuffers"]["0"]["buffer"][i]["z"];
                gfxWorld->waterBuffers[0].buffer[i].w = j["waterBuffers"]["0"]["buffer"][i]["w"];
            }


            gfxWorld->waterBuffers[1].bufferSize = j["waterBuffers"]["1"]["bufferSize"];
            if (gfxWorld->waterBuffers[1].bufferSize > 0)
                gfxWorld->waterBuffers[1].buffer = m_memory.Alloc<vec4_t>(gfxWorld->waterBuffers[1].bufferSize / 16);
            else
                gfxWorld->waterBuffers[1].buffer = nullptr;
            for (unsigned int i = 0; i < gfxWorld->waterBuffers[1].bufferSize / 16; i++)
            {
                gfxWorld->waterBuffers[1].buffer[i].x = j["waterBuffers"]["1"]["buffer"][i]["x"];
                gfxWorld->waterBuffers[1].buffer[i].y = j["waterBuffers"]["1"]["buffer"][i]["y"];
                gfxWorld->waterBuffers[1].buffer[i].z = j["waterBuffers"]["1"]["buffer"][i]["z"];
                gfxWorld->waterBuffers[1].buffer[i].w = j["waterBuffers"]["1"]["buffer"][i]["w"];
            }
           
            std::string waterMaterial = j["waterMaterial"];
            if (waterMaterial.length() > 0)
            {
                auto* waterInfo = context.LoadDependency<AssetMaterial>(waterMaterial);
                if (!waterInfo)
                {
                    printf("can't find water\n");
                    return AssetCreationResult::Failure();
                }

                gfxWorld->waterMaterial = waterInfo->Asset();
            }
            else
                gfxWorld->waterMaterial = nullptr;

            std::string coronaMaterial = j["coronaMaterial"];
            if (coronaMaterial.length() > 0)
            {
                auto* coronaInfo = context.LoadDependency<AssetMaterial>(coronaMaterial);
                if (!coronaInfo)
                {
                    printf("can't find corona\n");
                    return AssetCreationResult::Failure();
                }

                gfxWorld->coronaMaterial = coronaInfo->Asset();
            }
            else
                gfxWorld->coronaMaterial = nullptr;

            std::string ropeMaterial = j["ropeMaterial"];
            if (ropeMaterial.length() > 0)
            {
                auto* ropeInfo = context.LoadDependency<AssetMaterial>(ropeMaterial);
                if (!ropeInfo)
                {
                    printf("can't find rope\n");
                    return AssetCreationResult::Failure();
                }

                gfxWorld->ropeMaterial = ropeInfo->Asset();
            }
            else
                gfxWorld->ropeMaterial = nullptr;

            std::string lutMaterial = j["lutMaterial"];
            if (lutMaterial.length() > 0)
            {
                auto* lutInfo = context.LoadDependency<AssetMaterial>(lutMaterial);
                if (!lutInfo)
                {
                    printf("can't find lut\n");
                    return AssetCreationResult::Failure();
                }

                gfxWorld->lutMaterial = lutInfo->Asset();
            }
            else
                gfxWorld->lutMaterial = nullptr;

            gfxWorld->numOccluders = j["numOccluders"];
            if (gfxWorld->numOccluders > 0)
                gfxWorld->occluders = m_memory.Alloc<Occluder>(gfxWorld->numOccluders);
            else
                gfxWorld->occluders = nullptr;
            for (unsigned int i = 0; i < gfxWorld->numOccluders; i++)
            {
                auto currentOccluder = &gfxWorld->occluders[i];
                auto currentOccluderData = j["occluders"][i];

                std::string occluderName = currentOccluderData["name"];
                assert(occluderName.length() < 15);
                strcpy(currentOccluder->name, occluderName.c_str());

                currentOccluder->flags = currentOccluderData["flags"];
                currentOccluder->points[0].x = currentOccluderData["points"]["0"]["x"];
                currentOccluder->points[0].y = currentOccluderData["points"]["0"]["y"];
                currentOccluder->points[0].z = currentOccluderData["points"]["0"]["z"];
                currentOccluder->points[1].x = currentOccluderData["points"]["1"]["x"];
                currentOccluder->points[1].y = currentOccluderData["points"]["1"]["y"];
                currentOccluder->points[1].z = currentOccluderData["points"]["1"]["z"];
                currentOccluder->points[2].x = currentOccluderData["points"]["2"]["x"];
                currentOccluder->points[2].y = currentOccluderData["points"]["2"]["y"];
                currentOccluder->points[2].z = currentOccluderData["points"]["2"]["z"];
                currentOccluder->points[3].x = currentOccluderData["points"]["3"]["x"];
                currentOccluder->points[3].y = currentOccluderData["points"]["3"]["y"];
                currentOccluder->points[3].z = currentOccluderData["points"]["3"]["z"];
            }

            gfxWorld->numOutdoorBounds = j["numOutdoorBounds"];
            if (gfxWorld->numOutdoorBounds > 0)
                gfxWorld->outdoorBounds = m_memory.Alloc<GfxOutdoorBounds>(gfxWorld->numOutdoorBounds);
            else
                gfxWorld->outdoorBounds = nullptr;
            for (unsigned int i = 0; i < gfxWorld->numOutdoorBounds; i++)
            {
                auto currentOutdoor = &gfxWorld->outdoorBounds[i];
                auto currentOutdoorData = j["outdoorBounds"][i];

                currentOutdoor->bounds[0].x = currentOutdoorData["bounds"]["0"]["x"];
                currentOutdoor->bounds[0].y = currentOutdoorData["bounds"]["0"]["y"];
                currentOutdoor->bounds[0].z = currentOutdoorData["bounds"]["0"]["z"];
                currentOutdoor->bounds[1].x = currentOutdoorData["bounds"]["1"]["x"];
                currentOutdoor->bounds[1].y = currentOutdoorData["bounds"]["1"]["y"];
                currentOutdoor->bounds[1].z = currentOutdoorData["bounds"]["1"]["z"];
            }

            

            gfxWorld->heroLightCount = j["heroLightCount"];
            if (gfxWorld->heroLightCount > 0)
                gfxWorld->heroLights = m_memory.Alloc<GfxHeroLight>(gfxWorld->heroLightCount);
            else
                gfxWorld->heroLights = nullptr;
            for (unsigned int i = 0; i < gfxWorld->heroLightCount; i++)
            {
                auto currentHeroLight = &gfxWorld->heroLights[i];
                auto currentHeroLightData = j["heroLights"][i];

                currentHeroLight->type              = static_cast<unsigned char>(currentHeroLightData["type"]);
                currentHeroLight->radius            = currentHeroLightData["radius"];
                currentHeroLight->cosHalfFovOuter   = currentHeroLightData["cosHalfFovOuter"];
                currentHeroLight-> cosHalfFovInner  = currentHeroLightData["cosHalfFovInner"];
                currentHeroLight->exponent          = currentHeroLightData["exponent"];
                currentHeroLight->unused[0]         = static_cast<unsigned char>(currentHeroLightData["unused"]["0"]);
                currentHeroLight->unused[1]         = static_cast<unsigned char>(currentHeroLightData["unused"]["1"]);
                currentHeroLight->unused[2]         = static_cast<unsigned char>(currentHeroLightData["unused"]["2"]);
                currentHeroLight->color.x           = currentHeroLightData["color"]["x"];
                currentHeroLight->color.y           = currentHeroLightData["color"]["y"];
                currentHeroLight->color.z           = currentHeroLightData["color"]["z"];
                currentHeroLight->dir.x             = currentHeroLightData["dir"]["x"];
                currentHeroLight->dir.y             = currentHeroLightData["dir"]["y"];
                currentHeroLight->dir.z             = currentHeroLightData["dir"]["z"];
                currentHeroLight->origin.x          = currentHeroLightData["origin"]["x"];
                currentHeroLight->origin.y          = currentHeroLightData["origin"]["y"];
                currentHeroLight->origin.z          = currentHeroLightData["origin"]["z"];
            }

            gfxWorld->heroLightTreeCount = j["heroLightTreeCount"];
            if (gfxWorld->heroLightTreeCount > 0)
                gfxWorld->heroLightTree = m_memory.Alloc<GfxHeroLightTree>(gfxWorld->heroLightTreeCount);
            else
                gfxWorld->heroLightTree = nullptr;
            for (unsigned int i = 0; i < gfxWorld->heroLightTreeCount; i++)
            {
                auto currentHeroLightTree = &gfxWorld->heroLightTree[i];
                auto currentHeroLightTreeData = j["heroLightTree"][i];

                currentHeroLightTree->mins.x = currentHeroLightTreeData["mins"]["x"];
                currentHeroLightTree->mins.y = currentHeroLightTreeData["mins"]["y"];
                currentHeroLightTree->mins.z = currentHeroLightTreeData["mins"]["z"];
                currentHeroLightTree->maxs.x = currentHeroLightTreeData["maxs"]["x"];
                currentHeroLightTree->maxs.y = currentHeroLightTreeData["maxs"]["y"];
                currentHeroLightTree->maxs.z = currentHeroLightTreeData["maxs"]["z"];
                currentHeroLightTree->leftNode = currentHeroLightTreeData["leftNode"];
                currentHeroLightTree->rightNode = currentHeroLightTreeData["rightNode"];
            }

            gfxWorld->lightingFlags = j["lightingFlags"];
            gfxWorld->lightingQuality = j["lightingQuality"];

            return AssetCreationResult::Success(context.AddAsset<AssetGfxWorld>(assetName, gfxWorld));
        }

    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
    };
} // namespace

namespace T6
{
    std::unique_ptr<AssetCreator<AssetGfxWorld>> CreateGfxWorldLoader(MemoryManager& memory, ISearchPath& searchPath)
    {
        return std::make_unique<GfxWorldLoader>(memory, searchPath);
    }
} // namespace T6
