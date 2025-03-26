#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/T6/T6.h"

namespace T6
{
    class AssetDumperComWorld final : public AbstractAssetDumper<ComWorld>
    {
    protected:
        bool ShouldDump(XAssetInfo<ComWorld>* asset) override;
        void DumpAsset(AssetDumpingContext& context, XAssetInfo<ComWorld>* asset) override;
    };
} // namespace T6
