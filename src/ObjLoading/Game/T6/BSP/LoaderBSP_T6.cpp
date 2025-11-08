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
            const std::string fastFilePath = "C:\\Users\\LJ\\Documents\\zombie_tutorial7.ff";
            auto t5Zone = ZoneLoading::LoadZone(fastFilePath, std::nullopt);

            if (!t5Zone)
            {
                con::error("Unable to open T5 FastFile {}.", fastFilePath);
                return false;
            }

            auto t5AssetPool = t5Zone.value()->m_pools.get();

            std::string mapName = m_zone.m_name;

            std::string T5BSPName = "maps/" + t5Zone.value()->m_name + ".d3dbsp";

            BSPLinker linker(m_memory, m_search_path, context);
            return linker.linkBSP(t5AssetPool, mapName, T5BSPName);
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
