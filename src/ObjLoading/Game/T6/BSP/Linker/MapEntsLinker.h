#pragma once

#include "../BSP.h"
#include "Asset/IAssetCreator.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

class MapEntsLinker
{
public:
    virtual ~MapEntsLinker() = default;

    static std::unique_ptr<MapEntsLinker> Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);

    virtual MapEnts* linkMapEnts(BSP::BSPData* bsp) = 0;
};
