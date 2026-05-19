#pragma once

#include "../BSP.h"
#include "Asset/IAssetCreator.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

class SkinnedVertsLinker
{
public:
    virtual ~SkinnedVertsLinker() = default;

    static std::unique_ptr<SkinnedVertsLinker> Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);

    virtual SkinnedVertsDef* linkSkinnedVerts(BSP::BSPData* bsp) = 0;
};
