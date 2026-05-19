#pragma once

#include "Asset/IAssetCreator.h"
#include "BSP/BSP.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

class GameWorldMpLinker
{
public:
    virtual ~GameWorldMpLinker() = default;

    static std::unique_ptr<GameWorldMpLinker> Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);

    virtual GameWorldMp* linkGameWorldMp(BSP::BSPData* bsp) = 0;
};
