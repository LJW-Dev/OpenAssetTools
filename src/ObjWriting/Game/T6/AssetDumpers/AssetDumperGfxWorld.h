#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/T6/T6.h"

namespace T6
{
    class AssetDumperGfxWorld final : public AbstractAssetDumper<GfxWorld>
    {
    protected:
        bool ShouldDump(XAssetInfo<GfxWorld>* asset) override;
        void DumpAsset(AssetDumpingContext& context, XAssetInfo<GfxWorld>* asset) override;
    };
} // namespace T6
