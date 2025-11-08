#include "GfxWorldLinker.h"

#include "../BSPUtil.h"
#include "Utils/Pack.h"

namespace BSP
{
    GfxWorldLinker::GfxWorldLinker(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
        : m_memory(memory),
          m_search_path(searchPath),
          m_context(context)
    {
    }

    void GfxWorldLinker::loadDrawData(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
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
            T6Vertex->binormalSign = T5Vertex->binormalSign;
            T6Vertex->color.packed = T5Vertex->color.packed;
            T6Vertex->texCoord.packed = pack32::Vec2PackTexCoordsUV(T5Vertex->texCoord);
            T6Vertex->normal.packed = T5Vertex->normal.packed;
            T6Vertex->tangent.packed = T5Vertex->tangent.packed;
            T6Vertex->lmapCoord.packed = pack32::Vec2PackTexCoordsUV(T5Vertex->lmapCoord);
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

    bool GfxWorldLinker::loadMapSurfaces(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        loadDrawData(T5GfxWorld, T6GfxWorld);

        int surfaceCount = T5GfxWorld->surfaceCount;
        T6GfxWorld->surfaceCount = T5GfxWorld->surfaceCount;
        T6GfxWorld->dpvs.staticSurfaceCount = T5GfxWorld->dpvs.staticSurfaceCount;

        // doesn't seem to matter what order the sorted surfs go in
        T6GfxWorld->dpvs.sortedSurfIndex = m_memory.Alloc<uint16_t>(surfaceCount);
        for (size_t surfIdx = 0; surfIdx < surfaceCount; surfIdx++)
            T6GfxWorld->dpvs.sortedSurfIndex[surfIdx] = T5GfxWorld->dpvs.sortedSurfIndex[surfIdx];

        // surface materials are written to by the game
        T6GfxWorld->dpvs.surfaceMaterials = m_memory.Alloc<T6::GfxDrawSurf_align4>(surfaceCount);

        // set all surface types to lit opaque
        T6GfxWorld->dpvs.litSurfsBegin = 0;
        T6GfxWorld->dpvs.litSurfsEnd = static_cast<unsigned int>(surfaceCount);
        T6GfxWorld->dpvs.emissiveOpaqueSurfsBegin = static_cast<unsigned int>(surfaceCount);
        T6GfxWorld->dpvs.emissiveOpaqueSurfsEnd = static_cast<unsigned int>(surfaceCount);
        T6GfxWorld->dpvs.emissiveTransSurfsBegin = static_cast<unsigned int>(surfaceCount);
        T6GfxWorld->dpvs.emissiveTransSurfsEnd = static_cast<unsigned int>(surfaceCount);
        T6GfxWorld->dpvs.litTransSurfsBegin = static_cast<unsigned int>(surfaceCount);
        T6GfxWorld->dpvs.litTransSurfsEnd = static_cast<unsigned int>(surfaceCount);

        // visdata is written to by the game
        // all visdata is alligned by 128
        size_t allignedSurfaceCount = BSPUtil::allignBy128(surfaceCount);
        T6GfxWorld->dpvs.surfaceVisDataCount = static_cast<unsigned int>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceVisData[0] = m_memory.Alloc<char>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceVisData[1] = m_memory.Alloc<char>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceVisData[2] = m_memory.Alloc<char>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceVisDataCameraSaved = m_memory.Alloc<char>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceCastsShadow = m_memory.Alloc<char>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceCastsSunShadow = m_memory.Alloc<char>(allignedSurfaceCount);

        T6GfxWorld->dpvs.surfaces = m_memory.Alloc<T6::GfxSurface>(surfaceCount);
        for (size_t surfIdx = 0; surfIdx < surfaceCount; surfIdx++)
        {
            T5::GfxSurface* T5Surface = &T5GfxWorld->dpvs.surfaces[surfIdx];
            T6::GfxSurface* T6Surface = &T6GfxWorld->dpvs.surfaces[surfIdx];

            T6Surface->primaryLightIndex = BSPEditableConstants::DEFAULT_SURFACE_LIGHT;
            T6Surface->lightmapIndex = BSPEditableConstants::DEFAULT_SURFACE_LIGHTMAP;
            T6Surface->reflectionProbeIndex = BSPEditableConstants::DEFAULT_SURFACE_REFLECTION_PROBE;
            T6Surface->flags = BSPEditableConstants::DEFAULT_SURFACE_FLAGS;

            T6Surface->bounds[0].x = T5Surface->bounds[0][0];
            T6Surface->bounds[0].y = T5Surface->bounds[0][1];
            T6Surface->bounds[0].z = T5Surface->bounds[0][2];
            T6Surface->bounds[1].x = T5Surface->bounds[1][0];
            T6Surface->bounds[1].y = T5Surface->bounds[1][1];
            T6Surface->bounds[1].z = T5Surface->bounds[1][2];

            // auto surfMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>(T5Surface->material->info.name);
            auto surfMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>(BSPLinkingConstants::MISSING_IMAGE_NAME);
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

    void GfxWorldLinker::loadXModels(T6::GfxWorld* T6GfxWorld)
    {
        unsigned int modelCount = 0;
        T6GfxWorld->dpvs.smodelCount = modelCount;
        T6GfxWorld->dpvs.smodelInsts = m_memory.Alloc<T6::GfxStaticModelInst>(modelCount);
        T6GfxWorld->dpvs.smodelDrawInsts = m_memory.Alloc<T6::GfxStaticModelDrawInst>(modelCount);

        // visdata is written to by the game
        // all visdata is alligned by 128
        size_t allignedModelCount = BSPUtil::allignBy128(modelCount);
        T6GfxWorld->dpvs.smodelVisDataCount = static_cast<unsigned int>(allignedModelCount);
        T6GfxWorld->dpvs.smodelVisData[0] = m_memory.Alloc<char>(allignedModelCount);
        T6GfxWorld->dpvs.smodelVisData[1] = m_memory.Alloc<char>(allignedModelCount);
        T6GfxWorld->dpvs.smodelVisData[2] = m_memory.Alloc<char>(allignedModelCount);
        T6GfxWorld->dpvs.smodelVisDataCameraSaved = m_memory.Alloc<char>(allignedModelCount);
        T6GfxWorld->dpvs.smodelCastsShadow = m_memory.Alloc<char>(allignedModelCount);
        for (unsigned int i = 0; i < modelCount; i++)
        {
            if ((T6GfxWorld->dpvs.smodelDrawInsts[i].flags & T6::STATIC_MODEL_FLAG_NO_SHADOW) == 0)
                T6GfxWorld->dpvs.smodelCastsShadow[i] = 1;
            else
                T6GfxWorld->dpvs.smodelCastsShadow[i] = 0;
        }

        // official maps set this to 0
        T6GfxWorld->dpvs.usageCount = 0;
    }

    void GfxWorldLinker::cleanGfxWorld(T6::GfxWorld* T6GfxWorld)
    {
        // checksum is generated by the game
        T6GfxWorld->checksum = 0;

        // Remove Coronas
        T6GfxWorld->coronaCount = 0;
        T6GfxWorld->coronas = nullptr;

        // Remove exposure volumes
        T6GfxWorld->exposureVolumeCount = 0;
        T6GfxWorld->exposureVolumes = nullptr;
        T6GfxWorld->exposureVolumePlaneCount = 0;
        T6GfxWorld->exposureVolumePlanes = nullptr;

        // Remove hero lights
        T6GfxWorld->heroLightCount = 0;
        T6GfxWorld->heroLights = nullptr;
        T6GfxWorld->heroLightTreeCount = 0;
        T6GfxWorld->heroLightTree = nullptr;

        // remove LUT data
        T6GfxWorld->lutVolumeCount = 0;
        T6GfxWorld->lutVolumes = nullptr;
        T6GfxWorld->lutVolumePlaneCount = 0;
        T6GfxWorld->lutVolumePlanes = nullptr;

        // remove occluders
        T6GfxWorld->numOccluders = 0;
        T6GfxWorld->occluders = nullptr;

        // remove Siege Skins
        T6GfxWorld->numSiegeSkinInsts = 0;
        T6GfxWorld->siegeSkinInsts = nullptr;

        // remove outdoor bounds
        T6GfxWorld->numOutdoorBounds = 0;
        T6GfxWorld->outdoorBounds = nullptr;

        // remove materials
        T6GfxWorld->ropeMaterial = nullptr;
        T6GfxWorld->lutMaterial = nullptr;
        T6GfxWorld->waterMaterial = nullptr;
        T6GfxWorld->coronaMaterial = nullptr;

        // remove shadow maps
        T6GfxWorld->shadowMapVolumeCount = 0;
        T6GfxWorld->shadowMapVolumes = nullptr;
        T6GfxWorld->shadowMapVolumePlaneCount = 0;
        T6GfxWorld->shadowMapVolumePlanes = nullptr;

        // remove stream info
        T6GfxWorld->streamInfo.aabbTreeCount = 0;
        T6GfxWorld->streamInfo.aabbTrees = nullptr;
        T6GfxWorld->streamInfo.leafRefCount = 0;
        T6GfxWorld->streamInfo.leafRefs = nullptr;

        // remove sun data
        memset(&T6GfxWorld->sun, 0, sizeof(T6::sunflare_t));
        T6GfxWorld->sun.hasValidData = false;

        // Remove Water
        T6GfxWorld->waterDirection = 0.0f;
        T6GfxWorld->waterBuffers[0].bufferSize = 0;
        T6GfxWorld->waterBuffers[0].buffer = nullptr;
        T6GfxWorld->waterBuffers[1].bufferSize = 0;
        T6GfxWorld->waterBuffers[1].buffer = nullptr;

        // Remove Fog
        T6GfxWorld->worldFogModifierVolumeCount = 0;
        T6GfxWorld->worldFogModifierVolumes = nullptr;
        T6GfxWorld->worldFogModifierVolumePlaneCount = 0;
        T6GfxWorld->worldFogModifierVolumePlanes = nullptr;
        T6GfxWorld->worldFogVolumeCount = 0;
        T6GfxWorld->worldFogVolumes = nullptr;
        T6GfxWorld->worldFogVolumePlaneCount = 0;
        T6GfxWorld->worldFogVolumePlanes = nullptr;

        // materialMemory is unused
        T6GfxWorld->materialMemoryCount = 0;
        T6GfxWorld->materialMemory = nullptr;

        // sunLight is overwritten by the game, just needs to be a valid pointer
        T6GfxWorld->sunLight = m_memory.Alloc<T6::GfxLight>();
    }

    void GfxWorldLinker::loadGfxLights(T6::GfxWorld* T6GfxWorld)
    {
        // there must be 2 or more lights, first is the static light and second is the sun light
        T6GfxWorld->primaryLightCount = BSPGameConstants::BSP_DEFAULT_LIGHT_COUNT;
        T6GfxWorld->sunPrimaryLightIndex = BSPGameConstants::SUN_LIGHT_INDEX;

        T6GfxWorld->shadowGeom = m_memory.Alloc<T6::GfxShadowGeometry>(T6GfxWorld->primaryLightCount);
        for (unsigned int lightIdx = 0; lightIdx < T6GfxWorld->primaryLightCount; lightIdx++)
        {
            T6GfxWorld->shadowGeom[lightIdx].smodelCount = 0;
            T6GfxWorld->shadowGeom[lightIdx].surfaceCount = 0;
            T6GfxWorld->shadowGeom[lightIdx].smodelIndex = nullptr;
            T6GfxWorld->shadowGeom[lightIdx].sortedSurfIndex = nullptr;
        }

        T6GfxWorld->lightRegion = m_memory.Alloc<T6::GfxLightRegion>(T6GfxWorld->primaryLightCount);
        for (unsigned int lightIdx = 0; lightIdx < T6GfxWorld->primaryLightCount; lightIdx++)
        {
            T6GfxWorld->lightRegion[lightIdx].hullCount = 0;
            T6GfxWorld->lightRegion[lightIdx].hulls = nullptr;
        }

        unsigned int lightEntShadowVisSize = (T6GfxWorld->primaryLightCount - T6GfxWorld->sunPrimaryLightIndex - 1) * 8192;
        if (lightEntShadowVisSize != 0)
            T6GfxWorld->primaryLightEntityShadowVis = m_memory.Alloc<unsigned int>(lightEntShadowVisSize);
        else
            T6GfxWorld->primaryLightEntityShadowVis = nullptr;
    }

    void GfxWorldLinker::loadLightGrid(T6::GfxWorld* T6GfxWorld)
    {
        // there is almost no basis for the values in this code, they were chosen based on what looks correct when reverse engineering.

        // mins and maxs define the range that the lightgrid will work in.
        // unknown how these values are calculated, but the below values are larger
        // than official map values
        T6GfxWorld->lightGrid.mins[0] = 0;
        T6GfxWorld->lightGrid.mins[1] = 0;
        T6GfxWorld->lightGrid.mins[2] = 0;
        T6GfxWorld->lightGrid.maxs[0] = 200;
        T6GfxWorld->lightGrid.maxs[1] = 200;
        T6GfxWorld->lightGrid.maxs[2] = 50;

        T6GfxWorld->lightGrid.rowAxis = 0; // default value
        T6GfxWorld->lightGrid.colAxis = 1; // default value
        T6GfxWorld->lightGrid.sunPrimaryLightIndex = BSPGameConstants::SUN_LIGHT_INDEX;
        T6GfxWorld->lightGrid.offset = 0.0f; // default value

        // setting all rowDataStart indexes to 0 will always index the first row in rawRowData
        int rowDataStartSize = T6GfxWorld->lightGrid.maxs[T6GfxWorld->lightGrid.rowAxis] - T6GfxWorld->lightGrid.mins[T6GfxWorld->lightGrid.rowAxis] + 1;
        T6GfxWorld->lightGrid.rowDataStart = m_memory.Alloc<uint16_t>(rowDataStartSize);

        // Adding 0x0F so the lookup table will be 0x10 bytes in size
        T6GfxWorld->lightGrid.rawRowDataSize = static_cast<unsigned int>(sizeof(T6::GfxLightGridRow) + 0x0F);
        T6::GfxLightGridRow* row = static_cast<T6::GfxLightGridRow*>(m_memory.AllocRaw(T6GfxWorld->lightGrid.rawRowDataSize));
        row->colStart = 0;
        row->colCount = 0x1000; // 0x1000 as this is large enough for all checks done by the game
        row->zStart = 0;
        row->zCount = 0xFF; // 0xFF as this is large enough for all checks done by the game, but small enough not to mess with other checks
        row->firstEntry = 0;
        for (int i = 0; i < 0x10; i++) // set the lookup table to all 0
            row->lookupTable[i] = 0;
        T6GfxWorld->lightGrid.rawRowData = reinterpret_cast<T6::aligned_byte_pointer*>(row);

        // entries are looked up based on the lightgrid sample pos (given ingame) and the lightgrid lookup table
        T6GfxWorld->lightGrid.entryCount = 60000; // 60000 as it should be enough entries to be indexed by all lightgrid sample positions
        T6::GfxLightGridEntry* entryArray = m_memory.Alloc<T6::GfxLightGridEntry>(T6GfxWorld->lightGrid.entryCount);
        for (unsigned int i = 0; i < T6GfxWorld->lightGrid.entryCount; i++)
        {
            entryArray[i].colorsIndex = 0; // always index first colour
            entryArray[i].primaryLightIndex = BSPGameConstants::SUN_LIGHT_INDEX;
            entryArray[i].visibility = 0;
        }
        T6GfxWorld->lightGrid.entries = entryArray;

        // colours are looked up with a lightgrid entries colorsIndex
        T6GfxWorld->lightGrid.colorCount = 0x1000; // 0x1000 as it should be enough to hold every index
        T6GfxWorld->lightGrid.colors = m_memory.Alloc<T6::GfxCompressedLightGridColors>(T6GfxWorld->lightGrid.colorCount);
        memset(T6GfxWorld->lightGrid.colors, BSPEditableConstants::LIGHTGRID_COLOUR, rowDataStartSize * sizeof(uint16_t));

        // we use the colours array instead of coeffs array
        T6GfxWorld->lightGrid.coeffCount = 0;
        T6GfxWorld->lightGrid.coeffs = nullptr;
        T6GfxWorld->lightGrid.skyGridVolumeCount = 0;
        T6GfxWorld->lightGrid.skyGridVolumes = nullptr;
    }

    void GfxWorldLinker::loadGfxCells(T6::GfxWorld* T6GfxWorld)
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
        T6GfxWorld->cells[0].reflectionProbes[0] = BSPEditableConstants::DEFAULT_SURFACE_REFLECTION_PROBE;

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

    void GfxWorldLinker::loadWorldBounds(T6::GfxWorld* T6GfxWorld)
    {
        T6GfxWorld->mins.x = 0.0f;
        T6GfxWorld->mins.y = 0.0f;
        T6GfxWorld->mins.z = 0.0f;
        T6GfxWorld->maxs.x = 0.0f;
        T6GfxWorld->maxs.y = 0.0f;
        T6GfxWorld->maxs.z = 0.0f;

        for (int surfIdx = 0; surfIdx < T6GfxWorld->surfaceCount; surfIdx++)
        {
            BSPUtil::updateAABB(T6GfxWorld->dpvs.surfaces[surfIdx].bounds[0], T6GfxWorld->dpvs.surfaces[surfIdx].bounds[1], T6GfxWorld->mins, T6GfxWorld->maxs);
        }
    }

    void GfxWorldLinker::loadModels(T6::GfxWorld* T6GfxWorld)
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

    void GfxWorldLinker::loadSunData(T6::GfxWorld* T6GfxWorld)
    {
        // default values taken from mp_dig
        T6GfxWorld->sunParse.fogTransitionTime = 0.001f;
        T6GfxWorld->sunParse.name[0] = 0x00;

        T6GfxWorld->sunParse.initWorldSun->control = 0;
        T6GfxWorld->sunParse.initWorldSun->exposure = 2.5f;
        T6GfxWorld->sunParse.initWorldSun->angles.x = -29.0f;
        T6GfxWorld->sunParse.initWorldSun->angles.y = 254.0f;
        T6GfxWorld->sunParse.initWorldSun->angles.z = 0.0f;
        T6GfxWorld->sunParse.initWorldSun->sunCd.x = 1.0f;
        T6GfxWorld->sunParse.initWorldSun->sunCd.y = 0.89f;
        T6GfxWorld->sunParse.initWorldSun->sunCd.z = 0.69f;
        T6GfxWorld->sunParse.initWorldSun->sunCd.w = 13.5f;
        T6GfxWorld->sunParse.initWorldSun->ambientColor.x = 0.0f;
        T6GfxWorld->sunParse.initWorldSun->ambientColor.y = 0.0f;
        T6GfxWorld->sunParse.initWorldSun->ambientColor.z = 0.0f;
        T6GfxWorld->sunParse.initWorldSun->ambientColor.w = 0.0f;
        T6GfxWorld->sunParse.initWorldSun->skyColor.x = 0.0f;
        T6GfxWorld->sunParse.initWorldSun->skyColor.y = 0.0f;
        T6GfxWorld->sunParse.initWorldSun->skyColor.z = 0.0f;
        T6GfxWorld->sunParse.initWorldSun->skyColor.w = 0.0f;
        T6GfxWorld->sunParse.initWorldSun->sunCs.x = 0.0f;
        T6GfxWorld->sunParse.initWorldSun->sunCs.y = 0.0f;
        T6GfxWorld->sunParse.initWorldSun->sunCs.z = 0.0f;
        T6GfxWorld->sunParse.initWorldSun->sunCs.w = 0.0f;

        T6GfxWorld->sunParse.initWorldFog->baseDist = 150.0f;
        T6GfxWorld->sunParse.initWorldFog->baseHeight = -100.0f;
        T6GfxWorld->sunParse.initWorldFog->fogColor.x = 2.35f;
        T6GfxWorld->sunParse.initWorldFog->fogColor.y = 3.10f;
        T6GfxWorld->sunParse.initWorldFog->fogColor.z = 3.84f;
        T6GfxWorld->sunParse.initWorldFog->fogOpacity = 0.52f;
        T6GfxWorld->sunParse.initWorldFog->halfDist = 4450.f;
        T6GfxWorld->sunParse.initWorldFog->halfHeight = 2000.f;
        T6GfxWorld->sunParse.initWorldFog->sunFogColor.x = 5.27f;
        T6GfxWorld->sunParse.initWorldFog->sunFogColor.y = 4.73f;
        T6GfxWorld->sunParse.initWorldFog->sunFogColor.z = 3.88f;
        T6GfxWorld->sunParse.initWorldFog->sunFogInner = 0.0f;
        T6GfxWorld->sunParse.initWorldFog->sunFogOpacity = 0.67f;
        T6GfxWorld->sunParse.initWorldFog->sunFogOuter = 80.84f;
        T6GfxWorld->sunParse.initWorldFog->sunFogPitch = -29.0f;
        T6GfxWorld->sunParse.initWorldFog->sunFogYaw = 254.0f;
    }

    bool GfxWorldLinker::loadReflectionProbeData(T6::GfxWorld* T6GfxWorld)
    {
        T6GfxWorld->draw.reflectionProbeCount = 1;

        T6GfxWorld->draw.reflectionProbeTextures = m_memory.Alloc<T6::GfxTexture>(T6GfxWorld->draw.reflectionProbeCount);

        // default values taken from mp_dig
        T6GfxWorld->draw.reflectionProbes = m_memory.Alloc<T6::GfxReflectionProbe>(T6GfxWorld->draw.reflectionProbeCount);
        T6GfxWorld->draw.reflectionProbes[0].mipLodBias = -8.0;
        T6GfxWorld->draw.reflectionProbes[0].origin.x = 0.0f;
        T6GfxWorld->draw.reflectionProbes[0].origin.y = 0.0f;
        T6GfxWorld->draw.reflectionProbes[0].origin.z = 0.0f;
        T6GfxWorld->draw.reflectionProbes[0].lightingSH.V0.x = 0.0f;
        T6GfxWorld->draw.reflectionProbes[0].lightingSH.V0.y = 0.0f;
        T6GfxWorld->draw.reflectionProbes[0].lightingSH.V0.z = 0.0f;
        T6GfxWorld->draw.reflectionProbes[0].lightingSH.V0.w = 0.0f;
        T6GfxWorld->draw.reflectionProbes[0].lightingSH.V1.x = 0.0f;
        T6GfxWorld->draw.reflectionProbes[0].lightingSH.V1.y = 0.0f;
        T6GfxWorld->draw.reflectionProbes[0].lightingSH.V1.z = 0.0f;
        T6GfxWorld->draw.reflectionProbes[0].lightingSH.V1.w = 0.0f;
        T6GfxWorld->draw.reflectionProbes[0].lightingSH.V2.x = 0.0f;
        T6GfxWorld->draw.reflectionProbes[0].lightingSH.V2.y = 0.0f;
        T6GfxWorld->draw.reflectionProbes[0].lightingSH.V2.z = 0.0f;
        T6GfxWorld->draw.reflectionProbes[0].lightingSH.V2.w = 0.0f;

        T6GfxWorld->draw.reflectionProbes[0].probeVolumeCount = 0;
        T6GfxWorld->draw.reflectionProbes[0].probeVolumes = nullptr;

        std::string probeImageName = "reflection_probe0";
        auto probeImageAsset = m_context.LoadDependency<T6::AssetImage>(probeImageName);
        if (probeImageAsset == nullptr)
        {
            con::error("ERROR! unable to find reflection probe image {}!", probeImageName);
            return false;
        }
        T6GfxWorld->draw.reflectionProbes[0].reflectionImage = probeImageAsset->Asset();

        return true;
    }

    bool GfxWorldLinker::loadLightmapData(T6::GfxWorld* T6GfxWorld)
    {
        T6GfxWorld->draw.lightmapCount = 1;

        T6GfxWorld->draw.lightmapPrimaryTextures = m_memory.Alloc<T6::GfxTexture>(T6GfxWorld->draw.lightmapCount);
        T6GfxWorld->draw.lightmapSecondaryTextures = m_memory.Alloc<T6::GfxTexture>(T6GfxWorld->draw.lightmapCount);

        std::string secondaryTexture = "lightmap0_secondary";
        auto secondaryTextureAsset = m_context.LoadDependency<T6::AssetImage>(secondaryTexture);
        if (secondaryTextureAsset == nullptr)
        {
            con::error("ERROR! unable to find lightmap image {}!", secondaryTexture);
            return false;
        }
        T6GfxWorld->draw.lightmaps = m_memory.Alloc<T6::GfxLightmapArray>(T6GfxWorld->draw.lightmapCount);
        T6GfxWorld->draw.lightmaps[0].primary = nullptr; // always nullptr
        T6GfxWorld->draw.lightmaps[0].secondary = secondaryTextureAsset->Asset();

        return true;
    }

    void GfxWorldLinker::loadSkyBox(T6::GfxWorld* T6GfxWorld, std::string& mapName)
    {
        std::string skyBoxName = "skybox_" + mapName;
        T6GfxWorld->skyBoxModel = m_memory.Dup(skyBoxName.c_str());

        if (m_context.LoadDependency<T6::AssetXModel>(skyBoxName) == nullptr)
        {
            con::warn("WARN: Unable to load the skybox xmodel {}!", skyBoxName);
        }

        // default skybox values from mp_dig
        T6GfxWorld->skyDynIntensity.angle0 = 0.0f;
        T6GfxWorld->skyDynIntensity.angle1 = 0.0f;
        T6GfxWorld->skyDynIntensity.factor0 = 1.0f;
        T6GfxWorld->skyDynIntensity.factor1 = 1.0f;
    }

    void GfxWorldLinker::loadDynEntData(T6::GfxWorld* T6fxWorld)
    {
        int dynEntCount = 0;
        // T6fxWorld->dpvsDyn.dynEntClientCount[0] = dynEntCount + 256; // the game allocs 256 empty dynents, as they may be used ingame
        T6fxWorld->dpvsDyn.dynEntClientCount[0] = dynEntCount;
        T6fxWorld->dpvsDyn.dynEntClientCount[1] = 0;

        // +100: there is a crash that happens when regdolls are created, and dynEntClientWordCount[0] is the issue.
        // Making the value much larger than required fixes it, but unsure what the root cause is
        T6fxWorld->dpvsDyn.dynEntClientWordCount[0] = ((T6fxWorld->dpvsDyn.dynEntClientCount[0] + 31) >> 5) + 100;
        T6fxWorld->dpvsDyn.dynEntClientWordCount[1] = 0;
        T6fxWorld->dpvsDyn.usageCount = 0;

        int dynEntCellBitsSize = T6fxWorld->dpvsDyn.dynEntClientWordCount[0] * T6fxWorld->dpvsPlanes.cellCount;
        T6fxWorld->dpvsDyn.dynEntCellBits[0] = m_memory.Alloc<unsigned int>(dynEntCellBitsSize);
        T6fxWorld->dpvsDyn.dynEntCellBits[1] = nullptr;

        int dynEntVisData0Size = T6fxWorld->dpvsDyn.dynEntClientWordCount[0] * 32;
        T6fxWorld->dpvsDyn.dynEntVisData[0][0] = m_memory.Alloc<char>(dynEntVisData0Size);
        T6fxWorld->dpvsDyn.dynEntVisData[0][1] = m_memory.Alloc<char>(dynEntVisData0Size);
        T6fxWorld->dpvsDyn.dynEntVisData[0][2] = m_memory.Alloc<char>(dynEntVisData0Size);
        T6fxWorld->dpvsDyn.dynEntVisData[1][0] = nullptr;
        T6fxWorld->dpvsDyn.dynEntVisData[1][1] = nullptr;
        T6fxWorld->dpvsDyn.dynEntVisData[1][2] = nullptr;

        unsigned int dynEntShadowVisCount = T6fxWorld->dpvsDyn.dynEntClientCount[0] * (T6fxWorld->primaryLightCount - T6fxWorld->sunPrimaryLightIndex - 1);
        T6fxWorld->primaryLightDynEntShadowVis[0] = m_memory.Alloc<unsigned int>(dynEntShadowVisCount);
        T6fxWorld->primaryLightDynEntShadowVis[1] = nullptr;

        T6fxWorld->sceneDynModel = m_memory.Alloc<T6::GfxSceneDynModel>(T6fxWorld->dpvsDyn.dynEntClientCount[0]);
        T6fxWorld->sceneDynBrush = nullptr;
    }

    bool GfxWorldLinker::loadOutdoors(T6::GfxWorld* T6GfxWorld)
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

    bool GfxWorldLinker::linkGfxWorld(ZoneAssetPools* T5AssetPool, std::string& mapName, std::string& bspName, std::string& T5BSPName)
    {
        auto T5GfxWorldAsset = T5AssetPool->GetAsset(T5::ASSET_TYPE_GFXWORLD, T5BSPName);
        if (T5GfxWorldAsset == nullptr)
        {
            con::error("Can't find T5 GfxWorld asset.");
            return false;
        }
        T5::GfxWorld* T5GfxWorld = static_cast<T5::GfxWorld*>(T5GfxWorldAsset->m_ptr);
        T6::GfxWorld* T6GfxWorld = m_memory.Alloc<T6::GfxWorld>();

        T6GfxWorld->baseName = m_memory.Dup(mapName.c_str());
        T6GfxWorld->name = m_memory.Dup(bspName.c_str());

        // Default values taken from official maps
        T6GfxWorld->lightingFlags = 0;
        T6GfxWorld->lightingQuality = 4096;

        cleanGfxWorld(T6GfxWorld);

        if (!loadMapSurfaces(T5GfxWorld, T6GfxWorld))
            return false;

        loadXModels(T6GfxWorld);

        if (!loadLightmapData(T6GfxWorld))
            return false;

        loadSkyBox(T6GfxWorld, mapName);

        if (!loadReflectionProbeData(T6GfxWorld))
            return false;

        // world bounds are based on loaded surface mins/maxs
        loadWorldBounds(T6GfxWorld);

        if (!loadOutdoors(T6GfxWorld))
            return false;

        // gfx cells depend on surface/smodel count
        loadGfxCells(T6GfxWorld);

        loadLightGrid(T6GfxWorld);

        loadGfxLights(T6GfxWorld);

        loadModels(T6GfxWorld);

        loadSunData(T6GfxWorld);

        loadDynEntData(T6GfxWorld);

        m_context.AddAsset<T6::AssetGfxWorld>(T6GfxWorld->name, T6GfxWorld);

        return true;
    }
} // namespace BSP
