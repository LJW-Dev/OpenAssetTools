#pragma once

#include "../BSP.h"
#include "Asset/IAssetCreator.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

class GfxWorldLinker
{
public:
    virtual ~GfxWorldLinker() = default;

    static std::unique_ptr<GfxWorldLinker> Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);

    virtual GfxWorld* linkGfxWorld(BSP::BSPData* bsp) = 0;
};
