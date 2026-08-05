#include "BSPLinker.h"

#include "ClipMapLinker.h"
#include "ComWorldLinker.h"
#include "GameWorldMpLinker.h"
#include "GfxWorldLinker.h"
#include "MapEntsLinker.h"
#include "SkinnedVertsLinker.h"

using namespace T6;
using namespace BSP;

class BSPLinkerImpl : public BSPLinker
{
private:
    MemoryManager& m_memory;
    ISearchPath& m_search_path;
    AssetCreationContext& m_context;

    FootstepTableDef* addEmptyFootstepTableAsset(std::string assetName)
    {
        if (assetName.length() == 0)
            return nullptr;

        auto asset = m_context.LoadDependency<AssetFootstepTable>(assetName);
        if (asset != nullptr)
            return asset->Asset();
        con::debug("Adding empty footsteptable {}", assetName);

        FootstepTableDef* footstepTable = m_memory.Alloc<FootstepTableDef>();
        footstepTable->name = m_memory.Dup(assetName.c_str());
        memset(footstepTable->sndAliasTable, 0, sizeof(footstepTable->sndAliasTable));
        m_context.AddAsset<AssetFootstepTable>(assetName, footstepTable);

        return footstepTable;
    }

    RawFile* addEmptyRawFileAsset(std::string assetName)
    {
        if (assetName.length() == 0)
            return nullptr;

        auto asset = m_context.LoadDependency<AssetRawFile>(assetName);
        if (asset != nullptr)
            return asset->Asset();
        con::debug("Adding empty rawfile {}", assetName);

        RawFile* rawFile = m_memory.Alloc<RawFile>();
        rawFile->name = m_memory.Dup(assetName.c_str());
        if (assetName.ends_with(".atr"))
        {
            const char* emptyAtrFile = "\x00\x00\x00\x00\x03\x00";
            rawFile->len = 6;
            char* destBuffer = m_memory.Alloc<char>(6);
            memcpy(destBuffer, emptyAtrFile, 6);
            rawFile->buffer = destBuffer;
        }
        else
        {
            rawFile->len = 1;
            rawFile->buffer = m_memory.Alloc<char>();
        }
        m_context.AddAsset<AssetRawFile>(assetName, rawFile);

        return rawFile;
    }

    bool addDefaultRequiredAssets(BSPData* bsp)
    {
        if (m_context.LoadDependency<AssetScript>("maps/mp/" + bsp->name + ".gsc") == nullptr)
        {
            con::error("maps/mp/" + bsp->name + ".gsc not found, make sure GSC file is in another location.");
        }
        else
        {
            if (m_context.LoadDependency<AssetScript>("maps/mp/" + bsp->name + "_amb.gsc") == nullptr)
                return false;
            if (m_context.LoadDependency<AssetScript>("maps/mp/" + bsp->name + "_fx.gsc") == nullptr)
                return false;

            if (m_context.LoadDependency<AssetScript>("clientscripts/mp/" + bsp->name + ".csc") == nullptr)
                return false;
            if (m_context.LoadDependency<AssetScript>("clientscripts/mp/" + bsp->name + "_amb.csc") == nullptr)
                return false;
            if (m_context.LoadDependency<AssetScript>("clientscripts/mp/" + bsp->name + "_fx.csc") == nullptr)
                return false;
        }

        addEmptyFootstepTableAsset("default_1st_person");
        addEmptyFootstepTableAsset("default_3rd_person");
        addEmptyFootstepTableAsset("default_1st_person_quiet");
        addEmptyFootstepTableAsset("default_3rd_person_quiet");
        addEmptyFootstepTableAsset("default_3rd_person_loud");
        addEmptyFootstepTableAsset("default_ai");

        addEmptyRawFileAsset("animtrees/fxanim_props.atr");
        addEmptyRawFileAsset(bsp->name);

        return true;
    }

public:
    explicit BSPLinkerImpl(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
        : m_memory(memory),
          m_search_path(searchPath),
          m_context(context)
    {
    }

    bool linkBSP(BSPData* bsp) override
    {
        con::info("------ Linking BSP Started ------");

        if (!addDefaultRequiredAssets(bsp))
            return false;

        auto comWorldLinker = ComWorldLinker::Create(m_memory, m_search_path, m_context);
        auto clipMapLinker = ClipMapLinker::Create(m_memory, m_search_path, m_context);
        auto gameWorldMpLinker = GameWorldMpLinker::Create(m_memory, m_search_path, m_context);
        auto gfxWorldLinker = GfxWorldLinker::Create(m_memory, m_search_path, m_context);
        auto mapEntsLinker = MapEntsLinker::Create(m_memory, m_search_path, m_context);
        auto skinnedVertsLinker = SkinnedVertsLinker::Create(m_memory, m_search_path, m_context);

        con::info("------ Linking Com World ------");
        ComWorld* comWorld = comWorldLinker->linkComWorld(bsp);
        if (comWorld == nullptr)
            return false;
        auto* oldCom = (m_context.LoadDependency<AssetComWorld>(bsp->bspName));
        if (oldCom)
        {
            assert(!oldCom->IsReference());
            ComWorld* oldComPtr = oldCom->Asset();
            memcpy(oldComPtr, comWorld, sizeof(ComWorld));
        }
        else
            m_context.AddAsset<AssetComWorld>(comWorld->name, comWorld);

        con::info("------ Linking Map Ents ------");
        MapEnts* mapEnts = mapEntsLinker->linkMapEnts(bsp);
        if (mapEnts == nullptr)
            return false;
        auto* oldEnts = (m_context.LoadDependency<AssetMapEnts>(bsp->bspName));
        if (oldEnts)
        {
            assert(!oldEnts->IsReference());
            MapEnts* oldEntsPtr = oldEnts->Asset();
            memcpy(oldEntsPtr, mapEnts, sizeof(MapEnts));
        }
        else
            m_context.AddAsset<AssetMapEnts>(mapEnts->name, mapEnts);

        con::info("------ Linking Game World Mp ------");
        GameWorldMp* gameWorldMp = gameWorldMpLinker->linkGameWorldMp(bsp);
        if (gameWorldMp == nullptr)
            return false;
        auto* oldGW = (m_context.LoadDependency<AssetGameWorldMp>(bsp->bspName));
        if (oldGW)
        {
            assert(!oldGW->IsReference());
            GameWorldMp* oldGWPtr = oldGW->Asset();
            memcpy(oldGWPtr, gameWorldMp, sizeof(GameWorldMp));
        }
        else
            m_context.AddAsset<AssetGameWorldMp>(gameWorldMp->name, gameWorldMp);

        con::info("------ Linking Skinned Verts ------");
        SkinnedVertsDef* skinnedVerts = skinnedVertsLinker->linkSkinnedVerts(bsp);
        if (skinnedVerts == nullptr)
            return false;
        auto* oldSV = (m_context.LoadDependency<AssetSkinnedVerts>("skinnedverts"));
        if (oldSV)
        {
            assert(!oldSV->IsReference());
            SkinnedVertsDef* oldSVPtr = oldSV->Asset();
            memcpy(oldSVPtr, skinnedVerts, sizeof(SkinnedVertsDef));
        }
        else
            m_context.AddAsset<AssetSkinnedVerts>(skinnedVerts->name, skinnedVerts);

        con::info("------ Linking GFX World ------");
        GfxWorld* gfxWorld = gfxWorldLinker->linkGfxWorld(bsp);
        if (gfxWorld == nullptr)
            return false;
        auto* oldGfx = (m_context.LoadDependency<AssetGfxWorld>(bsp->bspName));
        if (oldGfx)
        {
            assert(!oldGfx->IsReference());
            GfxWorld* oldGfxPtr = oldGfx->Asset();
            memcpy(oldGfxPtr, gfxWorld, sizeof(GfxWorld));
        }
        else
            m_context.AddAsset<AssetGfxWorld>(gfxWorld->name, gfxWorld);

        con::info("------ Linking ClipMap ------");
        clipMap_t* clipMap = clipMapLinker->linkClipMap(bsp); // requires mapents asset
        if (clipMap == nullptr)
            return false;
        auto* oldCol = (m_context.LoadDependency<AssetClipMapPvs>(bsp->bspName));
        if (oldCol)
        {
            assert(!oldCol->IsReference());
            clipMap_t* oldColPtr = oldCol->Asset();
            memcpy(oldColPtr, clipMap, sizeof(clipMap_t));
        }
        else
            m_context.AddAsset<AssetClipMapPvs>(clipMap->name, clipMap);

        return true;
    };
};

std::unique_ptr<BSPLinker> BSPLinker::Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
{
    return std::make_unique<BSPLinkerImpl>(memory, searchPath, context);
}
