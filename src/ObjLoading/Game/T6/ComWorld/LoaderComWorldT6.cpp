#include <nlohmann/json.hpp>

#include "LoaderComWorldT6.h"

#include "Game/T6/T6.h"

#include <cstring>

using namespace T6;

namespace
{
    class ComWorldLoader final : public AssetCreator<AssetComWorld>
    {
    public:
        ComWorldLoader(MemoryManager& memory, ISearchPath& searchPath)
            : m_memory(memory),
              m_search_path(searchPath)
        {
        }

        AssetCreationResult CreateAsset(const std::string& assetName, AssetCreationContext& context) override
        {
            const auto file = m_search_path.Open("bsp/comworld.json");
            if (!file.IsOpen())
                return AssetCreationResult::NoAction();

            nlohmann::json j = nlohmann::json::parse(*file.m_stream);
            
            auto* comWorld = m_memory.Alloc<ComWorld>();

            comWorld->name = m_memory.Dup(assetName.c_str());
            comWorld->isInUse = j["isInUse"];
            comWorld->primaryLightCount = j["primaryLightCount"];

            if (comWorld->primaryLightCount > 0)
                comWorld->primaryLights = m_memory.Alloc<ComPrimaryLight>(comWorld->primaryLightCount);
            else
                comWorld->primaryLights = nullptr;

            for (unsigned int i = 0; i < comWorld->primaryLightCount; i++)
            {
                auto currentLight = &comWorld->primaryLights[i];
                auto currentLightData = j["primaryLights"][i];

                currentLight->defName = m_memory.Dup(static_cast<std::string>(currentLightData["defName"]).c_str());

                currentLight->type = static_cast<unsigned char>(currentLightData["type"]);
                currentLight->canUseShadowMap = static_cast<unsigned char>(currentLightData["canUseShadowMap"]);
                currentLight->exponent = static_cast<unsigned char>(currentLightData["exponent"]);
                currentLight->priority = static_cast<unsigned char>(currentLightData["priority"]);
                currentLight->cullDist = currentLightData["cullDist"];
                currentLight->useCookie = static_cast<unsigned char>(currentLightData["useCookie"]);
                currentLight->shadowmapVolume = static_cast<unsigned char>(currentLightData["shadowmapVolume"]);

                currentLight->color.x = currentLightData["color"]["x"];
                currentLight->color.y = currentLightData["color"]["y"];
                currentLight->color.z = currentLightData["color"]["z"];

                currentLight->dir.x = currentLightData["dir"]["x"];
                currentLight->dir.y = currentLightData["dir"]["y"];
                currentLight->dir.z = currentLightData["dir"]["z"];

                currentLight->origin.x = currentLightData["origin"]["x"];
                currentLight->origin.y = currentLightData["origin"]["y"];
                currentLight->origin.z = currentLightData["origin"]["z"];

                currentLight->radius = currentLightData["radius"];
                currentLight->cosHalfFovOuter = currentLightData["cosHalfFovOuter"];
                currentLight->cosHalfFovInner = currentLightData["cosHalfFovInner"];
                currentLight->cosHalfFovExpanded = currentLightData["cosHalfFovExpanded"];
                currentLight->rotationLimit = currentLightData["rotationLimit"];
                currentLight->translationLimit = currentLightData["translationLimit"];
                currentLight->mipDistance = currentLightData["mipDistance"];
                currentLight->dAttenuation = currentLightData["dAttenuation"];
                currentLight->roundness = currentLightData["roundness"];

                currentLight->diffuseColor.x = currentLightData["diffuseColor"]["x"];
                currentLight->diffuseColor.y = currentLightData["diffuseColor"]["y"];
                currentLight->diffuseColor.z = currentLightData["diffuseColor"]["z"];
                currentLight->diffuseColor.w = currentLightData["diffuseColor"]["w"];

                currentLight->falloff.x = currentLightData["falloff"]["x"];
                currentLight->falloff.y = currentLightData["falloff"]["y"];
                currentLight->falloff.z = currentLightData["falloff"]["z"];
                currentLight->falloff.w = currentLightData["falloff"]["w"];

                currentLight->angle.x = currentLightData["angle"]["x"];
                currentLight->angle.y = currentLightData["angle"]["y"];
                currentLight->angle.z = currentLightData["angle"]["z"];
                currentLight->angle.w = currentLightData["angle"]["w"];

                currentLight->aAbB.x = currentLightData["aAbB"]["x"];
                currentLight->aAbB.y = currentLightData["aAbB"]["y"];
                currentLight->aAbB.z = currentLightData["aAbB"]["z"];
                currentLight->aAbB.w = currentLightData["aAbB"]["w"];

                currentLight->cookieControl0.x = currentLightData["cookieControl0"]["x"];
                currentLight->cookieControl0.y = currentLightData["cookieControl0"]["y"];
                currentLight->cookieControl0.z = currentLightData["cookieControl0"]["z"];
                currentLight->cookieControl0.w = currentLightData["cookieControl0"]["w"];

                currentLight->cookieControl1.x = currentLightData["cookieControl1"]["x"];
                currentLight->cookieControl1.y = currentLightData["cookieControl1"]["y"];
                currentLight->cookieControl1.z = currentLightData["cookieControl1"]["z"];
                currentLight->cookieControl1.w = currentLightData["cookieControl1"]["w"];

                currentLight->cookieControl2.x = currentLightData["cookieControl2"]["x"];
                currentLight->cookieControl2.y = currentLightData["cookieControl2"]["y"];
                currentLight->cookieControl2.z = currentLightData["cookieControl2"]["z"];
                currentLight->cookieControl2.w = currentLightData["cookieControl2"]["w"];
            }

            return AssetCreationResult::Success(context.AddAsset<AssetComWorld>(assetName, comWorld));
        }

    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
    };
} // namespace

namespace T6
{
    std::unique_ptr<AssetCreator<AssetComWorld>> CreateComWorldLoader(MemoryManager& memory, ISearchPath& searchPath)
    {
        return std::make_unique<ComWorldLoader>(memory, searchPath);
    }
} // namespace T6
