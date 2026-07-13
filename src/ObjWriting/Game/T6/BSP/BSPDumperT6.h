#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/T6/T6.h"

namespace T6
{
    namespace BSP
    {
        class DumperT6 final : public IAssetDumper
        {
        public:
            [[nodiscard]] std::optional<asset_type_t> GetHandlingAssetType() const override;
            [[nodiscard]] size_t GetProgressTotalCount(AssetDumpingContext& context) const override;
            void Dump(AssetDumpingContext& context) override;
        };
    } // namespace BSP

    namespace AddonMapEntsDumper
    {
        class DumperT6 final : public AbstractAssetDumper<AssetAddonMapEnts>
        {
        public:
            [[nodiscard]] std::optional<asset_type_t> GetHandlingAssetType() const override;
            [[nodiscard]] size_t GetProgressTotalCount(AssetDumpingContext& context) const override;

        protected:
            void DumpAsset(AssetDumpingContext& context, const XAssetInfo<AssetAddonMapEnts::Type>& asset) override;
        };
    } // namespace AddonMapEntsDumper

} // namespace T6
