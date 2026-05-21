#pragma once

#include "BSP/BSP.h"
#include "SearchPath/ISearchPath.h"

namespace T6
{
    namespace BSP
    {
        std::unique_ptr<BSPData> createBSPData(std::string& mapName, ISearchPath& searchPath, bool isZombiesMap);
    }; // namespace BSP
} // namespace T6
