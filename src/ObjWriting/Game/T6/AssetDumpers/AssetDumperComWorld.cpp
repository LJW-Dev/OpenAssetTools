#include <nlohmann/json.hpp>

#include "AssetDumperComWorld.h"

using namespace T6;

bool AssetDumperComWorld::ShouldDump(XAssetInfo<ComWorld>* asset)
{
    return true;
}

void AssetDumperComWorld::DumpAsset(AssetDumpingContext& context, XAssetInfo<ComWorld>* asset)
{
    const auto* comWorld = asset->Asset();
    const auto assetFile = context.OpenAssetFile("bsp/comworld.json");

    if (!assetFile)
        return;

    auto& stream = *assetFile;

    nlohmann::json j;
    j["name"] = comWorld->name;
    j["isInUse"] = comWorld->isInUse;
    

    auto lightArray = nlohmann::json::array();
    j["primaryLightCount"] = comWorld->primaryLightCount;
    for (unsigned int i = 0; i < comWorld->primaryLightCount; i++)
    {
        auto currentLight = comWorld->primaryLights[i];

        const char* defName = currentLight.defName;
        if (defName == NULL)
            defName = "";

        lightArray.push_back({
            {"type",               currentLight.type                                                                                                           },
            {"canUseShadowMap",    currentLight.canUseShadowMap                                                                                                },
            {"exponent",           currentLight.exponent                                                                                                       },
            {"priority",           currentLight.priority                                                                                                       },
            {"cullDist",           currentLight.cullDist                                                                                                       },
            {"useCookie",          currentLight.useCookie                                                                                                      },
            {"shadowmapVolume",    currentLight.shadowmapVolume                                                                                                },
            {"color",              {{"x", currentLight.color.x}, {"y", currentLight.color.y}, {"z", currentLight.color.z}}                                     },
            {"dir",                {{"x", currentLight.dir.x}, {"y", currentLight.dir.y}, {"z", currentLight.dir.z}}                                           },
            {"origin",             {{"x", currentLight.origin.x}, {"y", currentLight.origin.y}, {"z", currentLight.origin.z}}                                  },
            {"radius",             currentLight.radius                                                                                                         },
            {"cosHalfFovOuter",    currentLight.cosHalfFovOuter                                                                                                },
            {"cosHalfFovInner",    currentLight.cosHalfFovInner                                                                                                },
            {"cosHalfFovExpanded", currentLight.cosHalfFovExpanded                                                                                             },
            {"rotationLimit",      currentLight.rotationLimit                                                                                                  },
            {"translationLimit",   currentLight.translationLimit                                                                                               },
            {"mipDistance",        currentLight.mipDistance                                                                                                    },
            {"dAttenuation",       currentLight.dAttenuation                                                                                                   },
            {"roundness",          currentLight.roundness                                                                                                      },
            {"diffuseColor",       {{"x", currentLight.diffuseColor.x}, {"y", currentLight.diffuseColor.y}, {"z", currentLight.diffuseColor.z}, {"w", currentLight.diffuseColor.w}}},
            {"falloff",            {{"x", currentLight.falloff.x}, {"y", currentLight.falloff.y}, {"z", currentLight.falloff.z}, {"w", currentLight.falloff.w}}},
            {"angle",              {{"x", currentLight.angle.x}, {"y", currentLight.angle.y}, {"z", currentLight.angle.z}, {"w", currentLight.angle.w}}        },
            {"aAbB",               {{"x", currentLight.aAbB.x}, {"y", currentLight.aAbB.y}, {"z", currentLight.aAbB.z}, {"w", currentLight.aAbB.w}}            },
            {"cookieControl0",     {{"x", currentLight.cookieControl0.x}, {"y", currentLight.cookieControl0.y}, {"z", currentLight.cookieControl0.z}, {"w", currentLight.cookieControl0.w}}},
            {"cookieControl1",     {{"x", currentLight.cookieControl1.x}, {"y", currentLight.cookieControl1.y}, {"z", currentLight.cookieControl1.z}, {"w", currentLight.cookieControl1.w}}},
            {"cookieControl2",     {{"x", currentLight.cookieControl2.x}, {"y", currentLight.cookieControl2.y}, {"z", currentLight.cookieControl2.z}, {"w", currentLight.cookieControl2.w}}},
            {"defName",            defName                                                                                                                     }
        });
    }
    j["primaryLights"] = lightArray;

    std::string jsonString = j.dump(4);
    stream.write(jsonString.c_str(), jsonString.size());
}
