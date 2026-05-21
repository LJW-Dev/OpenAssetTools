#pragma once

#include "Asset/IAssetCreator.h"
#include "BSP/BSP.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

namespace T6
{
    namespace BSP
    {
        class BSPLinker
        {
        public:
            virtual ~BSPLinker() = default;
            static std::unique_ptr<BSPLinker> Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);
            virtual bool linkBSP(BSPData* bsp) = 0;
        };
    } // namespace BSP
} // namespace T6
