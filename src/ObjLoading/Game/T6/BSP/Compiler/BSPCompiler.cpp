#include "BSPCompiler.h"

#include "ClipMapCompiler.h"
#include "ComWorldCompiler.h"
#include "GameWorldMpCompiler.h"
#include "GfxWorldCompiler.h"
#include "MapEntsCompiler.h"
#include "SkinnedVertsCompiler.h"

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

    T6::FootstepTableDef* BSPCompiler::addEmptyFootstepTableAsset(std::string assetName)
    {
        if (assetName.length() == 0)
            return nullptr;

        T6::FootstepTableDef* footstepTable = m_memory.Alloc<T6::FootstepTableDef>();
        footstepTable->name = m_memory.Dup(assetName.c_str());
        memset(footstepTable->sndAliasTable, 0, sizeof(footstepTable->sndAliasTable));

        m_context.AddAsset<T6::AssetFootstepTable>(assetName, footstepTable);

        return footstepTable;
    }

    bool BSPCompiler::addDefaultRequiredAssets(std::string& mapName)
    {
        if (m_context.LoadDependency<T6::AssetScript>("maps/mp/" + mapName + ".gsc") == nullptr)
            return false;
        if (m_context.LoadDependency<T6::AssetScript>("maps/mp/" + mapName + "_amb.gsc") == nullptr)
            return false;
        if (m_context.LoadDependency<T6::AssetScript>("maps/mp/" + mapName + "_fx.gsc") == nullptr)
            return false;

        if (m_context.LoadDependency<T6::AssetScript>("clientscripts/mp/" + mapName + ".csc") == nullptr)
            return false;
        if (m_context.LoadDependency<T6::AssetScript>("clientscripts/mp/" + mapName + "_amb.csc") == nullptr)
            return false;
        if (m_context.LoadDependency<T6::AssetScript>("clientscripts/mp/" + mapName + "_fx.csc") == nullptr)
            return false;

        addEmptyFootstepTableAsset("default_1st_person");
        addEmptyFootstepTableAsset("default_3rd_person");
        addEmptyFootstepTableAsset("default_1st_person_quiet");
        addEmptyFootstepTableAsset("default_3rd_person_quiet");
        addEmptyFootstepTableAsset("default_3rd_person_loud");
        addEmptyFootstepTableAsset("default_ai");

        if (m_context.LoadDependency<T6::AssetRawFile>("animtrees/fxanim_props.atr") == nullptr)
            return false;

        return true;
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

        if (!addDefaultRequiredAssets(T6MapName))
            return false;

        GameWorldMpCompiler gameWorldMpCompiler(m_memory, m_search_path, m_context);
        bool result = gameWorldMpCompiler.linkGameWorldMp(t5AssetPool, T6MapName, T6BSPName, T5BSPName);
        if (!result)
            return false;

        ComWorldCompiler comWorldCompiler(m_memory, m_search_path, m_context);
        result = comWorldCompiler.linkComWorld(t5AssetPool, T6MapName, T6BSPName, T5BSPName);
        if (!result)
            return false;

        MapEntsCompiler mapEntsCompiler(m_memory, m_search_path, m_context);
        result = mapEntsCompiler.linkMapEnts(t5AssetPool, T6MapName, T6BSPName, T5BSPName);
        if (!result)
            return false;

        GfxWorldCompiler gfxworldCompiler(m_memory, m_search_path, m_context);
        result = gfxworldCompiler.linkGfxWorld(t5AssetPool, T6MapName, T6BSPName, T5BSPName);
        if (!result)
            return false;

        ClipMapCompiler clipmapCompiler(m_memory, m_search_path, m_context);
        result = clipmapCompiler.linkClipMap(t5AssetPool, T6MapName, T6BSPName, T5BSPName); // requires GfxWorld and map ents
        if (!result)
            return false;

        SkinnedVertsCompiler skinnedVertsCompiler(m_memory, m_search_path, m_context);
        result = skinnedVertsCompiler.linkSkinnedVerts(t5AssetPool, T6MapName, T6BSPName, T5BSPName); // requires GfxWorld
        if (!result)
            return false;

        return true;
    }
} // namespace BSP
