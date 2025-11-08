#include "BSPCompiler.h"

#include "ClipMapCompiler.h"
#include "Utils/Logging/Log.h"
#include "Utils/Pack.h"

#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace
{
    namespace CBSPGameConstants
    {
        constexpr unsigned int MAX_COLLISION_VERTS = UINT16_MAX;

        constexpr size_t MAX_AABB_TREE_CHILDREN = 128;

        enum BSPDefaultLights
        {
            STATIC_LIGHT_INDEX = 0,
            SUN_LIGHT_INDEX = 1,
            BSP_DEFAULT_LIGHT_COUNT = 2
        };

        inline const char* DEFENDER_SPAWN_POINT_NAMES[] = {"mp_ctf_spawn_allies",
                                                           "mp_ctf_spawn_allies_start",
                                                           "mp_sd_spawn_defender",
                                                           "mp_dom_spawn_allies_start",
                                                           "mp_dem_spawn_defender_start",
                                                           "mp_dem_spawn_defenderOT_start",
                                                           "mp_dem_spawn_defender",
                                                           "mp_tdm_spawn_allies_start",
                                                           "mp_tdm_spawn_team1_start",
                                                           "mp_tdm_spawn_team2_start",
                                                           "mp_tdm_spawn_team3_start"};

        inline const char* ATTACKER_SPAWN_POINT_NAMES[] = {"mp_ctf_spawn_axis",
                                                           "mp_ctf_spawn_axis_start",
                                                           "mp_sd_spawn_attacker",
                                                           "mp_dom_spawn_axis_start",
                                                           "mp_dem_spawn_attacker_start",
                                                           "mp_dem_spawn_attackerOT_start",
                                                           "mp_dem_spawn_defender",
                                                           "mp_tdm_spawn_axis_start",
                                                           "mp_tdm_spawn_team4_start",
                                                           "mp_tdm_spawn_team5_start",
                                                           "mp_tdm_spawn_team6_start"};

        inline const char* FFA_SPAWN_POINT_NAMES[] = {"mp_tdm_spawn", "mp_dm_spawn", "mp_dom_spawn"};
    } // namespace CBSPGameConstants

    // BSPLinkingConstants:
    // These values are BSP linking constants that are required for the link to be successful
    namespace CBSPLinkingConstants
    {
        constexpr const char* MISSING_IMAGE_NAME = ",mc/lambert1";
        constexpr const char* COLOR_ONLY_IMAGE_NAME = ",white";

        constexpr const char* DEFAULT_SPAWN_POINT_STRING = R"({
    "attackers": [
		{
			"origin": "0 0 0",
			"angles": "0 0 0"
		}
    ],
	"defenders": [
		{
			"origin": "0 0 0",
			"angles": "0 0 0"
		}
    ],
	"FFA": [
		{
			"origin": "0 0 0",
			"angles": "0 0 0"
		}
	]
    })";

        constexpr const char* DEFAULT_MAP_ENTS_STRING = R"({
    "entities": [
        {
            "classname": "worldspawn"
        },
        {
            "angles": "0 0 0",
            "classname": "info_player_start",
            "origin": "0 0 0"
        },
        {
            "angles": "0 0 0",
            "classname": "mp_global_intermission",
            "origin": "0 0 0"
        }
    ]
    })";
    } // namespace CBSPLinkingConstants

    namespace CBSPEditableConstants
    {
        // Default xmodel values
        // Unused as there is no support for xmodels right now
        constexpr float DEFAULT_SMODEL_CULL_DIST = 10000.0f;
        constexpr int DEFAULT_SMODEL_FLAGS = T6::STATIC_MODEL_FLAG_NO_SHADOW;
        constexpr int DEFAULT_SMODEL_LIGHT = 1;
        constexpr int DEFAULT_SMODEL_REFLECTION_PROBE = 0;

        // Default surface values
        constexpr int DEFAULT_SURFACE_LIGHT = CBSPGameConstants::SUN_LIGHT_INDEX;
        constexpr int DEFAULT_SURFACE_LIGHTMAP = 0;
        constexpr int DEFAULT_SURFACE_REFLECTION_PROBE = 0;
        constexpr int DEFAULT_SURFACE_FLAGS = (T6::GFX_SURFACE_CASTS_SUN_SHADOW | T6::GFX_SURFACE_CASTS_SHADOW);

        // material flags determine the features of a surface
        // unsure which flag type changes what right now
        // -1 results in: no running, water splashes all the time, low friction, slanted angles make you slide very fast
        // 1 results in: normal surface features, grenades work, seems normal
        constexpr int MATERIAL_SURFACE_FLAGS = 1;
        constexpr int MATERIAL_CONTENT_FLAGS = 1;

        // terrain/world flags: does not change the type of terrain or what features they have
        // from testing, as long at it isn't 0 things will work correctly
        constexpr int LEAF_TERRAIN_CONTENTS = 1;
        constexpr int WORLD_TERRAIN_CONTENTS = 1;

        // lightgrid (global) lighting colour
        // since lightgrids are not well understood, this colour is used for the R, G and B values right now
        constexpr unsigned char LIGHTGRID_COLOUR = 128;

        // Sunlight values
        constexpr T6::vec4_t SUNLIGHT_COLOR = {0.75f, 0.75f, 0.75f, 1.0f};
        constexpr T6::vec3_t SUNLIGHT_DIRECTION = {0.0f, 0.0f, 0.0f};
    }; // namespace CBSPEditableConstants

    void updateAABB(T6::vec3_t& newAABBMins, T6::vec3_t& newAABBMaxs, T6::vec3_t& AABBMins, T6::vec3_t& AABBMaxs)
    {
        if (AABBMins.x > newAABBMins.x)
            AABBMins.x = newAABBMins.x;

        if (newAABBMaxs.x > AABBMaxs.x)
            AABBMaxs.x = newAABBMaxs.x;

        if (AABBMins.y > newAABBMins.y)
            AABBMins.y = newAABBMins.y;

        if (newAABBMaxs.y > AABBMaxs.y)
            AABBMaxs.y = newAABBMaxs.y;

        if (AABBMins.z > newAABBMins.z)
            AABBMins.z = newAABBMins.z;

        if (newAABBMaxs.z > AABBMaxs.z)
            AABBMaxs.z = newAABBMaxs.z;
    }

    size_t allignBy128(size_t size)
    {
        return ((size + 127) & 0xFFFFFF80);
    }

    float distBetweenPoints(T6::vec3_t& p1, T6::vec3_t& p2)
    {
        float x = p2.x - p1.x;
        float y = p2.y - p1.y;
        float z = p2.z - p1.z;
        return sqrtf((x * x) + (y * y) + (z * z));
    }
} // namespace

namespace BSP
{
    BSPCompiler::BSPCompiler(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
        : m_memory(memory),
          m_search_path(searchPath),
          m_context(context)
    {
    }

    void BSPCompiler::loadDrawData(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        unsigned int vertexCount = T5GfxWorld->draw.vertexCount;
        T6GfxWorld->draw.vertexCount = vertexCount;
        T6GfxWorld->draw.vertexDataSize0 = static_cast<unsigned int>(vertexCount * sizeof(T6::GfxPackedWorldVertex));
        T6::GfxPackedWorldVertex* vertexBuffer = m_memory.Alloc<T6::GfxPackedWorldVertex>(vertexCount);
        for (unsigned int vertIdx = 0; vertIdx < vertexCount; vertIdx++)
        {
            T5::GfxWorldVertex* T5Vertex = &T5GfxWorld->draw.vd.vertices[vertIdx];
            T6::GfxPackedWorldVertex* T6Vertex = &vertexBuffer[vertIdx];

            T6Vertex->xyz.x = T5Vertex->xyz[0];
            T6Vertex->xyz.y = T5Vertex->xyz[1];
            T6Vertex->xyz.z = T5Vertex->xyz[2];

            T6Vertex->color.packed = T5Vertex->color.packed;
            T6Vertex->texCoord.packed = pack32::Vec2PackTexCoordsUV(T5Vertex->texCoord);
            T6Vertex->normal.packed = T5Vertex->normal.packed;
            T6Vertex->tangent.packed = T5Vertex->tangent.packed;

            // T6Vertex->binormalSign = T5Vertex->binormalSign;
            // T6Vertex->lmapCoord.packed = pack32::Vec2PackTexCoordsUV(T5Vertex->lmapCoord);
            T6Vertex->binormalSign = 0.0f;
            T6Vertex->lmapCoord.packed = 0;
        }
        T6GfxWorld->draw.vd0.data = reinterpret_cast<char*>(vertexBuffer);

        // vd1 is unused but still needs to be initialised
        // the data type varies and 0x20 is enough for all types
        T6GfxWorld->draw.vertexDataSize1 = 0x20;
        T6GfxWorld->draw.vd1.data = m_memory.Alloc<char>(T6GfxWorld->draw.vertexDataSize1);

        T6GfxWorld->draw.indexCount = T5GfxWorld->draw.indexCount;
        T6GfxWorld->draw.indices = m_memory.Alloc<uint16_t>(T5GfxWorld->draw.indexCount);
        memcpy(T6GfxWorld->draw.indices, T5GfxWorld->draw.indices, sizeof(uint16_t) * T5GfxWorld->draw.indexCount);
    }

    bool BSPCompiler::loadMapSurfaces(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        loadDrawData(T5GfxWorld, T6GfxWorld);

        // T6GfxWorld->surfaceCount = T5GfxWorld->surfaceCount;
        T6GfxWorld->surfaceCount = T5GfxWorld->dpvs.staticSurfaceCount;
        T6GfxWorld->dpvs.staticSurfaceCount = T5GfxWorld->dpvs.staticSurfaceCount;
        // unsigned int surfaceCount = T5GfxWorld->surfaceCount;
        unsigned int surfaceCount = T5GfxWorld->dpvs.staticSurfaceCount;
        unsigned int StaticSurfaceCount = T5GfxWorld->dpvs.staticSurfaceCount;

        // sortedSurfIndex is staticSurfaceCount size

        // 2
        // doesn't seem to matter what order the sorted surfs go in
        T6GfxWorld->dpvs.sortedSurfIndex = m_memory.Alloc<uint16_t>(surfaceCount);
        for (int surfIdx = 0; surfIdx < StaticSurfaceCount; surfIdx++)
            T6GfxWorld->dpvs.sortedSurfIndex[surfIdx] = T5GfxWorld->dpvs.sortedSurfIndex[surfIdx];

        // 8 - sizeof StaticSurfaceCount
        //  surface materials are written to by the game
        T6GfxWorld->dpvs.surfaceMaterials = m_memory.Alloc<T6::GfxDrawSurf_align4>(StaticSurfaceCount);

        // StaticSurfaceCount
        // set all surface types to lit opaque
        T6GfxWorld->dpvs.litSurfsBegin = 0;
        T6GfxWorld->dpvs.litSurfsEnd = static_cast<unsigned int>(StaticSurfaceCount);
        T6GfxWorld->dpvs.emissiveOpaqueSurfsBegin = static_cast<unsigned int>(StaticSurfaceCount);
        T6GfxWorld->dpvs.emissiveOpaqueSurfsEnd = static_cast<unsigned int>(StaticSurfaceCount);
        T6GfxWorld->dpvs.emissiveTransSurfsBegin = static_cast<unsigned int>(StaticSurfaceCount);
        T6GfxWorld->dpvs.emissiveTransSurfsEnd = static_cast<unsigned int>(StaticSurfaceCount);
        T6GfxWorld->dpvs.litTransSurfsBegin = static_cast<unsigned int>(StaticSurfaceCount);
        T6GfxWorld->dpvs.litTransSurfsEnd = static_cast<unsigned int>(StaticSurfaceCount);

        // 1
        // visdata is written to by the game
        // all visdata is alligned by 128
        size_t allignedSurfaceCount = allignBy128(StaticSurfaceCount);
        T6GfxWorld->dpvs.surfaceVisDataCount = static_cast<unsigned int>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceVisData[0] = m_memory.Alloc<char>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceVisData[1] = m_memory.Alloc<char>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceVisData[2] = m_memory.Alloc<char>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceVisDataCameraSaved = m_memory.Alloc<char>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceCastsShadow = m_memory.Alloc<char>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceCastsSunShadow = m_memory.Alloc<char>(allignedSurfaceCount);

        T6GfxWorld->dpvs.surfaces = m_memory.Alloc<T6::GfxSurface>(surfaceCount);
        for (int surfIdx = 0; surfIdx < surfaceCount; surfIdx++)
        {
            T5::GfxSurface* T5Surface = &T5GfxWorld->dpvs.surfaces[surfIdx];
            T6::GfxSurface* T6Surface = &T6GfxWorld->dpvs.surfaces[surfIdx];

            T6Surface->primaryLightIndex = CBSPEditableConstants::DEFAULT_SURFACE_LIGHT;
            T6Surface->lightmapIndex = CBSPEditableConstants::DEFAULT_SURFACE_LIGHTMAP;
            T6Surface->reflectionProbeIndex = CBSPEditableConstants::DEFAULT_SURFACE_REFLECTION_PROBE;
            T6Surface->flags = CBSPEditableConstants::DEFAULT_SURFACE_FLAGS;

            T6Surface->bounds[0].x = T5Surface->bounds[0][0];
            T6Surface->bounds[0].y = T5Surface->bounds[0][1];
            T6Surface->bounds[0].z = T5Surface->bounds[0][2];
            T6Surface->bounds[1].x = T5Surface->bounds[1][0];
            T6Surface->bounds[1].y = T5Surface->bounds[1][1];
            T6Surface->bounds[1].z = T5Surface->bounds[1][2];

            // auto surfMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>(T5Surface->material->info.name);
            auto surfMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>(CBSPLinkingConstants::MISSING_IMAGE_NAME);
            if (surfMaterialAsset == nullptr)
            {
                con::error("unable to load image texture {}!", T5Surface->material->info.name);
                return false;
            }
            T6Surface->material = surfMaterialAsset->Asset();

            T6Surface->tris.vertexDataOffset0 = T5Surface->tris.firstVertex * sizeof(T6::GfxPackedWorldVertex);
            T6Surface->tris.vertexDataOffset1 = 0;
            T6Surface->tris.triCount = T5Surface->tris.triCount;
            T6Surface->tris.baseIndex = T5Surface->tris.baseIndex;

            _ASSERT((T5Surface->tris.firstVertex + T5Surface->tris.vertexCount - 1) * sizeof(T6::GfxPackedWorldVertex) < T6GfxWorld->draw.vertexDataSize0);

            _ASSERT(T6Surface->tris.baseIndex + (T6Surface->tris.triCount * 3) - 1 < T5GfxWorld->draw.indexCount);

            // unused values
            T6Surface->tris.mins.x = 0.0f;
            T6Surface->tris.mins.y = 0.0f;
            T6Surface->tris.mins.z = 0.0f;
            T6Surface->tris.maxs.x = 0.0f;
            T6Surface->tris.maxs.y = 0.0f;
            T6Surface->tris.maxs.z = 0.0f;
            T6Surface->tris.himipRadiusInvSq = 0.0f;
            T6Surface->tris.vertexCount = 0;
            T6Surface->tris.firstVertex = 0;
        }

        return true;
    }

    void BSPCompiler::loadGfxCells(T6::GfxWorld* T6GfxWorld)
    {
        // Cells are basically data used to determine what can be seen and what cant be seen
        // Right now custom maps have no optimisation so there is only 1 cell
        int cellCount = 1;

        T6GfxWorld->dpvsPlanes.cellCount = cellCount;
        T6GfxWorld->cellBitsCount = ((cellCount + 127) >> 3) & 0x1FFFFFF0;

        int cellCasterBitsCount = cellCount * ((cellCount + 31) / 32);
        T6GfxWorld->cellCasterBits = m_memory.Alloc<unsigned int>(cellCasterBitsCount);

        int sceneEntCellBitsCount = cellCount * 512;
        T6GfxWorld->dpvsPlanes.sceneEntCellBits = m_memory.Alloc<unsigned int>(sceneEntCellBitsCount);

        T6GfxWorld->cells = m_memory.Alloc<T6::GfxCell>(cellCount);
        T6GfxWorld->cells[0].portalCount = 0;
        T6GfxWorld->cells[0].portals = nullptr;
        T6GfxWorld->cells[0].mins.x = T6GfxWorld->mins.x;
        T6GfxWorld->cells[0].mins.y = T6GfxWorld->mins.y;
        T6GfxWorld->cells[0].mins.z = T6GfxWorld->mins.z;
        T6GfxWorld->cells[0].maxs.x = T6GfxWorld->maxs.x;
        T6GfxWorld->cells[0].maxs.y = T6GfxWorld->maxs.y;
        T6GfxWorld->cells[0].maxs.z = T6GfxWorld->maxs.z;

        // there is only 1 reflection probe
        T6GfxWorld->cells[0].reflectionProbeCount = 1;
        T6GfxWorld->cells[0].reflectionProbes = m_memory.Alloc<char>(T6GfxWorld->cells[0].reflectionProbeCount);
        T6GfxWorld->cells[0].reflectionProbes[0] = CBSPEditableConstants::DEFAULT_SURFACE_REFLECTION_PROBE;

        // AABB trees are used to detect what should be rendered and what shouldn't
        // Just use the first AABB node to hold all models, no optimisation but all models/surfaces wil lbe drawn
        T6GfxWorld->cells[0].aabbTreeCount = 1;
        T6GfxWorld->cells[0].aabbTree = m_memory.Alloc<T6::GfxAabbTree>(T6GfxWorld->cells[0].aabbTreeCount);
        T6GfxWorld->cells[0].aabbTree[0].childCount = 0;
        T6GfxWorld->cells[0].aabbTree[0].childrenOffset = 0;
        T6GfxWorld->cells[0].aabbTree[0].startSurfIndex = 0;
        T6GfxWorld->cells[0].aabbTree[0].surfaceCount = static_cast<uint16_t>(T6GfxWorld->surfaceCount);
        T6GfxWorld->cells[0].aabbTree[0].smodelIndexCount = static_cast<uint16_t>(T6GfxWorld->dpvs.smodelCount);
        T6GfxWorld->cells[0].aabbTree[0].smodelIndexes = m_memory.Alloc<unsigned short>(T6GfxWorld->dpvs.smodelCount);
        for (unsigned short smodelIdx = 0; smodelIdx < T6GfxWorld->dpvs.smodelCount; smodelIdx++)
        {
            T6GfxWorld->cells[0].aabbTree[0].smodelIndexes[smodelIdx] = smodelIdx;
        }
        T6GfxWorld->cells[0].aabbTree[0].mins.x = T6GfxWorld->mins.x;
        T6GfxWorld->cells[0].aabbTree[0].mins.y = T6GfxWorld->mins.y;
        T6GfxWorld->cells[0].aabbTree[0].mins.z = T6GfxWorld->mins.z;
        T6GfxWorld->cells[0].aabbTree[0].maxs.x = T6GfxWorld->maxs.x;
        T6GfxWorld->cells[0].aabbTree[0].maxs.y = T6GfxWorld->maxs.y;
        T6GfxWorld->cells[0].aabbTree[0].maxs.z = T6GfxWorld->maxs.z;

        // nodes have the struct mnode_t, and there must be at least 1 node (similar to BSP nodes)
        // Nodes mnode_t.cellIndex indexes gfxWorld->cells
        // and (mnode_t.cellIndex - (world->dpvsPlanes.cellCount + 1) indexes world->dpvsPlanes.planes
        // Use only one node as there is no optimisation in custom maps
        T6GfxWorld->nodeCount = 1;
        T6GfxWorld->dpvsPlanes.nodes = m_memory.Alloc<uint16_t>(T6GfxWorld->nodeCount);
        T6GfxWorld->dpvsPlanes.nodes[0] = 1; // nodes reference cells by index + 1

        // planes are overwritten by the clipmap loading code ingame
        T6GfxWorld->planeCount = 0;
        T6GfxWorld->dpvsPlanes.planes = nullptr;
    }

    void BSPCompiler::loadWorldBounds(T6::GfxWorld* T6GfxWorld)
    {
        T6GfxWorld->mins.x = 0.0f;
        T6GfxWorld->mins.y = 0.0f;
        T6GfxWorld->mins.z = 0.0f;
        T6GfxWorld->maxs.x = 0.0f;
        T6GfxWorld->maxs.y = 0.0f;
        T6GfxWorld->maxs.z = 0.0f;

        for (int surfIdx = 0; surfIdx < T6GfxWorld->surfaceCount; surfIdx++)
        {
            updateAABB(T6GfxWorld->dpvs.surfaces[surfIdx].bounds[0], T6GfxWorld->dpvs.surfaces[surfIdx].bounds[1], T6GfxWorld->mins, T6GfxWorld->maxs);
        }
    }

    void BSPCompiler::loadModels(T6::GfxWorld* T6GfxWorld)
    {
        // Models (Submodels in the clipmap code) are used for the world and map ent collision (triggers, bomb zones, etc)
        // Right now there is only one submodel, the world sub model
        T6GfxWorld->modelCount = 1;
        T6GfxWorld->models = m_memory.Alloc<T6::GfxBrushModel>(T6GfxWorld->modelCount);

        // first model is always the world model
        T6GfxWorld->models[0].startSurfIndex = 0;
        T6GfxWorld->models[0].surfaceCount = static_cast<unsigned int>(T6GfxWorld->surfaceCount);
        T6GfxWorld->models[0].bounds[0].x = T6GfxWorld->mins.x;
        T6GfxWorld->models[0].bounds[0].y = T6GfxWorld->mins.y;
        T6GfxWorld->models[0].bounds[0].z = T6GfxWorld->mins.z;
        T6GfxWorld->models[0].bounds[1].x = T6GfxWorld->maxs.x;
        T6GfxWorld->models[0].bounds[1].y = T6GfxWorld->maxs.y;
        T6GfxWorld->models[0].bounds[1].z = T6GfxWorld->maxs.z;
        memset(&T6GfxWorld->models[0].writable, 0, sizeof(T6::GfxBrushModelWritable));

        // Other models aren't implemented yet
        // Code kept for future use
        // for (size_t i = 0; i < entityModelList.size(); i++)
        //{
        //    auto currEntModel = &gfxWorld->models[i + 1];
        //    entModelBounds currEntModelBounds = entityModelList[i];
        //
        //    currEntModel->startSurfIndex = 0;
        //    currEntModel->surfaceCount = -1; // -1 when it doesn't use map surfaces
        //    currEntModel->bounds[0].x = currEntModelBounds.mins.x;
        //    currEntModel->bounds[0].y = currEntModelBounds.mins.y;
        //    currEntModel->bounds[0].z = currEntModelBounds.mins.z;
        //    currEntModel->bounds[1].x = currEntModelBounds.maxs.x;
        //    currEntModel->bounds[1].y = currEntModelBounds.maxs.y;
        //    currEntModel->bounds[1].z = currEntModelBounds.maxs.z;
        //    memset(&gfxWorld->models[0].writable, 0, sizeof(GfxBrushModelWritable));
        //}
    }

    bool BSPCompiler::loadOutdoors(T6::GfxWorld* T6GfxWorld)
    {
        float xRecip = 1.0f / (T6GfxWorld->maxs.x - T6GfxWorld->mins.x);
        float xScale = -(xRecip * T6GfxWorld->mins.x);

        float yRecip = 1.0f / (T6GfxWorld->maxs.y - T6GfxWorld->mins.y);
        float yScale = -(yRecip * T6GfxWorld->mins.y);

        float zRecip = 1.0f / (T6GfxWorld->maxs.z - T6GfxWorld->mins.z);
        float zScale = -(zRecip * T6GfxWorld->mins.z);

        memset(T6GfxWorld->outdoorLookupMatrix, 0, sizeof(T6GfxWorld->outdoorLookupMatrix));

        T6GfxWorld->outdoorLookupMatrix[0].x = xRecip;
        T6GfxWorld->outdoorLookupMatrix[1].y = yRecip;
        T6GfxWorld->outdoorLookupMatrix[2].z = zRecip;
        T6GfxWorld->outdoorLookupMatrix[3].x = xScale;
        T6GfxWorld->outdoorLookupMatrix[3].y = yScale;
        T6GfxWorld->outdoorLookupMatrix[3].z = zScale;
        T6GfxWorld->outdoorLookupMatrix[3].w = 1.0f;

        std::string outdoorImageName = std::string("$outdoor");
        auto outdoorImageAsset = m_context.LoadDependency<T6::AssetImage>(outdoorImageName);
        if (outdoorImageAsset == nullptr)
        {
            con::error("ERROR! unable to find outdoor image $outdoor!");
            return false;
        }
        T6GfxWorld->outdoorImage = outdoorImageAsset->Asset();

        return true;
    }

    bool BSPCompiler::addGfxWorld(ZoneAssetPools* T5AssetPool, std::string& T6MapName, std::string& T6BspName, std::string& T5BSPName)
    {
        con::info("Adding T5 GFXWorld to zone.");

        auto T5GfxWorldAsset = T5AssetPool->GetAsset(T5::ASSET_TYPE_GFXWORLD, T5BSPName);
        auto T6GfxWorldAsset = m_context.LoadDependency<T6::AssetGfxWorld>(T6BspName);

        if (T5GfxWorldAsset == nullptr || T6GfxWorldAsset == nullptr)
        {
            con::error("Can't find T5 or T6 GfxWorld asset.");
            return false;
        }

        T5::GfxWorld* T5GfxWorld = static_cast<T5::GfxWorld*>(T5GfxWorldAsset->m_ptr);
        T6::GfxWorld* T6GfxWorld = T6GfxWorldAsset->Asset();

        loadMapSurfaces(T5GfxWorld, T6GfxWorld);
        loadWorldBounds(T6GfxWorld);

        loadGfxCells(T6GfxWorld);
        loadOutdoors(T6GfxWorld);
        loadModels(T6GfxWorld);

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

        bool result = addGfxWorld(t5AssetPool, T6MapName, T6BSPName, T5BSPName);
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
