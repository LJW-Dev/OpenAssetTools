#pragma once

#include "Asset/IAssetCreator.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

namespace BSP
{
    class BSPCompilerExternal
    {
    public:
        BSPCompilerExternal(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);
        bool compileT5BSPIntoT6Zone(std::string& T6MapName);

    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        AssetCreationContext& m_context;
    };
} // namespace BSP
