#pragma once

#include "Asset/IAssetCreator.h"
#include "Compiler.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

namespace BSP
{
    class ComWorldCompiler
    {
    public:
        ComWorldCompiler(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);
        bool linkComWorld(ZoneAssetPools* T5AssetPool, std::string& mapName, std::string& bspName, std::string& T5BSPName);

    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        AssetCreationContext& m_context;
    };
} // namespace BSP
