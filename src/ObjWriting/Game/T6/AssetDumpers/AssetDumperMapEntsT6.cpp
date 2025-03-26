#include <format>
#include <nlohmann/json.hpp>

#include "AssetDumperMapEntsT6.h"

using namespace T6;

bool AssetDumperMapEntsT6::ShouldDump(XAssetInfo<MapEnts>* asset)
{
    return true;
}

void AssetDumperMapEntsT6::DumpAsset(AssetDumpingContext& context, XAssetInfo<MapEnts>* asset)
{
    const auto* mapEnts = asset->Asset();
    const auto assetFile = context.OpenAssetFile("bsp/mapents.json");
    
    if (!assetFile)
        return;

    auto& stream = *assetFile;

    nlohmann::json j;
    j["name"] = mapEnts->name;
    j["entityString"] = mapEnts->entityString;

    j["trigger"]["modelCount"] = mapEnts->trigger.count;
    auto modelArray = nlohmann::json::array();
    for (unsigned int i = 0; i < mapEnts->trigger.count; i++)
    {
        auto currentModel = mapEnts->trigger.models[i];
        modelArray.push_back({
            {"contents",  currentModel.contents },
            {"hullCount", currentModel.hullCount},
            {"firstHull", currentModel.firstHull},
        });
    }
    j["trigger"]["models"] = modelArray;

    j["trigger"]["hullCount"] = mapEnts->trigger.hullCount;
    auto hullArray = nlohmann::json::array();
    for (unsigned int i = 0; i < mapEnts->trigger.hullCount; i++)
    {
        auto currentHull = mapEnts->trigger.hulls[i];
        hullArray.push_back({
            {"midPoint",  {{"x", currentHull.bounds.midPoint.x}, {"y", currentHull.bounds.midPoint.y}, {"z", currentHull.bounds.midPoint.z}}},
            {"halfSize",  {{"x", currentHull.bounds.halfSize.x}, {"y", currentHull.bounds.halfSize.y}, {"z", currentHull.bounds.halfSize.z}}},
            {"contents",  currentHull.contents                                                                         },
            {"hullCount", currentHull.slabCount                                                                        },
            {"firstHull", currentHull.firstSlab                                                                        },
        });
    }
    j["trigger"]["hulls"] = hullArray;

    j["trigger"]["slabCount"] = mapEnts->trigger.hullCount;
    auto slabArray = nlohmann::json::array();
    for (unsigned int i = 0; i < mapEnts->trigger.slabCount; i++)
    {
        auto currentSlab = mapEnts->trigger.slabs[i];
        slabArray.push_back({
            {"dir",      {{"x", currentSlab.dir.x}, {"y", currentSlab.dir.y}, {"z", currentSlab.dir.z}}},
            {"midPoint", currentSlab.midPoint                                     },
            {"halfSize", currentSlab.halfSize                                     },
        });
    }
    j["trigger"]["slabs"] = slabArray;
    
    std::string jsonString = j.dump(4);
    stream.write(jsonString.c_str(), jsonString.size());
}
