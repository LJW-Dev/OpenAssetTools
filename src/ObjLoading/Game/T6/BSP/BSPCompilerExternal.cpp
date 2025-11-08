#include "BSPCompilerExternal.h"

#include "BSPCompiler.h"

namespace BSP
{
    BSPCompilerExternal::BSPCompilerExternal(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
        : m_memory(memory),
          m_search_path(searchPath),
          m_context(context)
    {
    }

    bool BSPCompilerExternal::compileT5BSPIntoT6Zone(std::string& T6MapName)
    {
        BSPCompiler compiler(m_memory, m_search_path, m_context);
        return compiler.compileT5MapIntoZone(T6MapName);
    }
} // namespace BSP
