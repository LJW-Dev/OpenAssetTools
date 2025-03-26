#include <nlohmann/json.hpp>

#include "LoaderClipMapT6.h"

#include "Game/T6/T6.h"

#include <cstring>

using namespace T6;

namespace
{
    class ClipMapLoader final : public AssetCreator<AssetClipMapPvs>
    {
    public:
        ClipMapLoader(MemoryManager& memory, ISearchPath& searchPath, Zone& zone)
            : m_memory(memory),
              m_search_path(searchPath),
              m_zone(zone)
        {
        }

        AssetCreationResult CreateAsset(const std::string& assetName, AssetCreationContext& context) override
        {
            const auto file = m_search_path.Open("bsp/clipmap.json");
            if (!file.IsOpen())
                return AssetCreationResult::NoAction();

            nlohmann::json j = nlohmann::json::parse(*file.m_stream);
            
            auto* clipMap = m_memory.Alloc<clipMap_t>();

            clipMap->name = m_memory.Dup(assetName.c_str());
            clipMap->isInUse = j["isInUse"];
            clipMap->pInfo = nullptr;

            clipMap->info.planeCount = j["info"]["planeCount"];
            if (clipMap->info.planeCount > 0)
                clipMap->info.planes = m_memory.Alloc<cplane_s>(clipMap->info.planeCount);
            else
                clipMap->info.planes = nullptr;
            for (int i = 0; i < clipMap->info.planeCount; i++)
            {
                auto currentPlane = &clipMap->info.planes[i];
                auto currentPlaneData = j["info"]["planes"][i];

                currentPlane->normal.x = currentPlaneData["normal"]["x"];
                currentPlane->normal.y = currentPlaneData["normal"]["y"];
                currentPlane->normal.z = currentPlaneData["normal"]["z"];
                currentPlane->dist = currentPlaneData["dist"];
                currentPlane->type = static_cast<unsigned char>(currentPlaneData["type"]);
                currentPlane->signbits = static_cast<unsigned char>(currentPlaneData["signbits"]);
                currentPlane->pad[0] = static_cast<unsigned char>(currentPlaneData["pad"]["0"]);
                currentPlane->pad[1] = static_cast<unsigned char>(currentPlaneData["pad"]["1"]);
            }

            clipMap->info.numMaterials = j["info"]["numMaterials"];
            if (clipMap->info.numMaterials > 0)
                clipMap->info.materials = m_memory.Alloc<ClipMaterial>(clipMap->info.numMaterials);
            else
                clipMap->info.materials = nullptr;
            for (unsigned int i = 0; i < clipMap->info.numMaterials; i++)
            {
                auto currentMaterial = &clipMap->info.materials[i];
                auto currentMaterialData = j["info"]["materials"][i];

                currentMaterial->name = m_memory.Dup(static_cast<std::string>(currentMaterialData["name"]).c_str());
                currentMaterial->surfaceFlags = currentMaterialData["surfaceFlags"];
                currentMaterial->contentFlags = currentMaterialData["contentFlags"];
            }

            clipMap->info.numBrushSides = j["info"]["numBrushSides"];
            if (clipMap->info.numBrushSides > 0)
                clipMap->info.brushsides = m_memory.Alloc<cbrushside_t>(clipMap->info.numBrushSides);
            else
                clipMap->info.brushsides = nullptr;
            for (unsigned int i = 0; i < clipMap->info.numBrushSides; i++)
            {
                auto currentBrushSide = &clipMap->info.brushsides[i];
                auto currentBrushSideData = j["info"]["brushsides"][i];

                currentBrushSide->cflags = currentBrushSideData["cflags"];
                currentBrushSide->sflags = currentBrushSideData["sflags"];

                int planeReferenceIndex = currentBrushSideData["planeReferenceIndex"];
                currentBrushSide->plane = &clipMap->info.planes[planeReferenceIndex];
            }

            clipMap->info.leafbrushNodesCount = j["info"]["leafbrushNodesCount"];
            if (clipMap->info.leafbrushNodesCount > 0)
                clipMap->info.leafbrushNodes = m_memory.Alloc<cLeafBrushNode_s>(clipMap->info.leafbrushNodesCount);
            else
                clipMap->info.leafbrushNodes = nullptr;
            for (unsigned int i = 0; i < clipMap->info.leafbrushNodesCount; i++)
            {
                auto currentLeafBrushNode = &clipMap->info.leafbrushNodes[i];
                auto currentLeafBrushNodeData = j["info"]["leafbrushNodes"][i];

                currentLeafBrushNode->axis = static_cast<unsigned char>(currentLeafBrushNodeData["axis"]);
                currentLeafBrushNode->leafBrushCount = currentLeafBrushNodeData["leafBrushCount"];
                currentLeafBrushNode->contents = currentLeafBrushNodeData["contents"];

                if (currentLeafBrushNode->leafBrushCount > 0)
                {
                    currentLeafBrushNode->data.leaf.brushes = m_memory.Alloc<LeafBrush>(currentLeafBrushNode->leafBrushCount);
                    for (int k = 0; k < currentLeafBrushNode->leafBrushCount; k++)
                        currentLeafBrushNode->data.leaf.brushes[k] = currentLeafBrushNodeData["data"][k];
                }
                else
                {
                    currentLeafBrushNode->data.children.dist = currentLeafBrushNodeData["data"]["dist"];
                    currentLeafBrushNode->data.children.range = currentLeafBrushNodeData["data"]["range"];
                    currentLeafBrushNode->data.children.childOffset[0] = currentLeafBrushNodeData["data"]["childOffset0"];
                    currentLeafBrushNode->data.children.childOffset[1] = currentLeafBrushNodeData["data"]["childOffset1"];
                }
            }

            clipMap->info.numLeafBrushes = j["info"]["numLeafBrushes"];
            if (clipMap->info.numLeafBrushes > 0)
                clipMap->info.leafbrushes = m_memory.Alloc<LeafBrush>(clipMap->info.numLeafBrushes);
            else
                clipMap->info.leafbrushes = nullptr;
            for (unsigned int i = 0; i < clipMap->info.numLeafBrushes; i++)
            {
                clipMap->info.leafbrushes[i] = j["info"]["leafbrushes"][i];
            }

            clipMap->info.numBrushVerts = j["info"]["numBrushVerts"];
            if (clipMap->info.numBrushVerts > 0)
                clipMap->info.brushVerts = m_memory.Alloc<vec3_t>(clipMap->info.numBrushVerts);
            else
                clipMap->info.brushVerts = nullptr;
            for (unsigned int i = 0; i < clipMap->info.numBrushVerts; i++)
            {
                clipMap->info.brushVerts[i].x = j["info"]["brushVerts"][i]["x"];
                clipMap->info.brushVerts[i].y = j["info"]["brushVerts"][i]["y"];
                clipMap->info.brushVerts[i].z = j["info"]["brushVerts"][i]["z"];
            }

            clipMap->info.nuinds = j["info"]["nuinds"];
            if (clipMap->info.nuinds > 0)
                clipMap->info.uinds = m_memory.Alloc<uint16_t>(clipMap->info.nuinds);
            else
                clipMap->info.uinds = nullptr;
            for (unsigned int i = 0; i < clipMap->info.nuinds; i++)
            {
                clipMap->info.uinds[i] = j["info"]["uinds"][i];
            }

            clipMap->info.numBrushes = j["info"]["numBrushes"];
            if (j["info"]["brushes"].size() > 0)
                clipMap->info.brushes = m_memory.Alloc<cbrush_array_t>(clipMap->info.numBrushes);
            else
                clipMap->info.brushes = nullptr;
            if (j["info"]["brushBounds"].size() > 0)
                clipMap->info.brushBounds = m_memory.Alloc<BoundsArray>(clipMap->info.numBrushes);
            else
                clipMap->info.brushBounds = nullptr;
            if (j["info"]["brushContents"].size() > 0)
                clipMap->info.brushContents = m_memory.Alloc<int>(clipMap->info.numBrushes);
            else
                clipMap->info.brushContents = nullptr;
            for (int i = 0; i < clipMap->info.numBrushes; i++)
            {
                if (clipMap->info.brushes)
                {
                    auto currentBrush = &clipMap->info.brushes[i];
                    auto currentBrushData = j["info"]["brushes"][i];

                    currentBrush->contents = currentBrushData["contents"];
                    currentBrush->numsides = currentBrushData["numsides"];
                    currentBrush->numverts = currentBrushData["numverts"];
                    currentBrush->mins.x = currentBrushData["mins"]["x"];
                    currentBrush->mins.y = currentBrushData["mins"]["y"];
                    currentBrush->mins.z = currentBrushData["mins"]["z"];
                    currentBrush->maxs.x = currentBrushData["maxs"]["x"];
                    currentBrush->maxs.y = currentBrushData["maxs"]["y"];
                    currentBrush->maxs.z = currentBrushData["maxs"]["z"];
                    currentBrush->axial_cflags[0][0] = currentBrushData["axial_cflags00"];
                    currentBrush->axial_cflags[0][1] = currentBrushData["axial_cflags01"];
                    currentBrush->axial_cflags[0][2] = currentBrushData["axial_cflags02"];
                    currentBrush->axial_cflags[1][0] = currentBrushData["axial_cflags10"];
                    currentBrush->axial_cflags[1][1] = currentBrushData["axial_cflags11"];
                    currentBrush->axial_cflags[1][2] = currentBrushData["axial_cflags12"];
                    currentBrush->axial_sflags[0][0] = currentBrushData["axial_sflags00"];
                    currentBrush->axial_sflags[0][1] = currentBrushData["axial_sflags01"];
                    currentBrush->axial_sflags[0][2] = currentBrushData["axial_sflags02"];
                    currentBrush->axial_sflags[1][0] = currentBrushData["axial_sflags10"];
                    currentBrush->axial_sflags[1][1] = currentBrushData["axial_sflags11"];
                    currentBrush->axial_sflags[1][2] = currentBrushData["axial_sflags12"];


                    int sideReferenceIndex = currentBrushData["sidesArrayIndexStart"];
                    int vertReferenceIndex = currentBrushData["vertsArrayIndexStart"];

                    if (sideReferenceIndex == -1)
                        currentBrush->sides = nullptr;
                    else
                        currentBrush->sides = &clipMap->info.brushsides[sideReferenceIndex];

                    if (vertReferenceIndex == -1)
                        currentBrush->verts = nullptr;
                    else
                    currentBrush->verts = &clipMap->info.brushVerts[vertReferenceIndex];
                }

                if (clipMap->info.brushBounds)
                {
                    auto currentBrushBound = &clipMap->info.brushBounds[i];
                    auto currentBrushBoundData = j["info"]["brushBounds"][i];

                    currentBrushBound->midPoint.x = currentBrushBoundData["midPoint"]["x"];
                    currentBrushBound->midPoint.y = currentBrushBoundData["midPoint"]["y"];
                    currentBrushBound->midPoint.z = currentBrushBoundData["midPoint"]["z"];

                    currentBrushBound->halfSize.x = currentBrushBoundData["halfSize"]["x"];
                    currentBrushBound->halfSize.y = currentBrushBoundData["halfSize"]["y"];
                    currentBrushBound->halfSize.z = currentBrushBoundData["halfSize"]["z"];
                }

                if (clipMap->info.brushContents)
                {
                    clipMap->info.brushContents[i] = j["info"]["brushContents"][i];
                }
            }

            clipMap->numStaticModels = j["numStaticModels"];
            if (clipMap->numStaticModels > 0)
                clipMap->staticModelList = m_memory.Alloc<cStaticModel_s>(clipMap->numStaticModels);
            else
                clipMap->staticModelList = nullptr;
            for (unsigned int i = 0; i < clipMap->numStaticModels; i++)
            {
                auto currentStaticModel = &clipMap->staticModelList[i];
                auto currentStaticModelData = j["staticModelList"][i];
                
                auto xModelAsset = context.LoadDependency<AssetXModel>(std::string(currentStaticModelData["xmodel"]));
                if (!xModelAsset)
                {
                    printf("can't find xmodel\n");
                    return AssetCreationResult::Failure();
                }

                currentStaticModel->writable.nextModelInWorldSector = currentStaticModelData["nextModelInWorldSector"];
                currentStaticModel->xmodel = xModelAsset->Asset();
                currentStaticModel->contents = currentStaticModelData["contents"];
                currentStaticModel->origin.x = currentStaticModelData["origin"]["x"];
                currentStaticModel->origin.y = currentStaticModelData["origin"]["y"];
                currentStaticModel->origin.z = currentStaticModelData["origin"]["z"];
                currentStaticModel->invScaledAxis[0].x = currentStaticModelData["invScaledAxis0"]["x"];
                currentStaticModel->invScaledAxis[0].y = currentStaticModelData["invScaledAxis0"]["y"];
                currentStaticModel->invScaledAxis[0].z = currentStaticModelData["invScaledAxis0"]["z"];
                currentStaticModel->invScaledAxis[1].x = currentStaticModelData["invScaledAxis1"]["x"];
                currentStaticModel->invScaledAxis[1].y = currentStaticModelData["invScaledAxis1"]["y"];
                currentStaticModel->invScaledAxis[1].z = currentStaticModelData["invScaledAxis1"]["z"];
                currentStaticModel->invScaledAxis[2].x = currentStaticModelData["invScaledAxis2"]["x"];
                currentStaticModel->invScaledAxis[2].y = currentStaticModelData["invScaledAxis2"]["y"];
                currentStaticModel->invScaledAxis[2].z = currentStaticModelData["invScaledAxis2"]["z"];
                currentStaticModel->absmin.x = currentStaticModelData["absmin"]["x"];
                currentStaticModel->absmin.y = currentStaticModelData["absmin"]["y"];
                currentStaticModel->absmin.z = currentStaticModelData["absmin"]["z"];
                currentStaticModel->absmax.x = currentStaticModelData["absmax"]["x"];
                currentStaticModel->absmax.y = currentStaticModelData["absmax"]["y"];
                currentStaticModel->absmax.z = currentStaticModelData["absmax"]["z"];
            }

            clipMap->numNodes = j["numNodes"];
            if (clipMap->numNodes > 0)
                clipMap->nodes = m_memory.Alloc<cNode_t>(clipMap->numNodes);
            else
                clipMap->nodes = nullptr;
            for (unsigned int i = 0; i < clipMap->numNodes; i++)
            {
                auto currentNode = &clipMap->nodes[i];
                auto currentNodeData = j["nodes"][i];

                int planeIndex = currentNodeData["plane"];
                currentNode->plane = &clipMap->info.planes[planeIndex];
                currentNode->children[0] = currentNodeData["children0"];
                currentNode->children[1] = currentNodeData["children1"];
            }

            clipMap->numLeafs = j["numLeafs"];
            if (clipMap->numLeafs > 0)
                clipMap->leafs = m_memory.Alloc<cLeaf_s>(clipMap->numLeafs);
            else
                clipMap->leafs = nullptr;
            for (unsigned int i = 0; i < clipMap->numLeafs; i++)
            {
                auto currentLeaf = &clipMap->leafs[i];
                auto currentLeafData = j["leafs"][i];

                currentLeaf->firstCollAabbIndex = currentLeafData["firstCollAabbIndex"];
                currentLeaf->collAabbCount = currentLeafData["collAabbCount"];
                currentLeaf->brushContents = currentLeafData["brushContents"];
                currentLeaf->terrainContents = currentLeafData["terrainContents"];
                currentLeaf->mins.x = currentLeafData["mins"]["x"];
                currentLeaf->mins.y = currentLeafData["mins"]["y"];
                currentLeaf->mins.z = currentLeafData["mins"]["z"];
                currentLeaf->maxs.x = currentLeafData["maxs"]["x"];
                currentLeaf->maxs.y = currentLeafData["maxs"]["y"];
                currentLeaf->maxs.z = currentLeafData["maxs"]["z"];
                currentLeaf->leafBrushNode = currentLeafData["leafBrushNode"];
                currentLeaf->cluster = currentLeafData["cluster"];
            }

            clipMap->vertCount = j["vertCount"];
            if (clipMap->vertCount > 0)
                clipMap->verts = m_memory.Alloc<vec3_t>(clipMap->vertCount);
            else
                clipMap->verts = nullptr;
            for (unsigned int i = 0; i < clipMap->vertCount; i++)
            {
                auto currentVert = &clipMap->verts[i];
                auto currentVertData = j["verts"][i];

                currentVert->x = currentVertData["x"];
                currentVert->y = currentVertData["y"];
                currentVert->z = currentVertData["z"];
            }

            clipMap->triCount = j["triCount"];
            if (clipMap->triCount > 0)
                clipMap->triIndices = m_memory.Alloc<uint16_t[3]>(clipMap->triCount);
            else
                clipMap->triIndices = nullptr;
            for (int i = 0; i < clipMap->triCount; i++)
            {
                auto currentTriIndex = clipMap->triIndices[i];
                auto currentTriIndexData = j["triIndices"][i];

                currentTriIndex[0] = currentTriIndexData["0"];
                currentTriIndex[1] = currentTriIndexData["1"];
                currentTriIndex[2] = currentTriIndexData["2"];
            }

            int walkableEdgeCount = (3 * clipMap->triCount + 31) / 32 * 4;
            clipMap->triEdgeIsWalkable = m_memory.Alloc<char>(walkableEdgeCount);
            for (int i = 0; i < walkableEdgeCount; i++)
            {
                clipMap->triEdgeIsWalkable[i] = static_cast<unsigned char>(j["triEdgeIsWalkable"][i]);
            }

            clipMap->partitionCount = j["partitionCount"];
            if (clipMap->partitionCount > 0)
                clipMap->partitions = m_memory.Alloc<CollisionPartition>(clipMap->partitionCount);
            else
                clipMap->partitions = nullptr;
            for (int i = 0; i < clipMap->partitionCount; i++)
            {
                auto currentPartition = &clipMap->partitions[i];
                auto currentPartitionData = j["partitions"][i];

                currentPartition->triCount = static_cast<unsigned char>(currentPartitionData["triCount"]);
                currentPartition->firstTri = currentPartitionData["firstTri"];
                currentPartition->nuinds = currentPartitionData["nuinds"];
                currentPartition->fuind = currentPartitionData["fuind"];
            }

            clipMap->aabbTreeCount = j["aabbTreeCount"];
            if (clipMap->aabbTreeCount > 0)
                clipMap->aabbTrees = m_memory.Alloc<CollisionAabbTree>(clipMap->aabbTreeCount);
            else
                clipMap->aabbTrees = nullptr;
            for (int i = 0; i < clipMap->aabbTreeCount; i++)
            {
                auto currentAabbTree = &clipMap->aabbTrees[i];
                auto currentAabbTreeData = j["aabbTrees"][i];

                currentAabbTree->origin.x = currentAabbTreeData["origin"]["x"];
                currentAabbTree->origin.y = currentAabbTreeData["origin"]["y"];
                currentAabbTree->origin.z = currentAabbTreeData["origin"]["z"];
                currentAabbTree->materialIndex = currentAabbTreeData["materialIndex"];
                currentAabbTree->childCount = currentAabbTreeData["childCount"];
                currentAabbTree->halfSize.x = currentAabbTreeData["halfSize"]["x"];
                currentAabbTree->halfSize.y = currentAabbTreeData["halfSize"]["y"];
                currentAabbTree->halfSize.z = currentAabbTreeData["halfSize"]["z"];
                currentAabbTree->u.firstChildIndex = currentAabbTreeData["u"];
            }

            clipMap->numSubModels = j["numSubModels"];
            if (clipMap->numSubModels > 0)
                clipMap->cmodels = m_memory.Alloc<cmodel_t>(clipMap->numSubModels);
            else
                clipMap->cmodels = nullptr;
            for (unsigned int i = 0; i < clipMap->numSubModels; i++)
            {
                auto currentSubModel = &clipMap->cmodels[i];
                auto currentSubModelData = j["cmodels"][i];

                currentSubModel->info = clipMap->pInfo;
                currentSubModel->maxs.x = currentSubModelData["maxs"]["x"];
                currentSubModel->maxs.y = currentSubModelData["maxs"]["y"];
                currentSubModel->maxs.z = currentSubModelData["maxs"]["z"];
                currentSubModel->mins.x = currentSubModelData["mins"]["x"];
                currentSubModel->mins.y = currentSubModelData["mins"]["y"];
                currentSubModel->mins.z = currentSubModelData["mins"]["z"];
                currentSubModel->radius = currentSubModelData["radius"];
                currentSubModel->leaf.firstCollAabbIndex = currentSubModelData["leaf"]["firstCollAabbIndex"];
                currentSubModel->leaf.collAabbCount = currentSubModelData["leaf"]["collAabbCount"];
                currentSubModel->leaf.brushContents = currentSubModelData["leaf"]["brushContents"];
                currentSubModel->leaf.terrainContents = currentSubModelData["leaf"]["terrainContents"];
                currentSubModel->leaf.maxs.x = currentSubModelData["leaf"]["maxs"]["x"];
                currentSubModel->leaf.maxs.y = currentSubModelData["leaf"]["maxs"]["y"];
                currentSubModel->leaf.maxs.z = currentSubModelData["leaf"]["maxs"]["z"];
                currentSubModel->leaf.mins.x = currentSubModelData["leaf"]["mins"]["x"];
                currentSubModel->leaf.mins.y = currentSubModelData["leaf"]["mins"]["y"];
                currentSubModel->leaf.mins.z = currentSubModelData["leaf"]["mins"]["z"];
                currentSubModel->leaf.leafBrushNode = currentSubModelData["leaf"]["leafBrushNode"];
                currentSubModel->leaf.cluster = currentSubModelData["leaf"]["cluster"];
            }

            clipMap->numClusters = j["numClusters"];
            clipMap->clusterBytes = j["clusterBytes"];
            clipMap->vised = j["vised"];
            if (clipMap->numClusters * clipMap->clusterBytes > 0)
                clipMap->visibility = m_memory.Alloc<char>(clipMap->numClusters * clipMap->clusterBytes);
            else
                clipMap->visibility = nullptr;
            for (int i = 0; i < clipMap->numClusters * clipMap->clusterBytes; i++)
            {
                clipMap->visibility[i] = static_cast<unsigned char>(j["visibility"][i]);
            }

            auto mapEntAsset = context.LoadDependency<AssetMapEnts>(clipMap->name);
            if (!mapEntAsset)
            {
                printf("can't find map ents\n");
                return AssetCreationResult::Failure();
            }
            clipMap->mapEnts = mapEntAsset->Asset();

            clipMap->box_brush = m_memory.Alloc<cbrush_t>(1);
            clipMap->box_brush->mins.x = j["box_brush"]["mins"]["x"];
            clipMap->box_brush->mins.y = j["box_brush"]["mins"]["y"];
            clipMap->box_brush->mins.z = j["box_brush"]["mins"]["z"];
            clipMap->box_brush->maxs.x = j["box_brush"]["maxs"]["x"];
            clipMap->box_brush->maxs.y = j["box_brush"]["maxs"]["y"];
            clipMap->box_brush->maxs.z = j["box_brush"]["maxs"]["z"];
            clipMap->box_brush->axial_cflags[0][0] = j["box_brush"]["axial_cflags00"];
            clipMap->box_brush->axial_cflags[0][1] = j["box_brush"]["axial_cflags01"];
            clipMap->box_brush->axial_cflags[0][2] = j["box_brush"]["axial_cflags02"];
            clipMap->box_brush->axial_cflags[1][0] = j["box_brush"]["axial_cflags10"];
            clipMap->box_brush->axial_cflags[1][1] = j["box_brush"]["axial_cflags11"];
            clipMap->box_brush->axial_cflags[1][2] = j["box_brush"]["axial_cflags12"];
            clipMap->box_brush->axial_sflags[0][0] = j["box_brush"]["axial_sflags00"];
            clipMap->box_brush->axial_sflags[0][1] = j["box_brush"]["axial_sflags01"];
            clipMap->box_brush->axial_sflags[0][2] = j["box_brush"]["axial_sflags02"];
            clipMap->box_brush->axial_sflags[1][0] = j["box_brush"]["axial_sflags10"];
            clipMap->box_brush->axial_sflags[1][1] = j["box_brush"]["axial_sflags11"];
            clipMap->box_brush->axial_sflags[1][2] = j["box_brush"]["axial_sflags12"];
            clipMap->box_brush->contents = j["box_brush"]["contents"];
            clipMap->box_brush->numsides = j["box_brush"]["numsides"];
            clipMap->box_brush->numverts = j["box_brush"]["numverts"];
            clipMap->box_brush->sides = nullptr;
            clipMap->box_brush->verts = nullptr;
            //////////////////////////////////////////////////////////////////////////////////////////////////
            assert(clipMap->box_brush->numsides == 0);
            assert(clipMap->box_brush->numverts == 0);

            clipMap->box_model.mins.x = j["box_model"]["mins"]["x"];
            clipMap->box_model.mins.y = j["box_model"]["mins"]["y"];
            clipMap->box_model.mins.z = j["box_model"]["mins"]["z"];
            clipMap->box_model.maxs.x = j["box_model"]["maxs"]["x"];
            clipMap->box_model.maxs.y = j["box_model"]["maxs"]["y"];
            clipMap->box_model.maxs.z = j["box_model"]["maxs"]["z"];
            clipMap->box_model.radius = j["box_model"]["radius"];
            clipMap->box_model.info = clipMap->pInfo;

            clipMap->box_model.leaf.firstCollAabbIndex = j["box_model"]["leaf"]["firstCollAabbIndex"];
            clipMap->box_model.leaf.collAabbCount = j["box_model"]["leaf"]["collAabbCount"];
            clipMap->box_model.leaf.brushContents = j["box_model"]["leaf"]["brushContents"];
            clipMap->box_model.leaf.terrainContents = j["box_model"]["leaf"]["terrainContents"];
            clipMap->box_model.leaf.mins.x = j["box_model"]["leaf"]["mins"]["x"];
            clipMap->box_model.leaf.mins.y = j["box_model"]["leaf"]["mins"]["y"];
            clipMap->box_model.leaf.mins.z = j["box_model"]["leaf"]["mins"]["z"];
            clipMap->box_model.leaf.maxs.x = j["box_model"]["leaf"]["maxs"]["x"];
            clipMap->box_model.leaf.maxs.y = j["box_model"]["leaf"]["maxs"]["y"];
            clipMap->box_model.leaf.maxs.z = j["box_model"]["leaf"]["maxs"]["z"];
            clipMap->box_model.leaf.leafBrushNode = j["box_model"]["leaf"]["leafBrushNode"];
            clipMap->box_model.leaf.cluster = j["box_model"]["leaf"]["cluster"];

            clipMap->originalDynEntCount = j["originalDynEntCount"];
            clipMap->dynEntCount[0] = j["dynEntCount0"];
            clipMap->dynEntCount[1] = j["dynEntCount1"];
            clipMap->dynEntCount[2] = j["dynEntCount2"];
            clipMap->dynEntCount[3] = j["dynEntCount3"];

            if (clipMap->dynEntCount[0] > 0)
                clipMap->dynEntDefList[0] = m_memory.Alloc<DynEntityDef>(clipMap->dynEntCount[0]);
            else
                clipMap->dynEntDefList[0] = nullptr;
            for (int i = 0; i < clipMap->dynEntCount[0]; i++)
            {
                auto currDynEntDef = &clipMap->dynEntDefList[0][i];
                auto currDynEntDefData = j["dynEntDefList0"][i];
                
                std::string xModelName = currDynEntDefData["xModel"];
                std::string destroyedxModelName = currDynEntDefData["destroyedxModel"];
                std::string destroyFxName = currDynEntDefData["destroyFx"];
                std::string physPresetName = currDynEntDefData["physPreset"];
                
                XModel* xModel;
                if (xModelName.length() > 0)
                {
                    auto XModelAsset = context.LoadDependency<AssetXModel>(xModelName);
                    if (!XModelAsset)
                    {
                        printf("can't find XModelAsset\n");
                        return AssetCreationResult::Failure();
                    }
                    xModel = XModelAsset->Asset();
                }
                else
                    xModel = nullptr;

                XModel* destroyedxModel;
                if (destroyedxModelName.length() > 0)
                {
                    auto destroyedxModelAsset = context.LoadDependency<AssetXModel>(destroyedxModelName);
                    if (!destroyedxModelAsset)
                    {
                        printf("can't find destroyedxModel\n");
                        return AssetCreationResult::Failure();
                    }
                    destroyedxModel = destroyedxModelAsset->Asset();
                }
                else 
                    destroyedxModel = nullptr;

                FxEffectDef* destroyFx;
                if (destroyFxName.length() > 0)
                {
                    auto destroyFxAsset = context.LoadDependency<AssetFx>(destroyFxName);
                    if (!destroyFxAsset)
                    {
                        printf("can't find destroyFx\n");
                        return AssetCreationResult::Failure();
                    }
                    destroyFx = destroyFxAsset->Asset();
                }
                else
                    destroyFx = nullptr;

                PhysPreset* physPreset;
                if (physPresetName.length() > 0)
                {
                    auto physPresetAsset = context.LoadDependency<AssetPhysPreset>(physPresetName);
                    if (!physPresetAsset)
                    {
                        printf("can't find physPreset\n");
                        return AssetCreationResult::Failure();
                    }
                    physPreset = physPresetAsset->Asset();
                }
                else
                    physPreset = nullptr;

                if (currDynEntDefData["destroyPieces"].size() > 0)
                {
                    currDynEntDef->destroyPieces = m_memory.Alloc<XModelPieces>();
                    currDynEntDef->destroyPieces->name = m_memory.Dup(std::string(currDynEntDefData["destroyPieces"]["name"]).c_str());
                    currDynEntDef->destroyPieces->numpieces = currDynEntDefData["destroyPieces"]["numpieces"];
                    currDynEntDef->destroyPieces->pieces = m_memory.Alloc<XModelPiece>(currDynEntDef->destroyPieces->numpieces);
                    for (int k = 0; k < currDynEntDef->destroyPieces->numpieces; k++)
                    {
                        auto currentPieceData = currDynEntDefData["destroyPieces"]["pieces"][k];

                        auto pieceAsset = context.LoadDependency<AssetXModel>(currentPieceData["model"]);
                        if (!pieceAsset)
                        {
                            printf("can't find piece model\n");
                            return AssetCreationResult::Failure();
                        }
                        auto pieceModel = pieceAsset->Asset();

                        currDynEntDef->destroyPieces->pieces[k].model = pieceModel;
                        currDynEntDef->destroyPieces->pieces[k].offset.x = currentPieceData["offset"]["x"];
                        currDynEntDef->destroyPieces->pieces[k].offset.y = currentPieceData["offset"]["y"];
                        currDynEntDef->destroyPieces->pieces[k].offset.z = currentPieceData["offset"]["z"];
                    }
                }
                else
                    currDynEntDef->destroyPieces = nullptr;

                currDynEntDef->type               = currDynEntDefData["type"];
                currDynEntDef->pose.quat.x        = currDynEntDefData["quat"]["x"];
                currDynEntDef->pose.quat.y        = currDynEntDefData["quat"]["y"];
                currDynEntDef->pose.quat.z        = currDynEntDefData["quat"]["z"];
                currDynEntDef->pose.quat.w        = currDynEntDefData["quat"]["w"];
                currDynEntDef->pose.origin.x      = currDynEntDefData["origin"]["x"];
                currDynEntDef->pose.origin.y      = currDynEntDefData["origin"]["y"];
                currDynEntDef->pose.origin.z      = currDynEntDefData["origin"]["z"];
                currDynEntDef->xModel             = xModel;
                currDynEntDef->destroyedxModel    = destroyedxModel;
                currDynEntDef->brushModel         = currDynEntDefData["brushModel"];
                currDynEntDef->physicsBrushModel  = currDynEntDefData["physicsBrushModel"];
                currDynEntDef->destroyFx          = destroyFx;
                currDynEntDef->destroySound       = currDynEntDefData["destroySound"];
                currDynEntDef->physPreset         = physPreset;
                currDynEntDef->physConstraints[0] = currDynEntDefData["physConstraints0"];
                currDynEntDef->physConstraints[1] = currDynEntDefData["physConstraints1"];
                currDynEntDef->physConstraints[2] = currDynEntDefData["physConstraints2"];
                currDynEntDef->physConstraints[3] = currDynEntDefData["physConstraints3"];
                currDynEntDef->health             = currDynEntDefData["health"];
                currDynEntDef->flags              = currDynEntDefData["flags"];
                currDynEntDef->contents           = currDynEntDefData["contents"];
                currDynEntDef->targetname         = m_zone.m_script_strings.AddOrGetScriptString(currDynEntDefData["targetname"]);
                currDynEntDef->target             = m_zone.m_script_strings.AddOrGetScriptString(currDynEntDefData["target"]);
            }

            if (clipMap->dynEntCount[1] > 0)
                clipMap->dynEntDefList[1] = m_memory.Alloc<DynEntityDef>(clipMap->dynEntCount[1]);
            else
                clipMap->dynEntDefList[1] = nullptr;
            for (int i = 0; i < clipMap->dynEntCount[1]; i++)
            {
                auto currDynEntDef = &clipMap->dynEntDefList[1][i];
                auto currDynEntDefData = j["dynEntDefList1"][i];

                std::string xModelName = currDynEntDefData["xModel"];
                std::string destroyedxModelName = currDynEntDefData["destroyedxModel"];
                std::string destroyFxName = currDynEntDefData["destroyFx"];
                std::string physPresetName = currDynEntDefData["physPreset"];

                XModel* xModel;
                if (xModelName.length() > 0)
                {
                    auto XModelAsset = context.LoadDependency<AssetXModel>(xModelName);
                    if (!XModelAsset)
                    {
                        printf("can't find XModelAsset\n");
                        return AssetCreationResult::Failure();
                    }
                    xModel = XModelAsset->Asset();
                }
                else
                    xModel = nullptr;

                XModel* destroyedxModel;
                if (destroyedxModelName.length() > 0)
                {
                    auto destroyedxModelAsset = context.LoadDependency<AssetXModel>(destroyedxModelName);
                    if (!destroyedxModelAsset)
                    {
                        printf("can't find destroyedxModel\n");
                        return AssetCreationResult::Failure();
                    }
                    destroyedxModel = destroyedxModelAsset->Asset();
                }
                else
                    destroyedxModel = nullptr;

                FxEffectDef* destroyFx;
                if (destroyFxName.length() > 0)
                {
                    auto destroyFxAsset = context.LoadDependency<AssetFx>(destroyFxName);
                    if (!destroyFxAsset)
                    {
                        printf("can't find destroyFx\n");
                        return AssetCreationResult::Failure();
                    }
                    destroyFx = destroyFxAsset->Asset();
                }
                else
                    destroyFx = nullptr;

                PhysPreset* physPreset;
                if (physPresetName.length() > 0)
                {
                    auto physPresetAsset = context.LoadDependency<AssetPhysPreset>(physPresetName);
                    if (!physPresetAsset)
                    {
                        printf("can't find physPreset\n");
                        return AssetCreationResult::Failure();
                    }
                    physPreset = physPresetAsset->Asset();
                }
                else
                    physPreset = nullptr;

                if (currDynEntDefData["destroyPieces"].size() > 0)
                {
                    currDynEntDef->destroyPieces = m_memory.Alloc<XModelPieces>();
                    currDynEntDef->destroyPieces->name = m_memory.Dup(std::string(currDynEntDefData["destroyPieces"]["name"]).c_str());
                    currDynEntDef->destroyPieces->numpieces = currDynEntDefData["destroyPieces"]["numpieces"];
                    currDynEntDef->destroyPieces->pieces = m_memory.Alloc<XModelPiece>(currDynEntDef->destroyPieces->numpieces);
                    for (int k = 0; k < currDynEntDef->destroyPieces->numpieces; k++)
                    {
                        auto currentPieceData = currDynEntDefData["destroyPieces"]["pieces"][k];

                        auto pieceAsset = context.LoadDependency<AssetXModel>(currentPieceData["model"]);
                        if (!pieceAsset)
                        {
                            printf("can't find piece model\n");
                            return AssetCreationResult::Failure();
                        }
                        auto pieceModel = pieceAsset->Asset();

                        currDynEntDef->destroyPieces->pieces[k].model = pieceModel;
                        currDynEntDef->destroyPieces->pieces[k].offset.x = currentPieceData["offset"]["x"];
                        currDynEntDef->destroyPieces->pieces[k].offset.y = currentPieceData["offset"]["y"];
                        currDynEntDef->destroyPieces->pieces[k].offset.z = currentPieceData["offset"]["z"];
                    }
                }
                else
                    currDynEntDef->destroyPieces = nullptr;

                currDynEntDef->type = currDynEntDefData["type"];
                currDynEntDef->pose.quat.x = currDynEntDefData["quat"]["x"];
                currDynEntDef->pose.quat.y = currDynEntDefData["quat"]["y"];
                currDynEntDef->pose.quat.z = currDynEntDefData["quat"]["z"];
                currDynEntDef->pose.quat.w = currDynEntDefData["quat"]["w"];
                currDynEntDef->pose.origin.x = currDynEntDefData["origin"]["x"];
                currDynEntDef->pose.origin.y = currDynEntDefData["origin"]["y"];
                currDynEntDef->pose.origin.z = currDynEntDefData["origin"]["z"];
                currDynEntDef->xModel = xModel;
                currDynEntDef->destroyedxModel = destroyedxModel;
                currDynEntDef->brushModel = currDynEntDefData["brushModel"];
                currDynEntDef->physicsBrushModel = currDynEntDefData["physicsBrushModel"];
                currDynEntDef->destroyFx = destroyFx;
                currDynEntDef->destroySound = currDynEntDefData["destroySound"];
                currDynEntDef->physPreset = physPreset;
                currDynEntDef->physConstraints[0] = currDynEntDefData["physConstraints0"];
                currDynEntDef->physConstraints[1] = currDynEntDefData["physConstraints1"];
                currDynEntDef->physConstraints[2] = currDynEntDefData["physConstraints2"];
                currDynEntDef->physConstraints[3] = currDynEntDefData["physConstraints3"];
                currDynEntDef->health = currDynEntDefData["health"];
                currDynEntDef->flags = currDynEntDefData["flags"];
                currDynEntDef->contents = currDynEntDefData["contents"];
                currDynEntDef->targetname = currDynEntDefData["targetname"];
                currDynEntDef->target = currDynEntDefData["target"];
            }

            if (clipMap->dynEntCount[0] > 0)
                clipMap->dynEntPoseList[0] = m_memory.Alloc<DynEntityPose>(clipMap->dynEntCount[0]);
            else
                clipMap->dynEntPoseList[0] = nullptr;
            for (int i = 0; i < clipMap->dynEntCount[0]; i++)
            {
                auto currDynEntPose = &clipMap->dynEntPoseList[0][i];
                auto currDynEntPoseData = j["dynEntPoseList0"][i];

                currDynEntPose->radius = currDynEntPoseData["radius"];
                currDynEntPose->pose.quat.x = currDynEntPoseData["quat"]["x"];
                currDynEntPose->pose.quat.y = currDynEntPoseData["quat"]["y"];
                currDynEntPose->pose.quat.z = currDynEntPoseData["quat"]["z"];
                currDynEntPose->pose.quat.w = currDynEntPoseData["quat"]["w"];
                currDynEntPose->pose.origin.x = currDynEntPoseData["origin"]["x"];
                currDynEntPose->pose.origin.y = currDynEntPoseData["origin"]["y"];
                currDynEntPose->pose.origin.z = currDynEntPoseData["origin"]["z"];
            }

            if (clipMap->dynEntCount[1] > 0)
                clipMap->dynEntPoseList[1] = m_memory.Alloc<DynEntityPose>(clipMap->dynEntCount[1]);
            else
                clipMap->dynEntPoseList[1] = nullptr;
            for (int i = 0; i < clipMap->dynEntCount[1]; i++)
            {
                auto currDynEntPose = &clipMap->dynEntPoseList[1][i];
                auto currDynEntPoseData = j["dynEntPoseList1"][i];

                currDynEntPose->radius = currDynEntPoseData["radius"];
                currDynEntPose->pose.quat.x = currDynEntPoseData["quat"]["x"];
                currDynEntPose->pose.quat.y = currDynEntPoseData["quat"]["y"];
                currDynEntPose->pose.quat.z = currDynEntPoseData["quat"]["z"];
                currDynEntPose->pose.quat.w = currDynEntPoseData["quat"]["w"];
                currDynEntPose->pose.origin.x = currDynEntPoseData["origin"]["x"];
                currDynEntPose->pose.origin.y = currDynEntPoseData["origin"]["y"];
                currDynEntPose->pose.origin.z = currDynEntPoseData["origin"]["z"];
            }  
                
               if (clipMap->dynEntCount[0] > 0)
                clipMap->dynEntClientList[0] = m_memory.Alloc<DynEntityClient>(clipMap->dynEntCount[0]);
            else             
                clipMap->dynEntClientList[0] = nullptr;
            for (int i = 0; i < clipMap->dynEntCount[0]; i++)
            {
                auto currentDynEntClient = &clipMap->dynEntClientList[0][i];
                auto currentDynEntClientData = j["dynEntClientList0"][i];

                currentDynEntClient->physObjId = currentDynEntClientData["physObjId"];
                currentDynEntClient->flags = currentDynEntClientData["flags"];
                currentDynEntClient->lightingHandle = currentDynEntClientData["lightingHandle"];
                currentDynEntClient->health = currentDynEntClientData["health"];
                currentDynEntClient->burnTime = currentDynEntClientData["burnTime"];
                currentDynEntClient->fadeTime = currentDynEntClientData["fadeTime"];
                currentDynEntClient->physicsStartTime = currentDynEntClientData["physicsStartTime"];
            }

            if (clipMap->dynEntCount[1] > 0)
                clipMap->dynEntClientList[1] = m_memory.Alloc<DynEntityClient>(clipMap->dynEntCount[1]);
            else             
                clipMap->dynEntClientList[1] = nullptr;
            for (int i = 0; i < clipMap->dynEntCount[1]; i++)
            {
                auto currentDynEntClient = &clipMap->dynEntClientList[1][i];
                auto currentDynEntClientData = j["dynEntClientList1"][i];

                currentDynEntClient->physObjId = currentDynEntClientData["physObjId"];
                currentDynEntClient->flags = currentDynEntClientData["flags"];
                currentDynEntClient->lightingHandle = currentDynEntClientData["lightingHandle"];
                currentDynEntClient->health = currentDynEntClientData["health"];
                currentDynEntClient->burnTime = currentDynEntClientData["burnTime"];
                currentDynEntClient->fadeTime = currentDynEntClientData["fadeTime"];
                currentDynEntClient->physicsStartTime = currentDynEntClientData["physicsStartTime"];
            }

            if (clipMap->dynEntCount[2] > 0)
                clipMap->dynEntServerList[0] = m_memory.Alloc<DynEntityServer>(clipMap->dynEntCount[2]);
            else        
                clipMap->dynEntServerList[0] = nullptr;
            for (int i = 0; i < clipMap->dynEntCount[2]; i++)
            {
                auto currentDynEntServer = &clipMap->dynEntServerList[0][i];
                auto currentDynEntServerData = j["dynEntServerList0"][i];

                currentDynEntServer->flags = currentDynEntServerData["flags"];
                currentDynEntServer->health = currentDynEntServerData["health"];
            }

            if (clipMap->dynEntCount[3] > 0)
                clipMap->dynEntServerList[1] = m_memory.Alloc<DynEntityServer>(clipMap->dynEntCount[3]);
            else       
                clipMap->dynEntServerList[1] = nullptr;
            for (int i = 0; i < clipMap->dynEntCount[3]; i++)
            {
                auto currentDynEntServer = &clipMap->dynEntServerList[1][i];
                auto currentDynEntServerData = j["dynEntServerList1"][i];

                currentDynEntServer->flags = currentDynEntServerData["flags"];
                currentDynEntServer->health = currentDynEntServerData["health"];
            }

            if (clipMap->dynEntCount[0] > 0)
                clipMap->dynEntCollList[0] = m_memory.Alloc<DynEntityColl>(clipMap->dynEntCount[0]);
            else 
                clipMap->dynEntCollList[0] = nullptr;
            for (int i = 0; i < clipMap->dynEntCount[0]; i++)
            {
                auto currentDynEntColl = &clipMap->dynEntCollList[0][i];
                auto currentDynEntCollData = j["dynEntCollList0"][i];

                currentDynEntColl->sector = currentDynEntCollData["sector"];
                currentDynEntColl->nextEntInSector = currentDynEntCollData["nextEntInSector"];
                currentDynEntColl->linkMins.x = currentDynEntCollData["linkMins"]["x"];
                currentDynEntColl->linkMins.y = currentDynEntCollData["linkMins"]["y"];
                currentDynEntColl->linkMins.z = currentDynEntCollData["linkMins"]["z"];
                currentDynEntColl->linkMaxs.x = currentDynEntCollData["linkMaxs"]["x"];
                currentDynEntColl->linkMaxs.y = currentDynEntCollData["linkMaxs"]["y"];
                currentDynEntColl->linkMaxs.z = currentDynEntCollData["linkMaxs"]["z"];
                currentDynEntColl->contents = currentDynEntCollData["contents"];
            }

            if (clipMap->dynEntCount[1] > 0)
                clipMap->dynEntCollList[1] = m_memory.Alloc<DynEntityColl>(clipMap->dynEntCount[1]);
            else    
                clipMap->dynEntCollList[1] = nullptr;
            for (int i = 0; i < clipMap->dynEntCount[1]; i++)
            {
                auto currentDynEntColl = &clipMap->dynEntCollList[1][i];
                auto currentDynEntCollData = j["dynEntCollList1"][i];

                currentDynEntColl->sector = currentDynEntCollData["sector"];
                currentDynEntColl->nextEntInSector = currentDynEntCollData["nextEntInSector"];
                currentDynEntColl->linkMins.x = currentDynEntCollData["linkMins"]["x"];
                currentDynEntColl->linkMins.y = currentDynEntCollData["linkMins"]["y"];
                currentDynEntColl->linkMins.z = currentDynEntCollData["linkMins"]["z"];
                currentDynEntColl->linkMaxs.x = currentDynEntCollData["linkMaxs"]["x"];
                currentDynEntColl->linkMaxs.y = currentDynEntCollData["linkMaxs"]["y"];
                currentDynEntColl->linkMaxs.z = currentDynEntCollData["linkMaxs"]["z"];
                currentDynEntColl->contents = currentDynEntCollData["contents"];
            }

            if (clipMap->dynEntCount[2] > 0)
                clipMap->dynEntCollList[2] = m_memory.Alloc<DynEntityColl>(clipMap->dynEntCount[2]);
            else        
                clipMap->dynEntCollList[2] = nullptr;
            for (int i = 0; i < clipMap->dynEntCount[2]; i++)
            {
                auto currentDynEntColl = &clipMap->dynEntCollList[2][i];
                auto currentDynEntCollData = j["dynEntCollList2"][i];

                currentDynEntColl->sector = currentDynEntCollData["sector"];
                currentDynEntColl->nextEntInSector = currentDynEntCollData["nextEntInSector"];
                currentDynEntColl->linkMins.x = currentDynEntCollData["linkMins"]["x"];
                currentDynEntColl->linkMins.y = currentDynEntCollData["linkMins"]["y"];
                currentDynEntColl->linkMins.z = currentDynEntCollData["linkMins"]["z"];
                currentDynEntColl->linkMaxs.x = currentDynEntCollData["linkMaxs"]["x"];
                currentDynEntColl->linkMaxs.y = currentDynEntCollData["linkMaxs"]["y"];
                currentDynEntColl->linkMaxs.z = currentDynEntCollData["linkMaxs"]["z"];
                currentDynEntColl->contents = currentDynEntCollData["contents"];
            }

            if (clipMap->dynEntCount[3] > 0)
                clipMap->dynEntCollList[3] = m_memory.Alloc<DynEntityColl>(clipMap->dynEntCount[3]);
            else
                clipMap->dynEntCollList[3] = nullptr;
            for (int i = 0; i < clipMap->dynEntCount[3]; i++)
            {
                auto currentDynEntColl = &clipMap->dynEntCollList[3][i];
                auto currentDynEntCollData = j["dynEntCollList3"][i];

                currentDynEntColl->sector = currentDynEntCollData["sector"];
                currentDynEntColl->nextEntInSector = currentDynEntCollData["nextEntInSector"];
                currentDynEntColl->linkMins.x = currentDynEntCollData["linkMins"]["x"];
                currentDynEntColl->linkMins.y = currentDynEntCollData["linkMins"]["y"];
                currentDynEntColl->linkMins.z = currentDynEntCollData["linkMins"]["z"];
                currentDynEntColl->linkMaxs.x = currentDynEntCollData["linkMaxs"]["x"];
                currentDynEntColl->linkMaxs.y = currentDynEntCollData["linkMaxs"]["y"];
                currentDynEntColl->linkMaxs.z = currentDynEntCollData["linkMaxs"]["z"];
                currentDynEntColl->contents = currentDynEntCollData["contents"];
            }

            clipMap->num_constraints = j["num_constraints"];
            if (clipMap->num_constraints > 0)
                clipMap->constraints = m_memory.Alloc<PhysConstraint>(clipMap->num_constraints);
            else
                clipMap->constraints = nullptr;
            for (int i = 0; i < clipMap->num_constraints; i++)
            {
                auto currentConstraint = &clipMap->constraints[i];
                auto currentConstraintData = j["constraints"][i];

                std::string conMaterialName = currentConstraintData["material"];
                Material* conMaterial;
                if (conMaterialName.length() > 0)
                {
                    auto conMaterialAsset = context.LoadDependency<AssetMaterial>(conMaterialName);
                    if (!conMaterialAsset)
                    {
                        printf("can't find conMaterial\n");
                        return AssetCreationResult::Failure();
                    }
                    conMaterial = conMaterialAsset->Asset();
                }
                else
                    conMaterial = nullptr;

                currentConstraint->targetname           = currentConstraintData["targetname"];
                currentConstraint->type                 = currentConstraintData["type"];
                currentConstraint->attach_point_type1   = currentConstraintData["attach_point_type1"];
                currentConstraint->target_index1        = currentConstraintData["target_index1"];
                currentConstraint->target_ent1          = currentConstraintData["target_ent1"];
                currentConstraint->target_bone1         = m_memory.Dup(std::string(currentConstraintData["target_bone1"]).c_str());
                currentConstraint->attach_point_type2   = currentConstraintData["attach_point_type2"];
                currentConstraint->target_index2        = currentConstraintData["target_index2"];
                currentConstraint->target_ent2          = currentConstraintData["target_ent2"];
                currentConstraint->target_bone2         =  m_memory.Dup(std::string(currentConstraintData["target_bone2"]).c_str());
                currentConstraint->offset.x             = currentConstraintData["offset"]["x"];
                currentConstraint->offset.y             = currentConstraintData["offset"]["y"];
                currentConstraint->offset.z             = currentConstraintData["offset"]["z"];
                currentConstraint->pos.x                = currentConstraintData["pos"]["x"];
                currentConstraint->pos.y                = currentConstraintData["pos"]["y"];
                currentConstraint->pos.z                = currentConstraintData["pos"]["z"];
                currentConstraint->pos2.x               = currentConstraintData["pos2"]["x"];
                currentConstraint->pos2.y               = currentConstraintData["pos2"]["y"];
                currentConstraint->pos2.z               = currentConstraintData["pos2"]["z"];
                currentConstraint->dir.x                = currentConstraintData["dir"]["x"];
                currentConstraint->dir.y                = currentConstraintData["dir"]["y"];
                currentConstraint->dir.z                = currentConstraintData["dir"]["z"];
                currentConstraint->flags                 = currentConstraintData["flags"];
                currentConstraint->timeout               = currentConstraintData["timeout"];
                currentConstraint->min_health            = currentConstraintData["min_health"];
                currentConstraint->max_health            = currentConstraintData["max_health"];
                currentConstraint->distance              = currentConstraintData["distance"];
                currentConstraint->damp                  = currentConstraintData["damp"];
                currentConstraint->power                 = currentConstraintData["power"];
                currentConstraint->scale.x              = currentConstraintData["scale"]["x"];
                currentConstraint->scale.y              = currentConstraintData["scale"]["y"];
                currentConstraint->scale.z              = currentConstraintData["scale"]["z"];
                currentConstraint->spin_scale            = currentConstraintData["spin_scale"];
                currentConstraint->minAngle              = currentConstraintData["minAngle"];
                currentConstraint->maxAngle              = currentConstraintData["maxAngle"];
                currentConstraint->material              = conMaterial;
                currentConstraint->constraintHandle      = currentConstraintData["constraintHandle"];
                currentConstraint->rope_index            = currentConstraintData["rope_index"];
                currentConstraint->centity_num[0]        = currentConstraintData["centity_num0"];
                currentConstraint->centity_num[1]        = currentConstraintData["centity_num1"];
                currentConstraint->centity_num[2]        = currentConstraintData["centity_num2"];
                currentConstraint->centity_num[3]        = currentConstraintData["centity_num3"];

            }

            clipMap->max_ropes = clipMap->num_constraints + 32;
            clipMap->ropes = m_memory.Alloc<rope_t>(clipMap->max_ropes);

            clipMap->checksum = j["checksum"];

            return AssetCreationResult::Success(context.AddAsset<AssetClipMapPvs>(assetName, clipMap));
        }

    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        Zone& m_zone;
    };
} // namespace

namespace T6
{
    std::unique_ptr<AssetCreator<AssetClipMapPvs>> CreateClipMapLoader(MemoryManager& memory, ISearchPath& searchPath, Zone& zone)
    {
        return std::make_unique<ClipMapLoader>(memory, searchPath, zone);
    }
} // namespace T6
