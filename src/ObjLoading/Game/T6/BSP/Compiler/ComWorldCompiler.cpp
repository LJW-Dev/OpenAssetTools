#include "ComWorldCompiler.h"

#include "Utils/Logging/Log.h"

#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace BSP
{
    ComWorldCompiler::ComWorldCompiler(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
        : m_memory(memory),
          m_search_path(searchPath),
          m_context(context)
    {
    }

    bool ComWorldCompiler::linkComWorld(ZoneAssetPools* T5AssetPool, std::string& mapName, std::string& bspName, std::string& T5BSPName)
    {
        // all lights that aren't the sunlight or default light need their own GfxLightDef asset
        T6::ComWorld* comWorld = m_memory.Alloc<T6::ComWorld>();
        comWorld->name = m_memory.Dup(bspName.c_str());
        comWorld->isInUse = 1;
        comWorld->primaryLightCount = CBSPGameConstants::BSP_DEFAULT_LIGHT_COUNT;
        comWorld->primaryLights = m_memory.Alloc<T6::ComPrimaryLight>(comWorld->primaryLightCount);

        // first (static) light is always empty

        T6::ComPrimaryLight* sunLight = &comWorld->primaryLights[1];
        const T6::vec4_t sunLightColor = CBSPEditableConstants::SUNLIGHT_COLOR;
        const T6::vec3_t sunLightDirection = CBSPEditableConstants::SUNLIGHT_DIRECTION;
        sunLight->type = T6::GFX_LIGHT_TYPE_DIR;
        sunLight->diffuseColor.r = sunLightColor.r;
        sunLight->diffuseColor.g = sunLightColor.g;
        sunLight->diffuseColor.b = sunLightColor.b;
        sunLight->diffuseColor.a = sunLightColor.a;
        sunLight->dir.x = sunLightDirection.x;
        sunLight->dir.y = sunLightDirection.y;
        sunLight->dir.z = sunLightDirection.z;

        m_context.AddAsset<T6::AssetComWorld>(comWorld->name, comWorld);

        return true;
    }
} // namespace BSP
