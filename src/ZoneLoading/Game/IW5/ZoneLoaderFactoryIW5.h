#pragma once

#include "Loading/IZoneLoaderFactory.h"

#include <string>

namespace IW5
{
    class ZoneLoaderFactory final : public IZoneLoaderFactory
    {
    public:
        std::unique_ptr<ZoneLoader> CreateLoaderForHeader(ZoneHeader& header, std::string& fileName) const override;
    };
} // namespace IW5
