#pragma once

#include "Asset/IAssetCreator.h"
#include "BSP/BSP.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

namespace T6
{
    namespace BSP
    {
        class GfxWorldLinker
        {
        public:
            virtual ~GfxWorldLinker() = default;

            static std::unique_ptr<GfxWorldLinker> Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);

            virtual GfxWorld* linkGfxWorld(BSP::BSPData* bsp) = 0;
        };
    } // namespace BSP
} // namespace T6
