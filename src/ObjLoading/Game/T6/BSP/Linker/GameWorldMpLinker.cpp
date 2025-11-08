#include "GameWorldMpLinker.h"

namespace BSP
{
    GameWorldMpLinker::GameWorldMpLinker(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
        : m_memory(memory),
          m_search_path(searchPath),
          m_context(context)
    {
    }

    bool GameWorldMpLinker::linkGameWorldMp(ZoneAssetPools* T5AssetPool, std::string& mapName, std::string& bspName, std::string& T5BSPName)
    {
        // Seems to be light changes between T5 -> t6
        // TODO: Unimplemented as pathing is not important right now

        T6::GameWorldMp* gameWorldMp = m_memory.Alloc<T6::GameWorldMp>();

        gameWorldMp->name = m_memory.Dup(bspName.c_str());

        gameWorldMp->path.nodeCount = 0;
        gameWorldMp->path.originalNodeCount = 0;
        gameWorldMp->path.visBytes = 0;
        gameWorldMp->path.smoothBytes = 0;
        gameWorldMp->path.nodeTreeCount = 0;

        // The game has 128 empty nodes allocated
        int extraNodeCount = gameWorldMp->path.nodeCount + 128;
        gameWorldMp->path.nodes = m_memory.Alloc<T6::pathnode_t>(extraNodeCount);
        gameWorldMp->path.basenodes = m_memory.Alloc<T6::pathbasenode_t>(extraNodeCount);
        gameWorldMp->path.pathVis = nullptr;
        gameWorldMp->path.smoothCache = nullptr;
        gameWorldMp->path.nodeTree = nullptr;

        m_context.AddAsset<T6::AssetGameWorldMp>(gameWorldMp->name, gameWorldMp);

        return true;
    }
} // namespace BSP
