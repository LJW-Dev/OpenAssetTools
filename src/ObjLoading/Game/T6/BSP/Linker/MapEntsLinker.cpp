#include "MapEntsLinker.h"

#include "../BSPUtil.h"

namespace BSP
{
    MapEntsLinker::MapEntsLinker(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
        : m_memory(memory),
          m_search_path(searchPath),
          m_context(context)
    {
    }

    bool MapEntsLinker::linkMapEnts(ZoneAssetPools* T5AssetPool, std::string& bspName)
    {
        auto T5MapEntsAsset = T5AssetPool->GetAsset(T5::ASSET_TYPE_MAP_ENTS, bspName);
        if (T5MapEntsAsset == nullptr)
        {
            con::error("Can't find T5 Map Ents asset.");
            return false;
        }
        T5::MapEnts* T5MapEnts = static_cast<T5::MapEnts*>(T5MapEntsAsset->m_ptr);

        T6::MapEnts* T6MapEnts = m_memory.Alloc<T6::MapEnts>();
        T6MapEnts->name = m_memory.Dup(T5MapEnts->name);
        T6MapEnts->entityString = m_memory.Dup(T5MapEnts->entityString);
        T6MapEnts->numEntityChars = T5MapEnts->numEntityChars; // numEntityChars includes the null character

        // Added in T6
        T6MapEnts->trigger.count = 0;
        T6MapEnts->trigger.models = nullptr;
        T6MapEnts->trigger.hullCount = 0;
        T6MapEnts->trigger.hulls = nullptr;
        T6MapEnts->trigger.slabCount = 0;
        T6MapEnts->trigger.slabs = nullptr;

        m_context.AddAsset<T6::AssetMapEnts>(T6MapEnts->name, T6MapEnts);

        return true;
    }
} // namespace BSP
