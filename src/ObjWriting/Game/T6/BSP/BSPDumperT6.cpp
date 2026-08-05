#include "BSPDumperT6.h"

#include "BSPUnlinker.h"
#include "BSPWriter.h"

using namespace T6;
using namespace BSP;

[[nodiscard]] std::optional<asset_type_t> DumperT6::GetHandlingAssetType() const
{
    return std::nullopt;
}

[[nodiscard]] size_t DumperT6::GetProgressTotalCount(AssetDumpingContext& context) const
{
    return 0;
}

void DumperT6::Dump(AssetDumpingContext& context)
{
    const auto& gfxWorldPool = context.m_zone.m_pools.PoolAssets<T6::AssetGfxWorld>();
    if (gfxWorldPool.size() != 1)
        return;

    con::info("BSP Dumping Started");

    const auto& colWorldPool = context.m_zone.m_pools.PoolAssets<T6::AssetClipMapPvs>();
    const auto& mapEntsPool = context.m_zone.m_pools.PoolAssets<T6::AssetMapEnts>();
    const auto& comWorldPool = context.m_zone.m_pools.PoolAssets<T6::AssetComWorld>();
    const auto& gameWorldMpPool = context.m_zone.m_pools.PoolAssets<T6::AssetGameWorldMp>();
    const auto& skinnedvertsPool = context.m_zone.m_pools.PoolAssets<T6::AssetSkinnedVerts>();
    const auto& lightDefPool = context.m_zone.m_pools.PoolAssets<T6::AssetLightDef>();
    const auto* mapEntsInfo = *mapEntsPool.begin();
    const auto* colWorldInfo = *colWorldPool.begin();
    const auto* comWorldInfo = *comWorldPool.begin();
    const auto* gfxWorldInfo = *gfxWorldPool.begin();
    const auto* gameWorldMpInfo = *gameWorldMpPool.begin();
    const auto* skinnedvertsInfo = *skinnedvertsPool.begin();
    BSPData dumpData;
    BSPAssetPtrs assetPtrs;
    assetPtrs.mapEnts = mapEntsInfo->Asset();
    assetPtrs.clipmap = colWorldInfo->Asset();
    assetPtrs.comworld = comWorldInfo->Asset();
    assetPtrs.gfxworld = gfxWorldInfo->Asset();
    assetPtrs.gameWorldMp = gameWorldMpInfo->Asset();
    assetPtrs.skinnedverts = skinnedvertsInfo->Asset();
    for (const auto& lightDef : lightDefPool)
        assetPtrs.lightDefs.emplace_back(lightDef->Asset());
    dumpBSPData(dumpData, context.m_zone.m_name, assetPtrs);

    con::info("BSP Writing Started");

    const std::unique_ptr<std::ostream> gfxAssetFile = context.OpenAssetFile("bsp/map_gfx.glb");
    if (!gfxAssetFile)
    {
        con::error("Unable to open bsp output bsp/map_gfx.glb file.");
        return;
    }
    const std::unique_ptr<std::ostream> colAssetFile = context.OpenAssetFile("bsp/map_col.glb");
    if (!colAssetFile)
    {
        con::error("Unable to open bsp output bsp/map_col.glb file.");
        return;
    }

    writeDumpDataToGltf(dumpData, gfxAssetFile, colAssetFile);

    con::info("Dumped BSP \"{}\"", context.m_zone.m_name);

    context.IncrementProgress();
}
