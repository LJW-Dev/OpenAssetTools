#include <format>
#include <nlohmann/json.hpp>

#include "AssetDumperSkinnedVertsDef.h"

using namespace T6;

bool AssetDumperSkinnedVertsDef::ShouldDump(XAssetInfo<SkinnedVertsDef>* asset)
{
    return true;
}

void AssetDumperSkinnedVertsDef::DumpAsset(AssetDumpingContext& context, XAssetInfo<SkinnedVertsDef>* asset)
{
    const auto* skinnedVert = asset->Asset();
    const auto assetFile = context.OpenAssetFile(std::format("bsp/{}.json", asset->m_name));
    
    if (!assetFile)
        return;

    auto& stream = *assetFile;

    nlohmann::json j;
    j["name"] = skinnedVert->name;
    j["maxSkinnedVerts"] = skinnedVert->maxSkinnedVerts;

    std::string jsonString = j.dump(4);
    stream.write(jsonString.c_str(), jsonString.size());
}
