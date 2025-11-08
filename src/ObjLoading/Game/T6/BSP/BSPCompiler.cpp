#include "BSPCompiler.h"

#include "ClipMapCompiler.h"
#include "GfxWorldCompiler.h"
#include "Utils/Logging/Log.h"
#include "Utils/Pack.h"

#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace BSP
{
    BSPCompiler::BSPCompiler(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
        : m_memory(memory),
          m_search_path(searchPath),
          m_context(context)
    {
    }

    bool BSPCompiler::compileT5MapIntoZone(std::string& T6MapName)
    {
        const std::string fastFilePath = "C:\\Users\\LJ\\Documents\\zombiehouse\\zombie_house\\zombie_house.ff";

        auto t5Zone = ZoneLoading::LoadZone(fastFilePath, std::nullopt);
        if (!t5Zone)
        {
            con::error("Unable to open T5 FastFile {}.", fastFilePath);
            return false;
        }
        auto t5AssetPool = t5Zone.value()->m_pools.get();

        std::string T5BSPName = "maps/" + t5Zone.value()->m_name + ".d3dbsp";
        std::string T6BSPName = "maps/mp/" + T6MapName + ".d3dbsp";

        GfxWorldCompiler gfxworldCompiler(m_memory, m_search_path, m_context);
        bool result = gfxworldCompiler.linkGfxWorld(t5AssetPool, T6MapName, T6BSPName, T5BSPName);
        if (!result)
        {
            con::error("Failed adding T5 GfxWorld to zone.");
            return false;
        }
        else
            con::info("Sucessfully added T5 GfxWorld to zone.");

        ClipMapCompiler clipmapCompiler(m_memory, m_search_path, m_context);
        result = clipmapCompiler.linkClipMap(t5AssetPool, T6MapName, T6BSPName, T5BSPName); // requires GfxWorld and map ents

        if (!result)
        {
            con::error("Failed adding T5 ClipMap to zone.");
            return false;
        }
        else
            con::info("Sucessfully added T5 ClipMap to zone.");

        return true;
    }
} // namespace BSP
