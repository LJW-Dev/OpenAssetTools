#include "ContentLoaderT6.h"

#include "Game/T6/T6.h"
#include "Game/T6/XAssets/addonmapents/addonmapents_load_db.h"
#include "Game/T6/XAssets/clipmap_t/clipmap_t_load_db.h"
#include "Game/T6/XAssets/comworld/comworld_load_db.h"
#include "Game/T6/XAssets/ddlroot_t/ddlroot_t_load_db.h"
#include "Game/T6/XAssets/destructibledef/destructibledef_load_db.h"
#include "Game/T6/XAssets/emblemset/emblemset_load_db.h"
#include "Game/T6/XAssets/font_s/font_s_load_db.h"
#include "Game/T6/XAssets/fonticon/fonticon_load_db.h"
#include "Game/T6/XAssets/footstepfxtabledef/footstepfxtabledef_load_db.h"
#include "Game/T6/XAssets/footsteptabledef/footsteptabledef_load_db.h"
#include "Game/T6/XAssets/fxeffectdef/fxeffectdef_load_db.h"
#include "Game/T6/XAssets/fximpacttable/fximpacttable_load_db.h"
#include "Game/T6/XAssets/gameworldmp/gameworldmp_load_db.h"
#include "Game/T6/XAssets/gameworldsp/gameworldsp_load_db.h"
#include "Game/T6/XAssets/gfximage/gfximage_load_db.h"
#include "Game/T6/XAssets/gfxlightdef/gfxlightdef_load_db.h"
#include "Game/T6/XAssets/gfxworld/gfxworld_load_db.h"
#include "Game/T6/XAssets/glasses/glasses_load_db.h"
#include "Game/T6/XAssets/keyvaluepairs/keyvaluepairs_load_db.h"
#include "Game/T6/XAssets/leaderboarddef/leaderboarddef_load_db.h"
#include "Game/T6/XAssets/localizeentry/localizeentry_load_db.h"
#include "Game/T6/XAssets/mapents/mapents_load_db.h"
#include "Game/T6/XAssets/material/material_load_db.h"
#include "Game/T6/XAssets/materialtechniqueset/materialtechniqueset_load_db.h"
#include "Game/T6/XAssets/memoryblock/memoryblock_load_db.h"
#include "Game/T6/XAssets/menudef_t/menudef_t_load_db.h"
#include "Game/T6/XAssets/menulist/menulist_load_db.h"
#include "Game/T6/XAssets/physconstraints/physconstraints_load_db.h"
#include "Game/T6/XAssets/physpreset/physpreset_load_db.h"
#include "Game/T6/XAssets/qdb/qdb_load_db.h"
#include "Game/T6/XAssets/rawfile/rawfile_load_db.h"
#include "Game/T6/XAssets/scriptparsetree/scriptparsetree_load_db.h"
#include "Game/T6/XAssets/skinnedvertsdef/skinnedvertsdef_load_db.h"
#include "Game/T6/XAssets/slug/slug_load_db.h"
#include "Game/T6/XAssets/sndbank/sndbank_load_db.h"
#include "Game/T6/XAssets/snddriverglobals/snddriverglobals_load_db.h"
#include "Game/T6/XAssets/sndpatch/sndpatch_load_db.h"
#include "Game/T6/XAssets/stringtable/stringtable_load_db.h"
#include "Game/T6/XAssets/tracerdef/tracerdef_load_db.h"
#include "Game/T6/XAssets/vehicledef/vehicledef_load_db.h"
#include "Game/T6/XAssets/weaponattachment/weaponattachment_load_db.h"
#include "Game/T6/XAssets/weaponattachmentunique/weaponattachmentunique_load_db.h"
#include "Game/T6/XAssets/weaponcamo/weaponcamo_load_db.h"
#include "Game/T6/XAssets/weaponvariantdef/weaponvariantdef_load_db.h"
#include "Game/T6/XAssets/xanimparts/xanimparts_load_db.h"
#include "Game/T6/XAssets/xglobals/xglobals_load_db.h"
#include "Game/T6/XAssets/xmodel/xmodel_load_db.h"
#include "Game/T6/XAssets/zbarrierdef/zbarrierdef_load_db.h"
#include "Loading/Exception/UnsupportedAssetTypeException.h"

#include <cassert>

using namespace T6;

ContentLoader::ContentLoader(Zone& zone, ZoneInputStream& stream)
    : ContentLoaderBase(zone, stream),
      varXAsset(nullptr),
      varScriptStringList(nullptr)
{
}

void ContentLoader::LoadScriptStringList(const bool atStreamStart)
{
    m_stream.PushBlock(XFILE_BLOCK_VIRTUAL);

    if (atStreamStart)
        m_stream.Load<ScriptStringList>(varScriptStringList);

    if (varScriptStringList->strings != nullptr)
    {
        assert(GetZonePointerType(varScriptStringList->strings) == ZonePointerType::FOLLOWING);

        varScriptStringList->strings = m_stream.Alloc<const char*>(alignof(const char*));
        varXString = varScriptStringList->strings;
        LoadXStringArray(true, varScriptStringList->count);

        if (varScriptStringList->strings && varScriptStringList->count > 0)
            m_zone.m_script_strings.InitializeForExistingZone(varScriptStringList->strings, static_cast<size_t>(varScriptStringList->count));
    }

    m_stream.PopBlock();

    assert(m_zone.m_script_strings.Count() <= SCR_STRING_MAX + 1);
}

void ContentLoader::LoadXAsset(const bool atStreamStart) const
{
#define LOAD_ASSET(type_index, typeName, headerEntry)                                                                                                          \
    case type_index:                                                                                                                                           \
    {                                                                                                                                                          \
        Loader_##typeName loader(m_zone, m_stream);                                                                                                            \
        loader.Load(&varXAsset->header.headerEntry);                                                                                                           \
        break;                                                                                                                                                 \
    }

    assert(varXAsset != nullptr);

    if (atStreamStart)
        m_stream.Load<XAsset>(varXAsset);

    switch (varXAsset->type)
    {
        LOAD_ASSET(ASSET_TYPE_PHYSPRESET, PhysPreset, physPreset)
        LOAD_ASSET(ASSET_TYPE_PHYSCONSTRAINTS, PhysConstraints, physConstraints)
        LOAD_ASSET(ASSET_TYPE_DESTRUCTIBLEDEF, DestructibleDef, destructibleDef)
        LOAD_ASSET(ASSET_TYPE_XANIMPARTS, XAnimParts, parts)
        LOAD_ASSET(ASSET_TYPE_XMODEL, XModel, model)
        LOAD_ASSET(ASSET_TYPE_MATERIAL, Material, material)
        LOAD_ASSET(ASSET_TYPE_TECHNIQUE_SET, MaterialTechniqueSet, techniqueSet)
        LOAD_ASSET(ASSET_TYPE_IMAGE, GfxImage, image)
        LOAD_ASSET(ASSET_TYPE_SOUND, SndBank, sound)
        LOAD_ASSET(ASSET_TYPE_SOUND_PATCH, SndPatch, soundPatch)
        LOAD_ASSET(ASSET_TYPE_CLIPMAP, clipMap_t, clipMap)
        LOAD_ASSET(ASSET_TYPE_CLIPMAP_PVS, clipMap_t, clipMap)
        LOAD_ASSET(ASSET_TYPE_COMWORLD, ComWorld, comWorld)
        LOAD_ASSET(ASSET_TYPE_GAMEWORLD_SP, GameWorldSp, gameWorldSp)
        LOAD_ASSET(ASSET_TYPE_GAMEWORLD_MP, GameWorldMp, gameWorldMp)
        LOAD_ASSET(ASSET_TYPE_MAP_ENTS, MapEnts, mapEnts)
        LOAD_ASSET(ASSET_TYPE_GFXWORLD, GfxWorld, gfxWorld)
        LOAD_ASSET(ASSET_TYPE_LIGHT_DEF, GfxLightDef, lightDef)
        LOAD_ASSET(ASSET_TYPE_FONT, Font_s, font)
        LOAD_ASSET(ASSET_TYPE_FONTICON, FontIcon, fontIcon)
        LOAD_ASSET(ASSET_TYPE_MENULIST, MenuList, menuList)
        LOAD_ASSET(ASSET_TYPE_MENU, menuDef_t, menu)
        LOAD_ASSET(ASSET_TYPE_LOCALIZE_ENTRY, LocalizeEntry, localize)
        LOAD_ASSET(ASSET_TYPE_WEAPON, WeaponVariantDef, weapon)
        LOAD_ASSET(ASSET_TYPE_ATTACHMENT, WeaponAttachment, attachment)
        LOAD_ASSET(ASSET_TYPE_ATTACHMENT_UNIQUE, WeaponAttachmentUnique, attachmentUnique)
        LOAD_ASSET(ASSET_TYPE_WEAPON_CAMO, WeaponCamo, weaponCamo)
        LOAD_ASSET(ASSET_TYPE_SNDDRIVER_GLOBALS, SndDriverGlobals, sndDriverGlobals)
        LOAD_ASSET(ASSET_TYPE_FX, FxEffectDef, fx)
        LOAD_ASSET(ASSET_TYPE_IMPACT_FX, FxImpactTable, impactFx)
        LOAD_ASSET(ASSET_TYPE_RAWFILE, RawFile, rawfile)
        LOAD_ASSET(ASSET_TYPE_STRINGTABLE, StringTable, stringTable)
        LOAD_ASSET(ASSET_TYPE_LEADERBOARD, LeaderboardDef, leaderboardDef)
        LOAD_ASSET(ASSET_TYPE_XGLOBALS, XGlobals, xGlobals)
        LOAD_ASSET(ASSET_TYPE_DDL, ddlRoot_t, ddlRoot)
        LOAD_ASSET(ASSET_TYPE_GLASSES, Glasses, glasses)
        LOAD_ASSET(ASSET_TYPE_EMBLEMSET, EmblemSet, emblemSet)
        LOAD_ASSET(ASSET_TYPE_SCRIPTPARSETREE, ScriptParseTree, scriptParseTree)
        LOAD_ASSET(ASSET_TYPE_KEYVALUEPAIRS, KeyValuePairs, keyValuePairs)
        LOAD_ASSET(ASSET_TYPE_VEHICLEDEF, VehicleDef, vehicleDef)
        LOAD_ASSET(ASSET_TYPE_MEMORYBLOCK, MemoryBlock, memoryBlock);
        LOAD_ASSET(ASSET_TYPE_ADDON_MAP_ENTS, AddonMapEnts, addonMapEnts)
        LOAD_ASSET(ASSET_TYPE_TRACER, TracerDef, tracerDef)
        LOAD_ASSET(ASSET_TYPE_SKINNEDVERTS, SkinnedVertsDef, skinnedVertsDef)
        LOAD_ASSET(ASSET_TYPE_QDB, Qdb, qdb)
        LOAD_ASSET(ASSET_TYPE_SLUG, Slug, slug)
        LOAD_ASSET(ASSET_TYPE_FOOTSTEP_TABLE, FootstepTableDef, footstepTableDef)
        LOAD_ASSET(ASSET_TYPE_FOOTSTEPFX_TABLE, FootstepFXTableDef, footstepFXTableDef)
        LOAD_ASSET(ASSET_TYPE_ZBARRIER, ZBarrierDef, zbarrierDef)

    default:
    {
        throw UnsupportedAssetTypeException(varXAsset->type);
    }
    }

#undef LOAD_ASSET
}

void ContentLoader::LoadXAssetArray(const bool atStreamStart, const size_t count)
{
    assert(varXAsset != nullptr);

    if (atStreamStart)
        m_stream.Load<XAsset>(varXAsset, count);

    for (size_t index = 0; index < count; index++)
    {
        LoadXAsset(false);
        varXAsset++;
    }
}

void ContentLoader::Load()
{
    m_stream.PushBlock(XFILE_BLOCK_VIRTUAL);

    XAssetList assetList{};
    m_stream.LoadDataRaw(&assetList, sizeof(assetList));

    varScriptStringList = &assetList.stringList;
    LoadScriptStringList(false);

    if (assetList.depends != nullptr)
    {
        assert(GetZonePointerType(assetList.depends) == ZonePointerType::FOLLOWING);

        assetList.depends = m_stream.Alloc<const char*>(alignof(const char*));
        varXString = assetList.depends;
        LoadXStringArray(true, assetList.dependCount);
    }

    if (assetList.assets != nullptr)
    {
        assert(GetZonePointerType(assetList.assets) == ZonePointerType::FOLLOWING);

        assetList.assets = m_stream.Alloc<XAsset>(alignof(XAsset));
        varXAsset = assetList.assets;
        LoadXAssetArray(true, assetList.assetCount);
    }

    m_stream.PopBlock();
}
