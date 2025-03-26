#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/T6/T6.h"

namespace T6
{
    class AssetDumperGfxLightDefT6 final : public AbstractAssetDumper<GfxLightDef>
    {
    protected:
        bool ShouldDump(XAssetInfo<GfxLightDef>* asset) override;
        void DumpAsset(AssetDumpingContext& context, XAssetInfo<GfxLightDef>* asset) override;
    };
} // namespace T6
