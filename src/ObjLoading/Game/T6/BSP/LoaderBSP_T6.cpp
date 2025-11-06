#include "LoaderBSP_T6.h"

#include "BSPCreator.h"
#include "BSPUtil.h"
#include "Linker/BSPLinker.h"
#include "ZoneLoading.h"
#include "ZoneWriting.h"

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
            const std::string fastFilePath = "";
            auto t5Zone = ZoneLoading::LoadZone(fastFilePath, std::nullopt);
            auto t5AssetPool = t5Zone.value()->m_pools.get();

            std::string bspName = "maps/mp/" + t5Zone.value()->m_name + ".d3dbsp";

            return true;
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
