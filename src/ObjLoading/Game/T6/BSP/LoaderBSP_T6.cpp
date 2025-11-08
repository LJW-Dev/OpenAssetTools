#include "LoaderBSP_T6.h"

#include "Compiler/BSPCompiler.h"

namespace
{
    using namespace BSP;

    class BSPLoader final : public IAssetCreator
    {
    public:
        BSPLoader(MemoryManager& memory, ISearchPath& searchPath, Zone& zone)
            : m_memory(memory),
              m_search_path(searchPath),
              m_zone(zone)
        {
        }

        std::optional<asset_type_t> GetHandlingAssetType() const override
        {
            // don't handle any asset types
            return std::nullopt;
        }

        AssetCreationResult CreateAsset(const std::string& assetName, AssetCreationContext& context) override
        {
            // BSP assets are added in the finalize zone step
            return AssetCreationResult::NoAction();
        }

        bool FinalizeZone(AssetCreationContext& context) override
        {
            BSPCompiler t5BSPCompiler(m_memory, m_search_path, context);
            return t5BSPCompiler.compileT5MapIntoZone(m_zone.m_name);
        }

    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        Zone& m_zone;
    };
} // namespace

namespace BSP
{
    std::unique_ptr<IAssetCreator> CreateLoaderT6(MemoryManager& memory, ISearchPath& searchPath, Zone& zone)
    {
        return std::make_unique<BSPLoader>(memory, searchPath, zone);
    }
} // namespace BSP
