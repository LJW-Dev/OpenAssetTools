#include "ComWorldLinker.h"

namespace BSP
{
    ComWorldLinker::ComWorldLinker(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
        : m_memory(memory),
          m_search_path(searchPath),
          m_context(context)
    {
    }

    bool ComWorldLinker::linkComWorld(ZoneAssetPools* T5AssetPool, std::string& mapName, std::string& bspName, std::string& T5BSPName)
    {
        // all lights that aren't the sunlight or default light need their own GfxLightDef asset
        T6::ComWorld* comWorld = m_memory.Alloc<T6::ComWorld>();
        comWorld->name = m_memory.Dup(bspName.c_str());
        comWorld->isInUse = 1;
        comWorld->primaryLightCount = BSPGameConstants::BSP_DEFAULT_LIGHT_COUNT;
        comWorld->primaryLights = m_memory.Alloc<T6::ComPrimaryLight>(comWorld->primaryLightCount);

        // first (static) light is always empty

        T6::ComPrimaryLight* sunLight = &comWorld->primaryLights[1];
        const T6::vec4_t sunLightColor = BSPEditableConstants::SUNLIGHT_COLOR;
        const T6::vec3_t sunLightDirection = BSPEditableConstants::SUNLIGHT_DIRECTION;
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

    /*
    bool ComWorldLinker::linkLightDef(ZoneAssetPools* T5AssetPool, T5::GfxLightDef* T5LightDef)
    {
        T6::GfxLightDef* T6LightDef = m_memory.Alloc<T6::GfxLightDef>();
        T6LightDef->name = m_memory.Dup(T5LightDef->name);
        T6LightDef->lmapLookupStart = T5LightDef->lmapLookupStart;
        T6LightDef->attenuation.samplerState = T5LightDef->attenuation.samplerState;

        auto T6LightDefImage = m_context.LoadDependency<T6::AssetImage>(T5LightDef->attenuation.image->name);
        if (T6LightDefImage == nullptr)
            return false;
        T6LightDef->attenuation.image = T6LightDefImage->Asset();

        m_context.AddAsset<T6::AssetLightDef>(T6LightDef->name, T6LightDef);

        return true;
    }

    bool ComWorldLinker::linkComWorld(ZoneAssetPools* T5AssetPool, std::string& mapName, std::string& bspName, std::string& T5BSPName)
    {
        auto T5ComWorldAsset = T5AssetPool->GetAsset(T5::ASSET_TYPE_COMWORLD, T5BSPName);
        if (T5ComWorldAsset == nullptr)
        {
            con::error("Can't find T5 ComWorld asset.");
            return false;
        }
        T5::ComWorld* T5ComWorld = static_cast<T5::ComWorld*>(T5ComWorldAsset->m_ptr);

        T6::ComWorld* T6ComWorld = m_memory.Alloc<T6::ComWorld>();
        T6ComWorld->name = m_memory.Dup(bspName.c_str());
        T6ComWorld->isInUse = T5ComWorld->isInUse;
        T6ComWorld->primaryLightCount = T5ComWorld->primaryLightCount;
        T6ComWorld->primaryLights = m_memory.Alloc<T6::ComPrimaryLight>(T5ComWorld->primaryLightCount);

        for (unsigned int lightIdx = 0; lightIdx < T5ComWorld->primaryLightCount; lightIdx++)
        {
            T5::ComPrimaryLight* T5Light = &T5ComWorld->primaryLights[lightIdx];
            T6::ComPrimaryLight* T6Light = &T6ComWorld->primaryLights[lightIdx];

            if (T5Light->_pad[0] != 0 || T5Light->_pad[1] != 0)
                con::warn(
                    "ComWorld T5 light index {}: {} has non zero pad. Pad1: {}, Pad2: {}", lightIdx, T5Light->defName, T5Light->_pad[0], T5Light->_pad[1]);

            T6Light->type = T5Light->type;
            T6Light->canUseShadowMap = T5Light->canUseShadowMap;
            T6Light->exponent = T5Light->exponent;
            T6Light->priority = T5Light->priority;
            T6Light->cullDist = T5Light->cullDist;
            T6Light->color.x = T5Light->color[0];
            T6Light->color.y = T5Light->color[1];
            T6Light->color.z = T5Light->color[2];
            T6Light->dir.x = T5Light->dir[0];
            T6Light->dir.y = T5Light->dir[1];
            T6Light->dir.z = T5Light->dir[2];
            T6Light->origin.x = T5Light->origin[0];
            T6Light->origin.y = T5Light->origin[1];
            T6Light->origin.z = T5Light->origin[2];
            T6Light->radius = T5Light->radius;
            T6Light->cosHalfFovOuter = T5Light->cosHalfFovOuter;
            T6Light->cosHalfFovInner = T5Light->cosHalfFovInner;
            T6Light->cosHalfFovExpanded = T5Light->cosHalfFovExpanded;
            T6Light->rotationLimit = T5Light->rotationLimit;
            T6Light->translationLimit = T5Light->translationLimit;
            T6Light->mipDistance = T5Light->mipDistance;

            T6Light->diffuseColor.x = T5Light->diffuseColor[0];
            T6Light->diffuseColor.y = T5Light->diffuseColor[1];
            T6Light->diffuseColor.z = T5Light->diffuseColor[2];
            T6Light->diffuseColor.w = T5Light->diffuseColor[3];
            T6Light->falloff.x = T5Light->falloff[0];
            T6Light->falloff.y = T5Light->falloff[1];
            T6Light->falloff.z = T5Light->falloff[2];
            T6Light->falloff.w = T5Light->falloff[3];
            T6Light->angle.x = T5Light->angle[0];
            T6Light->angle.y = T5Light->angle[1];
            T6Light->angle.z = T5Light->angle[2];
            T6Light->angle.w = T5Light->angle[3];
            T6Light->aAbB.x = T5Light->aAbB[0];
            T6Light->aAbB.y = T5Light->aAbB[1];
            T6Light->aAbB.z = T5Light->aAbB[2];
            T6Light->aAbB.w = T5Light->aAbB[3];
            T6Light->cookieControl0.x = T5Light->cookieControl0[0];
            T6Light->cookieControl0.y = T5Light->cookieControl0[1];
            T6Light->cookieControl0.z = T5Light->cookieControl0[2];
            T6Light->cookieControl0.w = T5Light->cookieControl0[3];
            T6Light->cookieControl1.x = T5Light->cookieControl1[0];
            T6Light->cookieControl1.y = T5Light->cookieControl1[1];
            T6Light->cookieControl1.z = T5Light->cookieControl1[2];
            T6Light->cookieControl1.w = T5Light->cookieControl1[3];
            T6Light->cookieControl2.x = T5Light->cookieControl2[0];
            T6Light->cookieControl2.y = T5Light->cookieControl2[1];
            T6Light->cookieControl2.z = T5Light->cookieControl2[2];
            T6Light->cookieControl2.w = T5Light->cookieControl2[3];

            // t5 differences
            // specularColor[4] - removed t5 -> t6
            // attenuation[4] - vec4 t5 possibly converted to dAttenuation t6

            // T6 differences
            T6Light->useCookie = 0;       // added t5->t6, same position as T5Light->_pad[0]
            T6Light->shadowmapVolume = 0; // added t5->t6, same position as T5Light->_pad[1]
            T6Light->dAttenuation = 0.0f; // vec4 in t5, float in t6
            T6Light->roundness = 0.0f;    // added t5->t6

            if (T5Light->defName == nullptr)
            {
                T6Light->defName = nullptr;
            }
            else
            {
                T6Light->defName = m_memory.Dup(T5Light->defName);
                if (T5Light->defName[0] != '\0') // defName isn't empty
                {
                    auto T5LightDefAsset = T5AssetPool->GetAsset(T5::ASSET_TYPE_LIGHT_DEF, T5Light->defName);
                    if (T5LightDefAsset == nullptr)
                    {
                        con::error("Can't find T5 LightDef asset {}.", T5Light->defName);
                        return false;
                    }
                    T5::GfxLightDef* T5LightDef = static_cast<T5::GfxLightDef*>(T5LightDefAsset->m_ptr);

                    if (!linkLightDef(T5AssetPool, T5LightDef))
                        return false;
                }
            }
        }

        m_context.AddAsset<T6::AssetComWorld>(T6ComWorld->name, T6ComWorld);

        return true;
    }
    */
} // namespace BSP
