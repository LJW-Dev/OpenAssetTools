#pragma once

#include "../BSP.h"
#include "Asset/IAssetCreator.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

namespace BSP
{
    class BSPLinker
    {
    public:
        BSPLinker(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);
        bool linkBSP(ZoneAssetPools* T5AssetPool, std::string& mapName, std::string& T5BSPName);

    private:
        T6::FootstepTableDef* addEmptyFootstepTableAsset(std::string assetName);
        bool addDefaultRequiredAssets(std::string& bspName);

        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        AssetCreationContext& m_context;
    };
} // namespace BSP
