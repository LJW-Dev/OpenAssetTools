#pragma once

#include "Asset/IAssetCreator.h"
#include "BSP/BSP.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

namespace T6
{
    namespace BSP
    {
        class SkinnedVertsLinker
        {
        public:
            virtual ~SkinnedVertsLinker() = default;

            static std::unique_ptr<SkinnedVertsLinker> Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);

            virtual SkinnedVertsDef* linkSkinnedVerts(BSPData* bsp) = 0;
        };
    } // namespace BSP

} // namespace T6
