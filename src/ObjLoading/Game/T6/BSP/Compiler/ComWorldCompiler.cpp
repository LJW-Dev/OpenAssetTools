#include "ComWorldCompiler.h"

#include "Utils/Logging/Log.h"

#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace
{
    enum T5GfxLightType : __int32
    {
        GFX_LIGHT_TYPE_NONE = 0x0,
        GFX_LIGHT_TYPE_DIR = 0x1,
        GFX_LIGHT_TYPE_SPOT = 0x2,
        GFX_LIGHT_TYPE_OMNI = 0x3,
        GFX_LIGHT_TYPE_COUNT = 0x4,
        GFX_LIGHT_TYPE_DIR_SHADOWMAP = 0x4,
        GFX_LIGHT_TYPE_SPOT_SHADOWMAP = 0x5,
        GFX_LIGHT_TYPE_OMNI_SHADOWMAP = 0x6,
        GFX_LIGHT_TYPE_COUNT_WITH_SHADOWMAP_VERSIONS = 0x7,
    };

} // namespace

namespace BSP
{
    ComWorldCompiler::ComWorldCompiler(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
        : m_memory(memory),
          m_search_path(searchPath),
          m_context(context)
    {
    }

    bool ComWorldCompiler::linkLightDef(ZoneAssetPools* T5AssetPool, T5::GfxLightDef* T5LightDef)
    {
        T6::GfxLightDef* T6LightDef = m_memory.Alloc<T6::GfxLightDef>();
        T6LightDef->name = m_memory.Dup(T5LightDef->name);
        T6LightDef->lmapLookupStart = T5LightDef->lmapLookupStart;
        T6LightDef->attenuation.samplerState = 115; // T6 is always 115

        // auto T6LightDefImage = m_context.LoadDependency<T6::AssetImage>(T5LightDef->attenuation.image->name);
        auto T6LightDefImage = m_context.LoadDependency<T6::AssetImage>("gobo_caustics_rgb");
        if (T6LightDefImage == nullptr)
            return false;
        T6LightDef->attenuation.image = T6LightDefImage->Asset();

        m_context.AddAsset<T6::AssetLightDef>(T6LightDef->name, T6LightDef);

        return true;
    }

    bool ComWorldCompiler::linkComWorld(ZoneAssetPools* T5AssetPool, std::string& mapName, std::string& bspName, std::string& T5BSPName)
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
        T6ComWorld->isInUse = true;
        T6ComWorld->primaryLightCount = T5ComWorld->primaryLightCount;
        T6ComWorld->primaryLights = m_memory.Alloc<T6::ComPrimaryLight>(T5ComWorld->primaryLightCount);

        bool eee = true;
        for (unsigned int lightIdx = 0; lightIdx < T5ComWorld->primaryLightCount; lightIdx++)
        {
            T5::ComPrimaryLight* T5Light = &T5ComWorld->primaryLights[lightIdx];
            T6::ComPrimaryLight* T6Light = &T6ComWorld->primaryLights[lightIdx];

            /*
            if (lightIdx == 0)
            {
            }
            else if (lightIdx == 1)
            {
                T6Light->type = T6::GFX_LIGHT_TYPE_DIR;
                T6Light->diffuseColor.x = T5Light->diffuseColor[0];
                T6Light->diffuseColor.y = T5Light->diffuseColor[1];
                T6Light->diffuseColor.z = T5Light->diffuseColor[2];
                T6Light->diffuseColor.w = T5Light->diffuseColor[3];
                T6Light->color.x = T5Light->color[0];
                T6Light->color.y = T5Light->color[1];
                T6Light->color.z = T5Light->color[2];
                T6Light->dir.x = T5Light->dir[0];
                T6Light->dir.y = T5Light->dir[1];
                T6Light->dir.z = T5Light->dir[2];
            }
            else
            {
                T6Light->aAbB.x = 1.0f;
                T6Light->aAbB.y = 0.3984375f;
                T6Light->aAbB.z = 1.0f;
                T6Light->aAbB.w = 0.3984375f;
                T6Light->angle.x = 0.0f;
                T6Light->angle.y = 0.0f;
                T6Light->angle.z = 0.4363323152065277f;
                T6Light->angle.w = 0.0f;
                T6Light->canUseShadowMap = 0;
                T6Light->color.x = 16.0f;
                T6Light->color.y = 16.0f;
                T6Light->color.z = 16.0f;
                T6Light->cookieControl0.x = 0.0f;
                T6Light->cookieControl0.y = 0.0f;
                T6Light->cookieControl0.z = 5.0f;
                T6Light->cookieControl0.w = 4.0f;
                T6Light->cookieControl1.x = 0.0f;
                T6Light->cookieControl1.y = 0.0f;
                T6Light->cookieControl1.z = 0.15625f;
                T6Light->cookieControl1.w = 0.10000000149011612f;
                T6Light->cookieControl2.x = 0.0;
                T6Light->cookieControl2.y = 0.0;
                T6Light->cookieControl2.z = 0.0;
                T6Light->cookieControl2.w = 0.0;
                T6Light->cosHalfFovOuter = 0.8870108127593994f;
                T6Light->cosHalfFovInner = 0.9152581095695496f;
                T6Light->cosHalfFovExpanded = 0.8870108127593994f;
                T6Light->cullDist = 1000;
                T6Light->dAttenuation = 160000.0f;
                T6Light->defName = m_memory.Dup("gobo_caustics_rgb");
                T6Light->diffuseColor.x = 16.0f;
                T6Light->diffuseColor.y = 16.0f;
                T6Light->diffuseColor.z = 16.0f;
                T6Light->diffuseColor.w = 0.0f;
                T6Light->dir.x = 0.6047161817550659f;
                T6Light->dir.y = -0.31152045726776123f;
                T6Light->dir.z = 0.7329893112182617f;
                T6Light->exponent = 0;
                T6Light->falloff.x = 0.0f;
                T6Light->falloff.y = 200.0f;
                T6Light->falloff.z = 0.0f;
                T6Light->falloff.w = 0.0f;
                T6Light->mipDistance = 225.4763946533203f;
                T6Light->origin.x = T5Light->origin[0];
                T6Light->origin.y = T5Light->origin[1];
                T6Light->origin.z = T5Light->origin[2];
                T6Light->priority = 0;
                T6Light->radius = 200.0f;
                T6Light->rotationLimit = 1.0f;
                T6Light->roundness = 1.0f;
                T6Light->shadowmapVolume = 0;
                T6Light->translationLimit = 0.0f;
                T6Light->type = 2;
                T6Light->useCookie = -1;

                if (eee)
                {
                    eee = false;
                    T6::GfxLightDef* T6LightDef = m_memory.Alloc<T6::GfxLightDef>();
                    T6LightDef->name = m_memory.Dup("gobo_caustics_rgb");
                    T6LightDef->lmapLookupStart = 0;
                    T6LightDef->attenuation.samplerState = 115;
                    auto T6LightDefImage = m_context.LoadDependency<T6::AssetImage>("gobo_caustics_rgb");
                    if (T6LightDefImage == nullptr)
                        return false;
                    T6LightDef->attenuation.image = T6LightDefImage->Asset();
                    m_context.AddAsset<T6::AssetLightDef>(T6LightDef->name, T6LightDef);
                }
            }
            */

            if (T5Light->_pad[0] != 0 || T5Light->_pad[1] != 0)
                con::warn(
                    "ComWorld T5 light index {}: {} has non zero pad. Pad1: {}, Pad2: {}", lightIdx, T5Light->defName, T5Light->_pad[0], T5Light->_pad[1]);

            switch (T5Light->type)
            {
            case GFX_LIGHT_TYPE_NONE:
                T6Light->type = T6::GFX_LIGHT_TYPE_NONE;
                break;
            case GFX_LIGHT_TYPE_DIR:
                T6Light->type = T6::GFX_LIGHT_TYPE_DIR;
                break;
            case GFX_LIGHT_TYPE_SPOT:
                T6Light->type = T6::GFX_LIGHT_TYPE_SPOT;
                break;
            case GFX_LIGHT_TYPE_OMNI:
                T6Light->type = T6::GFX_LIGHT_TYPE_OMNI;
                break;
            default:
                assert(false);
            }
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
            // T6 added more light types
            // specularColor[4] - removed t5 -> t6
            // attenuation[4] - vec4 t5 possibly converted to dAttenuation t6

            // T6 differences
            T6Light->useCookie = -1;          // added t5->t6, same position as T5Light->_pad[0]
            T6Light->shadowmapVolume = 0;     // added t5->t6, same position as T5Light->_pad[1]
            T6Light->dAttenuation = 10000.0f; // vec4 in t5, float in t6 (10000 for testing: pre sure attenuation effects lights)
            T6Light->roundness = 0.0f;        // added t5->t6

            if (T5Light->defName == nullptr)
            {
                T6Light->defName = nullptr;
            }
            else
            {
                if (lightIdx == 0 || lightIdx == 1)
                    con::warn("T5 Light with index 0 or 1 with a valid defname, unknown why.");

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
} // namespace BSP
