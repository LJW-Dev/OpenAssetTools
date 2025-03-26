#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/T6/T6.h"

namespace T6
{
    class AssetDumperClipMap final : public AbstractAssetDumper<clipMap_t>
    {
    protected:
        bool ShouldDump(XAssetInfo<clipMap_t>* asset) override;
        void DumpAsset(AssetDumpingContext& context, XAssetInfo<clipMap_t>* asset) override;
    };
} // namespace T6
