#pragma once

#include "../BSP.h"
#include "../BSPCalculation.h"
#include "Asset/IAssetCreator.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

class ClipMapLinker
{
public:
    virtual ~ClipMapLinker() = default;

    static std::unique_ptr<ClipMapLinker> Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);

    virtual clipMap_t* linkClipMap(BSP::BSPData* bsp) = 0;
};
