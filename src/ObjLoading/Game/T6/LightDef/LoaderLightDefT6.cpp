#include <nlohmann/json.hpp>

#include "LoaderLightDefT6.h"

#include "Game/T6/T6.h"

#include <cstring>

using namespace T6;

namespace
{
    class AssetLightDefLoader final : public AssetCreator<AssetLightDef>
    {
    public:
        AssetLightDefLoader(MemoryManager& memory, ISearchPath& searchPath)
            : m_memory(memory),
              m_search_path(searchPath)
        {
        }

        AssetCreationResult CreateAsset(const std::string& assetName, AssetCreationContext& context) override
        {
            const auto file = m_search_path.Open("lightDef/" + assetName + ".json");
            if (!file.IsOpen())
                return AssetCreationResult::NoAction();

            nlohmann::json j = nlohmann::json::parse(*file.m_stream);
            
            auto* lightDef = m_memory.Alloc<GfxLightDef>();

            lightDef->name = m_memory.Dup(assetName.c_str());
            lightDef->lmapLookupStart = j["lmapLookupStart"];
            lightDef->attenuation.samplerState = (unsigned char)j["attenuation"]["samplerState"];

            std::string imageName = j["attenuation"]["image"];
            if (imageName.length() == 0)
            {
                printf("Image name is empty for gfx light %s\n", assetName.c_str());
                return AssetCreationResult::Failure();
            }
            auto* assetInfo = context.LoadDependency<AssetImage>(imageName);
            if (!assetInfo)
            {
                printf("can't find image %s\n", imageName.c_str());
                return AssetCreationResult::Failure();
            }
            lightDef->attenuation.image = assetInfo->Asset();

            return AssetCreationResult::Success(context.AddAsset<AssetLightDef>(assetName, lightDef));
        }

    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
    };
} // namespace

namespace T6
{
    std::unique_ptr<AssetCreator<AssetLightDef>> CreateLightDefLoader(MemoryManager& memory, ISearchPath& searchPath)
    {
        return std::make_unique<AssetLightDefLoader>(memory, searchPath);
    }
} // namespace T6
