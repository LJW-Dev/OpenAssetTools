#pragma once

#include "../BSP.h"
#include "Asset/IAssetCreator.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

class GameWorldMpLinker
{
public:
    virtual ~GameWorldMpLinker() = default;

    static std::unique_ptr<GameWorldMpLinker> Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);

    virtual GameWorldMp* linkGameWorldMp(BSP::BSPData* bsp) = 0;
};
