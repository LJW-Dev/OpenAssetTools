#pragma once

#include "Asset/IAssetCreator.h"
#include "BSP/BSP.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

class ComWorldLinker
{
public:
    virtual ~ComWorldLinker() = default;

    static std::unique_ptr<ComWorldLinker> Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);

    virtual ComWorld* linkComWorld(BSP::BSPData* bsp) = 0;
};
