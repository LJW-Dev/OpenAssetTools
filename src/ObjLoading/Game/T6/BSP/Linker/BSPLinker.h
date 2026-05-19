#pragma once

#include "../BSP.h"
#include "Asset/IAssetCreator.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

class BSPLinker
{
public:
    virtual ~BSPLinker() = default;
    static std::unique_ptr<BSPLinker> Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);
    virtual bool linkBSP(BSP::BSPData* bsp) = 0;
};
