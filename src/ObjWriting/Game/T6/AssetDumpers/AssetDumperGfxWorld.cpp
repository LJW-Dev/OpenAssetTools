#include <nlohmann/json.hpp>

#include "AssetDumperGfxWorld.h"

using namespace T6;

bool AssetDumperGfxWorld::ShouldDump(XAssetInfo<GfxWorld>* asset)
{
    return true;
}

const char* gfx_emptyString = "";
const char* gfx_sanitiseJsonString(const char* str) 
{
    if (str == NULL)
        return gfx_emptyString;
    else
        return str;
}

void AssetDumperGfxWorld::DumpAsset(AssetDumpingContext& context, XAssetInfo<GfxWorld>* asset)
{
    const auto* gfxWorld = asset->Asset();
    const auto assetFile = context.OpenAssetFile("bsp/gfxworld.json");

    if (!assetFile)
        return;

    auto& stream = *assetFile;

    nlohmann::json j;
    j["name"] = gfx_sanitiseJsonString(gfxWorld->name);
    j["baseName"] = gfx_sanitiseJsonString(gfxWorld->baseName);
    j["planeCount"] = gfxWorld->planeCount;
    j["nodeCount"] = gfxWorld->nodeCount;
    j["surfaceCount"] = gfxWorld->surfaceCount;

    j["streamInfo"]["aabbTreeCount"] = gfxWorld->streamInfo.aabbTreeCount;
    j["streamInfo"]["leafRefCount"] = gfxWorld->streamInfo.leafRefCount;
    auto aabbTrees = nlohmann::json::array();
    auto leafRefs = nlohmann::json::array();
    for (int i = 0; i < gfxWorld->streamInfo.aabbTreeCount; i++)
    {
        auto currentAabbTree = gfxWorld->streamInfo.aabbTrees[i];
        aabbTrees.push_back({
            {"mins",                 {{"x", currentAabbTree.mins.x}, {"y", currentAabbTree.mins.y}, {"z", currentAabbTree.mins.z}, {"w", currentAabbTree.mins.w}}},
            {"maxs",                 {{"x", currentAabbTree.maxs.x}, {"y", currentAabbTree.maxs.y}, {"z", currentAabbTree.maxs.z}, {"w", currentAabbTree.maxs.w}}},
            {"maxStreamingDistance", currentAabbTree.maxStreamingDistance                                                            },
            {"firstItem",            currentAabbTree.firstItem                                                                       },
            {"itemCount",            currentAabbTree.itemCount                                                                       },
            {"firstChild",           currentAabbTree.firstChild                                                                      },
            {"childCount",           currentAabbTree.childCount                                                                      },
            {"smodelCount",          currentAabbTree.smodelCount                                                                     },
            {"surfaceCount",         currentAabbTree.surfaceCount                                                                    },
        });
    }
    for (int i = 0; i < gfxWorld->streamInfo.leafRefCount; i++)
    {
        leafRefs.push_back(gfxWorld->streamInfo.leafRefs[i]);
    }
    j["streamInfo"]["aabbTrees"] = aabbTrees;
    j["streamInfo"]["leafRefs"] = leafRefs;

    j["skyBoxModel"] = gfx_sanitiseJsonString(gfxWorld->skyBoxModel);

    j["sunParse"]["name"] = gfx_sanitiseJsonString(gfxWorld->sunParse.name);
    j["sunParse"]["fogTransitionTime"] = gfxWorld->sunParse.fogTransitionTime;

    j["sunParse"]["initWorldSun"]["control"] = gfxWorld->sunParse.initWorldSun->control;
    j["sunParse"]["initWorldSun"]["exposure"] = gfxWorld->sunParse.initWorldSun->exposure;
    j["sunParse"]["initWorldSun"]["angles"] = {{"x", gfxWorld->sunParse.initWorldSun->angles.x}, 
                                               {"y", gfxWorld->sunParse.initWorldSun->angles.y}, 
                                               {"z", gfxWorld->sunParse.initWorldSun->angles.z}};
    j["sunParse"]["initWorldSun"]["ambientColor"] = {{"x", gfxWorld->sunParse.initWorldSun->ambientColor.x},
                                                     {"y", gfxWorld->sunParse.initWorldSun->ambientColor.y},
                                                     {"z", gfxWorld->sunParse.initWorldSun->ambientColor.z},
                                                     {"w", gfxWorld->sunParse.initWorldSun->ambientColor.w}};
    j["sunParse"]["initWorldSun"]["sunCd"] = {{"x", gfxWorld->sunParse.initWorldSun->sunCd.x},
                                              {"y", gfxWorld->sunParse.initWorldSun->sunCd.y},
                                              {"z", gfxWorld->sunParse.initWorldSun->sunCd.z},
                                              {"w", gfxWorld->sunParse.initWorldSun->sunCd.w}};
    j["sunParse"]["initWorldSun"]["sunCs"] = {{"x", gfxWorld->sunParse.initWorldSun->sunCs.x},
                                              {"y", gfxWorld->sunParse.initWorldSun->sunCs.y},
                                              {"z", gfxWorld->sunParse.initWorldSun->sunCs.z},
                                              {"w", gfxWorld->sunParse.initWorldSun->sunCs.w}};
    j["sunParse"]["initWorldSun"]["skyColor"] = {{"x", gfxWorld->sunParse.initWorldSun->skyColor.x},
                                                 {"y", gfxWorld->sunParse.initWorldSun->skyColor.y},
                                                 {"z", gfxWorld->sunParse.initWorldSun->skyColor.z},
                                                 {"w", gfxWorld->sunParse.initWorldSun->skyColor.w}};

    j["sunParse"]["initWorldFog"]["baseDist"] = gfxWorld->sunParse.initWorldFog->baseDist;
    j["sunParse"]["initWorldFog"]["halfDist"] = gfxWorld->sunParse.initWorldFog->halfDist;
    j["sunParse"]["initWorldFog"]["baseHeight"] = gfxWorld->sunParse.initWorldFog->baseHeight;
    j["sunParse"]["initWorldFog"]["halfHeight"] = gfxWorld->sunParse.initWorldFog->halfHeight;
    j["sunParse"]["initWorldFog"]["sunFogPitch"] = gfxWorld->sunParse.initWorldFog->sunFogPitch;
    j["sunParse"]["initWorldFog"]["sunFogYaw"] = gfxWorld->sunParse.initWorldFog->sunFogYaw;
    j["sunParse"]["initWorldFog"]["sunFogInner"] = gfxWorld->sunParse.initWorldFog->sunFogInner;
    j["sunParse"]["initWorldFog"]["sunFogOuter"] = gfxWorld->sunParse.initWorldFog->sunFogOuter;
    j["sunParse"]["initWorldFog"]["fogOpacity"] = gfxWorld->sunParse.initWorldFog->fogOpacity;
    j["sunParse"]["initWorldFog"]["sunFogOpacity"] = gfxWorld->sunParse.initWorldFog->sunFogOpacity;
    j["sunParse"]["initWorldFog"]["fogColor"] = {{"x", gfxWorld->sunParse.initWorldFog->fogColor.x}, 
                                                 {"y", gfxWorld->sunParse.initWorldFog->fogColor.y}, 
                                                 {"z", gfxWorld->sunParse.initWorldFog->fogColor.z}};
    j["sunParse"]["initWorldFog"]["sunFogColor"] = {{"x", gfxWorld->sunParse.initWorldFog->sunFogColor.x}, 
                                                    {"y", gfxWorld->sunParse.initWorldFog->sunFogColor.y}, 
                                                    {"z", gfxWorld->sunParse.initWorldFog->sunFogColor.z}};

    if (gfxWorld->sunLight != NULL)
    {
        j["sunLight"]["type"] = gfxWorld->sunLight->type;
        j["sunLight"]["canUseShadowMap"] = gfxWorld->sunLight->canUseShadowMap;
        j["sunLight"]["shadowmapVolume"] = gfxWorld->sunLight->shadowmapVolume;
        j["sunLight"]["cullDist"] = gfxWorld->sunLight->cullDist;
        j["sunLight"]["color"] = {
            {"x", gfxWorld->sunLight->color.x},
            {"y", gfxWorld->sunLight->color.y},
            {"z", gfxWorld->sunLight->color.z}
        };
        j["sunLight"]["dir"] = {
            {"x", gfxWorld->sunLight->dir.x},
            {"y", gfxWorld->sunLight->dir.y},
            {"z", gfxWorld->sunLight->dir.z}
        };
        j["sunLight"]["origin"] = {
            {"x", gfxWorld->sunLight->origin.x},
            {"y", gfxWorld->sunLight->origin.y},
            {"z", gfxWorld->sunLight->origin.z}
        };
        j["sunLight"]["radius"] = gfxWorld->sunLight->radius;
        j["sunLight"]["cosHalfFovOuter"] = gfxWorld->sunLight->cosHalfFovOuter;
        j["sunLight"]["cosHalfFovInner"] = gfxWorld->sunLight->cosHalfFovInner;
        j["sunLight"]["exponent"] = gfxWorld->sunLight->exponent;
        j["sunLight"]["spotShadowIndex"] = gfxWorld->sunLight->spotShadowIndex;
        j["sunLight"]["dAttenuation"] = gfxWorld->sunLight->dAttenuation;
        j["sunLight"]["roundness"] = gfxWorld->sunLight->roundness;
        j["sunLight"]["angles"] = {
            {"x", gfxWorld->sunLight->angles.x},
            {"y", gfxWorld->sunLight->angles.y},
            {"z", gfxWorld->sunLight->angles.z}
        };
        j["sunLight"]["spotShadowHiDistance"] = gfxWorld->sunLight->spotShadowHiDistance;
        j["sunLight"]["diffuseColor"] = {
            {"x", gfxWorld->sunLight->diffuseColor.x},
            {"y", gfxWorld->sunLight->diffuseColor.y},
            {"z", gfxWorld->sunLight->diffuseColor.z},
            {"w", gfxWorld->sunLight->diffuseColor.w}
        };
        j["sunLight"]["shadowColor"] = {
            {"x", gfxWorld->sunLight->shadowColor.x},
            {"y", gfxWorld->sunLight->shadowColor.y},
            {"z", gfxWorld->sunLight->shadowColor.z},
            {"w", gfxWorld->sunLight->shadowColor.w}
        };
        j["sunLight"]["falloff"] = {
            {"x", gfxWorld->sunLight->falloff.x},
            {"y", gfxWorld->sunLight->falloff.y},
            {"z", gfxWorld->sunLight->falloff.z},
            {"w", gfxWorld->sunLight->falloff.w}
        };
        j["sunLight"]["aAbB"] = {
            {"x", gfxWorld->sunLight->aAbB.x},
            {"y", gfxWorld->sunLight->aAbB.y},
            {"z", gfxWorld->sunLight->aAbB.z},
            {"w", gfxWorld->sunLight->aAbB.w}
        };
        j["sunLight"]["cookieControl0"] = {
            {"x", gfxWorld->sunLight->cookieControl0.x},
            {"y", gfxWorld->sunLight->cookieControl0.y},
            {"z", gfxWorld->sunLight->cookieControl0.z},
            {"w", gfxWorld->sunLight->cookieControl0.w}
        };
        j["sunLight"]["cookieControl1"] = {
            {"x", gfxWorld->sunLight->cookieControl1.x},
            {"y", gfxWorld->sunLight->cookieControl1.y},
            {"z", gfxWorld->sunLight->cookieControl1.z},
            {"w", gfxWorld->sunLight->cookieControl1.w}
        };
        j["sunLight"]["cookieControl2"] = {
            {"x", gfxWorld->sunLight->cookieControl2.x},
            {"y", gfxWorld->sunLight->cookieControl2.y},
            {"z", gfxWorld->sunLight->cookieControl2.z},
            {"w", gfxWorld->sunLight->cookieControl2.w}
        };

        j["sunLight"]["viewMatrix"] = {
            {"0",
             {
                 {"x", gfxWorld->sunLight->viewMatrix.m[0].x},
                 {"y", gfxWorld->sunLight->viewMatrix.m[0].y},
                 {"z", gfxWorld->sunLight->viewMatrix.m[0].z},
                 {"w", gfxWorld->sunLight->viewMatrix.m[0].w},
             }},
            {"1",
             {
                 {"x", gfxWorld->sunLight->viewMatrix.m[1].x},
                 {"y", gfxWorld->sunLight->viewMatrix.m[1].y},
                 {"z", gfxWorld->sunLight->viewMatrix.m[1].z},
                 {"w", gfxWorld->sunLight->viewMatrix.m[1].w},
             }},
            {"2",
             {
                 {"x", gfxWorld->sunLight->viewMatrix.m[2].x},
                 {"y", gfxWorld->sunLight->viewMatrix.m[2].y},
                 {"z", gfxWorld->sunLight->viewMatrix.m[2].z},
                 {"w", gfxWorld->sunLight->viewMatrix.m[2].w},
             }},
            {"3",
             {
                 {"x", gfxWorld->sunLight->viewMatrix.m[3].x},
                 {"y", gfxWorld->sunLight->viewMatrix.m[3].y},
                 {"z", gfxWorld->sunLight->viewMatrix.m[3].z},
                 {"w", gfxWorld->sunLight->viewMatrix.m[3].w},
             }}
        };

        j["sunLight"]["projMatrix"] = {
            {"0",
             {
                 {"x", gfxWorld->sunLight->projMatrix.m[0].x},
                 {"y", gfxWorld->sunLight->projMatrix.m[0].y},
                 {"z", gfxWorld->sunLight->projMatrix.m[0].z},
                 {"w", gfxWorld->sunLight->projMatrix.m[0].w},
             }},
            {"1",
             {
                 {"x", gfxWorld->sunLight->projMatrix.m[1].x},
                 {"y", gfxWorld->sunLight->projMatrix.m[1].y},
                 {"z", gfxWorld->sunLight->projMatrix.m[1].z},
                 {"w", gfxWorld->sunLight->projMatrix.m[1].w},
             }},
            {"2",
             {
                 {"x", gfxWorld->sunLight->projMatrix.m[2].x},
                 {"y", gfxWorld->sunLight->projMatrix.m[2].y},
                 {"z", gfxWorld->sunLight->projMatrix.m[2].z},
                 {"w", gfxWorld->sunLight->projMatrix.m[2].w},
             }},
            {"3",
             {
                 {"x", gfxWorld->sunLight->projMatrix.m[3].x},
                 {"y", gfxWorld->sunLight->projMatrix.m[3].y},
                 {"z", gfxWorld->sunLight->projMatrix.m[3].z},
                 {"w", gfxWorld->sunLight->projMatrix.m[3].w},
             }}
        };

        if (gfxWorld->sunLight->def == NULL)
            j["sunLight"]["defName"] = "";
        else
            j["sunLight"]["defName"] = gfx_sanitiseJsonString(gfxWorld->sunLight->def->name);
    }
    else
    {
        j["sunLight"] = nlohmann::json::array();
    }

    j["sunPrimaryLightIndex"] = gfxWorld->sunPrimaryLightIndex;
    j["primaryLightCount"] = gfxWorld->primaryLightCount;

    j["coronaCount"] = gfxWorld->coronaCount;
    auto coronas = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->coronaCount; i++)
    {
        auto currentCorona = gfxWorld->coronas[i];
        coronas.push_back({
            {"origin", {{"x", currentCorona.origin.x}, 
                        {"y", currentCorona.origin.y}, 
                        {"z", currentCorona.origin.z}}},
            {"radius", currentCorona.radius},
            "color", {{"x", currentCorona.color.x}, 
                      {"y", currentCorona.color.y}, 
                      {"z", currentCorona.color.z}},
            {"intensity", currentCorona.intensity}
        });
    }
    j["coronas"] = coronas;

    j["shadowMapVolumeCount"] = gfxWorld->shadowMapVolumeCount;
    auto shadowMapVolumes = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->shadowMapVolumeCount; i++)
    {
        auto currentShadowMapVolume = gfxWorld->shadowMapVolumes[i];
        shadowMapVolumes.push_back({
            {"control",  currentShadowMapVolume.control },
            {"padding1", currentShadowMapVolume.padding1},
            {"padding2", currentShadowMapVolume.padding2},
            {"padding3", currentShadowMapVolume.padding3},
        });
    }
    j["shadowMapVolumes"] = shadowMapVolumes;

    j["shadowMapVolumePlaneCount"] = gfxWorld->shadowMapVolumePlaneCount;
    auto shadowMapVolumePlanes = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->shadowMapVolumePlaneCount; i++)
    {
        auto currentShadowMapVolumePlane = gfxWorld->shadowMapVolumePlanes[i];
        shadowMapVolumePlanes.push_back({
            {"plane",
             {{"x", currentShadowMapVolumePlane.plane.x},
              {"y", currentShadowMapVolumePlane.plane.y},
              {"z", currentShadowMapVolumePlane.plane.z},
              {"w", currentShadowMapVolumePlane.plane.w}}}
        });
    }
    j["shadowMapVolumePlanes"] = shadowMapVolumePlanes;

    j["exposureVolumeCount"] = gfxWorld->exposureVolumeCount;
    auto exposureVolumes = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->exposureVolumeCount; i++)
    {
        auto currentShadowMapVolumePlane = gfxWorld->exposureVolumes[i];
        exposureVolumes.push_back({
            {"control",                currentShadowMapVolumePlane.control               },
            {"exposure",               currentShadowMapVolumePlane.exposure              },
            {"luminanceIncreaseScale", currentShadowMapVolumePlane.luminanceIncreaseScale},
            {"luminanceDecreaseScale", currentShadowMapVolumePlane.luminanceDecreaseScale},
            {"featherRange",           currentShadowMapVolumePlane.featherRange          },
            {"featherAdjust",          currentShadowMapVolumePlane.featherAdjust         }
        });
    }
    j["exposureVolumes"] = exposureVolumes;

    j["exposureVolumePlaneCount"] = gfxWorld->exposureVolumePlaneCount;
    auto exposureVolumePlanes = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->exposureVolumePlaneCount; i++)
    {
        auto currentExposureVolumePlane = gfxWorld->exposureVolumePlanes[i];
        exposureVolumePlanes.push_back({
            {"plane",
             {{"x", currentExposureVolumePlane.plane.x},
              {"y", currentExposureVolumePlane.plane.y},
              {"z", currentExposureVolumePlane.plane.z},
              {"w", currentExposureVolumePlane.plane.w}}}
        });
    }
    j["exposureVolumePlanes"] = exposureVolumePlanes;

    j["worldFogVolumeCount"] = gfxWorld->worldFogVolumeCount;
    auto worldFogVolumes = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->worldFogVolumeCount; i++)
    {
        auto currentExposureVolumePlane = gfxWorld->worldFogVolumes[i];
        worldFogVolumes.push_back({
            {"mins",              {{"x", currentExposureVolumePlane.mins.x}, 
                                   {"y", currentExposureVolumePlane.mins.y}, 
                                   {"z", currentExposureVolumePlane.mins.z}}},
            {"control",           currentExposureVolumePlane.control},
            {"maxs",              {{"x", currentExposureVolumePlane.maxs.x}, 
                                   {"y", currentExposureVolumePlane.maxs.y}, 
                                   {"z", currentExposureVolumePlane.maxs.z}}},
            {"fogTransitionTime", currentExposureVolumePlane.fogTransitionTime},
            {"controlEx",         currentExposureVolumePlane.controlEx},
            {"volumeWorldFog",
             {
                 {"baseDist", currentExposureVolumePlane.volumeWorldFog->baseDist},
                 {"halfDist", currentExposureVolumePlane.volumeWorldFog->halfDist},
                 {"baseHeight", currentExposureVolumePlane.volumeWorldFog->baseHeight},
                 {"halfHeight", currentExposureVolumePlane.volumeWorldFog->halfHeight},
                 {"sunFogPitch", currentExposureVolumePlane.volumeWorldFog->sunFogPitch},
                 {"sunFogYaw", currentExposureVolumePlane.volumeWorldFog->sunFogYaw},
                 {"sunFogInner", currentExposureVolumePlane.volumeWorldFog->sunFogInner},
                 {"sunFogOuter", currentExposureVolumePlane.volumeWorldFog->sunFogOuter},
                 {"fogOpacity", currentExposureVolumePlane.volumeWorldFog->fogOpacity},
                 {"sunFogOpacity", currentExposureVolumePlane.volumeWorldFog->sunFogOpacity},
                 {"fogColor",
                  {{"x", currentExposureVolumePlane.volumeWorldFog->fogColor.x},
                   {"y", currentExposureVolumePlane.volumeWorldFog->fogColor.y},
                   {"z", currentExposureVolumePlane.volumeWorldFog->fogColor.z}}},
                 {"sunFogColor",
                  {{"x", currentExposureVolumePlane.volumeWorldFog->sunFogColor.x},
                   {"y", currentExposureVolumePlane.volumeWorldFog->sunFogColor.y},
                   {"z", currentExposureVolumePlane.volumeWorldFog->sunFogColor.z}}},
             }                                                                                                                             }
        });
    }
    j["worldFogVolumes"] = worldFogVolumes;

    j["worldFogVolumePlaneCount"] = gfxWorld->worldFogVolumePlaneCount;
    auto worldFogVolumePlanes = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->worldFogVolumePlaneCount; i++)
    {
        auto currentWorldFogVolumePlane = gfxWorld->worldFogVolumePlanes[i];
        worldFogVolumePlanes.push_back({
            {"plane",
             {{"x", currentWorldFogVolumePlane.plane.x},
              {"y", currentWorldFogVolumePlane.plane.y},
              {"z", currentWorldFogVolumePlane.plane.z},
              {"w", currentWorldFogVolumePlane.plane.w}}}
        });
    }
    j["worldFogVolumePlanes"] = worldFogVolumePlanes;

    j["worldFogModifierVolumeCount"] = gfxWorld->worldFogModifierVolumeCount;
    auto worldFogModifierVolumes = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->worldFogModifierVolumeCount; i++)
    {
        auto currentWorldFogModifierVolume = gfxWorld->worldFogModifierVolumes[i];
        worldFogModifierVolumes.push_back({
            {"control",        currentWorldFogModifierVolume.control       },
            {"minX",           currentWorldFogModifierVolume.minX          },
            {"minY",           currentWorldFogModifierVolume.minY          },
            {"minZ",           currentWorldFogModifierVolume.minZ          },
            {"maxX",           currentWorldFogModifierVolume.maxX          },
            {"maxY",           currentWorldFogModifierVolume.maxY          },
            {"maxZ",           currentWorldFogModifierVolume.maxZ          },
            {"controlEx",      currentWorldFogModifierVolume.controlEx     },
            {"transitionTime", currentWorldFogModifierVolume.transitionTime},
            {"depthScale",     currentWorldFogModifierVolume.depthScale    },
            {"heightScale",    currentWorldFogModifierVolume.heightScale   },
            {"colorAdjust",
             {
                 {"x", currentWorldFogModifierVolume.colorAdjust.x},
                 {"y", currentWorldFogModifierVolume.colorAdjust.y},
                 {"z", currentWorldFogModifierVolume.colorAdjust.z},
                 {"w", currentWorldFogModifierVolume.colorAdjust.w},
             }                                                             },
        });
    }
    j["worldFogModifierVolumes"] = worldFogModifierVolumes;

    j["worldFogModifierVolumePlaneCount"] = gfxWorld->worldFogModifierVolumePlaneCount;
    auto worldFogModifierVolumePlanes = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->worldFogModifierVolumePlaneCount; i++)
    {
        auto currentWorldFogModifierVolumePlane = gfxWorld->worldFogModifierVolumePlanes[i];
        worldFogModifierVolumePlanes.push_back({
            {"plane",
             {{"x", currentWorldFogModifierVolumePlane.plane.x},
              {"y", currentWorldFogModifierVolumePlane.plane.y},
              {"z", currentWorldFogModifierVolumePlane.plane.z},
              {"w", currentWorldFogModifierVolumePlane.plane.w}}}
        });
    }
    j["worldFogModifierVolumePlanes"] = worldFogModifierVolumePlanes;

    j["lutVolumeCount"] = gfxWorld->lutVolumeCount;
    auto lutVolumes = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->lutVolumeCount; i++)
    {
        auto currentlutVolume = gfxWorld->lutVolumes[i];
        lutVolumes.push_back({
            {"mins",              {{"x", currentlutVolume.mins.x}, 
                                   {"y", currentlutVolume.mins.y}, 
                                   {"z", currentlutVolume.mins.z}}},
            {"control",           currentlutVolume.control                                                   },
            {"maxs",              {{"x", currentlutVolume.maxs.x}, 
                                   {"y", currentlutVolume.maxs.y}, 
                                   {"z", currentlutVolume.maxs.z}}},
            {"lutTransitionTime", currentlutVolume.lutTransitionTime                                         },
            {"lutIndex",          currentlutVolume.lutIndex                                                  },
        });
    }
    j["lutVolumes"] = lutVolumes;

    j["lutVolumePlaneCount"] = gfxWorld->lutVolumePlaneCount;
    auto lutVolumePlanes = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->lutVolumePlaneCount; i++)
    {
        auto currentLutVolumePlane = gfxWorld->lutVolumePlanes[i];
        lutVolumePlanes.push_back({
            {"plane", {{"x", currentLutVolumePlane.plane.x}, 
                       {"y", currentLutVolumePlane.plane.y}, 
                       {"z", currentLutVolumePlane.plane.z}, 
                       {"w", currentLutVolumePlane.plane.w}}}
        });
    }
    j["lutVolumePlanes"] = lutVolumePlanes;

    j["skyDynIntensity"]["angle0"] = gfxWorld->skyDynIntensity.angle0;
    j["skyDynIntensity"]["angle1"] = gfxWorld->skyDynIntensity.angle1;
    j["skyDynIntensity"]["factor0"] = gfxWorld->skyDynIntensity.factor0;
    j["skyDynIntensity"]["factor1"] = gfxWorld->skyDynIntensity.factor1;

    auto dpvsPlanes = nlohmann::json::array();
    auto dpvsNodes = nlohmann::json::array();
    for (int i = 0; i < gfxWorld->planeCount; i++)
    {
        auto currentDpvsPlane = gfxWorld->dpvsPlanes.planes[i];
        dpvsPlanes.push_back({
            {"normal",   {{"x", currentDpvsPlane.normal.x}, {"y", currentDpvsPlane.normal.y}, {"z", currentDpvsPlane.normal.z}}},
            {"dist",     currentDpvsPlane.dist                                                            },
            {"type",     currentDpvsPlane.type                                                            },
            {"signbits", currentDpvsPlane.signbits                                                        },
            {"pad",      {{"0", currentDpvsPlane.pad[0]}, {"1", currentDpvsPlane.pad[1]}}                 },
        });
    }
    for (int i = 0; i < gfxWorld->nodeCount; i++)
    {
        dpvsNodes.push_back(gfxWorld->dpvsPlanes.nodes[i]);
    }

    j["dpvsPlanes"]["cellCount"] = gfxWorld->dpvsPlanes.cellCount;
    j["dpvsPlanes"]["planes"] = dpvsPlanes;
    j["dpvsPlanes"]["nodes"] = dpvsNodes;
    j["dpvsPlanes"]["sceneEntCellBits_Filepath"] = "bsp/gfxworld/dpvsPlanes_sceneEntCellBits";
    if (gfxWorld->dpvsPlanes.cellCount * 512 > 0 && gfxWorld->dpvsPlanes.sceneEntCellBits)
    {
        auto sceneEntCellBitsFile = context.OpenAssetFile("bsp/gfxworld/dpvsPlanes_sceneEntCellBits");
        if (sceneEntCellBitsFile)
        {
            auto& sceneEntCellBitsStream = *sceneEntCellBitsFile;
            sceneEntCellBitsStream.write(
                reinterpret_cast<const char*>(gfxWorld->dpvsPlanes.sceneEntCellBits), 
                gfxWorld->dpvsPlanes.cellCount * 512 * sizeof(unsigned int)
            );
        }
    }

    j["cellBitsCount"] = gfxWorld->cellBitsCount;

    auto gfxCells = nlohmann::json::array();
    for (int i = 0; i < gfxWorld->dpvsPlanes.cellCount; i++)
    {
        auto currentCell = gfxWorld->cells[i];

        auto aabbTrees = nlohmann::json::array();
        auto portals = nlohmann::json::array();
        auto reflectionProbes = nlohmann::json::array();

        for (int k = 0; k < currentCell.aabbTreeCount; k++)
        {
            auto currentAabbTree = currentCell.aabbTree[k];

            auto smodelIndexes = nlohmann::json::array();
            for (unsigned int l = 0; l < currentAabbTree.smodelIndexCount; l++)
            {
                smodelIndexes.push_back(currentAabbTree.smodelIndexes[l]);
            }

            aabbTrees.push_back({
                {"mins",             {{"x", currentAabbTree.mins.x}, {"y", currentAabbTree.mins.y}, {"z", currentAabbTree.mins.z}}},
                {"maxs",             {{"x", currentAabbTree.maxs.x}, {"y", currentAabbTree.maxs.y}, {"z", currentAabbTree.maxs.z}}},
                {"childCount",       currentAabbTree.childCount                                              },
                {"surfaceCount",     currentAabbTree.surfaceCount                                            },
                {"startSurfIndex",   currentAabbTree.startSurfIndex                                          },
                {"smodelIndexCount", currentAabbTree.smodelIndexCount                                        },
                {"smodelIndexes",    smodelIndexes                                                           },
                {"childrenOffset",   currentAabbTree.childrenOffset                                          }
            });
        }
        for (int k = 0; k < currentCell.portalCount; k++)
        {
            auto currentPortal = currentCell.portals[k];

            auto portalVertices = nlohmann::json::array();
            for (int l = 0; l < currentPortal.vertexCount; l++)
            {
                auto currentVertex = currentPortal.vertices[l];
                portalVertices.push_back({
                    {"x", currentVertex.x},
                    {"y", currentVertex.y}, 
                    {"z", currentVertex.z}
                });
            }

            // find the cell that is referenced
            int cellReferenceIndex = -1;
            for (int l = 0; l < gfxWorld->dpvsPlanes.cellCount; l++)
            {
                if (&gfxWorld->cells[l] == currentPortal.cell)
                {
                    cellReferenceIndex = l;
                    break;
                }
            }
            assert(cellReferenceIndex != -1);

            assert(currentPortal.writable.hullPoints == nullptr);
            assert(currentPortal.writable.queuedParent == nullptr);
            portals.push_back({
                {"writable",
                 {{"isQueued", currentPortal.writable.isQueued},
                  {"isAncestor", currentPortal.writable.isAncestor},
                  {"recursionDepth", currentPortal.writable.recursionDepth},
                  {"hullPointCount", currentPortal.writable.hullPointCount}}                              },
                {"plane",
                 {{"coeffs",
                   {
                       {"x", currentPortal.plane.coeffs.x},
                       {"y", currentPortal.plane.coeffs.y},
                       {"z", currentPortal.plane.coeffs.z},
                       {"w", currentPortal.plane.coeffs.w},
                   }},
                  {"side", {{"0", currentPortal.plane.side[0]}, {"1", currentPortal.plane.side[1]}, {"2", currentPortal.plane.side[2]}}},
                  {"pad", currentPortal.plane.pad}}                                                       },
                {"cellReferenceIndex", cellReferenceIndex                                                 },
                {"vertexCount",        currentPortal.vertexCount                                          },
                {"vertices",           portalVertices                                                     },

                {"hullAxis",
                 {{"0", {{"x", currentPortal.hullAxis[0].x}, {"y", currentPortal.hullAxis[0].y}, {"z", currentPortal.hullAxis[0].z}}},
                  {"1", {{"x", currentPortal.hullAxis[1].x}, {"y", currentPortal.hullAxis[1].y}, {"z", currentPortal.hullAxis[1].z}}}}},
                {"bounds",
                 {{"0", {{"x", currentPortal.bounds[0].x}, {"y", currentPortal.bounds[0].y}, {"z", currentPortal.bounds[0].z}}},
                  {"1", {{"x", currentPortal.bounds[1].x}, {"y", currentPortal.bounds[1].y}, {"z", currentPortal.bounds[1].z}}}}},
            });
        }
        for (int k = 0; k < currentCell.reflectionProbeCount; k++)
        {
            reflectionProbes.push_back(currentCell.reflectionProbes[k]);
        }

        gfxCells.push_back({
            {"mins",                 {{"x", currentCell.mins.x}, {"y", currentCell.mins.y}, {"z", currentCell.mins.z}}},
            {"maxs",                 {{"x", currentCell.maxs.x}, {"y", currentCell.maxs.y}, {"z", currentCell.maxs.z}}},
            {"aabbTreeCount",        currentCell.aabbTreeCount                                   },
            {"aabbTree",             aabbTrees                                                   },
            {"portalCount",          currentCell.portalCount                                     },
            {"portals",              portals                                                     },
            {"reflectionProbeCount", currentCell.reflectionProbeCount                            },
            {"reflectionProbes",     reflectionProbes                                            },
        });
    }
    j["cells"] = gfxCells;

     auto reflectionProbes = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->draw.reflectionProbeCount; i++)
    {
        auto currentReflectionProbe = gfxWorld->draw.reflectionProbes[i];

        const char* reflectionImageName;
        if (currentReflectionProbe.reflectionImage == NULL)
            reflectionImageName = "";
        else
            reflectionImageName = gfx_sanitiseJsonString(currentReflectionProbe.reflectionImage->name);

        auto probeVolumes = nlohmann::json::array();
        for (unsigned int k = 0; k < currentReflectionProbe.probeVolumeCount; k++)
        {
            auto currentProbeVolume = currentReflectionProbe.probeVolumes[k];
            probeVolumes.push_back({
                {"volumePlanes0",
                    {
                        {"x", currentProbeVolume.volumePlanes[0].x},
                        {"y", currentProbeVolume.volumePlanes[0].y},
                        {"z", currentProbeVolume.volumePlanes[0].z},
                        {"w", currentProbeVolume.volumePlanes[0].w},
                    }},
                {"volumePlanes1",
                 {
                     {"x", currentProbeVolume.volumePlanes[1].x},
                     {"y", currentProbeVolume.volumePlanes[1].y},
                     {"z", currentProbeVolume.volumePlanes[1].z},
                     {"w", currentProbeVolume.volumePlanes[1].w},
                 }},
                {"volumePlanes2",
                 {
                     {"x", currentProbeVolume.volumePlanes[2].x},
                     {"y", currentProbeVolume.volumePlanes[2].y},
                     {"z", currentProbeVolume.volumePlanes[2].z},
                     {"w", currentProbeVolume.volumePlanes[2].w},
                 }},
                {"volumePlanes3",
                 {
                     {"x", currentProbeVolume.volumePlanes[3].x},
                     {"y", currentProbeVolume.volumePlanes[3].y},
                     {"z", currentProbeVolume.volumePlanes[3].z},
                     {"w", currentProbeVolume.volumePlanes[3].w},
                 }},
                {"volumePlanes4",
                 {
                     {"x", currentProbeVolume.volumePlanes[4].x},
                     {"y", currentProbeVolume.volumePlanes[4].y},
                     {"z", currentProbeVolume.volumePlanes[4].z},
                     {"w", currentProbeVolume.volumePlanes[4].w},
                 }},
                {"volumePlanes5",
                 {
                     {"x", currentProbeVolume.volumePlanes[5].x},
                     {"y", currentProbeVolume.volumePlanes[5].y},
                     {"z", currentProbeVolume.volumePlanes[5].z},
                     {"w", currentProbeVolume.volumePlanes[5].w},
                 }},
            });
        }

        reflectionProbes.push_back({
            {"origin",
                {
                    {"x", currentReflectionProbe.origin.x}, 
                    {"y", currentReflectionProbe.origin.y}, 
                    {"z", currentReflectionProbe.origin.z}
                }},
            {"lightingSH", {
                {"V0", 
                    {
                        {"x", currentReflectionProbe.lightingSH.V0.x},
                        {"y", currentReflectionProbe.lightingSH.V0.y},
                        {"z", currentReflectionProbe.lightingSH.V0.z},
                        {"w", currentReflectionProbe.lightingSH.V0.w},
                    }
                }, 
                {"V1",
                    {
                        {"x", currentReflectionProbe.lightingSH.V1.x},
                        {"y", currentReflectionProbe.lightingSH.V1.y},
                        {"z", currentReflectionProbe.lightingSH.V1.z},
                        {"w", currentReflectionProbe.lightingSH.V1.w},
                    }}, 
                {"V2",
                    {
                        {"x", currentReflectionProbe.lightingSH.V2.x},
                        {"y", currentReflectionProbe.lightingSH.V2.y},
                        {"z", currentReflectionProbe.lightingSH.V2.z},
                        {"w", currentReflectionProbe.lightingSH.V2.w},
                    }
                }}},
            {"reflectionImage", reflectionImageName},
            {"mipLodBias", currentReflectionProbe.mipLodBias},
            {"probeVolumeCount", currentReflectionProbe.probeVolumeCount},
            {"probeVolumes", probeVolumes}
        });
    }
    j["draw"]["reflectionProbeCount"] = gfxWorld->draw.reflectionProbeCount;
    j["draw"]["reflectionProbes"] = reflectionProbes;

    auto reflectionProbeTextures = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->draw.reflectionProbeCount; i++)
    {
        auto currentReflectionProbeTexture = gfxWorld->draw.reflectionProbeTextures[i];
        
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        assert(currentReflectionProbeTexture.basemap == NULL);
        reflectionProbeTextures.push_back({{"basemap", 0}});
    }
    j["draw"]["reflectionProbeTextures"] = reflectionProbeTextures;

    auto lightmaps = nlohmann::json::array();
    auto lightmapPrimaryTextures = nlohmann::json::array();
    auto lightmapSecondaryTextures = nlohmann::json::array();
    for (int i = 0; i < gfxWorld->draw.lightmapCount; i++)
    {
        auto currentLightmap = gfxWorld->draw.lightmaps[i];

        const char* primaryName;
        if (currentLightmap.primary == NULL)
            primaryName = "";
        else
            primaryName = gfx_sanitiseJsonString(currentLightmap.primary->name);

        const char* secondaryName;
        if (currentLightmap.secondary == NULL)
            secondaryName = "";
        else 
            secondaryName = gfx_sanitiseJsonString(currentLightmap.secondary->name);

        lightmaps.push_back({
            {"primary", primaryName},
            {"secondary", secondaryName},
        });
        
        assert(gfxWorld->draw.lightmapPrimaryTextures[i].basemap == NULL);
        assert(gfxWorld->draw.lightmapSecondaryTextures[i].basemap == NULL);
        lightmapPrimaryTextures.push_back({{"basemap", 0}});
        lightmapSecondaryTextures.push_back({{"basemap", 0}});
    }
    j["draw"]["lightmapCount"] = gfxWorld->draw.lightmapCount;
    j["draw"]["lightmaps"] = lightmaps;
    j["draw"]["lightmapPrimaryTextures"] = lightmapPrimaryTextures;
    j["draw"]["lightmapSecondaryTextures"] = lightmapSecondaryTextures;
    j["draw"]["vertexCount"] = gfxWorld->draw.vertexCount;

    j["draw"]["vertexDataSize0"] = gfxWorld->draw.vertexDataSize0;
    j["draw"]["vertexData0_Filename"] = "bsp/gfxworld/draw_vertexData0";
    if (gfxWorld->draw.vertexDataSize0 > 0 && gfxWorld->draw.vd0.data)
    {
        auto vertexData0File = context.OpenAssetFile("bsp/gfxworld/draw_vertexData0");
        if (vertexData0File)
        {
            auto& vertexData0Stream = *vertexData0File;
            vertexData0Stream.write(gfxWorld->draw.vd0.data, gfxWorld->draw.vertexDataSize0);
        }
    }
    
    j["draw"]["vertexDataSize1"] = gfxWorld->draw.vertexDataSize1;
    j["draw"]["vertexData1_Filename"] = "bsp/gfxworld/draw_vertexData1";
    if (gfxWorld->draw.vertexDataSize1 > 0 && gfxWorld->draw.vd1.data)
    {
        auto vertexData1File = context.OpenAssetFile("bsp/gfxworld/draw_vertexData1");
        if (vertexData1File)
        {
            auto& vertexData1Stream = *vertexData1File;
            vertexData1Stream.write(gfxWorld->draw.vd1.data, gfxWorld->draw.vertexDataSize1);
        }
    }

    j["draw"]["indexCount"] = gfxWorld->draw.indexCount;
    j["draw"]["indices_Filename"] = "bsp/gfxworld/draw_indices";
    if (gfxWorld->draw.indexCount > 0 && gfxWorld->draw.indices)
    {
        auto indicesFile = context.OpenAssetFile("bsp/gfxworld/draw_indices");
        if (indicesFile)
        {
            auto& indicesStream = *indicesFile;
            indicesStream.write(
                reinterpret_cast<const char*>(gfxWorld->draw.indices), 
                gfxWorld->draw.indexCount * sizeof(uint16_t)
            );
        }
    }

    j["lightGrid"]["sunPrimaryLightIndex"] = gfxWorld->lightGrid.sunPrimaryLightIndex;
    j["lightGrid"]["mins"] = {
        {"0", gfxWorld->lightGrid.mins[0]},
        {"1", gfxWorld->lightGrid.mins[1]},
        {"2", gfxWorld->lightGrid.mins[2]},
    };
    j["lightGrid"]["maxs"] = {
        {"0", gfxWorld->lightGrid.maxs[0]},
        {"1", gfxWorld->lightGrid.maxs[1]},
        {"2", gfxWorld->lightGrid.maxs[2]},
    };
    j["lightGrid"]["offset"] = gfxWorld->lightGrid.offset;
    j["lightGrid"]["rowAxis"] = gfxWorld->lightGrid.rowAxis;
    j["lightGrid"]["colAxis"] = gfxWorld->lightGrid.colAxis;

    j["lightGrid"]["rowDataStart_Filename"] = "bsp/gfxworld/lightGrid_rowDataStart";
    if (gfxWorld->lightGrid.rowDataStart)
    {
        auto rowDataStartFile = context.OpenAssetFile("bsp/gfxworld/lightGrid_rowDataStart");
        if (rowDataStartFile)
        {
            auto& rowDataStartStream = *rowDataStartFile;
            auto writeCount = gfxWorld->lightGrid.maxs[gfxWorld->lightGrid.rowAxis] - gfxWorld->lightGrid.mins[gfxWorld->lightGrid.rowAxis] + 1;
            rowDataStartStream.write(reinterpret_cast<const char*>(gfxWorld->lightGrid.rowDataStart), writeCount * sizeof(uint16_t));
        }
    }

    j["lightGrid"]["rawRowDataSize"] = gfxWorld->lightGrid.rawRowDataSize;
    j["lightGrid"]["rawRowData_Filename"] = "bsp/gfxworld/lightGrid_rawRowData";
    if (gfxWorld->lightGrid.rawRowDataSize > 0 && gfxWorld->lightGrid.rawRowData)
    {
        auto rawRowDataFile = context.OpenAssetFile("bsp/gfxworld/lightGrid_rawRowData");
        if (rawRowDataFile)
        {
            auto& rawRowDataStream = *rawRowDataFile;
            rawRowDataStream.write(
                gfxWorld->lightGrid.rawRowData, 
                gfxWorld->lightGrid.rawRowDataSize
            );
        }
    }

    j["lightGrid"]["entryCount"] = gfxWorld->lightGrid.entryCount;
    j["lightGrid"]["entries_Filename"] = "bsp/gfxworld/lightGrid_entries";
    if (gfxWorld->lightGrid.entryCount > 0 && gfxWorld->lightGrid.entries)
    {
        auto entryCountFile = context.OpenAssetFile("bsp/gfxworld/lightGrid_entries");
        if (entryCountFile)
        {
            auto& entryCountStream = *entryCountFile;
            entryCountStream.write(
                reinterpret_cast<const char*>(gfxWorld->lightGrid.entries), 
                gfxWorld->lightGrid.entryCount * sizeof(GfxLightGridEntry)
            );
        }
    }
    

    j["lightGrid"]["colorCount"] = gfxWorld->lightGrid.colorCount;
    j["lightGrid"]["colors_Filename"] = "bsp/gfxworld/lightGrid_colors";
    if (gfxWorld->lightGrid.colorCount > 0 && gfxWorld->lightGrid.colors)
    {
        auto colorFile = context.OpenAssetFile("bsp/gfxworld/lightGrid_colors");
        if (colorFile)
        {
            auto& colorStream = *colorFile;
            colorStream.write(
                reinterpret_cast<const char*>(gfxWorld->lightGrid.colors), 
                gfxWorld->lightGrid.colorCount * sizeof(GfxCompressedLightGridColors)
            );
        }
    }

    j["lightGrid"]["coeffCount"] = gfxWorld->lightGrid.coeffCount;
    j["lightGrid"]["coeffs_Filename"] = "bsp/gfxworld/lightGrid_coeffs";
    if (gfxWorld->lightGrid.coeffCount > 0 && gfxWorld->lightGrid.coeffs)
    {
        auto coeffsFile = context.OpenAssetFile("bsp/gfxworld/lightGrid_coeffs");
        if (coeffsFile)
        {
            auto& coeffsStream = *coeffsFile;
            coeffsStream.write(
                reinterpret_cast<const char*>(gfxWorld->lightGrid.coeffs),
                gfxWorld->lightGrid.coeffCount * sizeof(GfxCompressedLightGridCoeffs_align4)
            );
        }
    }

    auto skyGridVolumes = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->lightGrid.skyGridVolumeCount; i++)
    {
        auto currentskyGridVolume = gfxWorld->lightGrid.skyGridVolumes[i];
        skyGridVolumes.push_back({
            {"mins",
                {
                    {"x", currentskyGridVolume.mins.x},
                    {"y", currentskyGridVolume.mins.y},
                    {"z", currentskyGridVolume.mins.z},
                }},
            {"maxs",
                {
                    {"x", currentskyGridVolume.maxs.x},
                    {"y", currentskyGridVolume.maxs.y},
                    {"z", currentskyGridVolume.maxs.z},
                }},
            {"lightingOrigin",
                {
                    {"x", currentskyGridVolume.lightingOrigin.x},
                    {"y", currentskyGridVolume.lightingOrigin.y},
                    {"z", currentskyGridVolume.lightingOrigin.z},
                }},
            {"colorsIndex",       currentskyGridVolume.colorsIndex},
            {"primaryLightIndex", currentskyGridVolume.primaryLightIndex},
            {"visibility",        currentskyGridVolume.visibility       }
        });
    }
    j["lightGrid"]["skyGridVolumeCount"] = gfxWorld->lightGrid.skyGridVolumeCount;
    j["lightGrid"]["skyGridVolumes"] = skyGridVolumes;

    auto models = nlohmann::json::array();
    for (int i = 0; i < gfxWorld->modelCount; i++)
    {
        auto currentModel = gfxWorld->models[i];
        models.push_back({ 
            {"writable",
             {{"mins", {{"x", currentModel.writable.mins.x}, {"y", currentModel.writable.mins.y}, {"z", currentModel.writable.mins.z}}},
                {"padding1", currentModel.writable.padding1},
              {"maxs", {{"x", currentModel.writable.maxs.x}, {"y", currentModel.writable.maxs.y}, {"z", currentModel.writable.maxs.z}}},
                {"padding2", currentModel.writable.padding2}}
            },
            {"bounds", {
                {"0", {{"x", currentModel.bounds[0].x}, {"y", currentModel.bounds[0].y}, {"z", currentModel.bounds[0].z}}},
                {"1", {{"x", currentModel.bounds[1].x}, {"y", currentModel.bounds[1].y}, {"z", currentModel.bounds[1].z}}}}},
            {"surfaceCount", currentModel.surfaceCount},
            {"startSurfIndex", currentModel.startSurfIndex}
        });
    }
    j["modelCount"] = gfxWorld->modelCount;
    j["models"] = models;

    j["mins"] = {
        {"x", gfxWorld->mins.x}, 
        {"y", gfxWorld->mins.y}, 
        {"z", gfxWorld->mins.z}
    };
    j["maxs"] = {
        {"x", gfxWorld->maxs.x},
        {"y", gfxWorld->maxs.y},
        {"z", gfxWorld->maxs.z}
    };
    j["checksum"] = gfxWorld->checksum;

    auto materialMemory = nlohmann::json::array();
    for (int i = 0; i < gfxWorld->materialMemoryCount; i++)
    {
        auto currentMaterialMemory = gfxWorld->materialMemory[i];

        const char* matName;
        if (currentMaterialMemory.material == NULL)
            matName = "";
        else
            matName = gfx_sanitiseJsonString(currentMaterialMemory.material->info.name);

        materialMemory.push_back({
            {"material", matName                     },
            {"memory",   currentMaterialMemory.memory},
        });
    }
    j["materialMemoryCount"] = gfxWorld->materialMemoryCount;
    j["materialMemory"] = materialMemory;

    j["sun"]["hasValidData"] = gfxWorld->sun.hasValidData;
    j["sun"]["spriteSize"] = gfxWorld->sun.spriteSize;
    j["sun"]["flareMinSize"] = gfxWorld->sun.flareMinSize;
    j["sun"]["flareMinDot"] = gfxWorld->sun.flareMinDot;
    j["sun"]["flareMaxSize"] = gfxWorld->sun.flareMaxSize;
    j["sun"]["flareMaxDot"] = gfxWorld->sun.flareMaxDot;
    j["sun"]["flareMaxAlpha"] = gfxWorld->sun.flareMaxAlpha;
    j["sun"]["flareFadeInTime"] = gfxWorld->sun.flareFadeInTime;
    j["sun"]["flareFadeOutTime"] = gfxWorld->sun.flareFadeOutTime;
    j["sun"]["blindMinDot"] = gfxWorld->sun.blindMinDot;
    j["sun"]["blindMaxDot"] = gfxWorld->sun.blindMaxDot;
    j["sun"]["blindMaxDarken"] = gfxWorld->sun.blindMaxDarken;
    j["sun"]["blindFadeInTime"] = gfxWorld->sun.blindFadeInTime;
    j["sun"]["blindFadeOutTime"] = gfxWorld->sun.blindFadeOutTime;
    j["sun"]["glareMinDot"] = gfxWorld->sun.glareMinDot;
    j["sun"]["glareMaxDot"] = gfxWorld->sun.glareMaxDot;
    j["sun"]["glareMaxLighten"] = gfxWorld->sun.glareMaxLighten;
    j["sun"]["glareFadeInTime"] = gfxWorld->sun.glareFadeInTime;
    j["sun"]["glareFadeOutTime"] = gfxWorld->sun.glareFadeOutTime;
    j["sun"]["sunFxPosition"] = {
        {"x", gfxWorld->sun.sunFxPosition.x},
        {"y", gfxWorld->sun.sunFxPosition.y},
        {"z", gfxWorld->sun.sunFxPosition.z}
    };
    if (gfxWorld->sun.spriteMaterial == NULL)
        j["sun"]["spriteMaterial"] = "";
    else
        j["sun"]["spriteMaterial"] = gfx_sanitiseJsonString(gfxWorld->sun.spriteMaterial->info.name);
    if (gfxWorld->sun.flareMaterial == NULL)
        j["sun"]["flareMaterial"] = "";
    else
        j["sun"]["flareMaterial"] = gfx_sanitiseJsonString(gfxWorld->sun.flareMaterial->info.name);

    j["outdoorLookupMatrix"] = {
        {"0",
            {
                {"x", gfxWorld->outdoorLookupMatrix[0].x},
                {"y", gfxWorld->outdoorLookupMatrix[0].y},
                {"z", gfxWorld->outdoorLookupMatrix[0].z},
                {"w", gfxWorld->outdoorLookupMatrix[0].w},
            }
        },
        {"1",
            {
                {"x", gfxWorld->outdoorLookupMatrix[1].x},
                {"y", gfxWorld->outdoorLookupMatrix[1].y},
                {"z", gfxWorld->outdoorLookupMatrix[1].z},
                {"w", gfxWorld->outdoorLookupMatrix[1].w},
            }
        },
        {"2",
            {
                {"x", gfxWorld->outdoorLookupMatrix[2].x},
                {"y", gfxWorld->outdoorLookupMatrix[2].y},
                {"z", gfxWorld->outdoorLookupMatrix[2].z},
                {"w", gfxWorld->outdoorLookupMatrix[2].w},
            }
        },
        {"3",
            {
                {"x", gfxWorld->outdoorLookupMatrix[3].x},
                {"y", gfxWorld->outdoorLookupMatrix[3].y},
                {"z", gfxWorld->outdoorLookupMatrix[3].z},
                {"w", gfxWorld->outdoorLookupMatrix[3].w},
            }
        }
    };

    if (gfxWorld->outdoorImage == NULL)
        j["outdoorImage"] = "";
    else
        j["outdoorImage"] = gfx_sanitiseJsonString(gfxWorld->outdoorImage->name);

    auto cellCasterBits = nlohmann::json::array();
    for (int i = 0; i < gfxWorld->dpvsPlanes.cellCount * ((gfxWorld->dpvsPlanes.cellCount + 31) / 32); i++)
    {
        cellCasterBits.push_back(gfxWorld->cellCasterBits[i]);
    }
    j["cellCasterBits"] = cellCasterBits;


    auto sceneDynModel = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->dpvsDyn.dynEntClientCount[0]; i++)
    {
        auto currentsceneDynModel = gfxWorld->sceneDynModel[i];

        sceneDynModel.push_back({
            {"info_packed",currentsceneDynModel.info.packed},
            {"dynEntId", currentsceneDynModel.dynEntId},
            {"primaryLightIndex", currentsceneDynModel.primaryLightIndex},
            {"reflectionProbeIndex", currentsceneDynModel.reflectionProbeIndex},
        });
    }
    j["sceneDynModel"] = sceneDynModel;

    auto sceneDynBrush = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->dpvsDyn.dynEntClientCount[1]; i++)
    {
        auto currentsceneDynBrush = gfxWorld->sceneDynBrush[i];

        sceneDynBrush.push_back({
            {"info_surfId", currentsceneDynBrush.info.surfId},
            {"dynEntId",    currentsceneDynBrush.dynEntId   },
        });
    }
    j["sceneDynBrush"] = sceneDynBrush;

    j["primaryLightEntityShadowVis_Filename"] = "bsp/gfxworld/primaryLightEntityShadowVis";
    auto eee = (gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1) * 8192;
    if ((gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1) * 8192 > 0 && gfxWorld->primaryLightEntityShadowVis)
    {
        auto primaryLightEntityShadowVisFile = context.OpenAssetFile("bsp/gfxworld/primaryLightEntityShadowVis");
        if (primaryLightEntityShadowVisFile)
        {
            auto& primaryLightEntityShadowVisStream = *primaryLightEntityShadowVisFile;
            primaryLightEntityShadowVisStream.write(
                reinterpret_cast<const char*>(gfxWorld->primaryLightEntityShadowVis),
                ((gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1) * 8192) * sizeof(unsigned int)
            );
        }
    }
    
    j["primaryLightDynEntShadowVis0_Filename"] = "bsp/gfxworld/primaryLightDynEntShadowVis0";
    if (gfxWorld->dpvsDyn.dynEntClientCount[0] * (gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1) > 0
        && gfxWorld->primaryLightDynEntShadowVis[0])
    {
        auto primaryLightDynEntShadowVis0File = context.OpenAssetFile("bsp/gfxworld/primaryLightDynEntShadowVis0");
        if (primaryLightDynEntShadowVis0File)
        {
            auto& primaryLightDynEntShadowVis0Stream = *primaryLightDynEntShadowVis0File;
            primaryLightDynEntShadowVis0Stream.write(
                reinterpret_cast<const char*>(gfxWorld->primaryLightDynEntShadowVis[0]),
                (gfxWorld->dpvsDyn.dynEntClientCount[0] * (gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1)) * sizeof(unsigned int)
            );
        }
    }

    j["primaryLightDynEntShadowVis1_Filename"] = "bsp/gfxworld/primaryLightDynEntShadowVis1";
    if (gfxWorld->dpvsDyn.dynEntClientCount[1] * (gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1) > 0
        && gfxWorld->primaryLightDynEntShadowVis[1])
    {
        auto primaryLightDynEntShadowVis1File = context.OpenAssetFile("bsp/gfxworld/primaryLightDynEntShadowVis1");
        if (primaryLightDynEntShadowVis1File)
        {
            auto& primaryLightDynEntShadowVis1Stream = *primaryLightDynEntShadowVis1File;
            primaryLightDynEntShadowVis1Stream.write(
                reinterpret_cast<const char*>(gfxWorld->primaryLightDynEntShadowVis[1]),
                (gfxWorld->dpvsDyn.dynEntClientCount[1] * (gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1)) * sizeof(unsigned int));
        }
    }

    // TODO (is it even used in the game?) ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    assert(gfxWorld->numSiegeSkinInsts == 0);
    auto siegeSkinInsts = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->numSiegeSkinInsts; i++)
    {
        ;
    }
    //j["numSiegeSkinInsts"] = gfxWorld->numSiegeSkinInsts;
    j["numSiegeSkinInsts"] = 0;
    j["siegeSkinInsts"] = siegeSkinInsts;


    auto shadowGeom = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->primaryLightCount; i++)
    {
        auto currentShadowGeom = gfxWorld->shadowGeom[i];

        auto sortedSurfIndex = nlohmann::json::array();
        for (unsigned int k = 0; k < currentShadowGeom.surfaceCount; k++)
        {
            sortedSurfIndex.push_back(currentShadowGeom.sortedSurfIndex[k]);
        }

        auto smodelIndex = nlohmann::json::array();
        for (unsigned int k = 0; k < currentShadowGeom.smodelCount; k++)
        {
            smodelIndex.push_back(currentShadowGeom.smodelIndex[k]);
        }

        shadowGeom.push_back({
            {"surfaceCount", currentShadowGeom.surfaceCount},
            {"smodelCount",  currentShadowGeom.smodelCount},
            {"sortedSurfIndex",  sortedSurfIndex               },
            {"smodelIndex",  smodelIndex                   },
            });
    }
    j["shadowGeom"] = shadowGeom;

    auto lightRegion = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->primaryLightCount; i++)
    {
        auto currentLightRegion = gfxWorld->lightRegion[i];

        auto hulls = nlohmann::json::array();
        for (unsigned int k = 0; k < currentLightRegion.hullCount; k++)
        {
            auto currentHull = currentLightRegion.hulls[k];

            auto axisArray = nlohmann::json::array();
            for (unsigned int l = 0; l < currentHull.axisCount; l++)
            {
                auto currentAxis = currentHull.axis[l];

                axisArray.push_back({
                    {"dir", {{"x", currentAxis.dir.x}, {"y", currentAxis.dir.y}, {"z", currentAxis.dir.z}}},
                    {"midPoint", currentAxis.midPoint},
                    {"halfSize", currentAxis.halfSize}
                });
            }

            hulls.push_back({
                {"kdopMidPoint", 
                   {{"0", currentHull.kdopMidPoint[0]},
                    {"1", currentHull.kdopMidPoint[1]},
                    {"2", currentHull.kdopMidPoint[2]},
                    {"3", currentHull.kdopMidPoint[3]},
                    {"4", currentHull.kdopMidPoint[4]},
                    {"5", currentHull.kdopMidPoint[5]},
                    {"6", currentHull.kdopMidPoint[6]},
                    {"7", currentHull.kdopMidPoint[7]},
                    {"8", currentHull.kdopMidPoint[8]}}
                },
                {"kdopHalfSize",
                   {{"0", currentHull.kdopHalfSize[0]},
                    {"1", currentHull.kdopHalfSize[1]},
                    {"2", currentHull.kdopHalfSize[2]},
                    {"3", currentHull.kdopHalfSize[3]},
                    {"4", currentHull.kdopHalfSize[4]},
                    {"5", currentHull.kdopHalfSize[5]},
                    {"6", currentHull.kdopHalfSize[6]},
                    {"7", currentHull.kdopHalfSize[7]},
                    {"8", currentHull.kdopHalfSize[8]}}
                },
                {"axisCount", currentHull.axisCount},
                {"axis",         axisArray            },
                });
        }

        lightRegion.push_back({
            {"hullCount", currentLightRegion.hullCount},
            {"hulls", hulls                       },
            });
    }
    j["lightRegion"] = lightRegion;

    j["dpvs"]["smodelCount"] = gfxWorld->dpvs.smodelCount;
    j["dpvs"]["staticSurfaceCount"] = gfxWorld->dpvs.staticSurfaceCount;
    j["dpvs"]["litSurfsBegin"] = gfxWorld->dpvs.litSurfsBegin;
    j["dpvs"]["litSurfsEnd"] = gfxWorld->dpvs.litSurfsEnd;
    j["dpvs"]["litTransSurfsBegin"] = gfxWorld->dpvs.litTransSurfsBegin;
    j["dpvs"]["litTransSurfsEnd"] = gfxWorld->dpvs.litTransSurfsEnd;
    j["dpvs"]["emissiveOpaqueSurfsBegin"] = gfxWorld->dpvs.emissiveOpaqueSurfsBegin;
    j["dpvs"]["emissiveOpaqueSurfsEnd"] = gfxWorld->dpvs.emissiveOpaqueSurfsEnd;
    j["dpvs"]["emissiveTransSurfsBegin"] = gfxWorld->dpvs.emissiveTransSurfsBegin;
    j["dpvs"]["emissiveTransSurfsEnd"] = gfxWorld->dpvs.emissiveTransSurfsEnd;
    j["dpvs"]["smodelVisDataCount"] = gfxWorld->dpvs.smodelVisDataCount;
    j["dpvs"]["surfaceVisDataCount"] = gfxWorld->dpvs.surfaceVisDataCount;
    j["dpvs"]["usageCount"] = gfxWorld->dpvs.usageCount;

    j["dpvs"]["smodelVisData0_Filename"] = "bsp/gfxworld/dpvs_smodelVisData0";
    j["dpvs"]["smodelVisData1_Filename"] = "bsp/gfxworld/dpvs_smodelVisData1";
    j["dpvs"]["smodelVisData2_Filename"] = "bsp/gfxworld/dpvs_smodelVisData2";
    j["dpvs"]["smodelVisDataCameraSaved_Filename"] = "bsp/gfxworld/dpvs_smodelVisDataCameraSaved";
    j["dpvs"]["smodelCastsShadow_Filename"] = "bsp/gfxworld/dpvs_smodelCastsShadow";
    if (gfxWorld->dpvs.smodelVisDataCount > 0)
    {
        auto smodelVisData0File = context.OpenAssetFile("bsp/gfxworld/dpvs_smodelVisData0");
        if (smodelVisData0File)
        {
            auto& smodelVisData0Stream = *smodelVisData0File;
            smodelVisData0Stream.write(gfxWorld->dpvs.smodelVisData[0], gfxWorld->dpvs.smodelVisDataCount);
        }

        auto smodelVisData1File = context.OpenAssetFile("bsp/gfxworld/dpvs_smodelVisData1");
        if (smodelVisData1File)
        {
            auto& smodelVisData1Stream = *smodelVisData1File;
            smodelVisData1Stream.write(gfxWorld->dpvs.smodelVisData[1], gfxWorld->dpvs.smodelVisDataCount);
        }

        auto smodelVisData2File = context.OpenAssetFile("bsp/gfxworld/dpvs_smodelVisData2");
        if (smodelVisData2File)
        {
            auto& smodelVisData2Stream = *smodelVisData2File;
            smodelVisData2Stream.write(gfxWorld->dpvs.smodelVisData[2], gfxWorld->dpvs.smodelVisDataCount);
        }
        auto cameraSavedFile = context.OpenAssetFile("bsp/gfxworld/dpvs_smodelVisDataCameraSaved");
        if (cameraSavedFile)
        {
            auto& cameraSavedStream = *cameraSavedFile;
            cameraSavedStream.write(gfxWorld->dpvs.smodelVisDataCameraSaved, gfxWorld->dpvs.smodelVisDataCount);
        }
        auto smodelCastsShadowFile = context.OpenAssetFile("bsp/gfxworld/dpvs_smodelCastsShadow");
        if (smodelCastsShadowFile)
        {
            auto& smodelCastsShadowStream = *smodelCastsShadowFile;
            smodelCastsShadowStream.write(gfxWorld->dpvs.smodelCastsShadow, gfxWorld->dpvs.smodelVisDataCount);
        }
    }

    j["dpvs"]["surfaceVisData0_Filename"] = "bsp/gfxworld/dpvs_surfaceVisData0";
    j["dpvs"]["surfaceVisData1_Filename"] = "bsp/gfxworld/dpvs_surfaceVisData1";
    j["dpvs"]["surfaceVisData2_Filename"] = "bsp/gfxworld/dpvs_surfaceVisData2";
    j["dpvs"]["surfaceVisDataCameraSaved_Filename"] = "bsp/gfxworld/dpvs_surfaceVisDataCameraSaved";
    j["dpvs"]["surfaceCastsSunShadow_Filename"] = "bsp/gfxworld/dpvs_surfaceCastsSunShadow";
    j["dpvs"]["surfaceCastsShadow_Filename"] = "bsp/gfxworld/dpvs_surfaceCastsShadow";
    if (gfxWorld->dpvs.surfaceVisDataCount > 0)
    {
        auto surfaceVisData0File = context.OpenAssetFile("bsp/gfxworld/dpvs_surfaceVisData0");
        if (surfaceVisData0File)
        {
            auto& surfaceVisData0Stream = *surfaceVisData0File;
            surfaceVisData0Stream.write(gfxWorld->dpvs.surfaceVisData[0], gfxWorld->dpvs.surfaceVisDataCount);
        }

        auto surfaceVisData1File = context.OpenAssetFile("bsp/gfxworld/dpvs_surfaceVisData1");
        if (surfaceVisData1File)
        {
            auto& surfaceVisData1Stream = *surfaceVisData1File;
            surfaceVisData1Stream.write(gfxWorld->dpvs.surfaceVisData[1], gfxWorld->dpvs.surfaceVisDataCount);
        }

        auto surfaceVisData2File = context.OpenAssetFile("bsp/gfxworld/dpvs_surfaceVisData2");
        if (surfaceVisData2File)
        {
            auto& surfaceVisData2Stream = *surfaceVisData2File;
            surfaceVisData2Stream.write(gfxWorld->dpvs.surfaceVisData[2], gfxWorld->dpvs.surfaceVisDataCount);
        }

        auto surfaceVisDataCameraSavedFile = context.OpenAssetFile("bsp/gfxworld/dpvs_surfaceVisDataCameraSaved");
        if (surfaceVisDataCameraSavedFile)
        {
            auto& surfaceVisDataCameraSavedStream = *surfaceVisDataCameraSavedFile;
            surfaceVisDataCameraSavedStream.write(gfxWorld->dpvs.surfaceVisDataCameraSaved, gfxWorld->dpvs.surfaceVisDataCount);
        }

        auto surfaceCastsSunShadowFile = context.OpenAssetFile("bsp/gfxworld/dpvs_surfaceCastsSunShadow");
        if (surfaceCastsSunShadowFile)
        {
            auto& surfaceCastsSunShadowStream = *surfaceCastsSunShadowFile;
            surfaceCastsSunShadowStream.write(gfxWorld->dpvs.surfaceCastsSunShadow, gfxWorld->dpvs.surfaceVisDataCount);
        }

        auto surfaceCastsShadowFile = context.OpenAssetFile("bsp/gfxworld/dpvs_surfaceCastsShadow");
        if (surfaceCastsShadowFile)
        {
            auto& surfaceCastsShadowStream = *surfaceCastsShadowFile;
            surfaceCastsShadowStream.write(gfxWorld->dpvs.surfaceCastsShadow, gfxWorld->dpvs.surfaceVisDataCount);
        }
    }

    j["dpvs"]["sortedSurfIndex_Filename"] = "bsp/gfxworld/dpvs_sortedSurfIndex";
    if (gfxWorld->dpvs.staticSurfaceCount > 0)
    {
        auto sortedSurfIndexFile = context.OpenAssetFile("bsp/gfxworld/dpvs_sortedSurfIndex");
        if (sortedSurfIndexFile)
        {
            auto& sortedSurfIndexStream = *sortedSurfIndexFile;
            sortedSurfIndexStream.write(
                reinterpret_cast<const char*>(gfxWorld->dpvs.sortedSurfIndex), 
                gfxWorld->dpvs.staticSurfaceCount * sizeof(uint16_t)
            );
        }
    }
    auto smodelInsts = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->dpvs.smodelCount; i++)
    {
        auto currentSmodelInsts = gfxWorld->dpvs.smodelInsts[i];

        smodelInsts.push_back({
            {"mins", {{"x", currentSmodelInsts.mins.x}, {"y", currentSmodelInsts.mins.y}, {"z", currentSmodelInsts.mins.z}}      },
            {"maxs", {{"x", currentSmodelInsts.maxs.x}, {"y", currentSmodelInsts.maxs.y}, {"z", currentSmodelInsts.maxs.z}}},
            {"lightingOrigin", {{"x", currentSmodelInsts.lightingOrigin.x}, {"y", currentSmodelInsts.lightingOrigin.y}, {"z", currentSmodelInsts.lightingOrigin.z}}},
        });
    }
    j["dpvs"]["smodelInsts"] = smodelInsts;

    auto surfaces = nlohmann::json::array();
    for (int i = 0; i < gfxWorld->surfaceCount; i++)
    {
        auto currentSurface = gfxWorld->dpvs.surfaces[i];

        const char* materialName;
        if (currentSurface.material == NULL)
            materialName = "";
        else
            materialName = gfx_sanitiseJsonString(currentSurface.material->info.name);

        surfaces.push_back({
            {"tris", {
                 {"mins", {{"x", currentSurface.tris.mins.x}, {"y", currentSurface.tris.mins.y}, {"z", currentSurface.tris.mins.z}}},
                 {"maxs", {{"x", currentSurface.tris.maxs.x}, {"y", currentSurface.tris.maxs.y}, {"z", currentSurface.tris.maxs.z}}},
                 {"vertexDataOffset0", currentSurface.tris.vertexDataOffset0},
                 {"vertexDataOffset1", currentSurface.tris.vertexDataOffset1},
                 {"firstVertex", currentSurface.tris.firstVertex},
                 {"himipRadiusInvSq", currentSurface.tris.himipRadiusInvSq},
                 {"vertexCount", currentSurface.tris.vertexCount},
                 {"triCount", currentSurface.tris.triCount},
                 {"baseIndex", currentSurface.tris.baseIndex}}},
            {"material",             materialName                                                                                             },
            {"lightmapIndex", currentSurface.lightmapIndex},
            {"reflectionProbeIndex", currentSurface.reflectionProbeIndex                                                                      },
            {"primaryLightIndex",    currentSurface.primaryLightIndex                                                                         },
            {"flags",                currentSurface.flags                                                                                     },
            {"bounds0", {{"x", currentSurface.bounds[0].x}, {"y", currentSurface.bounds[0].y}, {"z", currentSurface.bounds[0].z}}},
            {"bounds1", {{"x", currentSurface.bounds[1].x}, {"y", currentSurface.bounds[1].y}, {"z", currentSurface.bounds[1].z}}},
        });
    }
    j["dpvs"]["surfaces"] = surfaces;

    auto surfaceMaterials = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->dpvs.staticSurfaceCount; i++)
    {
        auto currentSurfaceMaterials = gfxWorld->dpvs.surfaceMaterials[i];

        surfaceMaterials.push_back(gfxWorld->dpvs.surfaceMaterials[i].packed);
    }
    j["dpvs"]["surfaceMaterials"] = surfaceMaterials;

    auto smodelDrawInsts = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->dpvs.smodelCount; i++)
    {
        auto currentDrawInsts = gfxWorld->dpvs.smodelDrawInsts[i];

        const char* modelName;
        if (currentDrawInsts.model == NULL)
            modelName = "";
        else
            modelName = gfx_sanitiseJsonString(currentDrawInsts.model->name);

        auto lmapVertexColors0 = nlohmann::json::array();
        auto lmapVertexColors1 = nlohmann::json::array();
        auto lmapVertexColors2 = nlohmann::json::array();
        auto lmapVertexColors3 = nlohmann::json::array();
        for (int i = 0; i < currentDrawInsts.lmapVertexInfo[0].numLmapVertexColors; i++)
            lmapVertexColors0.push_back(currentDrawInsts.lmapVertexInfo[0].lmapVertexColors[i]);
        for (int i = 0; i < currentDrawInsts.lmapVertexInfo[1].numLmapVertexColors; i++)
            lmapVertexColors1.push_back(currentDrawInsts.lmapVertexInfo[1].lmapVertexColors[i]);
        for (int i = 0; i < currentDrawInsts.lmapVertexInfo[2].numLmapVertexColors; i++)
            lmapVertexColors2.push_back(currentDrawInsts.lmapVertexInfo[2].lmapVertexColors[i]);
        for (int i = 0; i < currentDrawInsts.lmapVertexInfo[3].numLmapVertexColors; i++)
            lmapVertexColors3.push_back(currentDrawInsts.lmapVertexInfo[3].lmapVertexColors[i]);


        smodelDrawInsts.push_back({
            {"cullDist", currentDrawInsts.cullDist},
            {"placement", {
                {"origin", {{"x", currentDrawInsts.placement.origin.x}, {"y", currentDrawInsts.placement.origin.y}, {"z", currentDrawInsts.placement.origin.z}}},
                {"axis0", {{"x", currentDrawInsts.placement.axis[0].x}, {"y", currentDrawInsts.placement.axis[0].y}, {"z", currentDrawInsts.placement.axis[0].z}}},
                {"axis1", {{"x", currentDrawInsts.placement.axis[1].x}, {"y", currentDrawInsts.placement.axis[1].y}, {"z", currentDrawInsts.placement.axis[1].z}}},
                {"axis2", {{"x", currentDrawInsts.placement.axis[2].x}, {"y", currentDrawInsts.placement.axis[2].y}, {"z", currentDrawInsts.placement.axis[2].z}}},
                {"scale", currentDrawInsts.placement.scale}}},
            {"model", modelName},
            {"flags", currentDrawInsts.flags},
            {"invScaleSq", currentDrawInsts.invScaleSq},
            {"lightingHandle", currentDrawInsts.lightingHandle},
            {"colorsIndex", currentDrawInsts.colorsIndex},
            {"lightingSH", {
                {"V00", currentDrawInsts.lightingSH.V0[0]},
                {"V01", currentDrawInsts.lightingSH.V0[1]},
                {"V02", currentDrawInsts.lightingSH.V0[2]},
                {"V03", currentDrawInsts.lightingSH.V0[3]},
                {"V10", currentDrawInsts.lightingSH.V1[0]},
                {"V11", currentDrawInsts.lightingSH.V1[1]},
                {"V12", currentDrawInsts.lightingSH.V1[2]},
                {"V13", currentDrawInsts.lightingSH.V1[3]},
                {"V20", currentDrawInsts.lightingSH.V2[0]},
                {"V21", currentDrawInsts.lightingSH.V2[1]},
                {"V22", currentDrawInsts.lightingSH.V2[2]},
                {"V23", currentDrawInsts.lightingSH.V2[3]}}},
            {"primaryLightIndex", currentDrawInsts.primaryLightIndex},
            {"visibility", currentDrawInsts.visibility},
            {"reflectionProbeIndex", currentDrawInsts.reflectionProbeIndex},
            {"smid", currentDrawInsts.smid},
            {"lmapVertexInfo0", {
                {"numLmapVertexColors", currentDrawInsts.lmapVertexInfo[0].numLmapVertexColors},
                {"lmapVertexColors", lmapVertexColors0}}
            },
            {"lmapVertexInfo1", {
                {"numLmapVertexColors", currentDrawInsts.lmapVertexInfo[1].numLmapVertexColors},
                {"lmapVertexColors", lmapVertexColors1}}
            },
            {"lmapVertexInfo2", {
                {"numLmapVertexColors", currentDrawInsts.lmapVertexInfo[2].numLmapVertexColors},
                {"lmapVertexColors", lmapVertexColors2}}
            },
            {"lmapVertexInfo3", {
                {"numLmapVertexColors", currentDrawInsts.lmapVertexInfo[3].numLmapVertexColors},
                {"lmapVertexColors", lmapVertexColors3}}
            },
        });
    }
    j["dpvs"]["smodelDrawInsts"] = smodelDrawInsts;


    j["dpvsDyn"]["dynEntClientWordCount0"] = gfxWorld->dpvsDyn.dynEntClientWordCount[0];
    j["dpvsDyn"]["dynEntClientWordCount1"] = gfxWorld->dpvsDyn.dynEntClientWordCount[1];
    j["dpvsDyn"]["dynEntClientCount0"] = gfxWorld->dpvsDyn.dynEntClientCount[0];
    j["dpvsDyn"]["dynEntClientCount1"] = gfxWorld->dpvsDyn.dynEntClientCount[1];
    j["dpvsDyn"]["usageCount"] = gfxWorld->dpvsDyn.usageCount;

    j["dpvsDyn"]["dynEntCellBits0_Filename"] = "bsp/gfxworld/dpvsDyn_dynEntCellBits0";
    if (gfxWorld->dpvsDyn.dynEntClientWordCount[0] * gfxWorld->dpvsPlanes.cellCount > 0)
    {
        auto dynEntCellBits0File = context.OpenAssetFile("bsp/gfxworld/dpvsDyn_dynEntCellBits0");
        if (dynEntCellBits0File)
        {
            auto& dynEntCellBits0Stream = *dynEntCellBits0File;
            dynEntCellBits0Stream.write(
                reinterpret_cast<const char*>(gfxWorld->dpvsDyn.dynEntCellBits[0]), 
                gfxWorld->dpvsDyn.dynEntClientWordCount[0] * gfxWorld->dpvsPlanes.cellCount * sizeof(unsigned int)
            );
        }
    }

    j["dpvsDyn"]["dynEntCellBits1_Filename"] = "bsp/gfxworld/dpvsDyn_dynEntCellBits1";
    if (gfxWorld->dpvsDyn.dynEntClientWordCount[1] * gfxWorld->dpvsPlanes.cellCount > 0)
    {
        auto dynEntCellBits1File = context.OpenAssetFile("bsp/gfxworld/dpvsDyn_dynEntCellBits1");
        if (dynEntCellBits1File)
        {
            auto& dynEntCellBits1Stream = *dynEntCellBits1File;
            dynEntCellBits1Stream.write(
                reinterpret_cast<const char*>(gfxWorld->dpvsDyn.dynEntCellBits[1]),
                gfxWorld->dpvsDyn.dynEntClientWordCount[1] * gfxWorld->dpvsPlanes.cellCount * sizeof(unsigned int)
            );
        }
    }

    j["dpvsDyn"]["dynEntVisData00_Filename"] = "bsp/gfxworld/dpvsDyn_dynEntVisData00";
    j["dpvsDyn"]["dynEntVisData01_Filename"] = "bsp/gfxworld/dpvsDyn_dynEntVisData01";
    j["dpvsDyn"]["dynEntVisData02_Filename"] = "bsp/gfxworld/dpvsDyn_dynEntVisData02";
    if (gfxWorld->dpvsDyn.dynEntClientWordCount[0] * 32 > 0)
    {
        auto visData00File = context.OpenAssetFile("bsp/gfxworld/dpvsDyn_dynEntVisData00");
        if (visData00File)
        {
            auto& visData00Stream = *visData00File;
            visData00Stream.write(gfxWorld->dpvsDyn.dynEntVisData[0][0], gfxWorld->dpvsDyn.dynEntClientWordCount[0] * 32
            );
        }

        auto visData01File = context.OpenAssetFile("bsp/gfxworld/dpvsDyn_dynEntVisData01");
        if (visData01File)
        {
            auto& visData01Stream = *visData01File;
            visData01Stream.write(gfxWorld->dpvsDyn.dynEntVisData[0][1], gfxWorld->dpvsDyn.dynEntClientWordCount[0] * 32);
        }

        auto visData02File = context.OpenAssetFile("bsp/gfxworld/dpvsDyn_dynEntVisData02");
        if (visData02File)
        {
            auto& visData02Stream = *visData02File;
            visData02Stream.write(gfxWorld->dpvsDyn.dynEntVisData[0][2], gfxWorld->dpvsDyn.dynEntClientWordCount[0] * 32);
        }
    }

    j["dpvsDyn"]["dynEntVisData10_Filename"] = "bsp/gfxworld/dpvsDyn_dynEntVisData10";
    j["dpvsDyn"]["dynEntVisData11_Filename"] = "bsp/gfxworld/dpvsDyn_dynEntVisData11";
    j["dpvsDyn"]["dynEntVisData12_Filename"] = "bsp/gfxworld/dpvsDyn_dynEntVisData12";
    if (gfxWorld->dpvsDyn.dynEntClientWordCount[1] * 32 > 0)
    {
        auto visData10File = context.OpenAssetFile("bsp/gfxworld/dpvsDyn_dynEntVisData10");
        if (visData10File)
        {
            auto& visData10Stream = *visData10File;
            visData10Stream.write(gfxWorld->dpvsDyn.dynEntVisData[1][0], gfxWorld->dpvsDyn.dynEntClientWordCount[1] * 32);
        }

        auto visData11File = context.OpenAssetFile("bsp/gfxworld/dpvsDyn_dynEntVisData11");
        if (visData11File)
        {
            auto& visData11Stream = *visData11File;
            visData11Stream.write(gfxWorld->dpvsDyn.dynEntVisData[1][1], gfxWorld->dpvsDyn.dynEntClientWordCount[1] * 32);
        }

        auto visData12File = context.OpenAssetFile("bsp/gfxworld/dpvsDyn_dynEntVisData12");
        if (visData12File)
        {
            auto& visData12Stream = *visData12File;
            visData12Stream.write(gfxWorld->dpvsDyn.dynEntVisData[1][2], gfxWorld->dpvsDyn.dynEntClientWordCount[1] * 32);
        }
    }

    auto waterBuffer0 = nlohmann::json::array();
    auto waterBuffer1 = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->waterBuffers[0].bufferSize / 16; i++)
    {
        waterBuffer0.push_back({
            {"x", gfxWorld->waterBuffers[0].buffer[i].x},
            {"y", gfxWorld->waterBuffers[0].buffer[i].y},
            {"z", gfxWorld->waterBuffers[0].buffer[i].z},
            {"w", gfxWorld->waterBuffers[0].buffer[i].w},
        });
    }
    for (unsigned int i = 0; i < gfxWorld->waterBuffers[1].bufferSize / 16; i++)
    {
        waterBuffer1.push_back({
            {"x", gfxWorld->waterBuffers[1].buffer[i].x},
            {"y", gfxWorld->waterBuffers[1].buffer[i].y},
            {"z", gfxWorld->waterBuffers[1].buffer[i].z},
            {"w", gfxWorld->waterBuffers[1].buffer[i].w},
        });
    }
    j["waterBuffers"] = {
        {"0",
            {
                {"bufferSize", gfxWorld->waterBuffers[0].bufferSize},
                {"buffer", waterBuffer0},
            }
        },
        {"1",
            {
                {"bufferSize", gfxWorld->waterBuffers[1].bufferSize},
                {"buffer", waterBuffer1},
            },
        }
    };

    j["waterDirection"] = gfxWorld->waterDirection;

    if (gfxWorld->waterMaterial == NULL)
        j["waterMaterial"] = "";
    else
        j["waterMaterial"] = gfx_sanitiseJsonString(gfxWorld->waterMaterial->info.name);

    if (gfxWorld->coronaMaterial == NULL)
        j["coronaMaterial"] = "";
    else
        j["coronaMaterial"] = gfx_sanitiseJsonString(gfxWorld->coronaMaterial->info.name);

    if (gfxWorld->ropeMaterial == NULL)
        j["ropeMaterial"] = "";
    else
        j["ropeMaterial"] = gfx_sanitiseJsonString(gfxWorld->ropeMaterial->info.name);

    if (gfxWorld->lutMaterial == NULL)
        j["lutMaterial"] = "";
    else
        j["lutMaterial"] = gfx_sanitiseJsonString(gfxWorld->lutMaterial->info.name);

    auto occluders = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->numOccluders; i++)
    {
        auto currentOccluder = gfxWorld->occluders[i];
        occluders.push_back({
            {"flags", currentOccluder.flags},
            {"name",  currentOccluder.name },
            {"points", {
                 {"0", {{"x", currentOccluder.points[0].x}, {"y", currentOccluder.points[0].y}, {"z", currentOccluder.points[0].z}}},
                 {"1", {{"x", currentOccluder.points[1].x}, {"y", currentOccluder.points[1].y}, {"z", currentOccluder.points[1].z}}},
                 {"2", {{"x", currentOccluder.points[2].x}, {"y", currentOccluder.points[2].y}, {"z", currentOccluder.points[2].z}}},
                 {"3", {{"x", currentOccluder.points[3].x}, {"y", currentOccluder.points[3].y}, {"z", currentOccluder.points[3].z}}},
                }
            }
        });
    }
    j["numOccluders"] = gfxWorld->numOccluders;
    j["occluders"] = occluders;
    
    auto outdoorBounds = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->numOutdoorBounds; i++)
    {
        auto currentOutdoorBound = gfxWorld->outdoorBounds[i];
        outdoorBounds.push_back({
            {"bounds", {
                    {"0", {{"x", currentOutdoorBound.bounds[0].x}, {"y", currentOutdoorBound.bounds[0].y}, {"z", currentOutdoorBound.bounds[0].z}}},
                    {"1", {{"x", currentOutdoorBound.bounds[1].x}, {"y", currentOutdoorBound.bounds[1].y}, {"z", currentOutdoorBound.bounds[1].z}}}
                }
            }
        });
    }
    j["numOutdoorBounds"] = gfxWorld->numOutdoorBounds;
    j["outdoorBounds"] = outdoorBounds;

    auto heroLights = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->heroLightCount; i++)
    {
        auto currentHeroLight = gfxWorld->heroLights[i];
        heroLights.push_back({
            {"type",            currentHeroLight.type                                                               },
            {"unused",          {{"0", currentHeroLight.unused[0]}, {"1", currentHeroLight.unused[1]}, {"2", currentHeroLight.unused[2]}}},
            {"color",           {{"x", currentHeroLight.color.x}, {"y", currentHeroLight.color.y}, {"z", currentHeroLight.color.z}}},
            {"dir",             {{"x", currentHeroLight.dir.x}, {"y", currentHeroLight.dir.y}, {"z", currentHeroLight.dir.z}}      },
            {"origin",          {{"x", currentHeroLight.origin.x}, {"y", currentHeroLight.origin.y}, {"z", currentHeroLight.origin.z}}},
            {"radius",          currentHeroLight.radius                                                             },
            {"cosHalfFovOuter", currentHeroLight.cosHalfFovOuter                                                    },
            {"cosHalfFovInner", currentHeroLight.cosHalfFovInner                                                    },
            {"exponent",        currentHeroLight.exponent                                                           },
        });
    }
    j["heroLightCount"] = gfxWorld->heroLightCount;
    j["heroLights"] = heroLights;

    auto heroLightTree = nlohmann::json::array();
    for (unsigned int i = 0; i < gfxWorld->heroLightTreeCount; i++)
    {
        auto currentHeroLightTree = gfxWorld->heroLightTree[i];
        heroLightTree.push_back({
            {"mins",      {{"x", currentHeroLightTree.mins.x}, {"y", currentHeroLightTree.mins.y}, {"z", currentHeroLightTree.mins.z}}},
            {"maxs",      {{"x", currentHeroLightTree.maxs.x}, {"y", currentHeroLightTree.maxs.y}, {"z", currentHeroLightTree.maxs.z}}},
            {"leftNode",  currentHeroLightTree.leftNode                                                          },
            {"rightNode", currentHeroLightTree.rightNode                                                         }
        });
    }
    j["heroLightTreeCount"] = gfxWorld->heroLightTreeCount;
    j["heroLightTree"] = heroLightTree;

    j["lightingFlags"] = gfxWorld->lightingFlags;
    j["lightingQuality"] = gfxWorld->lightingQuality;

    std::string jsonString = j.dump(4);
    stream.write(jsonString.c_str(), jsonString.size());
}