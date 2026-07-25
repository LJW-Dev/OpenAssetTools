#pragma once

#include "Asset/IAssetCreator.h"
#include "BSP/BSP.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

namespace T6
{
    namespace BSP
    {
        void writeDumpDataToGltf(BSPData dumpData, const std::unique_ptr<std::ostream>& gfxFile, const std::unique_ptr<std::ostream>& colFile);
    } // namespace BSP
} // namespace T6
