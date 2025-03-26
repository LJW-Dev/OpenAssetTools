#include <nlohmann/json.hpp>

#include "AssetDumperGfxLightDefT6.h"

using namespace T6;

bool AssetDumperGfxLightDefT6::ShouldDump(XAssetInfo<GfxLightDef>* asset)
{
    return true;
}

void AssetDumperGfxLightDefT6::DumpAsset(AssetDumpingContext& context, XAssetInfo<GfxLightDef>* asset)
{
    const auto* gfxLightDef = asset->Asset();
    const auto assetFile = context.OpenAssetFile("lightDef/" + asset->m_name + ".json");

    if (!assetFile)
        return;

    auto& stream = *assetFile;

    nlohmann::json j;
    j["attenuation"]["image"] = gfxLightDef->attenuation.image->name;
    j["attenuation"]["samplerState"] = gfxLightDef->attenuation.samplerState;
    j["lmapLookupStart"] = gfxLightDef->lmapLookupStart;
    

    std::string jsonString = j.dump(4);
    stream.write(jsonString.c_str(), jsonString.size());
}
