#pragma once

#include "Asset/IAssetCreator.h"
#include "Game/T5/T5.h"
#include "Game/T6/T6.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"
#include "ZoneLoading.h"
#include "ZoneWriting.h"

namespace BSP
{
    class BSPCompiler
    {
    public:
        BSPCompiler(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);
        bool compileT5MapIntoZone(std::string& T6MapName);
        bool addGfxWorld(ZoneAssetPools* T5AssetPool, std::string& T6MapName, std::string& T6BspName, std::string& T5BSPName);
        bool addClipMap(ZoneAssetPools* T5AssetPool, std::string& T6MapName, std::string& T6BspName, std::string& T5BSPName);

        void LoadSubModels(T6::clipMap_t* T6ClipMap);

        bool loadMapSurfaces(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld);
        void loadDrawData(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld);
        bool loadOutdoors(T6::GfxWorld* T6GfxWorld);
        void loadModels(T6::GfxWorld* T6GfxWorld);
        void loadWorldBounds(T6::GfxWorld* T6GfxWorld);
        void loadGfxCells(T6::GfxWorld* T6GfxWorld);

    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        AssetCreationContext& m_context;
    };
} // namespace BSP
