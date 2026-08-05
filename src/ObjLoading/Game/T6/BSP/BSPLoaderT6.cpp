#include "BSPLoaderT6.h"

#include "BSPCreator.h"
#include "Linker/BSPLinker.h"

using namespace T6;
using namespace BSP;

namespace
{
    class BSPLoader final : public IAssetCreator
    {
    public:
        BSPLoader(MemoryManager& memory, ISearchPath& searchPath, Zone& zone, ZoneDefinitionMapType mapType)
            : m_memory(memory),
              m_search_path(searchPath),
              m_zone(zone),
              m_mapType(mapType)
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
            if (m_mapType == ZoneDefinitionMapType::SP)
            {
                con::error("BSP Loader: Singleplayer maps are not supported right now.");
                return false;
            }

            std::unique_ptr<BSPData> bsp = createBSPData(m_zone.m_name, m_search_path, m_mapType == ZoneDefinitionMapType::ZM);
            if (bsp == nullptr)
                return false;

            std::unique_ptr<BSPLinker> linker = BSPLinker::Create(m_memory, m_search_path, context);
            bool result = linker->linkBSP(bsp.get());
            if (!result)
                con::error("BSP linker has failed.");
            else
                con::info("BSP linker completed successfully.");

            return result;
        }

    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        Zone& m_zone;
        ZoneDefinitionMapType m_mapType;
    };
} // namespace

std::unique_ptr<IAssetCreator> T6::BSP::CreateLoaderT6(MemoryManager& memory, ISearchPath& searchPath, Zone& zone, ZoneDefinitionMapType mapType)
{
    return std::make_unique<BSPLoader>(memory, searchPath, zone, mapType);
}
