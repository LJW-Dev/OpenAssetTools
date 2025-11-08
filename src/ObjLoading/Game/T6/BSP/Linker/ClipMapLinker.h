#pragma once

#include "../BSP.h"
#include "../BSPCalculation.h"
#include "Asset/IAssetCreator.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

namespace BSP
{
    class ClipMapLinker
    {
    public:
        ClipMapLinker(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context);
        bool linkClipMap(ZoneAssetPools* T5AssetPool, std::string& mapName, std::string& bspName, std::string& T5BSPName);

    private:
        void loadPlanes(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadMaterials(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadBrushSides(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadLeafBrushNodes(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadLeafBrushes(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadBrushVerts(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadUinds(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadBrushes(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        bool loadStaticModels(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadNodes(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadLeafs(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadVerts(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadTris(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadWalkableEdges(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadPartitions(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadAaBbs(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadSubModels(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadClusters(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadBoxHulls(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadDynEnts(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);
        void loadConstraints(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap);

        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        AssetCreationContext& m_context;
    };
} // namespace BSP
