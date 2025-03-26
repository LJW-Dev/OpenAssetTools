#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/T6/T6.h"

namespace T6
{
    class AssetDumperGameWorldMp final : public AbstractAssetDumper<GameWorldMp>
    {
    protected:
        bool ShouldDump(XAssetInfo<GameWorldMp>* asset) override;
        void DumpAsset(AssetDumpingContext& context, XAssetInfo<GameWorldMp>* asset) override;
    };
} // namespace T6
