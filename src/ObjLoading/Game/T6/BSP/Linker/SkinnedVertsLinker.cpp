#include "SkinnedVertsLinker.h"

using namespace T6;
using namespace BSP;

namespace
{

    class SkinnedVertsLinkerImpl : public SkinnedVertsLinker
    {
    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        AssetCreationContext& m_context;

    public:
        explicit SkinnedVertsLinkerImpl(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
            : m_memory(memory),
              m_search_path(searchPath),
              m_context(context)
        {
        }

        SkinnedVertsDef* linkSkinnedVerts(BSPData* bsp) override
        {
            // maxSkinnedVerts defines how many model verts can be drawn at once (includes viewmodel, xmodels, players, etc)
            // most MP maps use 0x24000 as their maximum but pushing it to 0x30000 seems to work and may fix some custom weapons not drawing
            SkinnedVertsDef* skinnedVerts = m_memory.Alloc<SkinnedVertsDef>();
            skinnedVerts->name = m_memory.Dup("skinnedverts");
            skinnedVerts->maxSkinnedVerts = 0x30000;

            return skinnedVerts;
        }
    };
} // namespace

std::unique_ptr<SkinnedVertsLinker> SkinnedVertsLinker::Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
{
    return std::make_unique<SkinnedVertsLinkerImpl>(memory, searchPath, context);
}
