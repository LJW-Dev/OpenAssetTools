#pragma once

#include "Asset/IAssetCreator.h"
#include "BSP/BSP.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

namespace T6
{
    namespace BSP
    {
        class ClipMapLinker
        {
        public:
            virtual ~ClipMapLinker() = default;

            static std::unique_ptr<ClipMapLinker> Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);

            virtual clipMap_t* linkClipMap(BSP::BSPData* bsp) = 0;
        };
    } // namespace BSP
} // namespace T6
