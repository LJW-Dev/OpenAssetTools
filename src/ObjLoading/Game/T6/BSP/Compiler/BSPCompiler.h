#pragma once

#include "Asset/IAssetCreator.h"
#include "Game/T5/T5.h"
#include "Game/T6/T6.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"
#include "ZoneLoading.h"
#include "ZoneWriting.h"

namespace BSP
{
    class BSPCompiler
    {
    public:
        BSPCompiler(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);
        bool compileT5MapIntoZone(std::string& T6MapName);

    private:
        T6::FootstepTableDef* addEmptyFootstepTableAsset(std::string assetName);
        bool addDefaultRequiredAssets(std::string& bspName);
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        AssetCreationContext& m_context;
    };
} // namespace BSP
