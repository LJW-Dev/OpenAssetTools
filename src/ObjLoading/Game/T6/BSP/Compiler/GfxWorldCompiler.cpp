#include "GfxWorldCompiler.h"

#include "Converter/T5ImageConverter.h"
#include "SearchPath/OutputPathFilesystem.h"
#include "Utils/Logging/Log.h"
#include "Utils/Pack.h"

#include <cassert>
#include <memory>
#include <nlohmann/json.hpp>
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
    GfxWorldCompiler::GfxWorldCompiler(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
        : m_memory(memory),
          m_search_path(searchPath),
          m_context(context)
    {
    }

    void GfxWorldCompiler::loadDrawData(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        unsigned int vertexCount = T5GfxWorld->draw.vertexCount;
        if (vertexCount == 0)
        {
            T6GfxWorld->draw.vertexCount = 0;
            T6GfxWorld->draw.vertexDataSize0 = 0;
            T6GfxWorld->draw.vertexDataSize1 = 0;
            T6GfxWorld->draw.vd0.data = nullptr;
            T6GfxWorld->draw.vd1.data = nullptr;
            T6GfxWorld->draw.indexCount = 0;
            T6GfxWorld->draw.indices = nullptr;
        }
        else
        {
            T6GfxWorld->draw.vertexCount = vertexCount;
            T6GfxWorld->draw.vertexDataSize0 = static_cast<unsigned int>(vertexCount * sizeof(T6::GfxPackedWorldVertex));
            T6::GfxPackedWorldVertex* vertexBuffer = m_memory.Alloc<T6::GfxPackedWorldVertex>(vertexCount);
            for (unsigned int vertIdx = 0; vertIdx < vertexCount; vertIdx++)
            {
                T5::GfxWorldVertex* T5Vertex = &T5GfxWorld->draw.vd.vertices[vertIdx];
                T6::GfxPackedWorldVertex* T6Vertex = &vertexBuffer[vertIdx];

                T6Vertex->xyz.x = T5Vertex->xyz[0];
                T6Vertex->xyz.y = T5Vertex->xyz[1];
                T6Vertex->xyz.z = T5Vertex->xyz[2];

                T6Vertex->color.packed = T5Vertex->color.packed;
                T6Vertex->texCoord.packed = pack32::Vec2PackTexCoordsUV(T5Vertex->texCoord);
                T6Vertex->normal.packed = T5Vertex->normal.packed;
                T6Vertex->tangent.packed = T5Vertex->tangent.packed;
                T6Vertex->binormalSign = T5Vertex->binormalSign;
                T6Vertex->lmapCoord.packed = pack32::Vec2PackTexCoordsUV(T5Vertex->lmapCoord);
            }
            T6GfxWorld->draw.vd0.data = reinterpret_cast<char*>(vertexBuffer);

            // vd1 is unused but still needs to be initialised
            // the data type varies and 0x20 is enough for all types
            T6GfxWorld->draw.vertexDataSize1 = 0x20;
            T6GfxWorld->draw.vd1.data = m_memory.Alloc<char>(T6GfxWorld->draw.vertexDataSize1);

            assert(T5GfxWorld->draw.indexCount != 0);
            T6GfxWorld->draw.indexCount = T5GfxWorld->draw.indexCount;
            T6GfxWorld->draw.indices = m_memory.Alloc<uint16_t>(T5GfxWorld->draw.indexCount);
            memcpy(T6GfxWorld->draw.indices, T5GfxWorld->draw.indices, sizeof(uint16_t) * T5GfxWorld->draw.indexCount);
        }
    }

    bool GfxWorldCompiler::loadMapSurfaces(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        if (T5GfxWorld->surfaceCount == 0 || T5GfxWorld->dpvs.staticSurfaceCount == 0)
        {
            con::error("Cannot convert map with 0 surfaces!");
            return false;
        }

        loadDrawData(T5GfxWorld, T6GfxWorld);

        T6GfxWorld->surfaceCount = T5GfxWorld->dpvs.staticSurfaceCount;
        T6GfxWorld->dpvs.staticSurfaceCount = T5GfxWorld->dpvs.staticSurfaceCount;
        unsigned int surfaceCount = T5GfxWorld->dpvs.staticSurfaceCount;
        unsigned int StaticSurfaceCount = T5GfxWorld->dpvs.staticSurfaceCount;

        // sortedSurfIndex is staticSurfaceCount size
        T6GfxWorld->dpvs.sortedSurfIndex = m_memory.Alloc<uint16_t>(surfaceCount);
        for (unsigned int surfIdx = 0; surfIdx < StaticSurfaceCount; surfIdx++)
            T6GfxWorld->dpvs.sortedSurfIndex[surfIdx] = T5GfxWorld->dpvs.sortedSurfIndex[surfIdx];

        // 8 - sizeof StaticSurfaceCount
        //  surface materials are written to by the game
        T6GfxWorld->dpvs.surfaceMaterials = m_memory.Alloc<T6::GfxDrawSurf_align4>(StaticSurfaceCount);

        // StaticSurfaceCount
        // set all surface types to lit opaque
        T6GfxWorld->dpvs.litSurfsBegin = 0;
        T6GfxWorld->dpvs.litSurfsEnd = static_cast<unsigned int>(StaticSurfaceCount);
        T6GfxWorld->dpvs.emissiveOpaqueSurfsBegin = static_cast<unsigned int>(StaticSurfaceCount);
        T6GfxWorld->dpvs.emissiveOpaqueSurfsEnd = static_cast<unsigned int>(StaticSurfaceCount);
        T6GfxWorld->dpvs.emissiveTransSurfsBegin = static_cast<unsigned int>(StaticSurfaceCount);
        T6GfxWorld->dpvs.emissiveTransSurfsEnd = static_cast<unsigned int>(StaticSurfaceCount);
        T6GfxWorld->dpvs.litTransSurfsBegin = static_cast<unsigned int>(StaticSurfaceCount);
        T6GfxWorld->dpvs.litTransSurfsEnd = static_cast<unsigned int>(StaticSurfaceCount);

        // 1
        // visdata is written to by the game
        // all visdata is alligned by 128
        size_t allignedSurfaceCount = BSPUtil::allignBy128(StaticSurfaceCount);
        T6GfxWorld->dpvs.surfaceVisDataCount = static_cast<unsigned int>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceVisData[0] = m_memory.Alloc<char>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceVisData[1] = m_memory.Alloc<char>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceVisData[2] = m_memory.Alloc<char>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceVisDataCameraSaved = m_memory.Alloc<char>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceCastsShadow = m_memory.Alloc<char>(allignedSurfaceCount);
        T6GfxWorld->dpvs.surfaceCastsSunShadow = m_memory.Alloc<char>(allignedSurfaceCount);

        T6GfxWorld->dpvs.surfaces = m_memory.Alloc<T6::GfxSurface>(surfaceCount);
        for (unsigned int surfIdx = 0; surfIdx < surfaceCount; surfIdx++)
        {
            T5::GfxSurface* T5Surface = &T5GfxWorld->dpvs.surfaces[surfIdx];
            T6::GfxSurface* T6Surface = &T6GfxWorld->dpvs.surfaces[surfIdx];

            T6Surface->primaryLightIndex = T5Surface->primaryLightIndex;
            T6Surface->lightmapIndex = T5Surface->lightmapIndex;
            T6Surface->reflectionProbeIndex = T5Surface->reflectionProbeIndex;

            // flags are different from T5 and mess with shadows if the original values are used
            T6Surface->flags = 0;
            // T6Surface->flags = T5Surface->flags;

            T6Surface->bounds[0].x = T5Surface->bounds[0][0];
            T6Surface->bounds[0].y = T5Surface->bounds[0][1];
            T6Surface->bounds[0].z = T5Surface->bounds[0][2];
            T6Surface->bounds[1].x = T5Surface->bounds[1][0];
            T6Surface->bounds[1].y = T5Surface->bounds[1][1];
            T6Surface->bounds[1].z = T5Surface->bounds[1][2];

            /*
            Surface and material properties that determine lighting
            Surface Flags:
            If material game flags 0x40 is true then the surface will always cast a shadow (dpvs.surfaceCastsShadow to true) therefore GFX_SURFACE_CASTS_SHADOW
                is redundant
            - if the material flag is set then surface flag GFX_SURFACE_CASTS_SUN_SHADOW is checked and added (dpvs.surfaceCastsSunShadow to true)

            Material flags:
            0x40 - must be true for the game to check if any lights or shadows effect the surface
            0x02 - true:
                    - The surface primary light is used as the light for determining shadows
                 - false:
                    - Each light in the world is checked to see if it effects the surf (and added if true)

            material techsets define if lightmaps and/or the light grid are used or not (im pretty sure)
            */
            auto surfMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>(T5Surface->material->info.name);
            // auto surfMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>("material_template");
            if (surfMaterialAsset == nullptr)
            {
                // surfMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>(CBSPLinkingConstants::MISSING_MATERIAL_NAME);
                surfMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>("material_template");
                if (surfMaterialAsset == nullptr)
                {
                    con::error("unable to load the default material {}!", "material_template");
                    return false;
                }
            }
            T6Surface->material = surfMaterialAsset->Asset();

            T6Surface->tris.vertexDataOffset0 = T5Surface->tris.firstVertex * sizeof(T6::GfxPackedWorldVertex);
            T6Surface->tris.vertexDataOffset1 = 0;
            T6Surface->tris.triCount = T5Surface->tris.triCount;
            T6Surface->tris.baseIndex = T5Surface->tris.baseIndex;
            T6Surface->tris.mins.x = T5Surface->tris.mins[0];
            T6Surface->tris.mins.y = T5Surface->tris.mins[1];
            T6Surface->tris.mins.z = T5Surface->tris.mins[2];
            T6Surface->tris.maxs.x = T5Surface->tris.maxs[0];
            T6Surface->tris.maxs.y = T5Surface->tris.maxs[1];
            T6Surface->tris.maxs.z = T5Surface->tris.maxs[2];
            T6Surface->tris.himipRadiusInvSq = 1.0f / T5Surface->tris.himipRadiusSq;
            T6Surface->tris.vertexCount = T5Surface->tris.vertexCount;
            T6Surface->tris.firstVertex = T5Surface->tris.firstVertex;

            assert((T5Surface->tris.firstVertex + T5Surface->tris.vertexCount - 1) * sizeof(T6::GfxPackedWorldVertex) < T6GfxWorld->draw.vertexDataSize0);
            assert(T6Surface->tris.baseIndex + (T6Surface->tris.triCount * 3) - 1 < T5GfxWorld->draw.indexCount);
        }

        return true;

        // nlohmann::json js;
        // js["surfaces"] = nlohmann::json::array();
        // for (unsigned int surfIdx = 0; surfIdx < T5GfxWorld->surfaceCount; surfIdx++)
        //{
        //     T5::GfxSurface* T5Surface = &T5GfxWorld->dpvs.surfaces[surfIdx];
        //
        //     js["surfaces"][surfIdx]["aSurfIndex"] = surfIdx;
        //     js["surfaces"][surfIdx]["lightmapIndex"] = T5Surface->lightmapIndex;
        //     js["surfaces"][surfIdx]["reflectionProbeIndex"] = T5Surface->reflectionProbeIndex;
        //     js["surfaces"][surfIdx]["primaryLightIndex"] = T5Surface->primaryLightIndex;
        //     js["surfaces"][surfIdx]["flags"] = T5Surface->flags;
        //     js["surfaces"][surfIdx]["mins"] = {T5Surface->bounds[0][0], T5Surface->bounds[0][1], T5Surface->bounds[0][2]};
        //     js["surfaces"][surfIdx]["maxs"] = {T5Surface->bounds[1][0], T5Surface->bounds[1][1], T5Surface->bounds[1][2]};
        //     js["surfaces"][surfIdx]["material"] = T5Surface->material->info.name;
        //     js["surfaces"][surfIdx]["tris"]["vertexLayerData"] = T5Surface->tris.vertexLayerData;
        //     js["surfaces"][surfIdx]["tris"]["firstVertex"] = T5Surface->tris.firstVertex;
        //     js["surfaces"][surfIdx]["tris"]["vertexCount"] = T5Surface->tris.vertexCount;
        //     js["surfaces"][surfIdx]["tris"]["triCount"] = T5Surface->tris.triCount;
        //     js["surfaces"][surfIdx]["tris"]["baseIndex"] = T5Surface->tris.baseIndex;
        //     js["surfaces"][surfIdx]["tris"]["himipRadiusSq"] = T5Surface->tris.himipRadiusSq;
        //     js["surfaces"][surfIdx]["tris"]["stream2ByteOffset"] = T5Surface->tris.stream2ByteOffset;
        //     js["surfaces"][surfIdx]["tris"]["mins"] = {T5Surface->tris.mins[0], T5Surface->tris.mins[1], T5Surface->tris.mins[2]};
        //     js["surfaces"][surfIdx]["tris"]["maxs"] = {T5Surface->tris.maxs[0], T5Surface->tris.maxs[1], T5Surface->tris.maxs[2]};
        // }
        // OutputPathFilesystem fs("C:\\Users\\LJ\\Documents");
        // const auto assetFile = fs.Open("surfs.json");
        // std::string jsonString = js.dump(4);
        // assetFile->write(jsonString.c_str(), jsonString.size());

        // TODO: crash relating to an inavlid model ptr caused by bad values in this function
        // working but incorrect code is kept in, non-working code below
        /*
        bool GfxWorldCompiler::loadMapSurfaces(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
        {
            loadDrawData(T5GfxWorld, T6GfxWorld);

            T6GfxWorld->surfaceCount = T5GfxWorld->surfaceCount;
            T6GfxWorld->dpvs.staticSurfaceCount = T5GfxWorld->dpvs.staticSurfaceCount;
            unsigned int surfaceCount = T5GfxWorld->surfaceCount;
            unsigned int staticSurfaceCount = T5GfxWorld->dpvs.staticSurfaceCount;
            assert(staticSurfaceCount == T5GfxWorld->models->surfaceCount);

            // doesn't seem to matter what order the sorted surfs go in
            T6GfxWorld->dpvs.sortedSurfIndex = m_memory.Alloc<uint16_t>(staticSurfaceCount); // T5: staticSurfaceCount T6: staticSurfaceCount
            for (unsigned int surfIdx = 0; surfIdx < staticSurfaceCount; surfIdx++)
                T6GfxWorld->dpvs.sortedSurfIndex[surfIdx] = T5GfxWorld->dpvs.sortedSurfIndex[surfIdx];

            //  empty, surface materials are written to by the game
            T6GfxWorld->dpvs.surfaceMaterials = m_memory.Alloc<T6::GfxDrawSurf_align4>(staticSurfaceCount); // T5: staticSurfaceCount T6: staticSurfaceCount

            // TODO: setting all surface types to lit opaque
            T6GfxWorld->dpvs.litSurfsBegin = 0;
            T6GfxWorld->dpvs.litSurfsEnd = static_cast<unsigned int>(staticSurfaceCount);              // T5:  T6:
            T6GfxWorld->dpvs.emissiveOpaqueSurfsBegin = static_cast<unsigned int>(staticSurfaceCount); // T5:  T6:
            T6GfxWorld->dpvs.emissiveOpaqueSurfsEnd = static_cast<unsigned int>(staticSurfaceCount);   // T5:  T6:
            T6GfxWorld->dpvs.emissiveTransSurfsBegin = static_cast<unsigned int>(staticSurfaceCount);  // T5:  T6:
            T6GfxWorld->dpvs.emissiveTransSurfsEnd = static_cast<unsigned int>(staticSurfaceCount);    // T5:  T6:
            T6GfxWorld->dpvs.litTransSurfsBegin = static_cast<unsigned int>(staticSurfaceCount);       // T5:  T6:
            T6GfxWorld->dpvs.litTransSurfsEnd = static_cast<unsigned int>(staticSurfaceCount);         // T5:  T6:

            // visdata is written to by the game
            // all visdata is alligned by 128
            size_t allignedSurfaceCount = BSPUtil::allignBy128(staticSurfaceCount);
            T6GfxWorld->dpvs.surfaceVisDataCount = static_cast<unsigned int>(allignedSurfaceCount);
            T6GfxWorld->dpvs.surfaceVisData[0] = m_memory.Alloc<char>(allignedSurfaceCount);         // T5: staticSurfaceCount T6: surfaceVisDataCount
            T6GfxWorld->dpvs.surfaceVisData[1] = m_memory.Alloc<char>(allignedSurfaceCount);         // T5: staticSurfaceCount T6: surfaceVisDataCount
            T6GfxWorld->dpvs.surfaceVisData[2] = m_memory.Alloc<char>(allignedSurfaceCount);         // T5: staticSurfaceCount T6: surfaceVisDataCount
            T6GfxWorld->dpvs.surfaceVisDataCameraSaved = m_memory.Alloc<char>(allignedSurfaceCount); // T5: staticSurfaceCount T6: surfaceVisDataCount
            T6GfxWorld->dpvs.surfaceCastsShadow = m_memory.Alloc<char>(allignedSurfaceCount);        // T5: n/a T6: surfaceVisDataCount
            T6GfxWorld->dpvs.surfaceCastsSunShadow = m_memory.Alloc<char>(allignedSurfaceCount);     // T5: surfaceVisDataCount T6: surfaceVisDataCount

            T6GfxWorld->dpvs.surfaces = m_memory.Alloc<T6::GfxSurface>(surfaceCount); // T5: surfaceCount T6: surfaceCount
            for (unsigned int surfIdx = 0; surfIdx < surfaceCount; surfIdx++)
            {
                T5::GfxSurface* T5Surface = &T5GfxWorld->dpvs.surfaces[surfIdx];
                T6::GfxSurface* T6Surface = &T6GfxWorld->dpvs.surfaces[surfIdx];

                T6Surface->primaryLightIndex = T5Surface->primaryLightIndex;
                T6Surface->lightmapIndex = T5Surface->lightmapIndex;
                T6Surface->reflectionProbeIndex = T5Surface->reflectionProbeIndex;
                T6Surface->flags = T5Surface->flags;

                T6Surface->bounds[0].x = T5Surface->bounds[0][0];
                T6Surface->bounds[0].y = T5Surface->bounds[0][1];
                T6Surface->bounds[0].z = T5Surface->bounds[0][2];
                T6Surface->bounds[1].x = T5Surface->bounds[1][0];
                T6Surface->bounds[1].y = T5Surface->bounds[1][1];
                T6Surface->bounds[1].z = T5Surface->bounds[1][2];

                auto surfMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>(T5Surface->material->info.name);
                if (surfMaterialAsset == nullptr)
                {
                    surfMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>(CBSPLinkingConstants::MISSING_MATERIAL_NAME);
                    if (surfMaterialAsset == nullptr)
                    {
                        con::error("unable to load surface material {}!", T5Surface->material->info.name);
                        return false;
                    }
                }
                T6Surface->material = surfMaterialAsset->Asset();

                T6Surface->tris.vertexDataOffset0 = T5Surface->tris.firstVertex * sizeof(T6::GfxPackedWorldVertex);
                T6Surface->tris.vertexDataOffset1 = 0;
                T6Surface->tris.triCount = T5Surface->tris.triCount;
                T6Surface->tris.baseIndex = T5Surface->tris.baseIndex;

                _ASSERT((T5Surface->tris.firstVertex + T5Surface->tris.vertexCount - 1) * sizeof(T6::GfxPackedWorldVertex) < T6GfxWorld->draw.vertexDataSize0);

                _ASSERT(T6Surface->tris.baseIndex + (T6Surface->tris.triCount * 3) - 1 < T5GfxWorld->draw.indexCount);

                // unused values
                T6Surface->tris.mins.x = 0.0f;
                T6Surface->tris.mins.y = 0.0f;
                T6Surface->tris.mins.z = 0.0f;
                T6Surface->tris.maxs.x = 0.0f;
                T6Surface->tris.maxs.y = 0.0f;
                T6Surface->tris.maxs.z = 0.0f;
                T6Surface->tris.himipRadiusInvSq = 0.0f;
                T6Surface->tris.vertexCount = 0;
                T6Surface->tris.firstVertex = 0;
            }

            return true;
        }
        */
    }

    void GfxWorldCompiler::loadXModels(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        // XModels are unsupported right now
        T6GfxWorld->dpvs.smodelCount = 0;
        T6GfxWorld->dpvs.smodelInsts = nullptr;
        T6GfxWorld->dpvs.smodelDrawInsts = nullptr;
        T6GfxWorld->dpvs.smodelVisDataCount = 0;
        T6GfxWorld->dpvs.smodelVisData[0] = nullptr;
        T6GfxWorld->dpvs.smodelVisData[1] = nullptr;
        T6GfxWorld->dpvs.smodelVisData[2] = nullptr;
        T6GfxWorld->dpvs.smodelVisDataCameraSaved = nullptr;
        T6GfxWorld->dpvs.smodelCastsShadow = nullptr;

        /*
        unsigned int modelCount = 0;
        T6GfxWorld->dpvs.smodelCount = modelCount;
        T6GfxWorld->dpvs.smodelInsts = m_memory.Alloc<T6::GfxStaticModelInst>(modelCount);
        T6GfxWorld->dpvs.smodelDrawInsts = m_memory.Alloc<T6::GfxStaticModelDrawInst>(modelCount);

        // visdata is written to by the game
        // all visdata is alligned by 128
        size_t allignedModelCount = BSPUtil::allignBy128(modelCount);
        T6GfxWorld->dpvs.smodelVisDataCount = static_cast<unsigned int>(allignedModelCount);
        T6GfxWorld->dpvs.smodelVisData[0] = m_memory.Alloc<char>(allignedModelCount);
        T6GfxWorld->dpvs.smodelVisData[1] = m_memory.Alloc<char>(allignedModelCount);
        T6GfxWorld->dpvs.smodelVisData[2] = m_memory.Alloc<char>(allignedModelCount);
        T6GfxWorld->dpvs.smodelVisDataCameraSaved = m_memory.Alloc<char>(allignedModelCount);
        T6GfxWorld->dpvs.smodelCastsShadow = m_memory.Alloc<char>(allignedModelCount);
        for (unsigned int i = 0; i < modelCount; i++)
        {
            if ((T6GfxWorld->dpvs.smodelDrawInsts[i].flags & T6::STATIC_MODEL_FLAG_NO_SHADOW) == 0)
                T6GfxWorld->dpvs.smodelCastsShadow[i] = 1;
            else
                T6GfxWorld->dpvs.smodelCastsShadow[i] = 0;
        }
        */
    }

    void GfxWorldCompiler::loadCoronas(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        T6GfxWorld->coronaCount = T5GfxWorld->coronaCount;
        if (T5GfxWorld->coronaCount == 0)
            T6GfxWorld->coronas = nullptr;
        else
        {
            T6GfxWorld->coronas = m_memory.Alloc<T6::GfxLightCorona>(T5GfxWorld->coronaCount);
            for (unsigned int coronaIdx = 0; coronaIdx < T5GfxWorld->coronaCount; coronaIdx++)
            {
                T6GfxWorld->coronas[coronaIdx].radius = T5GfxWorld->coronas[coronaIdx].radius;
                T6GfxWorld->coronas[coronaIdx].intensity = T5GfxWorld->coronas[coronaIdx].intensity;
                T6GfxWorld->coronas[coronaIdx].origin.x = T5GfxWorld->coronas[coronaIdx].origin[0];
                T6GfxWorld->coronas[coronaIdx].origin.y = T5GfxWorld->coronas[coronaIdx].origin[1];
                T6GfxWorld->coronas[coronaIdx].origin.z = T5GfxWorld->coronas[coronaIdx].origin[2];
                T6GfxWorld->coronas[coronaIdx].color.x = T5GfxWorld->coronas[coronaIdx].color[0];
                T6GfxWorld->coronas[coronaIdx].color.y = T5GfxWorld->coronas[coronaIdx].color[1];
                T6GfxWorld->coronas[coronaIdx].color.z = T5GfxWorld->coronas[coronaIdx].color[2];
            }
        }
    }

    void GfxWorldCompiler::loadExposureVolumes(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        T6GfxWorld->exposureVolumeCount = T5GfxWorld->exposureVolumeCount;
        if (T5GfxWorld->exposureVolumeCount == 0)
            T6GfxWorld->exposureVolumes = nullptr;
        else
        {
            static_assert(sizeof(T5::GfxExposureVolume) == sizeof(T6::GfxExposureVolume));
            T6GfxWorld->exposureVolumes = m_memory.Alloc<T6::GfxExposureVolume>(T5GfxWorld->exposureVolumeCount);
            memcpy(T6GfxWorld->exposureVolumes, T5GfxWorld->exposureVolumes, sizeof(T5::GfxExposureVolume) * T5GfxWorld->exposureVolumeCount);
        }

        T6GfxWorld->exposureVolumePlaneCount = T5GfxWorld->exposureVolumePlaneCount;
        if (T5GfxWorld->exposureVolumePlaneCount == 0)
            T6GfxWorld->exposureVolumePlanes = nullptr;
        else
        {
            static_assert(sizeof(T5::GfxVolumePlane) == sizeof(T6::GfxVolumePlane));
            T6GfxWorld->exposureVolumePlanes = m_memory.Alloc<T6::GfxVolumePlane>(T5GfxWorld->exposureVolumePlaneCount);
            memcpy(T6GfxWorld->exposureVolumePlanes, T5GfxWorld->exposureVolumePlanes, sizeof(T5::GfxVolumePlane) * T5GfxWorld->exposureVolumePlaneCount);
        }
    }

    void GfxWorldCompiler::loadHeroLights(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        if (T5GfxWorld->heroLightCount != 0 || T5GfxWorld->heroLightTreeCount != 0)
            con::warn("T5 Map contains hero lights and they will not be added as they are not supported right now.");

        // Remove hero lights
        // Hero light trees added a left and right index to it's struct and it's use or how it works is unknown right now
        T6GfxWorld->heroLightCount = 0;
        T6GfxWorld->heroLights = nullptr;
        T6GfxWorld->heroLightTreeCount = 0;
        T6GfxWorld->heroLightTree = nullptr;
    }

    void GfxWorldCompiler::loadLUT(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        // New in T6
        // remove LUT data
        T6GfxWorld->lutVolumeCount = 0;
        T6GfxWorld->lutVolumes = nullptr;
        T6GfxWorld->lutVolumePlaneCount = 0;
        T6GfxWorld->lutVolumePlanes = nullptr;
    }

    void GfxWorldCompiler::loadOccluders(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        T6GfxWorld->numOccluders = T5GfxWorld->numOccluders;
        if (T5GfxWorld->numOccluders == 0)
            T6GfxWorld->occluders = nullptr;
        else
        {
            static_assert(sizeof(T5::Occluder) == sizeof(T6::Occluder));
            T6GfxWorld->occluders = m_memory.Alloc<T6::Occluder>(T5GfxWorld->numOccluders);
            memcpy(T6GfxWorld->occluders, T5GfxWorld->occluders, sizeof(T5::Occluder) * T5GfxWorld->numOccluders);
        }
    }

    void GfxWorldCompiler::loadSiegeSkins(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        // New in T6
        // remove Siege Skins
        T6GfxWorld->numSiegeSkinInsts = 0;
        T6GfxWorld->siegeSkinInsts = nullptr;
    }

    void GfxWorldCompiler::loadOutdoorBounds(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        T6GfxWorld->numOutdoorBounds = T5GfxWorld->numOutdoorBounds;
        if (T5GfxWorld->numOutdoorBounds == 0)
            T6GfxWorld->outdoorBounds = nullptr;
        else
        {
            static_assert(sizeof(T5::GfxOutdoorBounds) == sizeof(T6::GfxOutdoorBounds));
            T6GfxWorld->outdoorBounds = m_memory.Alloc<T6::GfxOutdoorBounds>(T5GfxWorld->numOutdoorBounds);
            memcpy(T6GfxWorld->outdoorBounds, T5GfxWorld->outdoorBounds, sizeof(T5::GfxOutdoorBounds) * T5GfxWorld->numOutdoorBounds);
        }
    }

    bool GfxWorldCompiler::loadMaterials(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        if (T5GfxWorld->ropeMaterial == nullptr)
            T6GfxWorld->ropeMaterial = nullptr;
        else
        {
            const char* ropeMaterialName = T5GfxWorld->ropeMaterial->info.name;
            auto ropeMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>(ropeMaterialName);
            if (ropeMaterialAsset == nullptr)
            {
                con::error("Unable to load T6 rope material {}.", ropeMaterialName);
                return false;
            }
            else
            {
                T6GfxWorld->ropeMaterial = ropeMaterialAsset->Asset();
            }
        }

        if (T5GfxWorld->coronaMaterial == nullptr)
            T6GfxWorld->coronaMaterial = nullptr;
        else
        {
            const char* coronaMaterialName = T5GfxWorld->coronaMaterial->info.name;
            auto coronaMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>(coronaMaterialName);
            if (coronaMaterialAsset == nullptr)
            {
                con::error("Unable to load T6 corona material {}.", coronaMaterialName);
                return false;
            }
            else
            {
                T6GfxWorld->coronaMaterial = coronaMaterialAsset->Asset();
            }
        }

        if (T5GfxWorld->waterMaterial == nullptr)
            T6GfxWorld->waterMaterial = nullptr;
        else
        {
            const char* waterMaterialName = T5GfxWorld->waterMaterial->info.name;
            auto waterMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>(waterMaterialName);
            if (waterMaterialAsset == nullptr)
            {
                con::error("Unable to load T6 water material {}.", waterMaterialName);
                return false;
            }
            else
            {
                T6GfxWorld->waterMaterial = waterMaterialAsset->Asset();
            }
        }

        // Luts are new in T6
        // Remove LUT material
        T6GfxWorld->lutMaterial = nullptr;

        T6GfxWorld->materialMemoryCount = T5GfxWorld->materialMemoryCount;
        if (T5GfxWorld->materialMemoryCount == 0)
            T6GfxWorld->materialMemory = nullptr;
        else
        {
            T6GfxWorld->materialMemory = m_memory.Alloc<T6::MaterialMemory>(T5GfxWorld->materialMemoryCount);
            for (int matIdx = 0; matIdx < T5GfxWorld->materialMemoryCount; matIdx++)
            {
                T6GfxWorld->materialMemory[matIdx].memory = T5GfxWorld->materialMemory[matIdx].memory;

                if (T5GfxWorld->materialMemory[matIdx].material == nullptr)
                    T6GfxWorld->materialMemory[matIdx].material = nullptr;
                else
                {
                    const char* materialName = T5GfxWorld->materialMemory[matIdx].material->info.name;
                    auto materialAsset = m_context.LoadDependency<T6::AssetMaterial>(materialName);
                    if (materialAsset == nullptr)
                    {
                        // surfMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>(CBSPLinkingConstants::MISSING_MATERIAL_NAME);
                        materialAsset = m_context.LoadDependency<T6::AssetMaterial>("material_template");
                        if (materialAsset == nullptr)
                        {
                            con::error("unable to load the default material {}!", "material_template");
                            return false;
                        }
                    }
                    else
                    {
                        T6GfxWorld->materialMemory[matIdx].material = materialAsset->Asset();
                    }
                }
            }
        }

        return true;
    }

    void GfxWorldCompiler::loadShadowMaps(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        T6GfxWorld->shadowMapVolumeCount = T5GfxWorld->shadowMapVolumeCount;
        if (T5GfxWorld->shadowMapVolumeCount == 0)
            T6GfxWorld->shadowMapVolumes = nullptr;
        else
        {
            static_assert(sizeof(T5::GfxShadowMapVolume) == sizeof(T6::GfxShadowMapVolume));
            T6GfxWorld->shadowMapVolumes = m_memory.Alloc<T6::GfxShadowMapVolume>(T5GfxWorld->shadowMapVolumeCount);
            memcpy(T6GfxWorld->shadowMapVolumes, T5GfxWorld->shadowMapVolumes, sizeof(T5::GfxShadowMapVolume) * T5GfxWorld->shadowMapVolumeCount);
        }

        T6GfxWorld->shadowMapVolumePlaneCount = T5GfxWorld->shadowMapVolumePlaneCount;
        if (T5GfxWorld->shadowMapVolumePlaneCount == 0)
            T6GfxWorld->shadowMapVolumePlanes = nullptr;
        else
        {
            static_assert(sizeof(T5::GfxVolumePlane) == sizeof(T6::GfxVolumePlane));
            T6GfxWorld->shadowMapVolumePlanes = m_memory.Alloc<T6::GfxVolumePlane>(T5GfxWorld->shadowMapVolumePlaneCount);
            memcpy(T6GfxWorld->shadowMapVolumePlanes, T5GfxWorld->shadowMapVolumePlanes, sizeof(T5::GfxVolumePlane) * T5GfxWorld->shadowMapVolumePlaneCount);
        }
    }

    void GfxWorldCompiler::loadStreamInfo(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        if (T5GfxWorld->streamInfo.aabbTreeCount != 0 || T5GfxWorld->streamInfo.leafRefCount != 0)
            con::warn("T5 Map contains stream info data and it will not be added as it is not supported right now.");

        // aabb trees have extra data that needs to be converted
        // remove stream info
        T6GfxWorld->streamInfo.aabbTreeCount = 0;
        T6GfxWorld->streamInfo.aabbTrees = nullptr;
        T6GfxWorld->streamInfo.leafRefCount = 0;
        T6GfxWorld->streamInfo.leafRefs = nullptr;
    }

    void GfxWorldCompiler::loadWater(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        T6GfxWorld->waterDirection = T5GfxWorld->waterDirection;

        T6GfxWorld->waterBuffers[0].bufferSize = T5GfxWorld->waterBuffers[0].bufferSize;
        if (T5GfxWorld->waterBuffers[0].bufferSize == 0)
            T6GfxWorld->waterBuffers[0].buffer = nullptr;
        else
        {
            static_assert(sizeof(T5::vec4_t) == sizeof(T6::vec4_t));
            T6GfxWorld->waterBuffers[0].buffer = static_cast<T6::vec4_t*>(m_memory.AllocRaw(T5GfxWorld->waterBuffers[0].bufferSize));
            memcpy(T6GfxWorld->waterBuffers[0].buffer, T5GfxWorld->waterBuffers[0].buffer, T5GfxWorld->waterBuffers[0].bufferSize);
        }

        T6GfxWorld->waterBuffers[1].bufferSize = T5GfxWorld->waterBuffers[1].bufferSize;
        if (T5GfxWorld->waterBuffers[1].bufferSize == 0)
            T6GfxWorld->waterBuffers[1].buffer = nullptr;
        else
        {
            static_assert(sizeof(T5::vec4_t) == sizeof(T6::vec4_t));
            T6GfxWorld->waterBuffers[1].buffer = static_cast<T6::vec4_t*>(m_memory.AllocRaw(T5GfxWorld->waterBuffers[1].bufferSize));
            memcpy(T6GfxWorld->waterBuffers[1].buffer, T5GfxWorld->waterBuffers[1].buffer, T5GfxWorld->waterBuffers[1].bufferSize);
        }
    }

    void GfxWorldCompiler::loadFog(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        // fog is new in T6
        // sunParse fog values taken from mp_dig
        T6GfxWorld->sunParse.fogTransitionTime = 0.001f;
        T6GfxWorld->sunParse.initWorldFog->baseDist = 150.0f;
        T6GfxWorld->sunParse.initWorldFog->baseHeight = -100.0f;
        T6GfxWorld->sunParse.initWorldFog->fogColor.x = 2.35f;
        T6GfxWorld->sunParse.initWorldFog->fogColor.y = 3.10f;
        T6GfxWorld->sunParse.initWorldFog->fogColor.z = 3.84f;
        T6GfxWorld->sunParse.initWorldFog->fogOpacity = 0.52f;
        T6GfxWorld->sunParse.initWorldFog->halfDist = 4450.f;
        T6GfxWorld->sunParse.initWorldFog->halfHeight = 2000.f;
        T6GfxWorld->sunParse.initWorldFog->sunFogColor.x = 5.27f;
        T6GfxWorld->sunParse.initWorldFog->sunFogColor.y = 4.73f;
        T6GfxWorld->sunParse.initWorldFog->sunFogColor.z = 3.88f;
        T6GfxWorld->sunParse.initWorldFog->sunFogInner = 0.0f;
        T6GfxWorld->sunParse.initWorldFog->sunFogOpacity = 0.67f;
        T6GfxWorld->sunParse.initWorldFog->sunFogOuter = 80.84f;
        T6GfxWorld->sunParse.initWorldFog->sunFogPitch = -29.0f;
        T6GfxWorld->sunParse.initWorldFog->sunFogYaw = 254.0f;

        // remove fog volumes
        T6GfxWorld->worldFogModifierVolumeCount = 0;
        T6GfxWorld->worldFogModifierVolumes = nullptr;
        T6GfxWorld->worldFogModifierVolumePlaneCount = 0;
        T6GfxWorld->worldFogModifierVolumePlanes = nullptr;
        T6GfxWorld->worldFogVolumeCount = 0;
        T6GfxWorld->worldFogVolumes = nullptr;
        T6GfxWorld->worldFogVolumePlaneCount = 0;
        T6GfxWorld->worldFogVolumePlanes = nullptr;
    }

    void GfxWorldCompiler::loadLightRegionHulls(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        assert(T5GfxWorld->primaryLightCount >= 2);
        T6GfxWorld->lightRegion = m_memory.Alloc<T6::GfxLightRegion>(T5GfxWorld->primaryLightCount);
        for (unsigned int lightIdx = 0; lightIdx < T5GfxWorld->primaryLightCount; lightIdx++)
        {
            T6GfxWorld->lightRegion[lightIdx].hullCount = T5GfxWorld->lightRegion[lightIdx].hullCount;
            if (T5GfxWorld->lightRegion[lightIdx].hullCount == 0)
                T6GfxWorld->lightRegion[lightIdx].hulls = nullptr;
            else
            {
                T6GfxWorld->lightRegion[lightIdx].hulls = m_memory.Alloc<T6::GfxLightRegionHull>(T5GfxWorld->lightRegion[lightIdx].hullCount);
                for (unsigned int hullIdx = 0; hullIdx < T5GfxWorld->lightRegion[lightIdx].hullCount; hullIdx++)
                {
                    T5::GfxLightRegionHull* T5Hull = &T5GfxWorld->lightRegion[lightIdx].hulls[hullIdx];
                    T6::GfxLightRegionHull* T6Hull = &T6GfxWorld->lightRegion[lightIdx].hulls[hullIdx];

                    T6Hull->kdopHalfSize[0] = T5Hull->kdopHalfSize[0];
                    T6Hull->kdopHalfSize[1] = T5Hull->kdopHalfSize[1];
                    T6Hull->kdopHalfSize[2] = T5Hull->kdopHalfSize[2];
                    T6Hull->kdopHalfSize[3] = T5Hull->kdopHalfSize[3];
                    T6Hull->kdopHalfSize[4] = T5Hull->kdopHalfSize[4];
                    T6Hull->kdopHalfSize[5] = T5Hull->kdopHalfSize[5];
                    T6Hull->kdopHalfSize[6] = T5Hull->kdopHalfSize[6];
                    T6Hull->kdopHalfSize[7] = T5Hull->kdopHalfSize[7];
                    T6Hull->kdopHalfSize[8] = T5Hull->kdopHalfSize[8];

                    T6Hull->kdopMidPoint[0] = T5Hull->kdopMidPoint[0];
                    T6Hull->kdopMidPoint[1] = T5Hull->kdopMidPoint[1];
                    T6Hull->kdopMidPoint[2] = T5Hull->kdopMidPoint[2];
                    T6Hull->kdopMidPoint[3] = T5Hull->kdopMidPoint[3];
                    T6Hull->kdopMidPoint[4] = T5Hull->kdopMidPoint[4];
                    T6Hull->kdopMidPoint[5] = T5Hull->kdopMidPoint[5];
                    T6Hull->kdopMidPoint[6] = T5Hull->kdopMidPoint[6];
                    T6Hull->kdopMidPoint[7] = T5Hull->kdopMidPoint[7];
                    T6Hull->kdopMidPoint[8] = T5Hull->kdopMidPoint[8];

                    if (T5Hull->axisCount == 0)
                    {
                        T6Hull->axisCount = 0;
                        T6Hull->axis = nullptr;
                    }
                    else
                    {
                        T6Hull->axisCount = T5Hull->axisCount;
                        T6Hull->axis = m_memory.Alloc<T6::GfxLightRegionAxis>(T5Hull->axisCount);
                        for (unsigned int axisIdx = 0; axisIdx < T5Hull->axisCount; axisIdx++)
                        {
                            T6Hull->axis[axisIdx].dir.x = T5Hull->axis[axisIdx].dir[0];
                            T6Hull->axis[axisIdx].dir.y = T5Hull->axis[axisIdx].dir[1];
                            T6Hull->axis[axisIdx].dir.z = T5Hull->axis[axisIdx].dir[2];
                            T6Hull->axis[axisIdx].halfSize = T5Hull->axis[axisIdx].halfSize;
                            T6Hull->axis[axisIdx].midPoint = T5Hull->axis[axisIdx].midPoint;
                        }
                    }
                }
            }
        }
    }

    void GfxWorldCompiler::loadGfxLights(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        assert(T5GfxWorld->primaryLightCount >= 2);

        T6GfxWorld->primaryLightCount = T5GfxWorld->primaryLightCount;
        T6GfxWorld->sunPrimaryLightIndex = T5GfxWorld->sunPrimaryLightIndex;

        T6GfxWorld->shadowGeom = m_memory.Alloc<T6::GfxShadowGeometry>(T5GfxWorld->primaryLightCount);
        for (unsigned int lightIdx = 0; lightIdx < T5GfxWorld->primaryLightCount; lightIdx++)
        {
            // TODO: smodels aren't implemented right now
            T6GfxWorld->shadowGeom[lightIdx].smodelCount = 0;
            T6GfxWorld->shadowGeom[lightIdx].smodelIndex = nullptr;

            if (T5GfxWorld->shadowGeom[lightIdx].surfaceCount == 0)
            {
                T6GfxWorld->shadowGeom[lightIdx].surfaceCount = 0;
                T6GfxWorld->shadowGeom[lightIdx].sortedSurfIndex = nullptr;
            }
            else
            {
                // sorted surf index and surfaceCount is overwritten by the game and re-initialised each frame
                // Using T5 values results in a buffer overflow, so we set each surf count to the surface count
                T6GfxWorld->shadowGeom[lightIdx].surfaceCount = T5GfxWorld->dpvs.staticSurfaceCount;
                T6GfxWorld->shadowGeom[lightIdx].sortedSurfIndex = m_memory.Alloc<uint16_t>(T5GfxWorld->dpvs.staticSurfaceCount);
            }
        }
        loadLightRegionHulls(T5GfxWorld, T6GfxWorld);

        unsigned int lightEntShadowVisCount = (T5GfxWorld->primaryLightCount - T5GfxWorld->sunPrimaryLightIndex - 1) * 8192;
        if (lightEntShadowVisCount != 0)
            T6GfxWorld->primaryLightEntityShadowVis = m_memory.Alloc<unsigned int>(lightEntShadowVisCount);
        else
            T6GfxWorld->primaryLightEntityShadowVis = nullptr;
    }

    void GfxWorldCompiler::loadLightGrid(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        T6GfxWorld->lightGrid.sunPrimaryLightIndex = T5GfxWorld->lightGrid.sunPrimaryLightIndex;
        T6GfxWorld->lightGrid.mins[0] = T5GfxWorld->lightGrid.mins[0];
        T6GfxWorld->lightGrid.mins[1] = T5GfxWorld->lightGrid.mins[1];
        T6GfxWorld->lightGrid.mins[2] = T5GfxWorld->lightGrid.mins[2];
        T6GfxWorld->lightGrid.maxs[0] = T5GfxWorld->lightGrid.maxs[0];
        T6GfxWorld->lightGrid.maxs[1] = T5GfxWorld->lightGrid.maxs[1];
        T6GfxWorld->lightGrid.maxs[2] = T5GfxWorld->lightGrid.maxs[2];
        T6GfxWorld->lightGrid.rowAxis = T5GfxWorld->lightGrid.rowAxis;
        T6GfxWorld->lightGrid.colAxis = T5GfxWorld->lightGrid.colAxis;

        int rowDataStartSize = T5GfxWorld->lightGrid.maxs[T5GfxWorld->lightGrid.rowAxis] - T5GfxWorld->lightGrid.mins[T5GfxWorld->lightGrid.rowAxis] + 1;
        T6GfxWorld->lightGrid.rowDataStart = m_memory.Alloc<uint16_t>(rowDataStartSize);
        memcpy(T6GfxWorld->lightGrid.rowDataStart, T5GfxWorld->lightGrid.rowDataStart, sizeof(uint16_t) * rowDataStartSize);

        T6GfxWorld->lightGrid.rawRowDataSize = T5GfxWorld->lightGrid.rawRowDataSize;
        T6GfxWorld->lightGrid.rawRowData = m_memory.Alloc<T6::aligned_byte_pointer>(T5GfxWorld->lightGrid.rawRowDataSize);
        memcpy(T6GfxWorld->lightGrid.rawRowData, T5GfxWorld->lightGrid.rawRowData, T5GfxWorld->lightGrid.rawRowDataSize);

        static_assert(sizeof(T5::GfxLightGridEntry) == sizeof(T6::GfxLightGridEntry));
        T6GfxWorld->lightGrid.entryCount = T5GfxWorld->lightGrid.entryCount;
        T6GfxWorld->lightGrid.entries = m_memory.Alloc<T6::GfxLightGridEntry>(T5GfxWorld->lightGrid.entryCount);
        memcpy(T6GfxWorld->lightGrid.entries, T5GfxWorld->lightGrid.entries, sizeof(T5::GfxLightGridEntry) * T5GfxWorld->lightGrid.entryCount);

        static_assert(sizeof(T5::GfxCompressedLightGridColors) == sizeof(T6::GfxCompressedLightGridColors));
        T6GfxWorld->lightGrid.colorCount = T5GfxWorld->lightGrid.colorCount;
        T6GfxWorld->lightGrid.colors = m_memory.Alloc<T6::GfxCompressedLightGridColors>(T5GfxWorld->lightGrid.colorCount);
        memcpy(T6GfxWorld->lightGrid.colors, T5GfxWorld->lightGrid.colors, sizeof(T5::GfxCompressedLightGridColors) * T5GfxWorld->lightGrid.colorCount);

        // new in T6
        T6GfxWorld->lightGrid.offset = 0.0f; // default value from mp_dig
        T6GfxWorld->lightGrid.coeffCount = 0;
        T6GfxWorld->lightGrid.coeffs = nullptr; // we use the colours array instead of coeffs array
        T6GfxWorld->lightGrid.skyGridVolumeCount = 0;
        T6GfxWorld->lightGrid.skyGridVolumes = nullptr;
    }

    void GfxWorldCompiler::loadGfxCells(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        int cellCount = T5GfxWorld->dpvsPlanes.cellCount;
        T6GfxWorld->dpvsPlanes.cellCount = T5GfxWorld->dpvsPlanes.cellCount;
        T6GfxWorld->cellBitsCount = T5GfxWorld->cellBitsCount;

        int cellCasterBitsCount = cellCount * ((cellCount + 31) / 32);
        T6GfxWorld->cellCasterBits = m_memory.Alloc<unsigned int>(cellCasterBitsCount);

        int sceneEntCellBitsCount = cellCount * 512;
        T6GfxWorld->dpvsPlanes.sceneEntCellBits = m_memory.Alloc<unsigned int>(sceneEntCellBitsCount);

        T6GfxWorld->cells = m_memory.Alloc<T6::GfxCell>(cellCount);
        for (int cellIdx = 0; cellIdx < cellCount; cellIdx++)
        {
            T5::GfxCell* T5Cell = &T5GfxWorld->cells[cellIdx];
            T6::GfxCell* T6Cell = &T6GfxWorld->cells[cellIdx];

            T6Cell->mins.x = T5Cell->mins[0];
            T6Cell->mins.y = T5Cell->mins[1];
            T6Cell->mins.z = T5Cell->mins[2];
            T6Cell->maxs.x = T5Cell->maxs[0];
            T6Cell->maxs.y = T5Cell->maxs[1];
            T6Cell->maxs.z = T5Cell->maxs[2];

            T6Cell->reflectionProbeCount = T5Cell->reflectionProbeCount;
            if (T5Cell->reflectionProbeCount == 0)
                T6Cell->reflectionProbes = nullptr;
            else
            {
                T6Cell->reflectionProbes = m_memory.Alloc<char>(T5Cell->reflectionProbeCount);
                for (char probeIdx = 0; probeIdx < T5Cell->reflectionProbeCount; probeIdx++)
                    T6Cell->reflectionProbes[probeIdx] = T5Cell->reflectionProbes[probeIdx];
            }

            T6Cell->aabbTreeCount = T5Cell->aabbTreeCount;
            if (T5Cell->aabbTreeCount == 0)
                T6Cell->aabbTree = nullptr;
            else
            {
                T6Cell->aabbTree = m_memory.Alloc<T6::GfxAabbTree>(T5Cell->aabbTreeCount);
                for (int aabbIdx = 0; aabbIdx < T5Cell->aabbTreeCount; aabbIdx++)
                {
                    T5::GfxAabbTree* T5AABB = &T5Cell->aabbTree[aabbIdx];
                    T6::GfxAabbTree* T6AABB = &T6Cell->aabbTree[aabbIdx];

                    T6AABB->mins.x = T5AABB->mins[0];
                    T6AABB->mins.y = T5AABB->mins[1];
                    T6AABB->mins.z = T5AABB->mins[2];
                    T6AABB->maxs.x = T5AABB->maxs[0];
                    T6AABB->maxs.y = T5AABB->maxs[1];
                    T6AABB->maxs.z = T5AABB->maxs[2];

                    T6AABB->childCount = T5AABB->childCount;
                    T6AABB->surfaceCount = T5AABB->surfaceCount;
                    T6AABB->startSurfIndex = T5AABB->startSurfIndex;
                    T6AABB->childrenOffset = T5AABB->childrenOffset;

                    // xmodels are unimplemented right now
                    T6AABB->smodelIndexCount = 0;
                    T6AABB->smodelIndexes = nullptr;
                }
            }

            T6Cell->portalCount = T5Cell->portalCount;
            if (T5Cell->portalCount == 0)
                T6Cell->portals = nullptr;
            else
            {
                T6Cell->portals = m_memory.Alloc<T6::GfxPortal>(T5Cell->portalCount);
                for (char portalIdx = 0; portalIdx < T5Cell->portalCount; portalIdx++)
                {
                    T5::GfxPortal* T5Portal = &T5Cell->portals[portalIdx];
                    T6::GfxPortal* T6Portal = &T6Cell->portals[portalIdx];

                    // writable is always zeroed
                    T6Portal->plane.coeffs.x = T5Portal->plane.coeffs[0];
                    T6Portal->plane.coeffs.y = T5Portal->plane.coeffs[1];
                    T6Portal->plane.coeffs.z = T5Portal->plane.coeffs[2];
                    T6Portal->plane.coeffs.w = T5Portal->plane.coeffs[3];
                    T6Portal->plane.side[0] = T5Portal->plane.side[0];
                    T6Portal->plane.side[1] = T5Portal->plane.side[1];
                    T6Portal->plane.side[2] = T5Portal->plane.side[2];
                    T6Portal->plane.pad = T5Portal->plane.pad;

                    T6Portal->hullAxis[0].x = T5Portal->hullAxis[0][0];
                    T6Portal->hullAxis[0].y = T5Portal->hullAxis[0][1];
                    T6Portal->hullAxis[0].z = T5Portal->hullAxis[0][2];
                    T6Portal->hullAxis[1].x = T5Portal->hullAxis[1][0];
                    T6Portal->hullAxis[1].y = T5Portal->hullAxis[1][1];
                    T6Portal->hullAxis[1].z = T5Portal->hullAxis[1][2];

                    T6Portal->vertexCount = T5Portal->vertexCount;
                    T6Portal->vertices = m_memory.Alloc<T6::vec3_t>(T5Portal->vertexCount);
                    memcpy(T6Portal->vertices, T5Portal->vertices, sizeof(T5::vec3_t) * T5Portal->vertexCount);

                    // new in T6
                    T6Portal->bounds[0].x = T5Portal->vertices[0].x;
                    T6Portal->bounds[0].y = T5Portal->vertices[0].y;
                    T6Portal->bounds[0].z = T5Portal->vertices[0].z;
                    T6Portal->bounds[1].x = T5Portal->vertices[0].x;
                    T6Portal->bounds[1].y = T5Portal->vertices[0].y;
                    T6Portal->bounds[1].z = T5Portal->vertices[0].z;
                    for (char vertIdx = 0; vertIdx < T5Portal->vertexCount; vertIdx++)
                    {
                        BSPUtil::updateAABBWithPoint(T6Portal->vertices[vertIdx], T6Portal->bounds[0], T6Portal->bounds[1]);
                    }

                    int foundIdx = -1;
                    for (int idx = 0; idx < cellCount; idx++)
                    {
                        if (T5Portal->cell == &T5GfxWorld->cells[idx])
                        {
                            foundIdx = idx;
                            break;
                        }
                    }
                    assert(foundIdx != -1);
                    T6Portal->cell = &T6GfxWorld->cells[foundIdx];
                }
            }
        }

        T6GfxWorld->nodeCount = T5GfxWorld->nodeCount;
        T6GfxWorld->dpvsPlanes.nodes = m_memory.Alloc<uint16_t>(T5GfxWorld->nodeCount);
        memcpy(T6GfxWorld->dpvsPlanes.nodes, T5GfxWorld->dpvsPlanes.nodes, sizeof(uint16_t) * T5GfxWorld->nodeCount);

        T6GfxWorld->planeCount = T5GfxWorld->planeCount;
        T6GfxWorld->dpvsPlanes.planes = m_memory.Alloc<T6::cplane_s>(T5GfxWorld->planeCount);
        static_assert(sizeof(T5::cplane_s) == sizeof(T6::cplane_s));
        memcpy(T6GfxWorld->dpvsPlanes.planes, T5GfxWorld->dpvsPlanes.planes, sizeof(T5::cplane_s) * T5GfxWorld->planeCount);
    }

    void GfxWorldCompiler::loadWorldBounds(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        T6GfxWorld->mins.x = T5GfxWorld->mins[0];
        T6GfxWorld->mins.y = T5GfxWorld->mins[1];
        T6GfxWorld->mins.z = T5GfxWorld->mins[2];
        T6GfxWorld->maxs.x = T5GfxWorld->maxs[0];
        T6GfxWorld->maxs.y = T5GfxWorld->maxs[1];
        T6GfxWorld->maxs.z = T5GfxWorld->maxs[2];
    }

    void GfxWorldCompiler::loadModels(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        // Models (Submodels in the clipmap code) are used for the world and map ent collision (triggers, bomb zones, etc)
        // Right now there is only one submodel, the world sub model
        T6GfxWorld->modelCount = 1;
        T6GfxWorld->models = m_memory.Alloc<T6::GfxBrushModel>(1);

        // first model is always the world model
        T6GfxWorld->models[0].startSurfIndex = 0;
        T6GfxWorld->models[0].surfaceCount = static_cast<unsigned int>(T5GfxWorld->dpvs.staticSurfaceCount); // uses static not total surface count
        T6GfxWorld->models[0].bounds[0].x = T5GfxWorld->mins[0];
        T6GfxWorld->models[0].bounds[0].y = T5GfxWorld->mins[1];
        T6GfxWorld->models[0].bounds[0].z = T5GfxWorld->mins[2];
        T6GfxWorld->models[0].bounds[1].x = T5GfxWorld->maxs[0];
        T6GfxWorld->models[0].bounds[1].y = T5GfxWorld->maxs[1];
        T6GfxWorld->models[0].bounds[1].z = T5GfxWorld->maxs[2];
        memset(&T6GfxWorld->models[0].writable, 0, sizeof(T6::GfxBrushModelWritable));

        // Other models aren't implemented yet
        // Code kept for future use
        // for (size_t i = 0; i < entityModelList.size(); i++)
        //{
        //    auto currEntModel = &gfxWorld->models[i + 1];
        //    entModelBounds currEntModelBounds = entityModelList[i];
        //
        //    currEntModel->startSurfIndex = 0;
        //    currEntModel->surfaceCount = -1; // -1 when it doesn't use map surfaces
        //    currEntModel->bounds[0].x = currEntModelBounds.mins.x;
        //    currEntModel->bounds[0].y = currEntModelBounds.mins.y;
        //    currEntModel->bounds[0].z = currEntModelBounds.mins.z;
        //    currEntModel->bounds[1].x = currEntModelBounds.maxs.x;
        //    currEntModel->bounds[1].y = currEntModelBounds.maxs.y;
        //    currEntModel->bounds[1].z = currEntModelBounds.maxs.z;
        //    memset(&gfxWorld->models[0].writable, 0, sizeof(GfxBrushModelWritable));
        //}
    }

    bool GfxWorldCompiler::loadSunData(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        //// default values taken from mp_dig
        // T6GfxWorld->sunParse.fogTransitionTime = 0.001f;
        // T6GfxWorld->sunParse.name[0] = 0x00;
        //
        // T6GfxWorld->sunParse.initWorldSun->control = 0;
        // T6GfxWorld->sunParse.initWorldSun->exposure = 2.5f;
        // T6GfxWorld->sunParse.initWorldSun->angles.x = -29.0f;
        // T6GfxWorld->sunParse.initWorldSun->angles.y = 254.0f;
        // T6GfxWorld->sunParse.initWorldSun->angles.z = 0.0f;
        // T6GfxWorld->sunParse.initWorldSun->sunCd.x = 1.0f;
        // T6GfxWorld->sunParse.initWorldSun->sunCd.y = 0.89f;
        // T6GfxWorld->sunParse.initWorldSun->sunCd.z = 0.69f;
        // T6GfxWorld->sunParse.initWorldSun->sunCd.w = 13.5f;
        // T6GfxWorld->sunParse.initWorldSun->ambientColor.x = 0.0f;
        // T6GfxWorld->sunParse.initWorldSun->ambientColor.y = 0.0f;
        // T6GfxWorld->sunParse.initWorldSun->ambientColor.z = 0.0f;
        // T6GfxWorld->sunParse.initWorldSun->ambientColor.w = 0.0f;
        // T6GfxWorld->sunParse.initWorldSun->skyColor.x = 0.0f;
        // T6GfxWorld->sunParse.initWorldSun->skyColor.y = 0.0f;
        // T6GfxWorld->sunParse.initWorldSun->skyColor.z = 0.0f;
        // T6GfxWorld->sunParse.initWorldSun->skyColor.w = 0.0f;
        // T6GfxWorld->sunParse.initWorldSun->sunCs.x = 0.0f;
        // T6GfxWorld->sunParse.initWorldSun->sunCs.y = 0.0f;
        // T6GfxWorld->sunParse.initWorldSun->sunCs.z = 0.0f;
        // T6GfxWorld->sunParse.initWorldSun->sunCs.w = 0.0f;
        //
        // T6GfxWorld->sunParse.initWorldFog->baseDist = 150.0f;
        // T6GfxWorld->sunParse.initWorldFog->baseHeight = -100.0f;
        // T6GfxWorld->sunParse.initWorldFog->fogColor.x = 2.35f;
        // T6GfxWorld->sunParse.initWorldFog->fogColor.y = 3.10f;
        // T6GfxWorld->sunParse.initWorldFog->fogColor.z = 3.84f;
        // T6GfxWorld->sunParse.initWorldFog->fogOpacity = 0.52f;
        // T6GfxWorld->sunParse.initWorldFog->halfDist = 4450.f;
        // T6GfxWorld->sunParse.initWorldFog->halfHeight = 2000.f;
        // T6GfxWorld->sunParse.initWorldFog->sunFogColor.x = 5.27f;
        // T6GfxWorld->sunParse.initWorldFog->sunFogColor.y = 4.73f;
        // T6GfxWorld->sunParse.initWorldFog->sunFogColor.z = 3.88f;
        // T6GfxWorld->sunParse.initWorldFog->sunFogInner = 0.0f;
        // T6GfxWorld->sunParse.initWorldFog->sunFogOpacity = 0.67f;
        // T6GfxWorld->sunParse.initWorldFog->sunFogOuter = 80.84f;
        // T6GfxWorld->sunParse.initWorldFog->sunFogPitch = -29.0f;
        // T6GfxWorld->sunParse.initWorldFog->sunFogYaw = 254.0f;

        memcpy(T6GfxWorld->sunParse.name, T5GfxWorld->sunParse.name, sizeof(T5GfxWorld->sunParse.name));

        T6GfxWorld->sunParse.initWorldSun->control = T5GfxWorld->sunParse.sunSettings->control;
        T6GfxWorld->sunParse.initWorldSun->angles.x = T5GfxWorld->sunParse.sunSettings->angles[0];
        T6GfxWorld->sunParse.initWorldSun->angles.y = T5GfxWorld->sunParse.sunSettings->angles[1];
        T6GfxWorld->sunParse.initWorldSun->angles.z = T5GfxWorld->sunParse.sunSettings->angles[2];
        T6GfxWorld->sunParse.initWorldSun->ambientColor.x = T5GfxWorld->sunParse.sunSettings->ambientColor[0];
        T6GfxWorld->sunParse.initWorldSun->ambientColor.y = T5GfxWorld->sunParse.sunSettings->ambientColor[1];
        T6GfxWorld->sunParse.initWorldSun->ambientColor.z = T5GfxWorld->sunParse.sunSettings->ambientColor[2];
        T6GfxWorld->sunParse.initWorldSun->ambientColor.w = T5GfxWorld->sunParse.sunSettings->ambientColor[3];
        T6GfxWorld->sunParse.initWorldSun->sunCd.x = T5GfxWorld->sunParse.sunSettings->sunDiffuseColor[0];
        T6GfxWorld->sunParse.initWorldSun->sunCd.y = T5GfxWorld->sunParse.sunSettings->sunDiffuseColor[1];
        T6GfxWorld->sunParse.initWorldSun->sunCd.z = T5GfxWorld->sunParse.sunSettings->sunDiffuseColor[2];
        T6GfxWorld->sunParse.initWorldSun->sunCd.w = T5GfxWorld->sunParse.sunSettings->sunDiffuseColor[3];
        T6GfxWorld->sunParse.initWorldSun->sunCs.x = T5GfxWorld->sunParse.sunSettings->sunSpecularColor[0];
        T6GfxWorld->sunParse.initWorldSun->sunCs.y = T5GfxWorld->sunParse.sunSettings->sunSpecularColor[1];
        T6GfxWorld->sunParse.initWorldSun->sunCs.z = T5GfxWorld->sunParse.sunSettings->sunSpecularColor[2];
        T6GfxWorld->sunParse.initWorldSun->sunCs.w = T5GfxWorld->sunParse.sunSettings->sunSpecularColor[3];
        T6GfxWorld->sunParse.initWorldSun->skyColor.x = T5GfxWorld->sunParse.sunSettings->skyColor[0];
        T6GfxWorld->sunParse.initWorldSun->skyColor.y = T5GfxWorld->sunParse.sunSettings->skyColor[1];
        T6GfxWorld->sunParse.initWorldSun->skyColor.z = T5GfxWorld->sunParse.sunSettings->skyColor[2];
        T6GfxWorld->sunParse.initWorldSun->skyColor.w = T5GfxWorld->sunParse.sunSettings->skyColor[3];
        T6GfxWorld->sunParse.initWorldSun->exposure = T5GfxWorld->sunParse.sunSettings->exposure;

        T6GfxWorld->sun.hasValidData = T5GfxWorld->sun.hasValidData;
        T6GfxWorld->sun.spriteSize = T5GfxWorld->sun.spriteSize;
        T6GfxWorld->sun.flareMinSize = T5GfxWorld->sun.flareMinSize;
        T6GfxWorld->sun.flareMinDot = T5GfxWorld->sun.flareMinDot;
        T6GfxWorld->sun.flareMaxSize = T5GfxWorld->sun.flareMaxSize;
        T6GfxWorld->sun.flareMaxDot = T5GfxWorld->sun.flareMaxDot;
        T6GfxWorld->sun.flareMaxAlpha = T5GfxWorld->sun.flareMaxAlpha;
        T6GfxWorld->sun.flareFadeInTime = T5GfxWorld->sun.flareFadeInTime;
        T6GfxWorld->sun.flareFadeOutTime = T5GfxWorld->sun.flareFadeOutTime;
        T6GfxWorld->sun.blindMinDot = T5GfxWorld->sun.blindMinDot;
        T6GfxWorld->sun.blindMaxDot = T5GfxWorld->sun.blindMaxDot;
        T6GfxWorld->sun.blindMaxDarken = T5GfxWorld->sun.blindMaxDarken;
        T6GfxWorld->sun.blindFadeInTime = T5GfxWorld->sun.blindFadeInTime;
        T6GfxWorld->sun.blindFadeOutTime = T5GfxWorld->sun.blindFadeOutTime;
        T6GfxWorld->sun.glareMinDot = T5GfxWorld->sun.glareMinDot;
        T6GfxWorld->sun.glareMaxDot = T5GfxWorld->sun.glareMaxDot;
        T6GfxWorld->sun.glareMaxLighten = T5GfxWorld->sun.glareMaxLighten;
        T6GfxWorld->sun.glareFadeInTime = T5GfxWorld->sun.glareFadeInTime;
        T6GfxWorld->sun.glareFadeOutTime = T5GfxWorld->sun.glareFadeOutTime;
        T6GfxWorld->sun.sunFxPosition.x = T5GfxWorld->sun.sunFxPosition[0];
        T6GfxWorld->sun.sunFxPosition.y = T5GfxWorld->sun.sunFxPosition[1];
        T6GfxWorld->sun.sunFxPosition.z = T5GfxWorld->sun.sunFxPosition[2];

        if (T5GfxWorld->sun.spriteMaterial == nullptr)
            T6GfxWorld->sun.spriteMaterial = nullptr;
        else
        {
            const char* spriteMaterialName = T5GfxWorld->sun.spriteMaterial->info.name;
            auto spriteMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>(spriteMaterialName);
            if (spriteMaterialAsset == nullptr)
            {
                con::error("Unable to load T6 sun sprite material {}.", spriteMaterialName);
                return false;
            }
            else
            {
                T6GfxWorld->sun.spriteMaterial = spriteMaterialAsset->Asset();
            }
        }

        if (T5GfxWorld->sun.flareMaterial == nullptr)
            T6GfxWorld->sun.flareMaterial = nullptr;
        else
        {
            const char* flareMaterialName = T5GfxWorld->sun.flareMaterial->info.name;
            auto flareMaterialAsset = m_context.LoadDependency<T6::AssetMaterial>(flareMaterialName);
            if (flareMaterialAsset == nullptr)
            {
                con::error("Unable to load T6 sun flare material {}.", flareMaterialName);
                return false;
            }
            else
            {
                T6GfxWorld->sun.flareMaterial = flareMaterialAsset->Asset();
            }
        }

        T6GfxWorld->sunLight = m_memory.Alloc<T6::GfxLight>();
        T5::GfxLight* T5Light = T5GfxWorld->sunLight;
        T6::GfxLight* T6Light = T6GfxWorld->sunLight;
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
        T6Light->exponent = T5Light->exponent;
        T6Light->spotShadowIndex = T5Light->spotShadowIndex;
        T6Light->angles.x = T5Light->angles[0];
        T6Light->angles.y = T5Light->angles[1];
        T6Light->angles.z = T5Light->angles[2];
        T6Light->spotShadowHiDistance = T5Light->spotShadowHiDistance;
        T6Light->diffuseColor.x = T5Light->diffuseColor[0];
        T6Light->diffuseColor.y = T5Light->diffuseColor[1];
        T6Light->diffuseColor.z = T5Light->diffuseColor[2];
        T6Light->diffuseColor.w = T5Light->diffuseColor[3];
        T6Light->shadowColor.x = T5Light->shadowColor[0];
        T6Light->shadowColor.y = T5Light->shadowColor[1];
        T6Light->shadowColor.z = T5Light->shadowColor[2];
        T6Light->shadowColor.w = T5Light->shadowColor[3];
        T6Light->falloff.x = T5Light->falloff[0];
        T6Light->falloff.y = T5Light->falloff[1];
        T6Light->falloff.z = T5Light->falloff[2];
        T6Light->falloff.w = T5Light->falloff[3];
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

        static_assert(sizeof(T5::float44) == sizeof(T6::float44));
        memcpy(&T6Light->viewMatrix, &T5Light->viewMatrix, sizeof(T5::float44));
        memcpy(&T6Light->projMatrix, &T5Light->projMatrix, sizeof(T5::float44));

        if (T5Light->def == nullptr)
            T6Light->def = nullptr;
        else
        {
            auto lightDefAsset = m_context.LoadDependency<T6::AssetLightDef>(T5Light->def->name);
            if (lightDefAsset == nullptr)
                assert(false);
            else
                T6Light->def = lightDefAsset->Asset();
        }
        // T6 differences
        T6Light->shadowmapVolume = 0;     // added t5->t6, same position as T5Light->_pad[1]
        T6Light->dAttenuation = 10000.0f; // vec4 in t5, float in t6 (10000 for testing: pre sure attenuation effects lights)
        T6Light->roundness = 0.0f;        // added t5->t6

        return true;
    }

    bool GfxWorldCompiler::loadReflectionProbeData(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        assert(T5GfxWorld->draw.reflectionProbeCount != 0);
        // max 31 probes
        T6GfxWorld->draw.reflectionProbeCount = T5GfxWorld->draw.reflectionProbeCount;

        // reflectionProbeTextures is always empty
        T6GfxWorld->draw.reflectionProbeTextures = m_memory.Alloc<T6::GfxTexture>(T5GfxWorld->draw.reflectionProbeCount);

        T6GfxWorld->draw.reflectionProbes = m_memory.Alloc<T6::GfxReflectionProbe>(T5GfxWorld->draw.reflectionProbeCount);
        for (unsigned int probeIdx = 0; probeIdx < T5GfxWorld->draw.reflectionProbeCount; probeIdx++)
        {
            T5::GfxReflectionProbe* T5probe = &T5GfxWorld->draw.reflectionProbes[probeIdx];
            T6::GfxReflectionProbe* T6probe = &T6GfxWorld->draw.reflectionProbes[probeIdx];

            T6probe->origin.x = T5probe->origin[0];
            T6probe->origin.y = T5probe->origin[1];
            T6probe->origin.z = T5probe->origin[2];

            auto probeImageAsset = m_context.LoadDependency<T6::AssetImage>(T5probe->reflectionImage->name);
            if (probeImageAsset == nullptr)
            {
                con::error("ERROR! unable to load reflection probe image {}!", T5probe->reflectionImage->name);
                return false;
            }
            T6probe->reflectionImage = probeImageAsset->Asset();

            // TODO: new in T6, mipLodBias taken from mp_dig
            T6probe->mipLodBias = -8.0;
            T6probe->lightingSH.V0.x = 0.0f;
            T6probe->lightingSH.V0.y = 0.0f;
            T6probe->lightingSH.V0.z = 0.0f;
            T6probe->lightingSH.V0.w = 0.0f;
            T6probe->lightingSH.V1.x = 0.0f;
            T6probe->lightingSH.V1.y = 0.0f;
            T6probe->lightingSH.V1.z = 0.0f;
            T6probe->lightingSH.V1.w = 0.0f;
            T6probe->lightingSH.V2.x = 0.0f;
            T6probe->lightingSH.V2.y = 0.0f;
            T6probe->lightingSH.V2.z = 0.0f;
            T6probe->lightingSH.V2.w = 0.0f;

            T6probe->probeVolumeCount = T5probe->probeVolumeCount;
            T6probe->probeVolumes = m_memory.Alloc<T6::GfxReflectionProbeVolumeData>(T5probe->probeVolumeCount);
            for (unsigned int volIdx = 0; volIdx < T5probe->probeVolumeCount; volIdx++)
            {
                T6probe->probeVolumes[volIdx].volumePlanes[0].x = T5probe->probeVolumes[volIdx].volumePlanes[0][0];
                T6probe->probeVolumes[volIdx].volumePlanes[0].y = T5probe->probeVolumes[volIdx].volumePlanes[0][1];
                T6probe->probeVolumes[volIdx].volumePlanes[0].z = T5probe->probeVolumes[volIdx].volumePlanes[0][2];
                T6probe->probeVolumes[volIdx].volumePlanes[0].w = T5probe->probeVolumes[volIdx].volumePlanes[0][3];

                T6probe->probeVolumes[volIdx].volumePlanes[1].x = T5probe->probeVolumes[volIdx].volumePlanes[1][0];
                T6probe->probeVolumes[volIdx].volumePlanes[1].y = T5probe->probeVolumes[volIdx].volumePlanes[1][1];
                T6probe->probeVolumes[volIdx].volumePlanes[1].z = T5probe->probeVolumes[volIdx].volumePlanes[1][2];
                T6probe->probeVolumes[volIdx].volumePlanes[1].w = T5probe->probeVolumes[volIdx].volumePlanes[1][3];

                T6probe->probeVolumes[volIdx].volumePlanes[2].x = T5probe->probeVolumes[volIdx].volumePlanes[2][0];
                T6probe->probeVolumes[volIdx].volumePlanes[2].y = T5probe->probeVolumes[volIdx].volumePlanes[2][1];
                T6probe->probeVolumes[volIdx].volumePlanes[2].z = T5probe->probeVolumes[volIdx].volumePlanes[2][2];
                T6probe->probeVolumes[volIdx].volumePlanes[2].w = T5probe->probeVolumes[volIdx].volumePlanes[2][3];

                T6probe->probeVolumes[volIdx].volumePlanes[3].x = T5probe->probeVolumes[volIdx].volumePlanes[3][0];
                T6probe->probeVolumes[volIdx].volumePlanes[3].y = T5probe->probeVolumes[volIdx].volumePlanes[3][1];
                T6probe->probeVolumes[volIdx].volumePlanes[3].z = T5probe->probeVolumes[volIdx].volumePlanes[3][2];
                T6probe->probeVolumes[volIdx].volumePlanes[3].w = T5probe->probeVolumes[volIdx].volumePlanes[3][3];

                T6probe->probeVolumes[volIdx].volumePlanes[4].x = T5probe->probeVolumes[volIdx].volumePlanes[4][0];
                T6probe->probeVolumes[volIdx].volumePlanes[4].y = T5probe->probeVolumes[volIdx].volumePlanes[4][1];
                T6probe->probeVolumes[volIdx].volumePlanes[4].z = T5probe->probeVolumes[volIdx].volumePlanes[4][2];
                T6probe->probeVolumes[volIdx].volumePlanes[4].w = T5probe->probeVolumes[volIdx].volumePlanes[4][3];

                T6probe->probeVolumes[volIdx].volumePlanes[5].x = T5probe->probeVolumes[volIdx].volumePlanes[5][0];
                T6probe->probeVolumes[volIdx].volumePlanes[5].y = T5probe->probeVolumes[volIdx].volumePlanes[5][1];
                T6probe->probeVolumes[volIdx].volumePlanes[5].z = T5probe->probeVolumes[volIdx].volumePlanes[5][2];
                T6probe->probeVolumes[volIdx].volumePlanes[5].w = T5probe->probeVolumes[volIdx].volumePlanes[5][3];
            }
        }

        return true;
    }

    bool GfxWorldCompiler::loadLightmapData(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        assert(T5GfxWorld->draw.lightmapCount != 0);

        T6GfxWorld->draw.lightmapCount = T5GfxWorld->draw.lightmapCount;

        // always empty
        T6GfxWorld->draw.lightmapPrimaryTextures = m_memory.Alloc<T6::GfxTexture>(T5GfxWorld->draw.lightmapCount);
        T6GfxWorld->draw.lightmapSecondaryTextures = m_memory.Alloc<T6::GfxTexture>(T5GfxWorld->draw.lightmapCount);

        T6GfxWorld->draw.lightmaps = m_memory.Alloc<T6::GfxLightmapArray>(T5GfxWorld->draw.lightmapCount);
        for (int lmapIdx = 0; lmapIdx < T5GfxWorld->draw.lightmapCount; lmapIdx++)
        {
            auto secondaryTextureAsset = m_context.LoadDependency<T6::AssetImage>(T5GfxWorld->draw.lightmaps[lmapIdx].secondary->name);
            if (secondaryTextureAsset == nullptr)
            {
                con::error("ERROR! unable to find lightmap image {}!", T5GfxWorld->draw.lightmaps[lmapIdx].secondary->name);
                return false;
            }

            T6GfxWorld->draw.lightmaps[lmapIdx].primary = nullptr; // always nullptr
            T6GfxWorld->draw.lightmaps[lmapIdx].secondary = secondaryTextureAsset->Asset();
        }

        return true;
    }

    void GfxWorldCompiler::loadSkyBox(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld, std::string& mapName)
    {
        T6GfxWorld->skyBoxModel = m_memory.Dup(T5GfxWorld->skyBoxModel);

        if (m_context.LoadDependency<T6::AssetXModel>(T5GfxWorld->skyBoxModel) == nullptr)
        {
            con::warn("WARN: Unable to load the skybox xmodel {}!", T5GfxWorld->skyBoxModel);
        }

        // default skybox values from mp_dig
        T6GfxWorld->skyDynIntensity.angle0 = T5GfxWorld->skyDynIntensity.angle0;
        T6GfxWorld->skyDynIntensity.angle1 = T5GfxWorld->skyDynIntensity.angle1;
        T6GfxWorld->skyDynIntensity.factor0 = T5GfxWorld->skyDynIntensity.factor0;
        T6GfxWorld->skyDynIntensity.factor1 = T5GfxWorld->skyDynIntensity.factor1;
    }

    void GfxWorldCompiler::loadDynEntData(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6fxWorld)
    {
        int dynEntCount = 0;
        T6fxWorld->dpvsDyn.dynEntClientCount[0] = dynEntCount + 256; // the game allocs 256 empty dynents, as they may be used ingame
        T6fxWorld->dpvsDyn.dynEntClientCount[1] = 0;

        // +100: there is a crash that happens when regdolls are created, and dynEntClientWordCount[0] is the issue.
        // Making the value much larger than required fixes it, but unsure what the root cause is
        T6fxWorld->dpvsDyn.dynEntClientWordCount[0] = ((T6fxWorld->dpvsDyn.dynEntClientCount[0] + 31) >> 5) + 100;
        T6fxWorld->dpvsDyn.dynEntClientWordCount[1] = 0;
        T6fxWorld->dpvsDyn.usageCount = 0;

        int dynEntCellBitsSize = T6fxWorld->dpvsDyn.dynEntClientWordCount[0] * T6fxWorld->dpvsPlanes.cellCount;
        T6fxWorld->dpvsDyn.dynEntCellBits[0] = m_memory.Alloc<unsigned int>(dynEntCellBitsSize);
        T6fxWorld->dpvsDyn.dynEntCellBits[1] = nullptr;

        int dynEntVisData0Size = T6fxWorld->dpvsDyn.dynEntClientWordCount[0] * 32;
        T6fxWorld->dpvsDyn.dynEntVisData[0][0] = m_memory.Alloc<char>(dynEntVisData0Size);
        T6fxWorld->dpvsDyn.dynEntVisData[0][1] = m_memory.Alloc<char>(dynEntVisData0Size);
        T6fxWorld->dpvsDyn.dynEntVisData[0][2] = m_memory.Alloc<char>(dynEntVisData0Size);
        T6fxWorld->dpvsDyn.dynEntVisData[1][0] = nullptr;
        T6fxWorld->dpvsDyn.dynEntVisData[1][1] = nullptr;
        T6fxWorld->dpvsDyn.dynEntVisData[1][2] = nullptr;

        unsigned int dynEntShadowVisCount = T6fxWorld->dpvsDyn.dynEntClientCount[0] * (T6fxWorld->primaryLightCount - T6fxWorld->sunPrimaryLightIndex - 1);
        T6fxWorld->primaryLightDynEntShadowVis[0] = m_memory.Alloc<unsigned int>(dynEntShadowVisCount);
        T6fxWorld->primaryLightDynEntShadowVis[1] = nullptr;

        T6fxWorld->sceneDynModel = m_memory.Alloc<T6::GfxSceneDynModel>(T6fxWorld->dpvsDyn.dynEntClientCount[0]);
        T6fxWorld->sceneDynBrush = nullptr;
    }

    bool GfxWorldCompiler::loadOutdoors(T5::GfxWorld* T5GfxWorld, T6::GfxWorld* T6GfxWorld)
    {
        T6GfxWorld->outdoorLookupMatrix[0].x = T5GfxWorld->outdoorLookupMatrix[0][0];
        T6GfxWorld->outdoorLookupMatrix[0].y = T5GfxWorld->outdoorLookupMatrix[0][1];
        T6GfxWorld->outdoorLookupMatrix[0].z = T5GfxWorld->outdoorLookupMatrix[0][2];
        T6GfxWorld->outdoorLookupMatrix[0].w = T5GfxWorld->outdoorLookupMatrix[0][3];
        T6GfxWorld->outdoorLookupMatrix[1].x = T5GfxWorld->outdoorLookupMatrix[1][0];
        T6GfxWorld->outdoorLookupMatrix[1].y = T5GfxWorld->outdoorLookupMatrix[1][1];
        T6GfxWorld->outdoorLookupMatrix[1].z = T5GfxWorld->outdoorLookupMatrix[1][2];
        T6GfxWorld->outdoorLookupMatrix[1].w = T5GfxWorld->outdoorLookupMatrix[1][3];
        T6GfxWorld->outdoorLookupMatrix[2].x = T5GfxWorld->outdoorLookupMatrix[2][0];
        T6GfxWorld->outdoorLookupMatrix[2].y = T5GfxWorld->outdoorLookupMatrix[2][1];
        T6GfxWorld->outdoorLookupMatrix[2].z = T5GfxWorld->outdoorLookupMatrix[2][2];
        T6GfxWorld->outdoorLookupMatrix[2].w = T5GfxWorld->outdoorLookupMatrix[2][3];
        T6GfxWorld->outdoorLookupMatrix[3].x = T5GfxWorld->outdoorLookupMatrix[3][0];
        T6GfxWorld->outdoorLookupMatrix[3].y = T5GfxWorld->outdoorLookupMatrix[3][1];
        T6GfxWorld->outdoorLookupMatrix[3].z = T5GfxWorld->outdoorLookupMatrix[3][2];
        T6GfxWorld->outdoorLookupMatrix[3].w = T5GfxWorld->outdoorLookupMatrix[3][3];

        auto outdoorImageAsset = m_context.LoadDependency<T6::AssetImage>(T5GfxWorld->outdoorImage->name);
        if (outdoorImageAsset == nullptr)
        {
            con::error("ERROR! unable to find outdoor image {}.", T5GfxWorld->outdoorImage->name);
            return false;
        }
        T6GfxWorld->outdoorImage = outdoorImageAsset->Asset();

        return true;
    }

    bool GfxWorldCompiler::linkGfxWorld(ZoneAssetPools* T5AssetPool, std::string& mapName, std::string& bspName, std::string& T5BSPName)
    {
        auto T5GfxWorldAsset = T5AssetPool->GetAsset(T5::ASSET_TYPE_GFXWORLD, T5BSPName);
        if (T5GfxWorldAsset == nullptr)
        {
            con::error("Can't find T5 GfxWorld asset.");
            return false;
        }
        T5::GfxWorld* T5GfxWorld = static_cast<T5::GfxWorld*>(T5GfxWorldAsset->m_ptr);
        T6::GfxWorld* T6GfxWorld = m_memory.Alloc<T6::GfxWorld>();

        T6GfxWorld->baseName = m_memory.Dup(mapName.c_str());
        T6GfxWorld->name = m_memory.Dup(bspName.c_str());

        // New in T6, taken from official T6 maps
        T6GfxWorld->lightingFlags = 0;
        T6GfxWorld->lightingQuality = 4096;

        // checksum is generated by the game
        T6GfxWorld->checksum = 0;

        // Unsure what this relates to
        T6GfxWorld->dpvs.usageCount = T5GfxWorld->dpvs.usageCount;

        if (!loadMapSurfaces(T5GfxWorld, T6GfxWorld))
            return false;

        if (!loadMaterials(T5GfxWorld, T6GfxWorld))
            return false;

        if (!loadLightmapData(T5GfxWorld, T6GfxWorld))
            return false;

        if (!loadReflectionProbeData(T5GfxWorld, T6GfxWorld))
            return false;

        if (!loadOutdoors(T5GfxWorld, T6GfxWorld))
            return false;

        if (!loadSunData(T5GfxWorld, T6GfxWorld))
            return false;

        loadXModels(T5GfxWorld, T6GfxWorld);
        loadSkyBox(T5GfxWorld, T6GfxWorld, mapName);
        loadWorldBounds(T5GfxWorld, T6GfxWorld);
        loadCoronas(T5GfxWorld, T6GfxWorld);
        loadExposureVolumes(T5GfxWorld, T6GfxWorld);
        loadHeroLights(T5GfxWorld, T6GfxWorld);
        loadLUT(T5GfxWorld, T6GfxWorld);
        loadOccluders(T5GfxWorld, T6GfxWorld);
        loadSiegeSkins(T5GfxWorld, T6GfxWorld);
        loadOutdoorBounds(T5GfxWorld, T6GfxWorld);
        loadMaterials(T5GfxWorld, T6GfxWorld);
        loadShadowMaps(T5GfxWorld, T6GfxWorld);
        loadStreamInfo(T5GfxWorld, T6GfxWorld);
        loadWater(T5GfxWorld, T6GfxWorld);
        loadFog(T5GfxWorld, T6GfxWorld);
        loadGfxCells(T5GfxWorld, T6GfxWorld);
        loadGfxLights(T5GfxWorld, T6GfxWorld);
        loadLightGrid(T5GfxWorld, T6GfxWorld);
        loadModels(T5GfxWorld, T6GfxWorld);
        loadSunData(T5GfxWorld, T6GfxWorld);

        // requires T6 cells and lights
        loadDynEntData(T5GfxWorld, T6GfxWorld);

        m_context.AddAsset<T6::AssetGfxWorld>(T6GfxWorld->name, T6GfxWorld);

        return true;
    }
} // namespace BSP
