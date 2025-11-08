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
        void loadXModels(T6::GfxWorld* T6GfxWorld);
        void cleanGfxWorld(T6::GfxWorld* gfxWorld);
        void loadGfxLights(T6::GfxWorld* gfxWorld);
        void loadLightGrid(T6::GfxWorld* gfxWorld);
        void loadGfxCells(T6::GfxWorld* gfxWorld);
        void loadModels(T6::GfxWorld* gfxWorld);
        bool loadReflectionProbeData(T6::GfxWorld* gfxWorld);
        bool loadLightmapData(T6::GfxWorld* gfxWorld);
        void loadSkyBox(T6::GfxWorld* T6GfxWorld, std::string& mapName);
        void loadDynEntData(T6::GfxWorld* gfxWorld);
        bool loadOutdoors(T6::GfxWorld* gfxWorld);
        void loadSunData(T6::GfxWorld* gfxWorld);
        void loadWorldBounds(T6::GfxWorld* gfxWorld);
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        AssetCreationContext& m_context;
    };
} // namespace BSP
