#pragma once

#include "Asset/IAssetCreator.h"
#include "Compiler.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

namespace BSP
{
    class GfxWorldCompiler
    {
    public:
        GfxWorldCompiler(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);
        bool linkGfxWorld(ZoneAssetPools* T5AssetPool, std::string& mapName, std::string& bspName, std::string& T5BSPName);

    private:
        void loadDrawData(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld);
        bool loadMapSurfaces(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld);
        void loadXModels(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld);
        void loadCoronas(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadExposureVolumes(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadHeroLights(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadLUT(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadOccluders(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadSiegeSkins(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadOutdoorBounds(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        bool loadMaterials(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadShadowMaps(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadStreamInfo(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadWater(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadFog(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadLightRegionHulls(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadGfxLights(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadLightGrid(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadGfxCells(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadModels(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        bool loadReflectionProbeData(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        bool loadLightmapData(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadSkyBox(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld, std::string& mapName);
        void loadDynEntData(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        bool loadOutdoors(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        bool loadSunData(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        void loadWorldBounds(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* gfxWorld);
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        AssetCreationContext& m_context;
    };
} // namespace BSP
