#include "BSPLinker.h"

#include "ClipMapLinker.h"
#include "ComWorldLinker.h"
#include "GameWorldMpLinker.h"
#include "GfxWorldLinker.h"
#include "MapEntsLinker.h"
#include "SkinnedVertsLinker.h"

namespace BSP
{
    T6::FootstepTableDef* BSPLinker::addEmptyFootstepTableAsset(std::string assetName)
    {
        if (assetName.length() == 0)
            return nullptr;

        T6::FootstepTableDef* footstepTable = m_memory.Alloc<T6::FootstepTableDef>();
        footstepTable->name = m_memory.Dup(assetName.c_str());
        memset(footstepTable->sndAliasTable, 0, sizeof(footstepTable->sndAliasTable));

        m_context.AddAsset<T6::AssetFootstepTable>(assetName, footstepTable);

        return footstepTable;
    }

    bool BSPLinker::addDefaultRequiredAssets(std::string& mapName)
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

    BSPLinker::BSPLinker(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
        : m_memory(memory),
          m_search_path(searchPath),
          m_context(context)
    {
    }

    bool BSPLinker::linkBSP(ZoneAssetPools* T5AssetPool, std::string& mapName, std::string& T5BSPName)
    {
        std::string bspName = "maps/mp/" + mapName + ".d3dbsp";

        if (!addDefaultRequiredAssets(mapName))
            return false;

        ComWorldLinker comWorldLinker(m_memory, m_search_path, m_context);
        ClipMapLinker clipMapLinker(m_memory, m_search_path, m_context);
        GameWorldMpLinker gameWorldMpLinker(m_memory, m_search_path, m_context);
        GfxWorldLinker gfxWorldLinker(m_memory, m_search_path, m_context);
        MapEntsLinker mapEntsLinker(m_memory, m_search_path, m_context);
        SkinnedVertsLinker skinnedVertsLinker(m_memory, m_search_path, m_context);

        if (comWorldLinker.linkComWorld(T5AssetPool, mapName, bspName, T5BSPName) == false)
            return false;

        if (mapEntsLinker.linkMapEnts(T5AssetPool, mapName, bspName, T5BSPName) == false)
            return false;

        if (gameWorldMpLinker.linkGameWorldMp(T5AssetPool, mapName, bspName, T5BSPName) == false)
            return false;

        if (gfxWorldLinker.linkGfxWorld(T5AssetPool, mapName, bspName, T5BSPName) == false)
            return false;

        if (clipMapLinker.linkClipMap(T5AssetPool, mapName, bspName, T5BSPName) == false)
            return false;

        if (skinnedVertsLinker.linkSkinnedVerts(T5AssetPool, mapName, bspName, T5BSPName) == false) // requires GfxWorld asset
            return false;

        return true;
    }
} // namespace BSP
