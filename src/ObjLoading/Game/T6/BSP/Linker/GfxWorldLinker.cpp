#include "GfxWorldLinker.h"

#include "../BSPUtil.h"
#include "Utils/Pack.h"

namespace BSP
{
    GfxWorldLinker::GfxWorldLinker(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
        : m_memory(memory),
          m_search_path(searchPath),
          m_context(context)
    {
    }

    bool GfxWorldLinker::linkGfxWorld(ZoneAssetPools* T5AssetPool, std::string& bspName)
    {
        auto T5GfxWorldAsset = T5AssetPool->GetAsset(T5::ASSET_TYPE_GFXWORLD, bspName);
        if (T5GfxWorldAsset == nullptr)
        {
            con::error("Can't find T5 GfxWorld asset.");
            return false;
        }
        T5::GfxWorld* T5GfxWorld = static_cast<T5::GfxWorld*>(T5GfxWorldAsset->m_ptr);
        T6::GfxWorld* T6GfxWorld = m_memory.Alloc<T6::GfxWorld>();

        T6GfxWorld->name = m_memory.Dup(T5GfxWorld->name);
        T6GfxWorld->baseName = m_memory.Dup(T5GfxWorld->baseName);

        T6GfxWorld->planeCount = T5GfxWorld->planeCount;
        T6GfxWorld->nodeCount = T5GfxWorld->nodeCount;
        T6GfxWorld->surfaceCount = T5GfxWorld->surfaceCount;

        m_context.AddAsset<T6::AssetGfxWorld>(T6GfxWorld->name, T6GfxWorld);

        /*
         struct GfxWorld
    {
        const char* name;
        const char* baseName;
        int planeCount;
        int nodeCount;
        int surfaceCount;
        GfxWorldStreamInfo streamInfo;
        const char* skyBoxModel;
        SunLightParseParams sunParse;
        GfxLight* sunLight;
        unsigned int sunPrimaryLightIndex;
        unsigned int primaryLightCount;
        unsigned int coronaCount;
        GfxLightCorona* coronas;
        unsigned int shadowMapVolumeCount;
        GfxShadowMapVolume* shadowMapVolumes;
        unsigned int shadowMapVolumePlaneCount;
        GfxVolumePlane* shadowMapVolumePlanes;
        unsigned int exposureVolumeCount;
        GfxExposureVolume* exposureVolumes;
        unsigned int exposureVolumePlaneCount;
        GfxVolumePlane* exposureVolumePlanes;
        unsigned int worldFogVolumeCount;
        GfxWorldFogVolume* worldFogVolumes;
        unsigned int worldFogVolumePlaneCount;
        GfxVolumePlane* worldFogVolumePlanes;
        unsigned int worldFogModifierVolumeCount;
        GfxWorldFogModifierVolume* worldFogModifierVolumes;
        unsigned int worldFogModifierVolumePlaneCount;
        GfxVolumePlane* worldFogModifierVolumePlanes;
        unsigned int lutVolumeCount;
        GfxLutVolume* lutVolumes;
        unsigned int lutVolumePlaneCount;
        GfxVolumePlane* lutVolumePlanes;
        GfxSkyDynamicIntensity skyDynIntensity;
        GfxWorldDpvsPlanes dpvsPlanes;
        int cellBitsCount;
        GfxCell* cells;
        GfxWorldDraw draw;
        GfxLightGrid lightGrid;
        int modelCount;
        GfxBrushModel* models;
        vec3_t mins;
        vec3_t maxs;
        unsigned int checksum;
        int materialMemoryCount;
        MaterialMemory* materialMemory;
        sunflare_t sun;
        vec4_t outdoorLookupMatrix[4];
        GfxImage* outdoorImage;
        unsigned int* cellCasterBits;
        GfxSceneDynModel* sceneDynModel;
        GfxSceneDynBrush* sceneDynBrush;
        unsigned int* primaryLightEntityShadowVis;
        unsigned int* primaryLightDynEntShadowVis[2];
        unsigned int numSiegeSkinInsts;
        SSkinInstance* siegeSkinInsts;
        GfxShadowGeometry* shadowGeom;
        GfxLightRegion* lightRegion;
        GfxWorldDpvsStatic dpvs;
        GfxWorldDpvsDynamic dpvsDyn;
        float waterDirection;
        GfxWaterBuffer waterBuffers[2];
        Material* waterMaterial;
        Material* coronaMaterial;
        Material* ropeMaterial;
        Material* lutMaterial;
        unsigned int numOccluders;
        Occluder* occluders;
        unsigned int numOutdoorBounds;
        GfxOutdoorBounds* outdoorBounds;
        unsigned int heroLightCount;
        unsigned int heroLightTreeCount;
        GfxHeroLight* heroLights;
        GfxHeroLightTree* heroLightTree;
        unsigned int lightingFlags;
        int lightingQuality;
    };
        */
    }
} // namespace BSP
