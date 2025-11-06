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

    bool BSPLinker::addDefaultRequiredAssets(std::string& bspName)
    {
        if (m_context.LoadDependency<T6::AssetScript>("maps/mp/" + bspName + ".gsc") == nullptr)
            return false;
        if (m_context.LoadDependency<T6::AssetScript>("maps/mp/" + bspName + "_amb.gsc") == nullptr)
            return false;
        if (m_context.LoadDependency<T6::AssetScript>("maps/mp/" + bspName + "_fx.gsc") == nullptr)
            return false;

        if (m_context.LoadDependency<T6::AssetScript>("clientscripts/mp/" + bspName + ".csc") == nullptr)
            return false;
        if (m_context.LoadDependency<T6::AssetScript>("clientscripts/mp/" + bspName + "_amb.csc") == nullptr)
            return false;
        if (m_context.LoadDependency<T6::AssetScript>("clientscripts/mp/" + bspName + "_fx.csc") == nullptr)
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

    bool BSPLinker::linkBSP(ZoneAssetPools* T5AssetPool, std::string& bspName)
    {
        if (!addDefaultRequiredAssets(bspName))
            return false;

        ComWorldLinker comWorldLinker(m_memory, m_search_path, m_context);
        ClipMapLinker clipMapLinker(m_memory, m_search_path, m_context);
        GameWorldMpLinker gameWorldMpLinker(m_memory, m_search_path, m_context);
        GfxWorldLinker gfxWorldLinker(m_memory, m_search_path, m_context);
        MapEntsLinker mapEntsLinker(m_memory, m_search_path, m_context);
        SkinnedVertsLinker skinnedVertsLinker(m_memory, m_search_path, m_context);

        if (comWorldLinker.linkComWorld(T5AssetPool, bspName) == false)
            return false;

        if (mapEntsLinker.linkMapEnts(T5AssetPool, bspName) == false)
            return false;

        if (gameWorldMpLinker.linkGameWorldMp(T5AssetPool, bspName) == false)
            return false;

        if (gfxWorldLinker.linkGfxWorld(T5AssetPool, bspName) == false)
            return false;

        if (clipMapLinker.linkClipMap(T5AssetPool, bspName) == false)
            return false;

        if (skinnedVertsLinker.linkSkinnedVerts(T5AssetPool, bspName) == false) // requires GfxWorld asset
            return false;

        return true;
    }
} // namespace BSP
