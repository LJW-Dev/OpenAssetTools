#pragma once

#include "Asset/IAssetCreator.h"
#include "BSP/BSP.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

class MapEntsLinker
{
public:
    virtual ~MapEntsLinker() = default;

    static std::unique_ptr<MapEntsLinker> Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);

    virtual MapEnts* linkMapEnts(BSP::BSPData* bsp) = 0;
};
