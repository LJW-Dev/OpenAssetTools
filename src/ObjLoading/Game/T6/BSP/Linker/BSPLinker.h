#pragma once

#include "Asset/IAssetCreator.h"
#include "BSP/BSP.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

class BSPLinker
{
public:
    virtual ~BSPLinker() = default;
    static std::unique_ptr<BSPLinker> Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);
    virtual bool linkBSP(BSP::BSPData* bsp) = 0;
};
