#include <nlohmann/json.hpp>

#include "LoaderSkinnedVertsT6.h"

#include "Game/T6/T6.h"

#include <cstring>

using namespace T6;

namespace
{
    class SkinnedVertsLoader final : public AssetCreator<AssetSkinnedVerts>
    {
    public:
        SkinnedVertsLoader(MemoryManager& memory, ISearchPath& searchPath)
            : m_memory(memory),
              m_search_path(searchPath)
        {
        }

        AssetCreationResult CreateAsset(const std::string& assetName, AssetCreationContext& context) override
        {
            const auto file = m_search_path.Open("bsp/skinnedverts.json");
            if (!file.IsOpen())
                return AssetCreationResult::NoAction();

            nlohmann::json j = nlohmann::json::parse(*file.m_stream);
            
            auto* skinnedVerts = m_memory.Alloc<SkinnedVertsDef>();

            skinnedVerts->name = m_memory.Dup(assetName.c_str());
            skinnedVerts->maxSkinnedVerts = j["maxSkinnedVerts"];

            return AssetCreationResult::Success(context.AddAsset<AssetSkinnedVerts>(assetName, skinnedVerts));
        }

    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
    };
} // namespace

namespace T6
{
    std::unique_ptr<AssetCreator<AssetSkinnedVerts>> CreateSkinnedVertsLoader(MemoryManager& memory, ISearchPath& searchPath)
    {
        return std::make_unique<SkinnedVertsLoader>(memory, searchPath);
    }
} // namespace T6
