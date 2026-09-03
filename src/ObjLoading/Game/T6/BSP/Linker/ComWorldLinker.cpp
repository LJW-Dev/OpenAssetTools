#include "ComWorldLinker.h"

#include "BSP/BSPUtil.h"

#include <numbers>

using namespace T6;
using namespace BSP;

namespace
{
    class ComWorldLinkerImpl : public ComWorldLinker
    {
    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        AssetCreationContext& m_context;

    public:
        explicit ComWorldLinkerImpl(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
            : m_memory(memory),
              m_search_path(searchPath),
              m_context(context)
        {
        }

        const char* createLightDefFromImage(std::string& imageName)
        {
            const char* lightDefName;
            if (imageName.empty())
            {
                imageName = ",$white";
                lightDefName = "white_light";
            }
            else
                lightDefName = m_memory.Dup(std::format("image_{}", imageName).c_str());
            if (m_context.LoadDependency<T6::AssetLightDef>(lightDefName) != nullptr)
                return lightDefName;

            T6::GfxLightDef* lightDef = m_memory.Alloc<T6::GfxLightDef>();
            lightDef->name = lightDefName;
            lightDef->lmapLookupStart = 0;            // always 0
            lightDef->attenuation.samplerState = 115; // always 115
            auto imageAsset = m_context.LoadDependency<T6::AssetImage>(imageName);
            if (imageAsset == nullptr)
                return nullptr;
            lightDef->attenuation.image = imageAsset->Asset();
            m_context.AddAsset<T6::AssetLightDef>(lightDef->name, lightDef);

            return lightDefName;
        }

        ComWorld* linkComWorld(BSPData* bsp) override
        {
            ComWorld* comWorld = m_memory.Alloc<ComWorld>();
            comWorld->name = m_memory.Dup(bsp->bspName.c_str());
            comWorld->isInUse = 1;

            

            // first two lights are the empty light and the sun light.
            size_t totalLightCount = bsp->lights.size() + BSP_DEFAULT_LIGHT_COUNT;
            if (totalLightCount > 254)
            {
                con::error("Exceeded 254 lights in a map (count: {})", totalLightCount);
                return nullptr;
            }
            comWorld->primaryLightCount = static_cast<unsigned int>(totalLightCount);
            comWorld->primaryLights = m_memory.Alloc<ComPrimaryLight>(totalLightCount);

            for (size_t lightIdx = 0; lightIdx < totalLightCount; lightIdx++)
            {
                ComPrimaryLight* light = &comWorld->primaryLights[lightIdx];
                BSPLight* bspLight;
                if (lightIdx == EMPTY_LIGHT_INDEX)
                    continue; // first (empty) light has no data
                else if (lightIdx == SUN_LIGHT_INDEX)
                    bspLight = &bsp->sunlight;
                else
                    bspLight = &bsp->lights.at(lightIdx - BSP_DEFAULT_LIGHT_COUNT);

                light->dir.x = bspLight->forwardVector.x;
                light->dir.y = bspLight->forwardVector.y;
                light->dir.z = bspLight->forwardVector.z;
                light->diffuseColor.x = bspLight->colour.x;
                light->diffuseColor.y = bspLight->colour.y;
                light->diffuseColor.z = bspLight->colour.z;
                light->color.x = bspLight->colour.x;
                light->color.y = bspLight->colour.y;
                light->color.z = bspLight->colour.z;
                if (lightIdx == SUN_LIGHT_INDEX)
                {
                    light->type = GFX_LIGHT_TYPE_DIR;
                    continue;
                }

                switch (bspLight->type)
                {
                case LIGHT_TYPE_DIRECTIONAL:
                    light->type = GFX_LIGHT_TYPE_DIR;
                    break;
                case LIGHT_TYPE_SPOT:
                    light->type = GFX_LIGHT_TYPE_SPOT;
                    light->cosHalfFovInner = cosf(bspLight->innerConeAngle);
                    light->cosHalfFovOuter = cosf(bspLight->outerConeAngle);
                    light->cosHalfFovExpanded = cosf(bspLight->outerConeAngle);
                    break;
                case LIGHT_TYPE_POINT:
                    light->type = GFX_LIGHT_TYPE_OMNI;
                    // point lights in BO2 aren't implemented correctly, and show up similar to spot lights.
                    //  inner/outer cone values still need to be initialised or the light will not output any light
                    //  the best workaround I found was to set the lights to have a 180 degree field of view, so at least half the light shows up
                    light->cosHalfFovInner = 0;
                    light->cosHalfFovOuter = 1.57079632f * 2;
                    light->cosHalfFovExpanded = 1.57079632f * 2;
                    break;
                }
                light->defName = createLightDefFromImage(bspLight->image);
                if (light->defName == nullptr)
                {
                    con::error("failed to create lightdef with image {}", bspLight->image);
                    return nullptr;
                }
                light->angle.z = bspLight->rollAngle;
                light->origin.x = bspLight->pos.x;
                light->origin.y = bspLight->pos.y;
                light->origin.z = bspLight->pos.z;
                light->falloff.y = bspLight->range;
                light->radius = bspLight->range;
                light->mipDistance = bspLight->range;
                light->dAttenuation = bspLight->intensity;
                light->aAbB = bspLight->superEllipse;
                assert(bspLight->cullDistance <= INT16_MAX);
                light->cullDist = static_cast<int16_t>(bspLight->cullDistance);
                light->roundness = bspLight->roundness;
                light->rotationLimit = 1.0f;    // 1.0f - doesn't rotate, -1.0f - unclamped rotation
                light->translationLimit = 0.0f; // 0.0f - doesn't translate, above 0.0f - distance per game update translated
                light->canUseShadowMap = 1;     // light does not show up with this set to 0
                light->shadowmapVolume = 0;
                light->cookieControl0.z = 1.0f; // always set to 1.0f
                light->cookieControl0.w = 1.0f; // always set to 1.0f
            }

            return comWorld;
        }
    };
} // namespace

std::unique_ptr<ComWorldLinker> ComWorldLinker::Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
{
    return std::make_unique<ComWorldLinkerImpl>(memory, searchPath, context);
}
