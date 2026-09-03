#include "GfxWorldLinker.h"

#include "BSP/BSPUtil.h"
#include "Utils/Pack.h"

using namespace T6;
using namespace BSP;
using namespace BSPFlags;

namespace
{
    constexpr const char* DEFAULT_IMAGE_NAME = ",mc/lambert1";
    constexpr char DEFAULT_PRIMARYLIGHT_INDEX = 1; // max 254
    constexpr char DEFAULT_LIGHTMAP_INDEX = 0;     // max 30
    constexpr char DEFAULT_RPROBE_INDEX = 0;       // max 254
    constexpr float XMODEL_CULL_DIST = 10000.0f;
    constexpr char DEFAULT_LIGHTGRID_COLOUR = 32;

    unsigned int packLmapCoord(float X, float Y)
    {
        float XStep = X * 65535.0f;
        float YStep = Y * 65535.0f;

        unsigned int result = 0;
        result |= (unsigned int)XStep;
        result &= 0xFFFF;
        result |= (unsigned int)YStep << 16;
        result &= 0xFFFFFFFF;

        return result;
    }

    bool flagsMatchExact(int flag1, int flag2)
    {
        return (flag1 & flag2) == flag1;
    }

    bool flagsMatchAny(int flag1, int flag2)
    {
        return (flag1 & flag2) != 0;
    }

    class GfxWorldLinkerImpl : public GfxWorldLinker
    {
    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        AssetCreationContext& m_context;

    public:
        explicit GfxWorldLinkerImpl(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
            : m_memory(memory),
              m_search_path(searchPath),
              m_context(context)
        {
        }

        void loadDrawData(BSPData* bsp, GfxWorld* gfxWorld)
        {
            size_t vertexCount = bsp->gfxWorld.vertices.size();
            gfxWorld->draw.vertexCount = static_cast<unsigned int>(vertexCount);
            gfxWorld->draw.vertexDataSize0 = static_cast<unsigned int>(vertexCount * sizeof(GfxPackedWorldVertex));
            GfxPackedWorldVertex* vertexBuffer = m_memory.Alloc<GfxPackedWorldVertex>(vertexCount);
            for (size_t vertIdx = 0; vertIdx < vertexCount; vertIdx++)
            {
                BSPVertex& bspVertex = bsp->gfxWorld.vertices.at(vertIdx);
                GfxPackedWorldVertex* gfxVertex = &vertexBuffer[vertIdx];

                gfxVertex->xyz = bspVertex.pos;
                gfxVertex->color.packed = pack32::Vec4PackGfxColor(bspVertex.color.v);
                gfxVertex->texCoord.packed = pack32::Vec2PackTexCoordsUV(bspVertex.texCoord.v);
                gfxVertex->normal.packed = pack32::Vec3PackUnitVecThirdBased(bspVertex.normal.v);
                gfxVertex->tangent.packed = pack32::Vec3PackUnitVecThirdBased(bspVertex.tangent.v);
                gfxVertex->binormalSign = bspVertex.binormal.v[0] > 0.0f ? 1.0f : -1.0f;
                gfxVertex->lmapCoord.packed = packLmapCoord(0.0f, 0.0f);
            }
            gfxWorld->draw.vd0.data = reinterpret_cast<char*>(vertexBuffer);

            // vd1 is unused but still needs to be initialised
            // the data type varies and 0x20 is enough for all types
            gfxWorld->draw.vertexDataSize1 = 0x20;
            gfxWorld->draw.vd1.data = m_memory.Alloc<char>(gfxWorld->draw.vertexDataSize1);

            size_t indexCount = bsp->gfxWorld.indices.size();
            assert(indexCount % 3 == 0);
            gfxWorld->draw.indexCount = static_cast<int>(indexCount);
            gfxWorld->draw.indices = m_memory.Alloc<uint16_t>(indexCount); // overflow checked in bspcreator
            static_assert(sizeof(bsp->gfxWorld.indices.data()[0]) == sizeof(uint16_t));
            memcpy(gfxWorld->draw.indices, bsp->gfxWorld.indices.data(), sizeof(uint16_t) * indexCount);
        }

        bool loadMapSurfaces(BSPData* bsp, GfxWorld* gfxWorld)
        {
            loadDrawData(bsp, gfxWorld);

            if (bsp->staticSurfaceCount > 0xffff)
            {
                con::error("There are more than 65535 Static surfaces. count: {}", bsp->staticSurfaceCount);
                return false;
            }

            gfxWorld->surfaceCount = static_cast<int>(bsp->gfxWorld.surfaces.size());
            gfxWorld->dpvs.surfaces = m_memory.Alloc<GfxSurface>(bsp->gfxWorld.surfaces.size());
            for (size_t surfIdx = 0; surfIdx < bsp->gfxWorld.surfaces.size(); surfIdx++)
            {
                BSPSurface& bspSurface = bsp->gfxWorld.surfaces.at(surfIdx);
                GfxSurface* gfxSurface = &gfxWorld->dpvs.surfaces[surfIdx];

                gfxSurface->tris.triCount = static_cast<uint16_t>(bspSurface.triCount); // overflow checked in bspcreator
                gfxSurface->tris.baseIndex = static_cast<int>(bspSurface.indexOfFirstIndex);
                gfxSurface->tris.vertexCount = static_cast<uint16_t>(bspSurface.vertexCount); // overflow checked in bspcreator
                gfxSurface->tris.firstVertex = static_cast<int>(bspSurface.indexOfFirstVertex);

                gfxSurface->tris.vertexDataOffset0 = static_cast<int>(bspSurface.indexOfFirstVertex * sizeof(GfxPackedWorldVertex));
                gfxSurface->tris.vertexDataOffset1 = 0; // vd1 is unused

                BSPMaterial bspMaterial = bsp->gfxWorld.materials.at(bspSurface.materialIndex);

                std::string materialName;
                if (bspMaterial.materialType == MATERIAL_TYPE_TEXTURE)
                    materialName = bspMaterial.materialName;
                else // MATERIAL_TYPE_COLOUR
                    materialName = DEFAULT_IMAGE_NAME;

                auto surfMaterialAsset = m_context.LoadDependency<AssetMaterial>(materialName);
                if (surfMaterialAsset == nullptr)
                {
                    surfMaterialAsset = m_context.LoadDependency<AssetMaterial>(DEFAULT_IMAGE_NAME);
                    assert(surfMaterialAsset != nullptr);
                }
                gfxSurface->material = surfMaterialAsset->Asset();

                GfxPackedWorldVertex* firstVert = reinterpret_cast<GfxPackedWorldVertex*>(&gfxWorld->draw.vd0.data[gfxSurface->tris.vertexDataOffset0]);
                gfxSurface->bounds[0] = firstVert[0].xyz;
                gfxSurface->bounds[1] = firstVert[0].xyz;
                for (size_t indexIdx = 0; indexIdx < static_cast<size_t>(gfxSurface->tris.triCount * 3); indexIdx++)
                {
                    uint16_t vertIndex = gfxWorld->draw.indices[gfxSurface->tris.baseIndex + indexIdx];
                    BSPUtil::updateAABBWithPoint(firstVert[vertIndex].xyz, gfxSurface->bounds[0], gfxSurface->bounds[1]);
                }
                gfxSurface->tris.mins = gfxSurface->bounds[0];
                gfxSurface->tris.maxs = gfxSurface->bounds[1];

                gfxSurface->flags = bspMaterial.surfaceFlags;
                gfxSurface->primaryLightIndex = bspSurface.lightIndex;
                gfxSurface->lightmapIndex = DEFAULT_LIGHTMAP_INDEX;
                gfxSurface->reflectionProbeIndex = DEFAULT_RPROBE_INDEX;
            }

            // Some code uses Sorted surfs to index surfaces, so for simplicity keep the indexes sequential and from 0
            assert(bsp->staticSurfaceStart == 0); // Static surfaces go first in the array then script surfaces after
            size_t staticSurfaceCount = bsp->staticSurfaceCount;
            gfxWorld->dpvs.staticSurfaceCount = static_cast<unsigned int>(staticSurfaceCount);
            gfxWorld->dpvs.sortedSurfIndex = m_memory.Alloc<uint16_t>(staticSurfaceCount);
            for (size_t surfIdx = 0; surfIdx < staticSurfaceCount; surfIdx++)
                gfxWorld->dpvs.sortedSurfIndex[surfIdx] = static_cast<uint16_t>(surfIdx);

            // surface materials are written to by the game
            gfxWorld->dpvs.surfaceMaterials = m_memory.Alloc<GfxDrawSurf>(staticSurfaceCount);

            gfxWorld->dpvs.litSurfsBegin = static_cast<unsigned int>(bsp->litOpaqueSurfaceStart);
            gfxWorld->dpvs.litSurfsEnd = static_cast<unsigned int>(bsp->litOpaqueSurfaceStart + bsp->litOpaqueSurfaceCount);
            gfxWorld->dpvs.litTransSurfsBegin = static_cast<unsigned int>(bsp->litTransparentSurfaceStart);
            gfxWorld->dpvs.litTransSurfsEnd = static_cast<unsigned int>(bsp->litTransparentSurfaceStart + bsp->litTransparentSurfaceCount);
            gfxWorld->dpvs.emissiveOpaqueSurfsBegin = static_cast<unsigned int>(bsp->emissiveOpaqueSurfaceStart);
            gfxWorld->dpvs.emissiveOpaqueSurfsEnd = static_cast<unsigned int>(bsp->emissiveOpaqueSurfaceStart + bsp->emissiveOpaqueSurfaceCount);
            gfxWorld->dpvs.emissiveTransSurfsBegin = static_cast<unsigned int>(bsp->emissiveTransparentSurfaceStart);
            gfxWorld->dpvs.emissiveTransSurfsEnd = static_cast<unsigned int>(bsp->emissiveTransparentSurfaceStart + bsp->emissiveTransparentSurfaceCount);

            // visdata is written to by the game
            // all visdata is alligned by 128
            size_t allignedSurfaceCount = BSPUtil::allignBy128(staticSurfaceCount);
            gfxWorld->dpvs.surfaceVisDataCount = static_cast<unsigned int>(allignedSurfaceCount);
            gfxWorld->dpvs.surfaceVisData[0] = m_memory.Alloc<char>(allignedSurfaceCount);
            gfxWorld->dpvs.surfaceVisData[1] = m_memory.Alloc<char>(allignedSurfaceCount);
            gfxWorld->dpvs.surfaceVisData[2] = m_memory.Alloc<char>(allignedSurfaceCount);
            gfxWorld->dpvs.surfaceVisDataCameraSaved = m_memory.Alloc<char>(allignedSurfaceCount);
            gfxWorld->dpvs.surfaceCastsShadow = m_memory.Alloc<char>(allignedSurfaceCount);
            gfxWorld->dpvs.surfaceCastsSunShadow = m_memory.Alloc<char>(allignedSurfaceCount);

            return true;
        }

        bool loadXModels(BSPData* bsp, GfxWorld* gfxWorld)
        {
            size_t modelCount = bsp->gfxWorld.xmodels.size();
            gfxWorld->dpvs.smodelCount = (unsigned int)modelCount;
            gfxWorld->dpvs.smodelInsts = m_memory.Alloc<GfxStaticModelInst>(modelCount);
            gfxWorld->dpvs.smodelDrawInsts = m_memory.Alloc<GfxStaticModelDrawInst>(modelCount);

            if (modelCount == 0)
                return true;
            if (modelCount > 0xFFFF)
            {
                con::error("GFXWorld exceeded the maximum number of static xmodels (65535 max, map count: {})", modelCount);
                return false;
            }

            for (size_t modelIdx = 0; modelIdx < modelCount; modelIdx++)
            {
                auto currModel = &gfxWorld->dpvs.smodelDrawInsts[modelIdx];
                auto currModelInst = &gfxWorld->dpvs.smodelInsts[modelIdx];
                BSPXModel& bspModel = bsp->gfxWorld.xmodels.at(modelIdx);

                auto xModelAsset = m_context.LoadDependency<AssetXModel>(bspModel.name);
                if (xModelAsset == nullptr)
                {
                    con::error("Unable to load xmodel asset: \"{}\"", bspModel.name);
                    return false;
                }
                else
                    currModel->model = (XModel*)xModelAsset->Asset();

                currModel->placement.origin = bspModel.origin;
                BSPUtil::convertQuaternionToAxis(&bspModel.rotationQuaternion, currModel->placement.axis);
                vec3_t mScale;
                mScale.x = std::round(bspModel.scale.x * 100.0f) / 100.0f;
                mScale.y = std::round(bspModel.scale.y * 100.0f) / 100.0f;
                mScale.z = std::round(bspModel.scale.z * 100.0f) / 100.0f;
                if (mScale.x != mScale.y || mScale.x != mScale.z)
                    con::warn("GFX xmodel has non-uniform scaling, only X scale value will be used.");
                currModel->placement.scale = bspModel.scale.x;

                currModel->flags = 0;
                if (!bspModel.doesCastShadow)
                    currModel->flags |= STATIC_MODEL_FLAG_NO_SHADOW;

                currModel->cullDist = XMODEL_CULL_DIST;
                currModel->primaryLightIndex = DEFAULT_PRIMARYLIGHT_INDEX;
                currModel->reflectionProbeIndex = DEFAULT_RPROBE_INDEX;
                currModel->smid = static_cast<unsigned int>(modelIdx);

                currModelInst->lightingOrigin = bspModel.origin;

                if (!xModelAsset->IsReference())
                {
                    vec3_t scaledAxis[3];
                    scaledAxis[0].x = currModel->placement.axis[0].x * currModel->placement.scale;
                    scaledAxis[0].y = currModel->placement.axis[0].y * currModel->placement.scale;
                    scaledAxis[0].z = currModel->placement.axis[0].z * currModel->placement.scale;
                    scaledAxis[1].x = currModel->placement.axis[1].x * currModel->placement.scale;
                    scaledAxis[1].y = currModel->placement.axis[1].y * currModel->placement.scale;
                    scaledAxis[1].z = currModel->placement.axis[1].z * currModel->placement.scale;
                    scaledAxis[2].x = currModel->placement.axis[2].x * currModel->placement.scale;
                    scaledAxis[2].y = currModel->placement.axis[2].y * currModel->placement.scale;
                    scaledAxis[2].z = currModel->placement.axis[2].z * currModel->placement.scale;
                    BSPUtil::calculateXmodelGfxBounds(currModel->model, scaledAxis, currModelInst->mins, currModelInst->maxs);
                    currModelInst->mins.x = currModelInst->mins.x + bspModel.origin.x;
                    currModelInst->mins.y = currModelInst->mins.y + bspModel.origin.y;
                    currModelInst->mins.z = currModelInst->mins.z + bspModel.origin.z;
                    currModelInst->maxs.x = currModelInst->maxs.x + bspModel.origin.x;
                    currModelInst->maxs.y = currModelInst->maxs.y + bspModel.origin.y;
                    currModelInst->maxs.z = currModelInst->maxs.z + bspModel.origin.z;

                    assert(currModel->model->numLods <= 4);
                    for (uint16_t lodIdx = 0; lodIdx < currModel->model->numLods; lodIdx++)
                    {
                        uint16_t vertCount = 0;
                        for (auto surfaceIndex = 0u; surfaceIndex < currModel->model->lodInfo[lodIdx].numsurfs; surfaceIndex++)
                        {
                            const auto& surface = currModel->model->surfs[surfaceIndex + currModel->model->lodInfo[lodIdx].surfIndex];
                            vertCount += surface.vertCount;
                        }

                        currModel->lmapVertexInfo[lodIdx].numLmapVertexColors = vertCount;
                        currModel->lmapVertexInfo[lodIdx].lmapVertexColors = m_memory.Alloc<unsigned int>(vertCount);

                        // use the same method as lightmap to generate the colours
                        vec4_t sunlightModified = {bsp->sunlight.colour.x, bsp->sunlight.colour.y, bsp->sunlight.colour.z, 0.0f};
                        sunlightModified.x /= 32.0f;
                        sunlightModified.x = sqrtf(sunlightModified.x);
                        sunlightModified.y /= 32.0f;
                        sunlightModified.y = sqrtf(sunlightModified.y);
                        sunlightModified.z /= 32.0f;
                        sunlightModified.z = sqrtf(sunlightModified.z);
                        uint32_t vertColor = pack32::Vec4PackGfxColor(sunlightModified.v);
                        for (uint16_t vertIdx = 0; vertIdx < vertCount; vertIdx++)
                            currModel->lmapVertexInfo[lodIdx].lmapVertexColors[vertIdx] = vertColor;
                    }
                }
                else
                {
                    if (bspModel.areBoundsValid)
                    {
                        currModelInst->mins = bspModel.mins;
                        currModelInst->maxs = bspModel.maxs;
                    }
                    else
                    {
                        // this may cause rendering issues as the mins/maxs determine if the xmodel is in view or not
                        con::debug("GFX: Unable to determine the bounds of xmodel: \"{}\" which may cause rendering issues", bspModel.name);
                        currModelInst->mins.x = bspModel.origin.x - 100.0f;
                        currModelInst->mins.y = bspModel.origin.y - 100.0f;
                        currModelInst->mins.z = bspModel.origin.z - 100.0f;
                        currModelInst->maxs.x = bspModel.origin.x + 100.0f;
                        currModelInst->maxs.y = bspModel.origin.y + 100.0f;
                        currModelInst->maxs.z = bspModel.origin.z + 100.0f;
                    }
                }
            }

            // visdata is written to by the game
            // all visdata is alligned by 128
            size_t allignedModelCount = BSPUtil::allignBy128(modelCount);
            gfxWorld->dpvs.smodelVisDataCount = static_cast<unsigned int>(allignedModelCount);
            gfxWorld->dpvs.smodelVisData[0] = m_memory.Alloc<char>(allignedModelCount);
            gfxWorld->dpvs.smodelVisData[1] = m_memory.Alloc<char>(allignedModelCount);
            gfxWorld->dpvs.smodelVisData[2] = m_memory.Alloc<char>(allignedModelCount);
            gfxWorld->dpvs.smodelVisDataCameraSaved = m_memory.Alloc<char>(allignedModelCount);
            gfxWorld->dpvs.smodelCastsShadow = m_memory.Alloc<char>(allignedModelCount);
            for (size_t i = 0; i < modelCount; i++)
            {
                if ((gfxWorld->dpvs.smodelDrawInsts[i].flags & STATIC_MODEL_FLAG_NO_SHADOW) == 0)
                    gfxWorld->dpvs.smodelCastsShadow[i] = 1;
            }

            return true;
        }

        void cleanGfxWorld(GfxWorld* gfxWorld)
        {
            // checksum is generated by the game
            gfxWorld->checksum = 0;

            // official maps set this to 0
            gfxWorld->dpvs.usageCount = 0;

            // Remove Coronas
            gfxWorld->coronaCount = 0;
            gfxWorld->coronas = nullptr;

            // Remove exposure volumes
            gfxWorld->exposureVolumeCount = 0;
            gfxWorld->exposureVolumes = nullptr;
            gfxWorld->exposureVolumePlaneCount = 0;
            gfxWorld->exposureVolumePlanes = nullptr;

            // Remove hero lights
            gfxWorld->heroLightCount = 0;
            gfxWorld->heroLights = nullptr;
            gfxWorld->heroLightTreeCount = 0;
            gfxWorld->heroLightTree = nullptr;

            // remove LUT data
            gfxWorld->lutVolumeCount = 0;
            gfxWorld->lutVolumes = nullptr;
            gfxWorld->lutVolumePlaneCount = 0;
            gfxWorld->lutVolumePlanes = nullptr;

            // remove occluders
            gfxWorld->numOccluders = 0;
            gfxWorld->occluders = nullptr;

            // remove Siege Skins
            gfxWorld->numSiegeSkinInsts = 0;
            gfxWorld->siegeSkinInsts = nullptr;

            // remove outdoor bounds
            gfxWorld->numOutdoorBounds = 0;
            gfxWorld->outdoorBounds = nullptr;

            // remove materials
            gfxWorld->ropeMaterial = nullptr;
            gfxWorld->lutMaterial = nullptr;
            gfxWorld->waterMaterial = nullptr;
            gfxWorld->coronaMaterial = nullptr;

            // remove shadow maps
            gfxWorld->shadowMapVolumeCount = 0;
            gfxWorld->shadowMapVolumes = nullptr;
            gfxWorld->shadowMapVolumePlaneCount = 0;
            gfxWorld->shadowMapVolumePlanes = nullptr;

            // remove stream info
            gfxWorld->streamInfo.aabbTreeCount = 0;
            gfxWorld->streamInfo.aabbTrees = nullptr;
            gfxWorld->streamInfo.leafRefCount = 0;
            gfxWorld->streamInfo.leafRefs = nullptr;

            // remove sun data
            memset(&gfxWorld->sun, 0, sizeof(sunflare_t));
            gfxWorld->sun.hasValidData = false;

            // Remove Water
            gfxWorld->waterDirection = 0.0f;
            gfxWorld->waterBuffers[0].bufferSize = 0;
            gfxWorld->waterBuffers[0].buffer = nullptr;
            gfxWorld->waterBuffers[1].bufferSize = 0;
            gfxWorld->waterBuffers[1].buffer = nullptr;

            // Remove Fog
            gfxWorld->worldFogModifierVolumeCount = 0;
            gfxWorld->worldFogModifierVolumes = nullptr;
            gfxWorld->worldFogModifierVolumePlaneCount = 0;
            gfxWorld->worldFogModifierVolumePlanes = nullptr;
            gfxWorld->worldFogVolumeCount = 0;
            gfxWorld->worldFogVolumes = nullptr;
            gfxWorld->worldFogVolumePlaneCount = 0;
            gfxWorld->worldFogVolumePlanes = nullptr;

            // materialMemory is unused
            gfxWorld->materialMemoryCount = 0;
            gfxWorld->materialMemory = nullptr;

            // sunLight is overwritten by the game, just needs to be a valid pointer
            gfxWorld->sunLight = m_memory.Alloc<GfxLight>();
        }

        bool loadGfxLights(BSPData* bsp, GfxWorld* gfxWorld)
        {
            // there must be 2 or more lights, first is the static light and second is the sun light
            gfxWorld->primaryLightCount = BSP_DEFAULT_LIGHT_COUNT + static_cast<unsigned int>(bsp->lights.size());
            gfxWorld->sunPrimaryLightIndex = SUN_LIGHT_INDEX;

            if (gfxWorld->primaryLightCount > 254)
            {
                con::error("Exceeded 254 lights in BSP, count: {}", gfxWorld->primaryLightCount);
                return false;
            }

            gfxWorld->shadowGeom = m_memory.Alloc<GfxShadowGeometry>(gfxWorld->primaryLightCount);
            for (unsigned int lightIdx = 0; lightIdx < gfxWorld->primaryLightCount; lightIdx++)
            {
                // smodelCount and smodelIndex is filled next loop
                gfxWorld->shadowGeom[lightIdx].smodelCount = 0;
                gfxWorld->shadowGeom[lightIdx].smodelIndex = m_memory.Alloc<uint16_t>(gfxWorld->dpvs.smodelCount);

                // sorted surfs and surfaceCount is recalculated each frame
                gfxWorld->shadowGeom[lightIdx].surfaceCount = gfxWorld->dpvs.staticSurfaceCount;
                gfxWorld->shadowGeom[lightIdx].sortedSurfIndex = m_memory.Alloc<uint16_t>(gfxWorld->dpvs.staticSurfaceCount);
            }
            for (unsigned int modelIdx = 0; modelIdx < gfxWorld->dpvs.smodelCount; modelIdx++)
            {
                if ((gfxWorld->dpvs.smodelDrawInsts[modelIdx].flags & STATIC_MODEL_FLAG_NO_SHADOW) != 0)
                    continue;
                unsigned char lightIndex = gfxWorld->dpvs.smodelDrawInsts[modelIdx].primaryLightIndex;
                gfxWorld->shadowGeom[lightIndex].smodelIndex[gfxWorld->shadowGeom[lightIndex].smodelCount] = modelIdx;
                gfxWorld->shadowGeom[lightIndex].smodelCount++;
            }

            gfxWorld->lightRegion = m_memory.Alloc<GfxLightRegion>(gfxWorld->primaryLightCount);
            for (unsigned int lightIdx = 0; lightIdx < gfxWorld->primaryLightCount; lightIdx++)
            {
                gfxWorld->lightRegion[lightIdx].hullCount = 0;
                gfxWorld->lightRegion[lightIdx].hulls = nullptr;
            }

            unsigned int lightEntShadowVisSize = (gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1) * 8192;
            if (lightEntShadowVisSize != 0)
                gfxWorld->primaryLightEntityShadowVis = m_memory.Alloc<unsigned int>(lightEntShadowVisSize);
            else
                gfxWorld->primaryLightEntityShadowVis = nullptr;

            return true;
        }

        char compressColorIntoLightmapChar(float color)
        {
            if (color < 0.0f)
                color = 0.0f;
            else if (color > 1.0f)
                color = 1.0f;

            color /= 32.0f;
            color = sqrtf(color);

            color *= 255.0f;
            return static_cast<char>(roundf(color));
        }

        bool loadLightGrid(BSPData* bsp, GfxWorld* gfxWorld)
        {
            // world to lightgrid coords conversion:
            // l_x = (w_x + 131072.0f) / 32.0f;
            // l_y = (w_y + 131072.0f) / 32.0f;
            // l_z = (w_z + 131072.0f) / 64.0f;
            //
            // lightgrid to world coords conversion:
            // w_x = (double)l_x * 32.0f - 131072.0f;
            // w_y = (double)l_y * 32.0f - 131072.0f;
            // w_z = (double)l_z * 64.0f - 131072.0f;
            //
            // Lightrid coords are bounded by the uint16 limits:
            // w_minX = -131072, w_maxX = 1966048
            // w_minY = -131072, w_maxY = 1966048
            // w_minZ = -131072, w_maxZ = 4063168
            //
            // 1 xy lightgrid unit equals 32 xy world units
            // 1 z lightgrid unit equals 64 z world units

            if (gfxWorld->mins.x < -131072.0f || gfxWorld->mins.y < -131072.0f || gfxWorld->mins.z < -131072.0f || gfxWorld->maxs.x > 1966048.0f
                || gfxWorld->maxs.y > 1966048.0f || gfxWorld->maxs.z > 4063168.0f)
            {
                con::error("Lightrid mins/maxs exceeded");
                return false;
            }

            // integer-only method used by the game
            gfxWorld->lightGrid.mins[0] = static_cast<uint16_t>((static_cast<int>(gfxWorld->mins.x) + 0x20000) >> 5);
            gfxWorld->lightGrid.mins[1] = static_cast<uint16_t>((static_cast<int>(gfxWorld->mins.y) + 0x20000) >> 5);
            gfxWorld->lightGrid.mins[2] = static_cast<uint16_t>((static_cast<int>(gfxWorld->mins.z) + 0x20000) >> 6);
            gfxWorld->lightGrid.maxs[0] = static_cast<uint16_t>((static_cast<int>(gfxWorld->maxs.x) + 0x20000) >> 5);
            gfxWorld->lightGrid.maxs[1] = static_cast<uint16_t>((static_cast<int>(gfxWorld->maxs.y) + 0x20000) >> 5);
            gfxWorld->lightGrid.maxs[2] = static_cast<uint16_t>((static_cast<int>(gfxWorld->maxs.z) + 0x20000) >> 6);

            gfxWorld->lightGrid.rowAxis = 0; // default value
            gfxWorld->lightGrid.colAxis = 1; // default value
            gfxWorld->lightGrid.sunPrimaryLightIndex = SUN_LIGHT_INDEX;
            gfxWorld->lightGrid.offset = 0.0f; // default value

            // setting all rowDataStart indexes to 0 will always index the first row in rawRowData
            int rowDataStartSize = gfxWorld->lightGrid.maxs[0] - gfxWorld->lightGrid.mins[0] + 1;
            gfxWorld->lightGrid.rowDataStart = m_memory.Alloc<uint16_t>(rowDataStartSize);

            

            std::vector<char> rowVec;
            std::vector<GfxLightGridEntry> entryVec;
            std::vector<GfxCompressedLightGridColors> colorVec;
            for (int i = 0; i < rowDataStartSize; i++)
            {
                assert(rowVec.size() % 4 == 0);
                gfxWorld->lightGrid.rowDataStart[i] = static_cast<uint16_t>(rowVec.size() / 4);

                size_t yAxisStart = gfxWorld->lightGrid.mins[1];
                size_t yAxisSize = gfxWorld->lightGrid.maxs[1] - gfxWorld->lightGrid.mins[1];
                size_t zAxisStart = gfxWorld->lightGrid.mins[2];
                size_t zAxisSize = gfxWorld->lightGrid.maxs[2] - gfxWorld->lightGrid.mins[2];

                GfxLightGridRow row;
                row.colStart = yAxisStart;
                row.colCount = yAxisSize;
                row.zStart = zAxisStart;
                row.zCount = zAxisSize;
                row.firstEntry = static_cast<unsigned int>(entryVec.size());

                size_t yResult = yAxisSize / 0xff;
                size_t yRemainder = yAxisSize % 0xff;
                if (yRemainder > 0)
                    yResult++;

                size_t zResult = zAxisSize / 0xff;
                size_t zRemainder = zAxisSize % 0xff;
                if (zRemainder > 0)
                    zResult++;

                std::unique_ptr<GfxLightGridRowData[]> dataArr = std::make_unique<GfxLightGridRowData[]>(yResult);

                size_t unaddedYCount = yAxisSize;
                
                for (size_t yIdx = 0; yIdx < yResult; yIdx++)
                {
                    char ySize = 0xFF;
                    if (unaddedYCount < 0xFF)
                        ySize = static_cast<unsigned char>(unaddedYCount);
                    else
                        unaddedYCount -= 0xFF;

                    size_t unaddedZCount = zAxisSize;
                    size_t addedZCount = 0;
                    char zSize = 0xFF;
                    if (unaddedZCount < 0xFF)
                        zSize = static_cast<unsigned char>(unaddedZCount);
                    else
                        unaddedZCount -= 0xFF;
                    addedZCount += zSize;

                    GfxLightGridRowData* data = &dataArr[yIdx];
                    data->yAxisCount = ySize;
                    data->zAxisSize = zSize;

                    data->zAxisStart = static_cast<unsigned char>(addedZCount / 0x100);
                    data->zAxisOffset = static_cast<unsigned char>(addedZCount % 0x100);

                    
                    for (size_t zIdx = 0; zIdx < zResult; zIdx++)
                    {
                        

                        
                    }
                }

            }

            gfxWorld->lightGrid.rawRowDataSize = static_cast<unsigned int>(rowVec.size());
            gfxWorld->lightGrid.rawRowData = m_memory.Alloc<aligned_byte_pointer>(rowVec.size());
            memcpy(gfxWorld->lightGrid.rawRowData, rowVec.data(), rowVec.size() * sizeof(char));

            gfxWorld->lightGrid.entryCount = static_cast<unsigned int>(entryVec.size());
            gfxWorld->lightGrid.entries = m_memory.Alloc<GfxLightGridEntry>(entryVec.size());
            memcpy(gfxWorld->lightGrid.entries, entryVec.data(), entryVec.size() * sizeof(GfxLightGridEntry));

            gfxWorld->lightGrid.colorCount = static_cast<unsigned int>(colorVec.size());
            gfxWorld->lightGrid.colors = m_memory.Alloc<GfxCompressedLightGridColors>(colorVec.size());
            memcpy(gfxWorld->lightGrid.colors, colorVec.data(), colorVec.size() * sizeof(GfxCompressedLightGridColors));

            // colours are looked up with a lightgrid entries colorsIndex
            //gfxWorld->lightGrid.colorCount = 0x1000; // 0x1000 as it should be enough to hold every index
            //gfxWorld->lightGrid.colors = m_memory.Alloc<GfxCompressedLightGridColors>(gfxWorld->lightGrid.colorCount);
            //for (size_t colIdx = 0; colIdx < gfxWorld->lightGrid.colorCount; colIdx++)
            //{
            //    for (size_t row = 0; row < 56; row++)
            //    {
            //        gfxWorld->lightGrid.colors[colIdx].rgb[row][0] = compressColorIntoLightmapChar(bsp->sunlight.colour.x);
            //        gfxWorld->lightGrid.colors[colIdx].rgb[row][1] = compressColorIntoLightmapChar(bsp->sunlight.colour.y);
            //        gfxWorld->lightGrid.colors[colIdx].rgb[row][2] = compressColorIntoLightmapChar(bsp->sunlight.colour.z);
            //    }
            //}

            // we use the colours array instead of coeffs array
            gfxWorld->lightGrid.coeffCount = 0;
            gfxWorld->lightGrid.coeffs = nullptr;
            gfxWorld->lightGrid.skyGridVolumeCount = 0;
            gfxWorld->lightGrid.skyGridVolumes = nullptr;

            return true;
        }

        void loadGfxCells(GfxWorld* gfxWorld)
        {
            // Cells are basically data used to determine what can be seen and what cant be seen
            // Right now custom maps have no optimisation so there is only 1 cell
            int cellCount = 1;

            gfxWorld->dpvsPlanes.cellCount = cellCount;
            gfxWorld->cellBitsCount = ((cellCount + 127) >> 3) & 0x1FFFFFF0;

            int cellCasterBitsCount = cellCount * ((cellCount + 31) / 32);
            gfxWorld->cellCasterBits = m_memory.Alloc<unsigned int>(cellCasterBitsCount);

            int sceneEntCellBitsCount = cellCount * 512;
            gfxWorld->dpvsPlanes.sceneEntCellBits = m_memory.Alloc<unsigned int>(sceneEntCellBitsCount);

            gfxWorld->cells = m_memory.Alloc<GfxCell>(cellCount);
            gfxWorld->cells[0].portalCount = 0;
            gfxWorld->cells[0].portals = nullptr;
            gfxWorld->cells[0].mins = gfxWorld->mins;
            gfxWorld->cells[0].maxs = gfxWorld->maxs;

            // there is only 1 reflection probe
            gfxWorld->cells[0].reflectionProbeCount = 1;
            gfxWorld->cells[0].reflectionProbes = m_memory.Alloc<char>(gfxWorld->cells[0].reflectionProbeCount);
            gfxWorld->cells[0].reflectionProbes[0] = DEFAULT_RPROBE_INDEX;

            // AABB trees are used to detect what should be rendered and what shouldn't
            // Just use the first AABB node to hold all models, no optimisation but all models/surfaces wil lbe drawn
            gfxWorld->cells[0].aabbTreeCount = 1;
            gfxWorld->cells[0].aabbTree = m_memory.Alloc<GfxAabbTree>(gfxWorld->cells[0].aabbTreeCount);
            gfxWorld->cells[0].aabbTree[0].childCount = 0;
            gfxWorld->cells[0].aabbTree[0].childrenOffset = 0;
            gfxWorld->cells[0].aabbTree[0].startSurfIndex = 0;
            gfxWorld->cells[0].aabbTree[0].surfaceCount = static_cast<uint16_t>(gfxWorld->dpvs.staticSurfaceCount);
            gfxWorld->cells[0].aabbTree[0].smodelIndexCount = static_cast<uint16_t>(gfxWorld->dpvs.smodelCount);
            gfxWorld->cells[0].aabbTree[0].smodelIndexes = m_memory.Alloc<unsigned short>(gfxWorld->dpvs.smodelCount);
            for (unsigned short smodelIdx = 0; smodelIdx < gfxWorld->dpvs.smodelCount; smodelIdx++)
            {
                gfxWorld->cells[0].aabbTree[0].smodelIndexes[smodelIdx] = smodelIdx;
            }
            gfxWorld->cells[0].aabbTree[0].mins = gfxWorld->mins;
            gfxWorld->cells[0].aabbTree[0].maxs = gfxWorld->maxs;

            // nodes have the struct mnode_t, and there must be at least 1 node (similar to BSP nodes)
            // Nodes mnode_t.cellIndex indexes gfxWorld->cells
            // and (mnode_t.cellIndex - (world->dpvsPlanes.cellCount + 1) indexes world->dpvsPlanes.planes
            // Use only one node as there is no optimisation in custom maps

            gfxWorld->nodeCount = 1;
            gfxWorld->dpvsPlanes.nodes = m_memory.Alloc<uint16_t>(gfxWorld->nodeCount);
            gfxWorld->dpvsPlanes.nodes[0] = 1; // nodes reference cells by index + 1

            // planes are overwritten by the clipmap loading code ingame
            gfxWorld->planeCount = 0;
            gfxWorld->dpvsPlanes.planes = nullptr;
        }

        void loadWorldBounds(GfxWorld* gfxWorld)
        {
            gfxWorld->mins = gfxWorld->dpvs.surfaces[0].bounds[0];
            gfxWorld->maxs = gfxWorld->dpvs.surfaces[0].bounds[1];
            for (int surfIdx = 0; surfIdx < gfxWorld->surfaceCount; surfIdx++)
            {
                BSPUtil::updateAABB(gfxWorld->dpvs.surfaces[surfIdx].bounds[0], gfxWorld->dpvs.surfaces[surfIdx].bounds[1], gfxWorld->mins, gfxWorld->maxs);
            }

            for (unsigned int smodeldx = 0; smodeldx < gfxWorld->dpvs.smodelCount; smodeldx++)
            {
                BSPUtil::updateAABB(gfxWorld->dpvs.smodelInsts[smodeldx].mins, gfxWorld->dpvs.smodelInsts[smodeldx].maxs, gfxWorld->mins, gfxWorld->maxs);
            }
        }

        void loadModels(BSPData* bsp, GfxWorld* gfxWorld)
        {
            // Models (Submodels in the clipmap code) are used for the world and map ent collision (triggers, bomb zones, etc)
            // bounds are checked in clipmap linker
            gfxWorld->modelCount = static_cast<int>(bsp->models.size() + 1);
            gfxWorld->models = m_memory.Alloc<GfxBrushModel>(gfxWorld->modelCount);

            // first model is always the world model
            gfxWorld->models[0].startSurfIndex = 0;
            gfxWorld->models[0].surfaceCount = static_cast<unsigned int>(gfxWorld->dpvs.staticSurfaceCount);
            for (unsigned int surfIdx = 0; surfIdx < gfxWorld->dpvs.staticSurfaceCount; surfIdx++)
            {
                if (surfIdx == 0)
                {
                    gfxWorld->models[0].bounds[0] = gfxWorld->dpvs.surfaces[surfIdx].bounds[0];
                    gfxWorld->models[0].bounds[1] = gfxWorld->dpvs.surfaces[surfIdx].bounds[1];
                }
                else
                    BSPUtil::updateAABB(gfxWorld->dpvs.surfaces[surfIdx].bounds[0],
                                        gfxWorld->dpvs.surfaces[surfIdx].bounds[1],
                                        gfxWorld->models[0].bounds[0],
                                        gfxWorld->models[0].bounds[1]);
            }
            if (gfxWorld->dpvs.staticSurfaceCount == 0)
            {
                gfxWorld->models[0].bounds[0] = {};
                gfxWorld->models[0].bounds[1] = {};
            }
            memset(&gfxWorld->models[0].writable, 0, sizeof(GfxBrushModelWritable));

            for (size_t modelIdx = 0; modelIdx < bsp->models.size(); modelIdx++)
            {
                auto currEntModel = &gfxWorld->models[modelIdx + 1];
                auto& bspModel = bsp->models.at(modelIdx);

                if (bspModel.surfaceSide == MSS_GFX || bspModel.surfaceSide == MSS_BOTH)
                {
                    currEntModel->startSurfIndex = static_cast<unsigned int>(bspModel.gfxSurfaceIndex);
                    currEntModel->surfaceCount = static_cast<unsigned int>(bspModel.gfxSurfaceCount);
                    for (size_t surfIdx = 0; surfIdx < bspModel.gfxSurfaceCount; surfIdx++)
                    {
                        GfxSurface* surf = &gfxWorld->dpvs.surfaces[currEntModel->startSurfIndex + surfIdx];
                        if (surfIdx == 0)
                        {
                            currEntModel->bounds[0] = surf->bounds[0];
                            currEntModel->bounds[1] = surf->bounds[1];
                        }
                        else
                            BSPUtil::updateAABB(surf->bounds[0], surf->bounds[1], currEntModel->bounds[0], currEntModel->bounds[1]);
                    }
                }
                else
                {
                    currEntModel->startSurfIndex = -1; // -1 when it doesn't use map surfaces
                    currEntModel->surfaceCount = 0;
                    currEntModel->bounds[0] = {};
                    currEntModel->bounds[1] = {};
                }
                memset(&gfxWorld->models[0].writable, 0, sizeof(GfxBrushModelWritable));
            }
        }

        void loadSunData(BSPData* bsp, GfxWorld* gfxWorld)
        {
            // only these values are actually used by the game, the rest are set by dvars
            gfxWorld->sunParse.initWorldSun->angles = BSPUtil::convertForwardVectorToViewAngles(bsp->sunlight.forwardVector);
            gfxWorld->sunParse.initWorldSun->sunCd.x = bsp->sunlight.colour.x;
            gfxWorld->sunParse.initWorldSun->sunCd.y = bsp->sunlight.colour.y;
            gfxWorld->sunParse.initWorldSun->sunCd.z = bsp->sunlight.colour.z;
            gfxWorld->sunParse.initWorldSun->sunCd.w = bsp->sunlight.intensity;

            // fog is not implemented yet, values taken from mp_dig
            gfxWorld->sunParse.initWorldFog->baseDist = 150.0f;
            gfxWorld->sunParse.initWorldFog->baseHeight = -100.0f;
            gfxWorld->sunParse.initWorldFog->fogColor.x = 2.35f;
            gfxWorld->sunParse.initWorldFog->fogColor.y = 3.10f;
            gfxWorld->sunParse.initWorldFog->fogColor.z = 3.84f;
            gfxWorld->sunParse.initWorldFog->fogOpacity = 0.52f;
            gfxWorld->sunParse.initWorldFog->halfDist = 4450.f;
            gfxWorld->sunParse.initWorldFog->halfHeight = 2000.f;
            gfxWorld->sunParse.initWorldFog->sunFogColor.x = 5.27f;
            gfxWorld->sunParse.initWorldFog->sunFogColor.y = 4.73f;
            gfxWorld->sunParse.initWorldFog->sunFogColor.z = 3.88f;
            gfxWorld->sunParse.initWorldFog->sunFogInner = 0.0f;
            gfxWorld->sunParse.initWorldFog->sunFogOpacity = 0.67f;
            gfxWorld->sunParse.initWorldFog->sunFogOuter = 80.84f;
            gfxWorld->sunParse.initWorldFog->sunFogPitch = -29.0f;
            gfxWorld->sunParse.initWorldFog->sunFogYaw = 254.0f;
        }

        bool loadReflectionProbeData(GfxWorld* gfxWorld)
        {
            gfxWorld->draw.reflectionProbeCount = 1;

            gfxWorld->draw.reflectionProbeTextures = m_memory.Alloc<GfxTexture>(gfxWorld->draw.reflectionProbeCount);

            // default values taken from mp_dig
            gfxWorld->draw.reflectionProbes = m_memory.Alloc<GfxReflectionProbe>(gfxWorld->draw.reflectionProbeCount);
            gfxWorld->draw.reflectionProbes[0].mipLodBias = -8.0; // always -8.0f
            gfxWorld->draw.reflectionProbes[0].origin = {};
            gfxWorld->draw.reflectionProbes[0].lightingSH.V0 = {};
            gfxWorld->draw.reflectionProbes[0].lightingSH.V1 = {};
            gfxWorld->draw.reflectionProbes[0].lightingSH.V2 = {};

            gfxWorld->draw.reflectionProbes[0].probeVolumeCount = 0;
            gfxWorld->draw.reflectionProbes[0].probeVolumes = nullptr;

            std::string probeImageName = ",$black";
            auto probeImageAsset = m_context.LoadDependency<AssetImage>(probeImageName);
            if (probeImageAsset == nullptr)
            {
                con::error("ERROR! unable to find reflection probe image {}!", probeImageName);
                return false;
            }
            gfxWorld->draw.reflectionProbes[0].reflectionImage = probeImageAsset->Asset();

            return true;
        }

        bool loadLightmapData(GfxWorld* gfxWorld)
        {
            gfxWorld->draw.lightmapCount = 1;

            gfxWorld->draw.lightmapPrimaryTextures = m_memory.Alloc<GfxTexture>(gfxWorld->draw.lightmapCount);
            gfxWorld->draw.lightmapSecondaryTextures = m_memory.Alloc<GfxTexture>(gfxWorld->draw.lightmapCount);

            std::string secondaryTexture = ",$gray"; //  gray makes shadows a nice looking shade of black
            auto secondaryTextureAsset = m_context.LoadDependency<AssetImage>(secondaryTexture);
            if (secondaryTextureAsset == nullptr)
            {
                con::error("ERROR! unable to find lightmap image {}!", secondaryTexture);
                return false;
            }
            gfxWorld->draw.lightmaps = m_memory.Alloc<GfxLightmapArray>(gfxWorld->draw.lightmapCount);
            gfxWorld->draw.lightmaps[0].primary = nullptr; // always nullptr
            gfxWorld->draw.lightmaps[0].secondary = secondaryTextureAsset->Asset();

            return true;
        }

        void loadSkyBox(BSPData* projInfo, GfxWorld* gfxWorld)
        {
            gfxWorld->skyBoxModel = m_memory.Dup(projInfo->skyboxName.c_str());
            if (m_context.LoadDependency<AssetXModel>(projInfo->skyboxName) == nullptr)
            {
                con::warn("Unable to load the skybox xmodel {}!", projInfo->skyboxName);
            }

            // always 0 and 1
            gfxWorld->skyDynIntensity.angle0 = 0.0f;
            gfxWorld->skyDynIntensity.angle1 = 0.0f;
            gfxWorld->skyDynIntensity.factor0 = 1.0f;
            gfxWorld->skyDynIntensity.factor1 = 1.0f;
        }

        void loadDynEntData(GfxWorld* gfxWorld)
        {
            int dynEntCount = 0;
            gfxWorld->dpvsDyn.dynEntClientCount[0] = dynEntCount + 256; // the game allocs 256 empty dynents, as they may be used ingame
            gfxWorld->dpvsDyn.dynEntClientCount[1] = 0;

            // +100: there is a crash that happens when ragdolls are created, and dynEntClientWordCount[0] is the issue.
            // Making the value much larger than required fixes it, but unsure what the root cause is
            gfxWorld->dpvsDyn.dynEntClientWordCount[0] = ((gfxWorld->dpvsDyn.dynEntClientCount[0] + 31) >> 5) + 100;
            gfxWorld->dpvsDyn.dynEntClientWordCount[1] = 0;
            gfxWorld->dpvsDyn.usageCount = 0;

            int dynEntCellBitsSize = gfxWorld->dpvsDyn.dynEntClientWordCount[0] * gfxWorld->dpvsPlanes.cellCount;
            gfxWorld->dpvsDyn.dynEntCellBits[0] = m_memory.Alloc<unsigned int>(dynEntCellBitsSize);
            gfxWorld->dpvsDyn.dynEntCellBits[1] = nullptr;

            int dynEntVisData0Size = gfxWorld->dpvsDyn.dynEntClientWordCount[0] * 32;
            gfxWorld->dpvsDyn.dynEntVisData[0][0] = m_memory.Alloc<char>(dynEntVisData0Size);
            gfxWorld->dpvsDyn.dynEntVisData[0][1] = m_memory.Alloc<char>(dynEntVisData0Size);
            gfxWorld->dpvsDyn.dynEntVisData[0][2] = m_memory.Alloc<char>(dynEntVisData0Size);
            gfxWorld->dpvsDyn.dynEntVisData[1][0] = nullptr;
            gfxWorld->dpvsDyn.dynEntVisData[1][1] = nullptr;
            gfxWorld->dpvsDyn.dynEntVisData[1][2] = nullptr;

            unsigned int dynEntShadowVisCount = gfxWorld->dpvsDyn.dynEntClientCount[0] * (gfxWorld->primaryLightCount - gfxWorld->sunPrimaryLightIndex - 1);
            gfxWorld->primaryLightDynEntShadowVis[0] = m_memory.Alloc<unsigned int>(dynEntShadowVisCount);
            gfxWorld->primaryLightDynEntShadowVis[1] = nullptr;

            gfxWorld->sceneDynModel = m_memory.Alloc<GfxSceneDynModel>(gfxWorld->dpvsDyn.dynEntClientCount[0]);
            gfxWorld->sceneDynBrush = nullptr;
        }

        bool loadOutdoors(GfxWorld* gfxWorld)
        {
            float xRecip = 1.0f / (gfxWorld->maxs.x - gfxWorld->mins.x);
            float xScale = -(xRecip * gfxWorld->mins.x);

            float yRecip = 1.0f / (gfxWorld->maxs.y - gfxWorld->mins.y);
            float yScale = -(yRecip * gfxWorld->mins.y);

            float zRecip = 1.0f / (gfxWorld->maxs.z - gfxWorld->mins.z);
            float zScale = -(zRecip * gfxWorld->mins.z);

            memset(gfxWorld->outdoorLookupMatrix, 0, sizeof(gfxWorld->outdoorLookupMatrix));

            gfxWorld->outdoorLookupMatrix[0].x = xRecip;
            gfxWorld->outdoorLookupMatrix[1].y = yRecip;
            gfxWorld->outdoorLookupMatrix[2].z = zRecip;
            gfxWorld->outdoorLookupMatrix[3].x = xScale;
            gfxWorld->outdoorLookupMatrix[3].y = yScale;
            gfxWorld->outdoorLookupMatrix[3].z = zScale;
            gfxWorld->outdoorLookupMatrix[3].w = 1.0f;

            std::string outdoorImageName = std::string(",$black");
            auto outdoorImageAsset = m_context.LoadDependency<AssetImage>(outdoorImageName);
            if (outdoorImageAsset == nullptr)
            {
                con::error("ERROR! unable to find outdoor image!");
                return false;
            }
            gfxWorld->outdoorImage = outdoorImageAsset->Asset();

            return true;
        }

        GfxWorld* linkGfxWorld(BSPData* bsp) override
        {
            GfxWorld* gfxWorld = m_memory.Alloc<GfxWorld>();
            gfxWorld->baseName = m_memory.Dup(bsp->name.c_str());
            gfxWorld->name = m_memory.Dup(bsp->bspName.c_str());

            // Default values taken from origins
            gfxWorld->lightingFlags = 4;
            gfxWorld->lightingQuality = 10000;

            cleanGfxWorld(gfxWorld);

            if (!loadMapSurfaces(bsp, gfxWorld))
                return nullptr;

            if (!loadXModels(bsp, gfxWorld))
                return nullptr;

            if (!loadLightmapData(gfxWorld))
                return nullptr;

            loadSkyBox(bsp, gfxWorld);

            if (!loadReflectionProbeData(gfxWorld))
                return nullptr;

            // world bounds are based on loaded surface mins/maxs and xmodels
            loadWorldBounds(gfxWorld);

            if (!loadOutdoors(gfxWorld)) // requires world mins/maxs
                return nullptr;

            // gfx cells depend on surface/smodel count
            loadGfxCells(gfxWorld);

            loadLightGrid(bsp, gfxWorld); // requires world mins/maxs

            if (!loadGfxLights(bsp, gfxWorld)) // requires xmodels and surfaces
                return nullptr;

            loadModels(bsp, gfxWorld); // requires surfaces

            loadSunData(bsp, gfxWorld);

            loadDynEntData(gfxWorld); // requires cells and lights

            // surf/xmodel light post porcessing:
            // any surfaces/xmodels within radius of light, light is assigned to it (check for light conflicts and choose best light)
            // check if sunlight can reach surface/xmodel, assign sunlight to it
            // add empty light if sunlight can't reach surf/xmodel

            // lightgrid post processing
            // break up level into 32x32x64 size chunks
            // chunks that can be seen by lights have their light calculated from attenuation
            // chunks that can be seen by sun have the sun light constant set
            // set chunks with no light data to low level of light (idk how to calc yet)

            // refection probes post processing
            // somehow load rprobes
            // use plutonium to create the rprobes (similar to pathing data)
            // load back in (posibly could hardcode rprobe image names based on how pluto loads them and load like how pathing is done)

            // lightmap post processing
            // lightmap image is used on surfs only, raw lighting data is used on xmodels
            // possibly just use colour of light illuminating it as data
            // generate image from this data

            return gfxWorld;
        }
    };
} // namespace

std::unique_ptr<GfxWorldLinker> GfxWorldLinker::Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
{
    return std::make_unique<GfxWorldLinkerImpl>(memory, searchPath, context);
}
