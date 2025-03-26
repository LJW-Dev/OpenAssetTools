#include <nlohmann/json.hpp>

#include "LoaderMapEntsT6.h"

#include "Game/T6/T6.h"

#include <cstring>

using namespace T6;

namespace
{
    class MapEntsLoader final : public AssetCreator<AssetMapEnts>
    {
    public:
        MapEntsLoader(MemoryManager& memory, ISearchPath& searchPath)
            : m_memory(memory),
              m_search_path(searchPath)
        {
        }

        AssetCreationResult CreateAsset(const std::string& assetName, AssetCreationContext& context) override
        {
            const auto file = m_search_path.Open("bsp/mapents.json");
            if (!file.IsOpen())
                return AssetCreationResult::NoAction();

            nlohmann::json j = nlohmann::json::parse(*file.m_stream);
            
            auto* mapEnts = m_memory.Alloc<MapEnts>();

            mapEnts->name = m_memory.Dup(assetName.c_str());
            mapEnts->entityString = m_memory.Dup(static_cast<std::string>(j["entityString"]).c_str());
            mapEnts->numEntityChars = strlen(mapEnts->entityString) + 1;

            mapEnts->trigger.count = j["trigger"]["modelCount"];
            mapEnts->trigger.hullCount = j["trigger"]["hullCount"];
            mapEnts->trigger.slabCount = j["trigger"]["slabCount"];
            
            for (unsigned int i = 0; i < mapEnts->trigger.count; i++)
            {
                auto currentModel = &mapEnts->trigger.models[i];
                auto currentModelData = j["trigger"]["models"][i];

                currentModel->contents = currentModelData["contents"];
                currentModel->hullCount = currentModelData["hullCount"];
                currentModel->firstHull = currentModelData["firstHull"];
            }

            for (unsigned int i = 0; i < mapEnts->trigger.hullCount; i++)
            {
                auto currentHull = &mapEnts->trigger.hulls[i];
                auto currentHullData = j["trigger"]["hulls"][i];

                currentHull->bounds.midPoint.x = currentHullData["midPoint"]["x"];
                currentHull->bounds.midPoint.y = currentHullData["midPoint"]["y"];
                currentHull->bounds.midPoint.z = currentHullData["midPoint"]["z"];
                currentHull->bounds.halfSize.x = currentHullData["halfSize"]["x"];
                currentHull->bounds.halfSize.y = currentHullData["halfSize"]["y"];
                currentHull->bounds.halfSize.z = currentHullData["halfSize"]["z"];
                currentHull->contents = currentHullData["contents"];
                currentHull->slabCount = currentHullData["slabCount"];
                currentHull->firstSlab = currentHullData["firstSlab"];
            }

            for (unsigned int i = 0; i < mapEnts->trigger.slabCount; i++)
            {
                auto currentSlab = &mapEnts->trigger.slabs[i];
                auto currentSlabData = j["trigger"]["slabs"][i];

                currentSlab->dir.x = currentSlabData["dir"]["x"];
                currentSlab->dir.y = currentSlabData["dir"]["y"];
                currentSlab->dir.z = currentSlabData["dir"]["z"];
                currentSlab->midPoint = currentSlabData["midPoint"];
                currentSlab->halfSize = currentSlabData["halfSize"];
            }

            return AssetCreationResult::Success(context.AddAsset<AssetMapEnts>(assetName, mapEnts));
        }

    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
    };
} // namespace

namespace T6
{
    std::unique_ptr<AssetCreator<AssetMapEnts>> CreateMapEntsLoader(MemoryManager& memory, ISearchPath& searchPath)
    {
        return std::make_unique<MapEntsLoader>(memory, searchPath);
    }
} // namespace T6
