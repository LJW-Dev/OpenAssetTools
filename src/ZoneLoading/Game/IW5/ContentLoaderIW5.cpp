#include "ContentLoaderIW5.h"

#include "Game/IW5/IW5.h"
#include "Game/IW5/XAssets/addonmapents/addonmapents_load_db.h"
#include "Game/IW5/XAssets/clipmap_t/clipmap_t_load_db.h"
#include "Game/IW5/XAssets/comworld/comworld_load_db.h"
#include "Game/IW5/XAssets/font_s/font_s_load_db.h"
#include "Game/IW5/XAssets/fxeffectdef/fxeffectdef_load_db.h"
#include "Game/IW5/XAssets/fximpacttable/fximpacttable_load_db.h"
#include "Game/IW5/XAssets/fxworld/fxworld_load_db.h"
#include "Game/IW5/XAssets/gfximage/gfximage_load_db.h"
#include "Game/IW5/XAssets/gfxlightdef/gfxlightdef_load_db.h"
#include "Game/IW5/XAssets/gfxworld/gfxworld_load_db.h"
#include "Game/IW5/XAssets/glassworld/glassworld_load_db.h"
#include "Game/IW5/XAssets/leaderboarddef/leaderboarddef_load_db.h"
#include "Game/IW5/XAssets/loadedsound/loadedsound_load_db.h"
#include "Game/IW5/XAssets/localizeentry/localizeentry_load_db.h"
#include "Game/IW5/XAssets/mapents/mapents_load_db.h"
#include "Game/IW5/XAssets/material/material_load_db.h"
#include "Game/IW5/XAssets/materialpixelshader/materialpixelshader_load_db.h"
#include "Game/IW5/XAssets/materialtechniqueset/materialtechniqueset_load_db.h"
#include "Game/IW5/XAssets/materialvertexdeclaration/materialvertexdeclaration_load_db.h"
#include "Game/IW5/XAssets/materialvertexshader/materialvertexshader_load_db.h"
#include "Game/IW5/XAssets/menudef_t/menudef_t_load_db.h"
#include "Game/IW5/XAssets/menulist/menulist_load_db.h"
#include "Game/IW5/XAssets/pathdata/pathdata_load_db.h"
#include "Game/IW5/XAssets/physcollmap/physcollmap_load_db.h"
#include "Game/IW5/XAssets/physpreset/physpreset_load_db.h"
#include "Game/IW5/XAssets/rawfile/rawfile_load_db.h"
#include "Game/IW5/XAssets/scriptfile/scriptfile_load_db.h"
#include "Game/IW5/XAssets/snd_alias_list_t/snd_alias_list_t_load_db.h"
#include "Game/IW5/XAssets/sndcurve/sndcurve_load_db.h"
#include "Game/IW5/XAssets/stringtable/stringtable_load_db.h"
#include "Game/IW5/XAssets/structureddatadefset/structureddatadefset_load_db.h"
#include "Game/IW5/XAssets/surfacefxtable/surfacefxtable_load_db.h"
#include "Game/IW5/XAssets/tracerdef/tracerdef_load_db.h"
#include "Game/IW5/XAssets/vehicledef/vehicledef_load_db.h"
#include "Game/IW5/XAssets/vehicletrack/vehicletrack_load_db.h"
#include "Game/IW5/XAssets/weaponattachment/weaponattachment_load_db.h"
#include "Game/IW5/XAssets/weaponcompletedef/weaponcompletedef_load_db.h"
#include "Game/IW5/XAssets/xanimparts/xanimparts_load_db.h"
#include "Game/IW5/XAssets/xmodel/xmodel_load_db.h"
#include "Game/IW5/XAssets/xmodelsurfs/xmodelsurfs_load_db.h"
#include "Loading/Exception/UnsupportedAssetTypeException.h"

#include <cassert>

using namespace IW5;

ContentLoader::ContentLoader(Zone& zone, ZoneInputStream& stream)
    : ContentLoaderBase(zone, stream),
      varXAssetList(nullptr),
      varXAsset(nullptr),
      varScriptStringList(nullptr)
{
}

void ContentLoader::LoadScriptStringList(const bool atStreamStart)
{
    assert(!atStreamStart);

    if (varScriptStringList->strings != nullptr)
    {
        assert(GetZonePointerType(varScriptStringList->strings) == ZonePointerType::FOLLOWING);

#ifdef ARCH_x86
        varScriptStringList->strings = m_stream.Alloc<const char*>(4);
#else
        varScriptStringList->strings = m_stream.AllocOutOfBlock<const char*>(4, varScriptStringList->count);
#endif
        varXString = varScriptStringList->strings;
        LoadXStringArray(true, varScriptStringList->count);

        if (varScriptStringList->strings && varScriptStringList->count > 0)
            m_zone.m_script_strings.InitializeForExistingZone(varScriptStringList->strings, static_cast<size_t>(varScriptStringList->count));
    }

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
#define SKIP_ASSET(type_index)                                                                                                                                 \
    case type_index:                                                                                                                                           \
        break;

    assert(varXAsset != nullptr);

    if (atStreamStart)
        m_stream.Load<XAsset>(varXAsset);

    switch (varXAsset->type)
    {
        LOAD_ASSET(ASSET_TYPE_PHYSPRESET, PhysPreset, physPreset)
        LOAD_ASSET(ASSET_TYPE_PHYSCOLLMAP, PhysCollmap, physCollmap)
        LOAD_ASSET(ASSET_TYPE_XANIMPARTS, XAnimParts, parts)
        LOAD_ASSET(ASSET_TYPE_XMODEL_SURFS, XModelSurfs, modelSurfs)
        LOAD_ASSET(ASSET_TYPE_XMODEL, XModel, model)
        LOAD_ASSET(ASSET_TYPE_MATERIAL, Material, material)
        LOAD_ASSET(ASSET_TYPE_PIXELSHADER, MaterialPixelShader, pixelShader)
        LOAD_ASSET(ASSET_TYPE_VERTEXSHADER, MaterialVertexShader, vertexShader)
        LOAD_ASSET(ASSET_TYPE_VERTEXDECL, MaterialVertexDeclaration, vertexDecl)
        LOAD_ASSET(ASSET_TYPE_TECHNIQUE_SET, MaterialTechniqueSet, techniqueSet)
        LOAD_ASSET(ASSET_TYPE_IMAGE, GfxImage, image)
        LOAD_ASSET(ASSET_TYPE_SOUND, snd_alias_list_t, sound)
        LOAD_ASSET(ASSET_TYPE_SOUND_CURVE, SndCurve, sndCurve)
        LOAD_ASSET(ASSET_TYPE_LOADED_SOUND, LoadedSound, loadSnd)
        LOAD_ASSET(ASSET_TYPE_CLIPMAP, clipMap_t, clipMap)
        LOAD_ASSET(ASSET_TYPE_COMWORLD, ComWorld, comWorld)
        LOAD_ASSET(ASSET_TYPE_GLASSWORLD, GlassWorld, glassWorld)
        LOAD_ASSET(ASSET_TYPE_PATHDATA, PathData, pathData)
        LOAD_ASSET(ASSET_TYPE_VEHICLE_TRACK, VehicleTrack, vehicleTrack)
        LOAD_ASSET(ASSET_TYPE_MAP_ENTS, MapEnts, mapEnts)
        LOAD_ASSET(ASSET_TYPE_FXWORLD, FxWorld, fxWorld)
        LOAD_ASSET(ASSET_TYPE_GFXWORLD, GfxWorld, gfxWorld)
        LOAD_ASSET(ASSET_TYPE_LIGHT_DEF, GfxLightDef, lightDef)
        LOAD_ASSET(ASSET_TYPE_FONT, Font_s, font)
        LOAD_ASSET(ASSET_TYPE_MENULIST, MenuList, menuList)
        LOAD_ASSET(ASSET_TYPE_MENU, menuDef_t, menu)
        LOAD_ASSET(ASSET_TYPE_LOCALIZE_ENTRY, LocalizeEntry, localize)
        LOAD_ASSET(ASSET_TYPE_ATTACHMENT, WeaponAttachment, attachment)
        LOAD_ASSET(ASSET_TYPE_WEAPON, WeaponCompleteDef, weapon)
        SKIP_ASSET(ASSET_TYPE_SNDDRIVER_GLOBALS)
        LOAD_ASSET(ASSET_TYPE_FX, FxEffectDef, fx)
        LOAD_ASSET(ASSET_TYPE_IMPACT_FX, FxImpactTable, impactFx)
        LOAD_ASSET(ASSET_TYPE_SURFACE_FX, SurfaceFxTable, surfaceFx)
        LOAD_ASSET(ASSET_TYPE_RAWFILE, RawFile, rawfile)
        LOAD_ASSET(ASSET_TYPE_SCRIPTFILE, ScriptFile, scriptfile)
        LOAD_ASSET(ASSET_TYPE_STRINGTABLE, StringTable, stringTable)
        LOAD_ASSET(ASSET_TYPE_LEADERBOARD, LeaderboardDef, leaderboardDef)
        LOAD_ASSET(ASSET_TYPE_STRUCTURED_DATA_DEF, StructuredDataDefSet, structuredDataDefSet)
        LOAD_ASSET(ASSET_TYPE_TRACER, TracerDef, tracerDef)
        LOAD_ASSET(ASSET_TYPE_VEHICLE, VehicleDef, vehDef)
        LOAD_ASSET(ASSET_TYPE_ADDON_MAP_ENTS, AddonMapEnts, addonMapEnts)

    default:
    {
        throw UnsupportedAssetTypeException(varXAsset->type);
    }
    }

#undef LOAD_ASSET
}

void ContentLoader::LoadXAssetArray(const bool atStreamStart, const size_t count)
{
    assert(count == 0 || varXAsset != nullptr);

    if (atStreamStart)
    {
#ifdef ARCH_x86
        m_stream.Load<XAsset>(varXAsset, count);
#else
        const auto fill = m_stream.LoadWithFill(8u * count);

        for (size_t index = 0; index < count; index++)
        {
            fill.Fill(varXAsset[index].type, 8u * index);
            fill.FillPtr(varXAsset[index].header.data, 8u * index + 4u);
            m_stream.AddPointerLookup(&varXAsset[index].header.data, fill.BlockBuffer(8u * index + 4u));
        }
#endif
    }

    for (size_t index = 0; index < count; index++)
    {
        LoadXAsset(false);
        varXAsset++;

#ifdef DEBUG_OFFSETS
        m_stream.DebugOffsets(index);
#endif
    }
}

void ContentLoader::Load()
{
    XAssetList assetList{};
    varXAssetList = &assetList;

#ifdef ARCH_x86
    m_stream.LoadDataRaw(&assetList, sizeof(assetList));
#else
    const auto fillAccessor = m_stream.LoadWithFill(16u);
    varScriptStringList = &varXAssetList->stringList;
    fillAccessor.Fill(varScriptStringList->count, 0u);
    fillAccessor.FillPtr(varScriptStringList->strings, 4u);

    fillAccessor.Fill(varXAssetList->assetCount, 8u);
    fillAccessor.FillPtr(varXAssetList->assets, 12u);
#endif

    m_stream.PushBlock(XFILE_BLOCK_VIRTUAL);

    varScriptStringList = &assetList.stringList;
    LoadScriptStringList(false);

    if (assetList.assets != nullptr)
    {
        assert(GetZonePointerType(assetList.assets) == ZonePointerType::FOLLOWING);

#ifdef ARCH_x86
        assetList.assets = m_stream.Alloc<XAsset>(4);
#else
        assetList.assets = m_stream.AllocOutOfBlock<XAsset>(4, assetList.assetCount);
#endif
        varXAsset = assetList.assets;
        LoadXAssetArray(true, assetList.assetCount);
    }

    m_stream.PopBlock();
}
