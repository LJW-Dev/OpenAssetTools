#include "SkinnedVertsLinker.h"

namespace BSP
{
    SkinnedVertsLinker::SkinnedVertsLinker(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
        : m_memory(memory),
          m_search_path(searchPath),
          m_context(context)
    {
    }

    bool SkinnedVertsLinker::linkSkinnedVerts(ZoneAssetPools* T5AssetPool, std::string& mapName, std::string& bspName, std::string& T5BSPName)
    {
        // Skinned verts are new in T6
        auto gfxWorldAsset = m_context.LoadDependency<T6::AssetGfxWorld>(bspName);
        assert(gfxWorldAsset != nullptr);

        // Pretty sure maxSkinnedVerts relates to the max amount of xmodel skinned verts a map will have
        // But setting it to the world vertex count seems to work
        T6::SkinnedVertsDef* skinnedVerts = m_memory.Alloc<T6::SkinnedVertsDef>();
        skinnedVerts->name = m_memory.Dup("skinnedverts");
        skinnedVerts->maxSkinnedVerts = static_cast<unsigned int>(gfxWorldAsset->Asset()->draw.vertexCount);

        m_context.AddAsset<T6::AssetSkinnedVerts>(skinnedVerts->name, skinnedVerts);

        return true;
    }
} // namespace BSP
