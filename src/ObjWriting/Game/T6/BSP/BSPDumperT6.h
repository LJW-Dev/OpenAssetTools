#pragma once

#include "Dumping/IAssetDumper.h"

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

} // namespace T6
