#include "ClipMapLinker.h"

#include "BSP/BSPUtil.h"
#include "BSPCalculation.h"

#include <limits>
#include <map>

using namespace T6;
using namespace BSP;

namespace
{
    struct ColPartition
    {
        size_t parentSurfaceIndex;
        size_t indexStartIndex;
        size_t triCount;
        vec3_t mins;
        vec3_t maxs;
    };

    struct ColSurface
    {
        size_t materialIndex;
        size_t partitionIndex;
        size_t partitionCount;
    };

    struct ColBrush
    {
        size_t materialIndex;
        size_t brushVertCount;
        size_t brushVertStartIndex;
        vec3_t mins;
        vec3_t maxs;
    };

    struct ColModel
    {
        bspModelSurfType type;
        size_t colBrushIndex;
        size_t colBrushCount;
        size_t colSurfaceIndex;
        size_t colSurfaceCount;
    };

    struct CollisionData
    {
        size_t staticSurfaceCount; // static terrain always starts at 0
        std::vector<ColSurface> surfaceVec;
        std::vector<ColPartition> partitionVec;
        std::vector<uint16_t> surfaceIndices;
        std::vector<vec3_t> surfaceVerts;

        size_t staticBrushCount; // static brushes always starts at 0
        std::vector<ColBrush> brushVec;
        std::vector<vec3_t> brushVerts;

        std::vector<ColModel> models;
    };

    struct CollisionOutput
    {
        std::vector<cNode_t> nodeVec;
        std::vector<cLeaf_s> leafVec;
        std::vector<cLeafBrushNode_s> brushNodeVec;
        std::vector<CollisionAabbTree> AABBTreeVec;

        std::vector<CollisionPartition> partitionVec;
        std::vector<uint16_t> uniqueVertIndexVec;

        std::vector<cmodel_t> modelVec;
    };

    // BO2 has a maximum limit of 128 children per AABB tree (essentially),
    // so this is fixed by adding multiple parent AABB trees that hold 128 children each
    constexpr size_t MAX_AABB_TREE_CHILDREN = 128;

    constexpr size_t MAX_PARTITION_TRIS = 2;

    constexpr vec3_t normalX = {1.0f, 0.0f, 0.0f};
    constexpr vec3_t normalY = {0.0f, 1.0f, 0.0f};
    constexpr vec3_t normalZ = {0.0f, 0.0f, 1.0f};

    class ClipMapLinkerImpl : public ClipMapLinker
    {
    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        AssetCreationContext& m_context;

        void loadDynEnts(clipMap_t* clipMap)
        {
            int dynEntCount = 0;
            clipMap->originalDynEntCount = dynEntCount;
            clipMap->dynEntCount[0] = clipMap->originalDynEntCount + 256; // the game allocs 256 empty dynents, as they may be used ingame
            clipMap->dynEntCount[1] = 0;
            clipMap->dynEntCount[2] = 0;
            clipMap->dynEntCount[3] = 0;

            clipMap->dynEntClientList[0] = m_memory.Alloc<DynEntityClient>(clipMap->dynEntCount[0]);
            clipMap->dynEntClientList[1] = nullptr;

            clipMap->dynEntServerList[0] = nullptr;
            clipMap->dynEntServerList[1] = nullptr;

            clipMap->dynEntCollList[0] = m_memory.Alloc<DynEntityColl>(clipMap->dynEntCount[0]);
            clipMap->dynEntCollList[1] = nullptr;
            clipMap->dynEntCollList[2] = nullptr;
            clipMap->dynEntCollList[3] = nullptr;

            clipMap->dynEntPoseList[0] = m_memory.Alloc<DynEntityPose>(clipMap->dynEntCount[0]);
            clipMap->dynEntPoseList[1] = nullptr;

            clipMap->dynEntDefList[0] = m_memory.Alloc<DynEntityDef>(clipMap->dynEntCount[0]);
            clipMap->dynEntDefList[1] = nullptr;
        }

        void loadVisibility(clipMap_t* clipMap)
        {
            // Only use one visbility cluster for the entire map
            clipMap->numClusters = 1;
            clipMap->vised = 0;
            clipMap->clusterBytes = ((clipMap->numClusters + 63) >> 3) & 0xFFFFFFF8;
            clipMap->visibility = m_memory.Alloc<char>(clipMap->clusterBytes);
            // Official maps set visibility to all 0xFF
            memset(clipMap->visibility, 0xFF, clipMap->clusterBytes);
        }

        void loadBoxData(clipMap_t* clipMap)
        {
            // box_model and box_brush are what are used by game traces as "temporary" collision when
            //  no brush or model is specified to do the trace with.
            // All values in this function are taken from official map BSPs

            // for some reason the maxs are negative, and mins are positive
            // float box_mins = 3.4028235e38;
            // float box_maxs = -3.4028235e38;
            // hack: the floats above can't be safely converted to 32 bit floats, and the game requires them to be exact
            //  so we use the hex representation and set it using int pointers.
            unsigned int box_mins = 0x7F7FFFFF;
            unsigned int box_maxs = 0xFF7FFFFF;
            *(reinterpret_cast<unsigned int*>(&clipMap->box_model.leaf.mins.x)) = box_mins;
            *(reinterpret_cast<unsigned int*>(&clipMap->box_model.leaf.mins.y)) = box_mins;
            *(reinterpret_cast<unsigned int*>(&clipMap->box_model.leaf.mins.z)) = box_mins;
            *(reinterpret_cast<unsigned int*>(&clipMap->box_model.leaf.maxs.x)) = box_maxs;
            *(reinterpret_cast<unsigned int*>(&clipMap->box_model.leaf.maxs.y)) = box_maxs;
            *(reinterpret_cast<unsigned int*>(&clipMap->box_model.leaf.maxs.z)) = box_maxs;
            // assert(clipMap->box_model.leaf.mins.x == std::numeric_limits<float>::max());
            // assert(clipMap->box_model.leaf.maxs.x == std::numeric_limits<float>::min());

            clipMap->box_model.leaf.brushContents = -1;
            clipMap->box_model.leaf.terrainContents = 0;
            clipMap->box_model.leaf.cluster = 0;
            clipMap->box_model.leaf.collAabbCount = 0;
            clipMap->box_model.leaf.firstCollAabbIndex = 0;
            clipMap->box_model.leaf.leafBrushNode = 0;
            clipMap->box_model.mins.x = 0.0f;
            clipMap->box_model.mins.y = 0.0f;
            clipMap->box_model.mins.z = 0.0f;
            clipMap->box_model.maxs.x = 0.0f;
            clipMap->box_model.maxs.y = 0.0f;
            clipMap->box_model.maxs.z = 0.0f;
            clipMap->box_model.radius = 0.0f;
            clipMap->box_model.info = nullptr;

            clipMap->box_brush = m_memory.Alloc<cbrush_t>();
            clipMap->box_brush->axial_sflags[0][0] = -1;
            clipMap->box_brush->axial_sflags[0][1] = -1;
            clipMap->box_brush->axial_sflags[0][2] = -1;
            clipMap->box_brush->axial_sflags[1][0] = -1;
            clipMap->box_brush->axial_sflags[1][1] = -1;
            clipMap->box_brush->axial_sflags[1][2] = -1;
            clipMap->box_brush->axial_cflags[0][0] = -1;
            clipMap->box_brush->axial_cflags[0][1] = -1;
            clipMap->box_brush->axial_cflags[0][2] = -1;
            clipMap->box_brush->axial_cflags[1][0] = -1;
            clipMap->box_brush->axial_cflags[1][1] = -1;
            clipMap->box_brush->axial_cflags[1][2] = -1;
            clipMap->box_brush->contents = -1;
            clipMap->box_brush->mins.x = 0.0f;
            clipMap->box_brush->mins.y = 0.0f;
            clipMap->box_brush->mins.z = 0.0f;
            clipMap->box_brush->maxs.x = 0.0f;
            clipMap->box_brush->maxs.y = 0.0f;
            clipMap->box_brush->maxs.z = 0.0f;
            clipMap->box_brush->numsides = 0;
            clipMap->box_brush->numverts = 0;
            clipMap->box_brush->sides = nullptr;
            clipMap->box_brush->verts = nullptr;
        }

        void loadRopesAndConstraints(clipMap_t* clipMap)
        {
            clipMap->num_constraints = 0; // max 511
            clipMap->constraints = nullptr;

            // The game allocates 32 empty ropes
            clipMap->max_ropes = 32; // max 300
            clipMap->ropes = m_memory.Alloc<rope_t>(clipMap->max_ropes);
        }

        bool loadXModelCollision(clipMap_t* clipMap, BSPData* bsp)
        {
            // it seems like for players to be able to collide with xmodels, it requires xmodel->collmaps to be valid.
            // A lot of XModels don't implement collmaps (OAT also doesn't generate these collmaps atm), and
            //  even official maps instead use terrain or brushes to cover where the collision should be.

            clipMap->numStaticModels = (unsigned int)bsp->colWorld.xmodels.size();
            clipMap->staticModelList = m_memory.Alloc<cStaticModel_s>(clipMap->numStaticModels);

            if (clipMap->numStaticModels == 0)
                return true;
            if (clipMap->numStaticModels > 0xFFFF)
            {
                con::error("COLWorld exceeded the maximum number of static xmodels (65535 max, map count: {})", clipMap->numStaticModels);
                return false;
            }

            for (unsigned int i = 0; i < clipMap->numStaticModels; i++)
            {
                cStaticModel_s* currModel = &clipMap->staticModelList[i];
                BSPXModel& bspModel = bsp->colWorld.xmodels.at(i);

                vec3_t xmodelAxis[3];
                BSPUtil::convertQuaternionToAxis(&bspModel.rotationQuaternion, xmodelAxis);

                auto xModelAsset = m_context.LoadDependency<AssetXModel>(bspModel.name);
                if (xModelAsset == nullptr)
                {
                    con::error("Unable to load xmodel asset: \"{}\"", bspModel.name);
                    return false;
                }
                else
                    currModel->xmodel = (XModel*)xModelAsset->Asset();

                currModel->origin.x = bspModel.origin.x;
                currModel->origin.y = bspModel.origin.y;
                currModel->origin.z = bspModel.origin.z;

                // tracing only checks if this is equal to 0 or not
                currModel->contents = 1;

                if (!xModelAsset->IsReference())
                {
                    BSPUtil::calculateXmodelBounds(currModel->xmodel, xmodelAxis, currModel->absmin, currModel->absmax);
                    currModel->absmin.x = (currModel->absmin.x * bspModel.scale) + bspModel.origin.x;
                    currModel->absmin.y = (currModel->absmin.y * bspModel.scale) + bspModel.origin.y;
                    currModel->absmin.z = (currModel->absmin.z * bspModel.scale) + bspModel.origin.z;
                    currModel->absmax.x = (currModel->absmax.x * bspModel.scale) + bspModel.origin.x;
                    currModel->absmax.y = (currModel->absmax.y * bspModel.scale) + bspModel.origin.y;
                    currModel->absmax.z = (currModel->absmax.z * bspModel.scale) + bspModel.origin.z;

                    if (currModel->xmodel->numCollmaps == 0)
                        con::warn("Xmodel \"{}\" has no colision data", bspModel.name);
                }
                else
                {
                    if (bspModel.areBoundsValid)
                    {
                        currModel->absmin = bspModel.mins;
                        currModel->absmax = bspModel.maxs;
                    }
                    else
                    {
                        con::warn("Unable to determine the bounds of xmodel: \"{}\"", bspModel.name);
                        currModel->absmin.x = bspModel.origin.x - 1.0f;
                        currModel->absmin.y = bspModel.origin.y - 1.0f;
                        currModel->absmin.z = bspModel.origin.z - 1.0f;
                        currModel->absmax.x = bspModel.origin.x + 1.0f;
                        currModel->absmax.y = bspModel.origin.y + 1.0f;
                        currModel->absmax.z = bspModel.origin.z + 1.0f;
                    }
                }

                BSPUtil::matrixTranspose3x3(xmodelAxis, currModel->invScaledAxis);
                currModel->invScaledAxis[0].x = (1.0f / bspModel.scale) * currModel->invScaledAxis[0].x;
                currModel->invScaledAxis[0].y = (1.0f / bspModel.scale) * currModel->invScaledAxis[0].y;
                currModel->invScaledAxis[0].z = (1.0f / bspModel.scale) * currModel->invScaledAxis[0].z;
                currModel->invScaledAxis[1].x = (1.0f / bspModel.scale) * currModel->invScaledAxis[1].x;
                currModel->invScaledAxis[1].y = (1.0f / bspModel.scale) * currModel->invScaledAxis[1].y;
                currModel->invScaledAxis[1].z = (1.0f / bspModel.scale) * currModel->invScaledAxis[1].z;
                currModel->invScaledAxis[2].x = (1.0f / bspModel.scale) * currModel->invScaledAxis[2].x;
                currModel->invScaledAxis[2].y = (1.0f / bspModel.scale) * currModel->invScaledAxis[2].y;
                currModel->invScaledAxis[2].z = (1.0f / bspModel.scale) * currModel->invScaledAxis[2].z;

                memset(&currModel->writable, 0, sizeof(cStaticModelWritable));
            }

            return true;
        }

        void addAABBTreeFromTerrain(BSPData* bsp,
                                    CollisionOutput& output,
                                    CollisionData& data,
                                    std::vector<size_t>& terrainPartitions,
                                    size_t* out_parentCount,
                                    size_t* out_parentStartIndex,
                                    vec3_t* out_mins,
                                    vec3_t* out_maxs,
                                    int* out_treeContents)
        {
            assert(terrainPartitions.size() > 0);
            std::map<size_t, std::vector<size_t>> uniqueMaterials;
            for (size_t partIdx = 0; partIdx < terrainPartitions.size(); partIdx++)
            {
                size_t parentSurfIndex = data.partitionVec.at(terrainPartitions.at(partIdx)).parentSurfaceIndex;
                size_t materialIndex = data.surfaceVec.at(parentSurfIndex).materialIndex;
                if (!uniqueMaterials.contains(materialIndex))
                    uniqueMaterials[materialIndex] = std::vector<size_t>();
                uniqueMaterials.at(materialIndex).emplace_back(partIdx);
            }

            size_t totalParentCount = 0;
            *out_treeContents = 0;
            for (auto& matData : uniqueMaterials)
            {
                *out_treeContents |= bsp->colWorld.materials.at(matData.first).contentFlags;

                size_t objCount = matData.second.size();
                size_t result = objCount / MAX_AABB_TREE_CHILDREN;
                size_t remainder = objCount % MAX_AABB_TREE_CHILDREN;
                if (remainder > 0)
                    result++;
                totalParentCount += result;
            }

            // every parent node needs to be contiguous in memory
            size_t parentAABBArrayIndex = output.AABBTreeVec.size();
            output.AABBTreeVec.resize(output.AABBTreeVec.size() + totalParentCount);
            *out_parentCount = totalParentCount;
            *out_parentStartIndex = parentAABBArrayIndex;

            for (auto& matData : uniqueMaterials)
            {
                size_t partitionCount = matData.second.size();
                size_t parentCount = partitionCount / MAX_AABB_TREE_CHILDREN;
                size_t remainder = partitionCount % MAX_AABB_TREE_CHILDREN;
                if (remainder > 0)
                    parentCount++;

                size_t unaddedObjectCount = partitionCount;
                size_t addedObjectCount = 0;
                for (size_t parentIdx = 0; parentIdx < parentCount; parentIdx++)
                {
                    size_t currChildObjectCount = MAX_AABB_TREE_CHILDREN;
                    if (unaddedObjectCount <= MAX_AABB_TREE_CHILDREN)
                        currChildObjectCount = unaddedObjectCount;
                    else
                        unaddedObjectCount -= MAX_AABB_TREE_CHILDREN;

                    vec3_t parentMins;
                    vec3_t parentMaxs;
                    size_t childObjectStartIndex = output.AABBTreeVec.size();
                    for (size_t objectIdx = 0; objectIdx < currChildObjectCount; objectIdx++)
                    {
                        // create a child AABBTree with the partition and add it to AABBTreeVec
                        size_t partitionIndex = matData.second.at(addedObjectCount + objectIdx);

                        ColPartition& partition = data.partitionVec.at(partitionIndex);
                        vec3_t partitionMins = partition.mins;
                        vec3_t partitionMaxs = partition.maxs;
                        // update the parent AABB with the child AABB
                        if (objectIdx == 0)
                        {
                            parentMins = partitionMins;
                            parentMaxs = partitionMaxs;
                        }
                        else
                            BSPUtil::updateAABB(partitionMins, partitionMaxs, parentMins, parentMaxs);

                        CollisionAabbTree childAABBTree;
                        childAABBTree.materialIndex = static_cast<uint16_t>(matData.first);
                        childAABBTree.childCount = 0;
                        childAABBTree.u.partitionIndex = static_cast<int>(partitionIndex);
                        childAABBTree.origin = BSPUtil::calcMiddleOfAABB(partitionMins, partitionMaxs);
                        childAABBTree.halfSize = BSPUtil::calcHalfSizeOfAABB(partitionMins, partitionMaxs);
                        output.AABBTreeVec.emplace_back(childAABBTree);
                    }

                    CollisionAabbTree parentAABB;
                    parentAABB.materialIndex = static_cast<uint16_t>(matData.first);
                    parentAABB.origin = BSPUtil::calcMiddleOfAABB(parentMins, parentMaxs);
                    parentAABB.halfSize = BSPUtil::calcHalfSizeOfAABB(parentMins, parentMaxs);
                    parentAABB.childCount = static_cast<uint16_t>(currChildObjectCount);
                    parentAABB.u.firstChildIndex = static_cast<int>(childObjectStartIndex);
                    output.AABBTreeVec.at(parentAABBArrayIndex + parentIdx) = parentAABB;

                    addedObjectCount += currChildObjectCount;
                }

                parentAABBArrayIndex += parentCount;
            }
        }

        size_t addBrushNodeFromBrushes(BSPData* bsp,
                                       CollisionOutput& output,
                                       CollisionData& data,
                                       std::vector<size_t>& brushes,
                                       vec3_t* out_mins,
                                       vec3_t* out_maxs,
                                       int* out_brushContents)
        {
            vec3_t totalMins{};
            vec3_t totalMaxs{};
            int totalBrushContents = 0;
            for (size_t brushIdx = 0; brushIdx < brushes.size(); brushIdx++)
            {
                ColBrush& colBrush = data.brushVec.at(brushes.at(brushIdx));
                vec3_t brushMins = colBrush.mins;
                vec3_t brushMaxs = colBrush.maxs;
                if (brushIdx == 0)
                {
                    totalMins = brushMins;
                    totalMaxs = brushMaxs;
                }
                else
                    BSPUtil::updateAABB(brushMins, brushMaxs, totalMins, totalMaxs);

                totalBrushContents |= bsp->colWorld.materials.at(colBrush.materialIndex).contentFlags;
            }

            *out_brushContents = totalBrushContents;
            *out_mins = totalMins;
            *out_maxs = totalMaxs;

            if (brushes.size() > INT16_MAX)
                con::error("ERROR: BRUSH SIZE EXCEEDS INT16_MAX - NOT IMPLEMENTED YET ");

            cLeafBrushNode_s brushNode{};
            brushNode.axis = 0;
            brushNode.contents = totalBrushContents;
            brushNode.leafBrushCount = static_cast<int16_t>(brushes.size());
            brushNode.data.leaf.brushes = m_memory.Alloc<LeafBrush>(brushes.size());
            for (size_t brushIdx = 0; brushIdx < brushes.size(); brushIdx++)
                brushNode.data.leaf.brushes[brushIdx] = static_cast<LeafBrush>(brushes.at(brushIdx));
            size_t brushNodeIdx = output.brushNodeVec.size();
            output.brushNodeVec.emplace_back(brushNode);
            return brushNodeIdx;
        }

        size_t leafCt = 0;
        size_t nodeCt = 0;
        size_t emptyCt = 0;
        size_t largestObjectCountLeaf = 0;
        size_t largestBrushCountLeaf = 0;
        size_t largestTriCountLeaf = 0;
        size_t TOTALBRUSHES = 0;

        // returns the index of the node/leaf parsed by the function
        // Nodes are indexed by their index in the node array
        // Leafs are indexed by (-1 - <leaf index>)
        // See https://developer.valvesoftware.com/wiki/BSP_(Source)
        int16_t loadBSPNode(BSPData* bsp, BSPTree* tree, bool isRoot, CollisionOutput& output, CollisionData& data)
        {
            if (tree->isLeaf)
            {
                if (isRoot)
                {
                    cNode_t node{};
                    node.plane = m_memory.Alloc<cplane_s>();
                    node.plane->normal = normalX;
                    uint16_t leafIndex = static_cast<uint16_t>(output.leafVec.size());
                    node.children[0] = -1 - leafIndex; // index next leaf
                    node.children[1] = -1 - leafIndex; // index next leaf
                    output.nodeVec.emplace_back(node);
                }

                if (tree->leaf->getObjectCount() > 0)
                {
                    cLeaf_s leaf{};

                    if (tree->leaf->getObjectCount() > largestObjectCountLeaf)
                        largestObjectCountLeaf = tree->leaf->getObjectCount();

                    leafCt++;
                    std::vector<size_t> brushes;
                    std::vector<size_t> triangles;
                    for (size_t objIdx = 0; objIdx < tree->leaf->getObjectCount(); objIdx++)
                    {
                        BSPObject* object = tree->leaf->getObject(objIdx);

                        if (object->isBrush)
                            brushes.emplace_back(object->objIndex);
                        else
                            triangles.emplace_back(object->objIndex);
                    }

                    if (brushes.size() > largestBrushCountLeaf)
                        largestBrushCountLeaf = brushes.size();

                    if (triangles.size() > largestTriCountLeaf)
                        largestTriCountLeaf = triangles.size();

                    TOTALBRUSHES += brushes.size();

                    if (!triangles.empty())
                    {
                        size_t parentCount = 0;
                        size_t parentStartIndex = 0;
                        addAABBTreeFromTerrain(bsp, output, data, triangles, &parentCount, &parentStartIndex, nullptr, nullptr, &leaf.terrainContents);
                        leaf.collAabbCount = static_cast<uint16_t>(parentCount);
                        leaf.firstCollAabbIndex = static_cast<uint16_t>(parentStartIndex);
                    }

                    if (!brushes.empty())
                        leaf.leafBrushNode = static_cast<int>(addBrushNodeFromBrushes(bsp, output, data, brushes, &leaf.mins, &leaf.maxs, &leaf.brushContents));

                    uint16_t leafIndex = static_cast<uint16_t>(output.leafVec.size());
                    output.leafVec.emplace_back(leaf);
                    return -1 - leafIndex;
                }
                else
                {
                    emptyCt++;
                    return -1 - 0; // first leaf is the empty leaf index
                }
            }
            else
            {
                nodeCt++;
                cNode_t node;
                node.plane = m_memory.Alloc<cplane_s>();
                node.plane->dist = tree->node->distance;
                if (tree->node->axis == AXIS_X)
                {
                    node.plane->normal = normalX;
                    node.plane->type = 0;
                }
                else if (tree->node->axis == AXIS_Y)
                {
                    node.plane->normal = normalY;
                    node.plane->type = 1;
                }
                else // tree->node->axis == AXIS_Z
                {
                    node.plane->normal = normalZ;
                    node.plane->type = 2;
                }
                node.plane->signbits = 0;
                if (node.plane->normal.x < 0.0f)
                    node.plane->signbits |= 1;
                if (node.plane->normal.y < 0.0f)
                    node.plane->signbits |= 2;
                if (node.plane->normal.z < 0.0f)
                    node.plane->signbits |= 4;
                size_t nodeIndex = output.nodeVec.size();
                output.nodeVec.emplace_back();
                node.children[0] = loadBSPNode(bsp, tree->node->front.get(), false, output, data);
                node.children[1] = loadBSPNode(bsp, tree->node->back.get(), false, output, data);
                output.nodeVec.at(nodeIndex) = node;

                return static_cast<uint16_t>(nodeIndex);
            }
        }

        void loadModelCollision(BSPData* bsp, CollisionData& data, CollisionOutput& output)
        {
            cmodel_t worldModel{};
            vec3_t worldMins = data.surfaceVerts.at(0);
            vec3_t worldMaxs = data.surfaceVerts.at(0);
            for (vec3_t& vertex : data.surfaceVerts)
                BSPUtil::updateAABBWithPoint(vertex, worldMins, worldMaxs);
            for (vec3_t& vertex : data.brushVerts)
                BSPUtil::updateAABBWithPoint(vertex, worldMins, worldMaxs);
            worldModel.mins = worldMins;
            worldModel.maxs = worldMaxs;
            worldModel.radius = BSPUtil::distBetweenPoints(worldMins, worldMaxs) / 2;
            output.modelVec.emplace_back(worldModel);

            for (const auto& model : data.models)
            {
                cmodel_t outputModel{};
                if (model.type == MST_NONE)
                {
                    output.modelVec.emplace_back(outputModel);
                    continue;
                }
                vec3_t modelMins;
                vec3_t modelMaxs;
                if (model.type == MST_TERRAIN || model.type == MST_BOTH)
                {
                    size_t parentCount = 0;
                    size_t parentStartIndex = 0;
                    std::vector<size_t> partitions;
                    for (size_t surfIdx = 0; surfIdx < model.colSurfaceCount; surfIdx++)
                    {
                        ColSurface& surf = data.surfaceVec.at(model.colSurfaceIndex + surfIdx);
                        for (size_t partIdx = 0; partIdx < surf.partitionCount; partIdx++)
                            partitions.emplace_back(surf.partitionIndex + partIdx);
                    }
                    addAABBTreeFromTerrain(
                        bsp, output, data, partitions, &parentCount, &parentStartIndex, &modelMins, &modelMaxs, &outputModel.leaf.terrainContents);
                    outputModel.leaf.collAabbCount = static_cast<uint16_t>(parentCount);
                    outputModel.leaf.firstCollAabbIndex = static_cast<uint16_t>(parentStartIndex);
                }
                if (model.type == MST_BRUSH || model.type == MST_BOTH)
                {
                    std::vector<size_t> brushes;
                    for (size_t brushIdx = 0; brushIdx < model.colBrushCount; brushIdx++)
                        brushes.emplace_back(model.colBrushIndex + brushIdx);
                    outputModel.leaf.leafBrushNode = static_cast<int>(
                        addBrushNodeFromBrushes(bsp, output, data, brushes, &outputModel.leaf.mins, &outputModel.leaf.maxs, &outputModel.leaf.brushContents));
                }

                if (model.type == MST_BRUSH)
                {
                    modelMins = outputModel.leaf.mins;
                    modelMaxs = outputModel.leaf.maxs;
                }
                else if (model.type == MST_BOTH)
                    BSPUtil::updateAABB(outputModel.leaf.mins, outputModel.leaf.maxs, modelMins, modelMaxs);
                outputModel.radius = BSPUtil::distBetweenPoints(modelMins, modelMaxs) / 2;
                output.modelVec.emplace_back(outputModel);
            }
        }

        void generateColBrushFromBsp(CollisionData& data, BSPData* bsp, BSPSurface& in_surface)
        {
            ColBrush brush{};
            brush.materialIndex = in_surface.materialIndex;
            brush.brushVertStartIndex = data.brushVerts.size();
            brush.brushVertCount = in_surface.vertexCount;

            vec3_t mins{};
            vec3_t maxs{};
            for (size_t vertIdx = 0; vertIdx < in_surface.vertexCount; vertIdx++)
            {
                vec3_t& vertex = bsp->colWorld.vertices.at(in_surface.indexOfFirstVertex + vertIdx).pos;
                if (vertIdx == 0)
                {
                    mins = vertex;
                    maxs = vertex;
                }
                else
                    BSPUtil::updateAABBWithPoint(vertex, mins, maxs);
                data.brushVerts.emplace_back(vertex);
            }
            brush.mins = mins;
            brush.maxs = maxs;
            data.brushVec.emplace_back(brush);
        }

        bool areVerticesEqual(BSPData* bsp, size_t vertIdx1, size_t vertIdx2)
        {
            const vec3_t& vert1 = bsp->colWorld.vertices.at(vertIdx1).pos;
            const vec3_t& vert2 = bsp->colWorld.vertices.at(vertIdx2).pos;
            return (vert1.x == vert2.x && vert1.y == vert2.y && vert1.z == vert2.z);
        }

        bool isTriConnectedToTri(BSPData* bsp, BSPSurface& in_surface, size_t triIdx1, size_t triIdx2)
        {
            size_t t1_index0 = bsp->colWorld.indices.at(in_surface.indexOfFirstIndex + (triIdx1 * 3));
            size_t t1_index1 = bsp->colWorld.indices.at(in_surface.indexOfFirstIndex + (triIdx1 * 3) + 1);
            size_t t1_index2 = bsp->colWorld.indices.at(in_surface.indexOfFirstIndex + (triIdx1 * 3) + 2);
            size_t t2_index0 = bsp->colWorld.indices.at(in_surface.indexOfFirstIndex + (triIdx2 * 3));
            size_t t2_index1 = bsp->colWorld.indices.at(in_surface.indexOfFirstIndex + (triIdx2 * 3) + 1);
            size_t t2_index2 = bsp->colWorld.indices.at(in_surface.indexOfFirstIndex + (triIdx2 * 3) + 2);

            size_t matchCount = 0;
            if (areVerticesEqual(bsp, t1_index0, t2_index0) || areVerticesEqual(bsp, t1_index0, t2_index1) || areVerticesEqual(bsp, t1_index0, t2_index2))
                matchCount++;
            if (areVerticesEqual(bsp, t1_index1, t2_index0) || areVerticesEqual(bsp, t1_index1, t2_index1) || areVerticesEqual(bsp, t1_index1, t2_index2))
                matchCount++;
            if (areVerticesEqual(bsp, t1_index2, t2_index0) || areVerticesEqual(bsp, t1_index2, t2_index1) || areVerticesEqual(bsp, t1_index2, t2_index2))
                matchCount++;

            if (matchCount < 2)
                return false;
            else if (matchCount == 2)
                return true;
            else
            {
                con::warn("Warning: two comparsion tris had every index match.");
                return true;
            }
        }

        size_t generateColSurfaceFromBsp(CollisionData& data,
                                         BSPData* bsp,
                                         BSPSurface& in_surface,
                                         std::vector<size_t>& out_terrainIndexBuffer,
                                         std::vector<vec3_t>& out_terrainVertexBuffer)
        {
            size_t count = in_surface.triCount;
            size_t maxElems = MAX_PARTITION_TRIS;

            std::vector<std::vector<size_t>> generatedPartitions; // list of tri indexes of the surface (0 indexed in surf tris)
            std::unique_ptr<bool[]> visitedTris = std::make_unique<bool[]>(in_surface.triCount);
            for (size_t triIdx1 = 0; triIdx1 < in_surface.triCount; triIdx1++)
            {
                if (visitedTris[triIdx1])
                    continue;
                else
                    visitedTris[triIdx1] = true;

                bool foundConnection = false;
                for (auto& genPart : generatedPartitions)
                {
                    if (genPart.size() == MAX_PARTITION_TRIS)
                        continue;

                    for (const auto& triIdx2 : genPart)
                    {
                        if (isTriConnectedToTri(bsp, in_surface, triIdx1, triIdx2))
                        {
                            genPart.emplace_back(triIdx1);
                            foundConnection = true;
                            break;
                        }
                    }
                    if (foundConnection)
                        break;
                }
                if (foundConnection)
                    continue;

                foundConnection = false;
                for (size_t triIdx2 = 0; triIdx2 < in_surface.triCount; triIdx2++)
                {
                    if (visitedTris[triIdx2])
                        continue;

                    if (isTriConnectedToTri(bsp, in_surface, triIdx1, triIdx2))
                    {
                        std::vector<size_t> partitiontris = {triIdx1, triIdx2};
                        visitedTris[triIdx2] = true;
                        generatedPartitions.emplace_back(partitiontris);
                        foundConnection = true;
                        break;
                    }
                }
                if (!foundConnection)
                {
                    std::vector<size_t> partitiontris = {triIdx1};
                    generatedPartitions.emplace_back(partitiontris);
                }
            }

            ColSurface surf{};
            surf.materialIndex = in_surface.materialIndex;
            surf.partitionCount = generatedPartitions.size();
            surf.partitionIndex = data.partitionVec.size();
            size_t surfaceIndex = data.surfaceVec.size();
            data.surfaceVec.emplace_back(surf);

            for (const auto& generatedPartition : generatedPartitions)
            {
                size_t triCount = generatedPartition.size();
                ColPartition partition{};
                partition.parentSurfaceIndex = surfaceIndex;
                partition.indexStartIndex = out_terrainIndexBuffer.size();
                partition.triCount = triCount;
                // mins/maxs initialised later
                data.partitionVec.emplace_back(partition);

                for (size_t triIdx : generatedPartition)
                {
                    size_t index0 = bsp->colWorld.indices.at(in_surface.indexOfFirstIndex + (triIdx * 3));
                    size_t index1 = bsp->colWorld.indices.at(in_surface.indexOfFirstIndex + (triIdx * 3) + 1);
                    size_t index2 = bsp->colWorld.indices.at(in_surface.indexOfFirstIndex + (triIdx * 3) + 2);
                    out_terrainIndexBuffer.emplace_back(out_terrainVertexBuffer.size() + index0); // indices cover the entire vert buffer
                    out_terrainIndexBuffer.emplace_back(out_terrainVertexBuffer.size() + index1); // indices cover the entire vert buffer
                    out_terrainIndexBuffer.emplace_back(out_terrainVertexBuffer.size() + index2); // indices cover the entire vert buffer
                }
            }

            for (size_t vertIdx = 0; vertIdx < in_surface.vertexCount; vertIdx++)
                out_terrainVertexBuffer.emplace_back(bsp->colWorld.vertices.at(in_surface.indexOfFirstVertex + vertIdx).pos);

            return surfaceIndex;
        }

        bool loadCollisionData(CollisionData& data, BSPData* bsp)
        {
            std::vector<size_t> tempTerrainIndexBuffer;
            std::vector<vec3_t> tempTerrainVertexBuffer;
            assert(data.surfaceVec.size() == 0);
            for (size_t surfIdx = 0; surfIdx < bsp->staticTerrainSurfaceCount; surfIdx++)
            {
                BSPSurface& surface = bsp->colWorld.surfaces.at(bsp->staticTerrainSurfaceStart + surfIdx);
                generateColSurfaceFromBsp(data, bsp, surface, tempTerrainIndexBuffer, tempTerrainVertexBuffer);
            }
            data.staticSurfaceCount = data.surfaceVec.size();

            assert(data.brushVec.size() == 0);
            for (size_t surfIdx = 0; surfIdx < bsp->staticBrushSurfaceCount; surfIdx++)
            {
                BSPSurface surface = bsp->colWorld.surfaces.at(bsp->staticBrushSurfaceStart + surfIdx);
                generateColBrushFromBsp(data, bsp, surface);
            }
            data.staticBrushCount = data.brushVec.size();

            for (const auto& model : bsp->models)
            {
                ColModel colModel{};

                if (model.surfaceSide != MSS_COL && model.surfaceSide != MSS_BOTH)
                {
                    data.models.emplace_back(colModel);
                    continue;
                }

                colModel.type = model.surfaceType;
                if (colModel.type == MST_TERRAIN || colModel.type == MST_BOTH)
                {
                    colModel.colSurfaceIndex = data.surfaceVec.size();
                    for (size_t surfIdx = 0; surfIdx < model.colTerrainSurfaceCount; surfIdx++)
                    {
                        BSPSurface surface = bsp->colWorld.surfaces.at(model.colTerrainSurfaceIndex + surfIdx);
                        generateColSurfaceFromBsp(data, bsp, surface, tempTerrainIndexBuffer, tempTerrainVertexBuffer);
                    }
                    colModel.colSurfaceCount = data.surfaceVec.size() - colModel.colSurfaceIndex;
                }

                if (colModel.type == MST_BRUSH || colModel.type == MST_BOTH)
                {
                    colModel.colBrushIndex = data.brushVec.size();
                    colModel.colBrushCount = model.colBrushSurfaceCount;
                    for (size_t surfIdx = 0; surfIdx < model.colBrushSurfaceCount; surfIdx++)
                    {
                        BSPSurface surface = bsp->colWorld.surfaces.at(model.colBrushSurfaceIndex + surfIdx);
                        generateColBrushFromBsp(data, bsp, surface);
                    }
                }

                data.models.emplace_back(colModel);
            }

            std::unique_ptr<size_t[]> resizedVertexMap = std::make_unique<size_t[]>(tempTerrainVertexBuffer.size());
            for (size_t i = 0; i < tempTerrainVertexBuffer.size(); i++)
            {
                bool found = false;
                size_t foundIdx = 0;
                const auto& testVertex = tempTerrainVertexBuffer.at(i);
                for (size_t j = 0; j < data.surfaceVerts.size(); j++)
                {
                    const auto& inVertex = data.surfaceVerts.at(j);
                    if (inVertex.x == testVertex.x && inVertex.y == testVertex.y && inVertex.z == testVertex.z)
                    {
                        found = true;
                        foundIdx = j;
                        break;
                    }
                }
                if (!found)
                {
                    if (data.surfaceVerts.size() == UINT16_MAX)
                    {
                        con::error("assert data.surfaceVerts.size() == UINT16_MAX");
                        return false;
                    }
                    resizedVertexMap[i] = data.surfaceVerts.size();
                    data.surfaceVerts.emplace_back(testVertex);
                }
                else
                    resizedVertexMap[i] = foundIdx;
            }

            for (size_t idx : tempTerrainIndexBuffer)
            {
                if (resizedVertexMap[idx] > UINT16_MAX)
                {
                    con::error("assert(resizedVertexMap[idx] > UINT16_MAX);");
                    return false;
                }
                data.surfaceIndices.emplace_back(static_cast<uint16_t>(resizedVertexMap[idx]));
            }

            return true;
        }

        std::unique_ptr<BSPTree> createBSPTree(CollisionData& data)
        {
            vec3_t worldMins = data.surfaceVerts.at(0);
            vec3_t worldMaxs = data.surfaceVerts.at(0);
            for (vec3_t& vertex : data.surfaceVerts)
                BSPUtil::updateAABBWithPoint(vertex, worldMins, worldMaxs);
            for (vec3_t& vertex : data.brushVerts)
                BSPUtil::updateAABBWithPoint(vertex, worldMins, worldMaxs);

            std::unique_ptr<BSPTree> tree = std::make_unique<BSPTree>(worldMins.x, worldMins.y, worldMins.z, worldMaxs.x, worldMaxs.y, worldMaxs.z, 0);

            for (size_t surfIdx = 0; surfIdx < data.staticSurfaceCount; surfIdx++)
            {
                ColSurface& surf = data.surfaceVec.at(surfIdx);
                for (size_t partIdx = 0; partIdx < surf.partitionCount; partIdx++)
                {
                    ColPartition& partition = data.partitionVec.at(surf.partitionIndex + partIdx);
                    vec3_t partMins = partition.mins;
                    vec3_t partMaxs = partition.maxs;
                    std::shared_ptr<BSPObject> object = std::make_shared<BSPObject>(
                        partMins.x, partMins.y, partMins.z, partMaxs.x, partMaxs.y, partMaxs.z, false, surf.partitionIndex + partIdx);
                    tree->addObjectToTree(std::move(object));
                }
            }

            for (size_t brushIdx = 0; brushIdx < data.staticBrushCount; brushIdx++)
            {
                ColBrush& brush = data.brushVec.at(brushIdx);
                vec3_t brushMins = brush.mins;
                vec3_t brushMaxs = brush.maxs;
                std::shared_ptr<BSPObject> object =
                    std::make_shared<BSPObject>(brushMins.x, brushMins.y, brushMins.z, brushMaxs.x, brushMaxs.y, brushMaxs.z, true, brushIdx);
                tree->addObjectToTree(std::move(object));
            }

            tree->optimiseTree();

            return tree;
        }

        void calcColDataBounds(CollisionData& data)
        {
            for (ColPartition& partition : data.partitionVec)
            {
                assert(partition.triCount != 0);
                for (size_t indexIdx = 0; indexIdx < partition.triCount * 3; indexIdx++)
                {
                    uint16_t index = data.surfaceIndices.at(partition.indexStartIndex + indexIdx);
                    vec3_t& vert = data.surfaceVerts.at(index);
                    if (indexIdx == 0)
                    {
                        partition.mins = vert;
                        partition.maxs = vert;
                    }
                    else
                        BSPUtil::updateAABBWithPoint(vert, partition.mins, partition.maxs);
                }
            }
        }

        void loadPartitions(BSPData* bsp, CollisionData& data, CollisionOutput& output)
        {
            for (const auto& in_partition : data.partitionVec)
            {
                assert(in_partition.triCount > 0 && in_partition.triCount <= MAX_PARTITION_TRIS);
                assert(in_partition.indexStartIndex % 3 == 0);

                CollisionPartition partition{};
                partition.triCount = static_cast<char>(in_partition.triCount);
                partition.firstTri = static_cast<int>(in_partition.indexStartIndex / 3);
                uint16_t uniqueIndices[MAX_PARTITION_TRIS * 3];
                size_t uniqueIndexSize = 0;
                for (size_t triIdx = 0; triIdx < in_partition.triCount; triIdx++)
                {
                    uint16_t idx0 = data.surfaceIndices.at(in_partition.indexStartIndex + (triIdx * 3) + 0);
                    uint16_t idx1 = data.surfaceIndices.at(in_partition.indexStartIndex + (triIdx * 3) + 1);
                    uint16_t idx2 = data.surfaceIndices.at(in_partition.indexStartIndex + (triIdx * 3) + 2);

                    bool isUnique = false;
                    for (size_t i = 0; i < uniqueIndexSize; i++)
                    {
                        if (uniqueIndices[i] == idx0)
                        {
                            isUnique = true;
                            break;
                        }
                    }
                    if (isUnique)
                        uniqueIndices[uniqueIndexSize++] = idx0;

                    isUnique = false;
                    for (size_t i = 0; i < uniqueIndexSize; i++)
                    {
                        if (uniqueIndices[i] == idx1)
                        {
                            isUnique = true;
                            break;
                        }
                    }
                    if (isUnique)
                        uniqueIndices[uniqueIndexSize++] = idx1;

                    isUnique = false;
                    for (size_t i = 0; i < uniqueIndexSize; i++)
                    {
                        if (uniqueIndices[i] == idx2)
                        {
                            isUnique = true;
                            break;
                        }
                    }
                    if (isUnique)
                        uniqueIndices[uniqueIndexSize++] = idx2;

                    output.uniqueVertIndexVec.emplace_back(idx0);
                    output.uniqueVertIndexVec.emplace_back(idx1);
                    output.uniqueVertIndexVec.emplace_back(idx2);
                }
                partition.nuinds = static_cast<int>(uniqueIndexSize);
                partition.fuind = static_cast<int>(output.uniqueVertIndexVec.size());
                for (size_t i = 0; i < uniqueIndexSize; i++)
                    output.uniqueVertIndexVec.emplace_back(uniqueIndices[i]);

                output.partitionVec.emplace_back(partition);
            }
        }

        bool loadWorldCollision(clipMap_t* clipMap, BSPData* bsp)
        {
            CollisionData data{};
            if (!loadCollisionData(data, bsp))
                return false;
            calcColDataBounds(data);

            std::unique_ptr<BSPTree> tree = createBSPTree(data);
            if (tree == nullptr)
                return false;

            CollisionOutput output{};
            cLeafBrushNode_s tempNode{};
            output.brushNodeVec.emplace_back(tempNode); // first brush node is always empty

            cLeaf_s leaf{};
            output.leafVec.emplace_back(leaf); // first leaf is always empty

            con::info("AABBTreeVec 1 count: {}", output.AABBTreeVec.size());

            loadPartitions(bsp, data, output);

            con::info("AABBTreeVec 2  count: {}", output.AABBTreeVec.size());

            loadBSPNode(bsp, tree.get(), true, output, data);
            // loadBSPNode adds 291980 AABB trees for 39290 partitions. largest leaf object count has 20618 objects in a single leaf
            // ????????????????????????????????
            // crash related too brushes being more than 127 and crashing. maybe nodes can have max 127 brushes?
            // a lot of brushnodes have exactly 19671 or 1793 brushes
            con::info("AABBTreeVec 3 count: {}", output.AABBTreeVec.size());

            loadModelCollision(bsp, data, output);

            con::info("AABBTreeVec 4 count: {}", output.AABBTreeVec.size());

            con::info("data.staticSurfaceCount count: {}", data.staticSurfaceCount);
            con::info("data.surfaceVec count: {}", data.surfaceVec.size());
            con::info("data.partitionVec count: {}", data.partitionVec.size());
            con::info("data.surfaceIndices count: {}", data.surfaceIndices.size());
            con::info("data.surfaceVerts count: {}", data.surfaceVerts.size());

            con::info("data.staticBrushCount count: {}", data.staticBrushCount);
            con::info("data.brushVec count: {}", data.brushVec.size());
            con::info("data.brushVerts count: {}", data.brushVerts.size());
            con::info("data.models count: {}", data.models.size());

            con::info("output.modelVec count: {}", output.modelVec.size());
            con::info("output.nodeVec count: {}", output.nodeVec.size());
            con::info("output.leafVec count: {}", output.leafVec.size());
            con::info("output.AABBTreeVec count: {}", output.AABBTreeVec.size());
            con::info("output.brushNodeVec count: {}", output.brushNodeVec.size());
            con::info("output.partitionVec count: {}", output.partitionVec.size());
            con::info("output.uniqueVertIndexVec count: {}", output.uniqueVertIndexVec.size());

            con::info("output leaf count: {}", leafCt);
            con::info("output node count: {}", nodeCt);
            con::info("empty unadded leafs count: {}", emptyCt);
            con::info("largestObjectCountLeaf: {}", largestObjectCountLeaf);
            con::info("largestBrushCountLeaf: {}", largestBrushCountLeaf);
            con::info("largestTriCountLeaf: {}", largestTriCountLeaf);
            con::info("TOTALBRUSHES: {}", TOTALBRUSHES);

            if (output.modelVec.size() > 0x3FFF)
            {
                con::error("ERROR: There are more than 0x3FFF entity brush models.");
                return false;
            }
            if (output.nodeVec.size() > 0xFFFF)
            {
                con::error("exceeded 0xFFFF nodes in clipmap");
                return false;
            }
            if (output.leafVec.size() > 0xFFFF)
            {
                con::error("exceeded 0xFFFF leafs in clipmap");
                return false;
            }
            if (output.AABBTreeVec.size() > 0xFFFF)
            {
                con::error("exceeded 0xFFFF AABBTrees in clipmap");
                return false;
            }
            if (data.brushVec.size() > 0xFFFF)
            {
                con::error("exceeded 0xFFFF brushes in clipmap");
                return false;
            }

            // unused brush data
            clipMap->info.planeCount = 0;
            clipMap->info.planes = nullptr;
            clipMap->info.numBrushSides = 0;
            clipMap->info.brushsides = nullptr;
            clipMap->info.numLeafBrushes = 0;
            clipMap->info.leafbrushes = nullptr;
            clipMap->info.brushBounds = nullptr;
            clipMap->info.brushContents = nullptr;

            clipMap->numNodes = static_cast<unsigned int>(output.nodeVec.size());
            clipMap->nodes = m_memory.Alloc<cNode_t>(output.nodeVec.size());
            memcpy(clipMap->nodes, output.nodeVec.data(), sizeof(cNode_t) * output.nodeVec.size());

            clipMap->numLeafs = static_cast<unsigned int>(output.leafVec.size());
            clipMap->leafs = m_memory.Alloc<cLeaf_s>(output.leafVec.size());
            memcpy(clipMap->leafs, output.leafVec.data(), sizeof(cLeaf_s) * output.leafVec.size());

            clipMap->aabbTreeCount = static_cast<int>(output.AABBTreeVec.size());
            clipMap->aabbTrees = m_memory.Alloc<CollisionAabbTree>(output.AABBTreeVec.size());
            memcpy(clipMap->aabbTrees, output.AABBTreeVec.data(), sizeof(CollisionAabbTree) * output.AABBTreeVec.size());

            clipMap->info.leafbrushNodesCount = static_cast<unsigned int>(output.brushNodeVec.size());
            clipMap->info.leafbrushNodes = m_memory.Alloc<cLeafBrushNode_s>(output.brushNodeVec.size());
            memcpy(clipMap->info.leafbrushNodes, output.brushNodeVec.data(), sizeof(cLeafBrushNode_s) * output.brushNodeVec.size());

            clipMap->info.numBrushVerts = static_cast<unsigned int>(data.brushVerts.size());
            clipMap->info.brushVerts = m_memory.Alloc<vec3_t>(data.brushVerts.size());
            memcpy(clipMap->info.brushVerts, data.brushVerts.data(), sizeof(vec3_t) * data.brushVerts.size());

            clipMap->info.numBrushes = static_cast<uint16_t>(data.brushVec.size());
            clipMap->info.brushes = m_memory.Alloc<cbrush_array_t>(data.brushVec.size());
            for (size_t brushIdx = 0; brushIdx < data.brushVec.size(); brushIdx++)
            {
                ColBrush& inBrush = data.brushVec.at(brushIdx);
                auto* outBrush = &clipMap->info.brushes[brushIdx];

                int brushSurfaceFlags = bsp->colWorld.materials.at(inBrush.materialIndex).surfaceFlags;
                int brushContentFlags = bsp->colWorld.materials.at(inBrush.materialIndex).contentFlags;

                outBrush->numverts = static_cast<unsigned int>(inBrush.brushVertCount);
                outBrush->verts = &clipMap->info.brushVerts[inBrush.brushVertStartIndex];
                outBrush->contents = brushContentFlags;
                outBrush->mins = inBrush.mins;
                outBrush->maxs = inBrush.maxs;
                outBrush->axial_cflags[0][0] = brushContentFlags;
                outBrush->axial_cflags[0][1] = brushContentFlags;
                outBrush->axial_cflags[0][2] = brushContentFlags;
                outBrush->axial_cflags[1][0] = brushContentFlags;
                outBrush->axial_cflags[1][1] = brushContentFlags;
                outBrush->axial_cflags[1][2] = brushContentFlags;
                outBrush->axial_sflags[0][0] = brushSurfaceFlags;
                outBrush->axial_sflags[0][1] = brushSurfaceFlags;
                outBrush->axial_sflags[0][2] = brushSurfaceFlags;
                outBrush->axial_sflags[1][0] = brushSurfaceFlags;
                outBrush->axial_sflags[1][1] = brushSurfaceFlags;
                outBrush->axial_sflags[1][2] = brushSurfaceFlags;
            }

            clipMap->numSubModels = static_cast<unsigned int>(output.modelVec.size());
            clipMap->cmodels = m_memory.Alloc<cmodel_t>(output.modelVec.size());
            memcpy(clipMap->cmodels, output.modelVec.data(), sizeof(cmodel_t) * output.modelVec.size());

            clipMap->partitionCount = static_cast<int>(output.partitionVec.size());
            clipMap->partitions = m_memory.Alloc<CollisionPartition>(output.partitionVec.size());
            memcpy(clipMap->partitions, output.partitionVec.data(), sizeof(CollisionPartition) * output.partitionVec.size());

            clipMap->info.nuinds = static_cast<int>(output.uniqueVertIndexVec.size());
            clipMap->info.uinds = m_memory.Alloc<uint16_t>(output.uniqueVertIndexVec.size());
            memcpy(clipMap->info.uinds, output.uniqueVertIndexVec.data(), sizeof(uint16_t) * output.uniqueVertIndexVec.size());

            clipMap->vertCount = static_cast<unsigned int>(data.surfaceVerts.size());
            clipMap->verts = m_memory.Alloc<vec3_t>(data.surfaceVerts.size());
            memcpy(clipMap->verts, data.surfaceVerts.data(), sizeof(vec3_t) * data.surfaceVerts.size());

            assert(data.surfaceIndices.size() % 3 == 0);
            clipMap->triCount = static_cast<unsigned int>(data.surfaceIndices.size() / 3);
            uint16_t* indices = m_memory.Alloc<uint16_t>(data.surfaceIndices.size());
            memcpy(indices, data.surfaceIndices.data(), sizeof(uint16_t) * data.surfaceIndices.size());
            clipMap->triIndices = reinterpret_cast<uint16_t (*)[3]>(indices);

            return true;
        }

        bool loadMaterials(clipMap_t* clipMap, BSPData* bsp)

        {
            // Clipmap materials define the properties of a material (bullet penetration, no collision, water, etc)

            if (bsp->colWorld.materials.size() > UINT16_MAX)
            {
                con::error("Collision map exceeds 0xFFFF materials");
                return false;
            }

            clipMap->info.numMaterials = static_cast<unsigned int>(bsp->colWorld.materials.size());
            clipMap->info.materials = m_memory.Alloc<ClipMaterial>(clipMap->info.numMaterials);
            for (size_t matIdx = 0; matIdx < bsp->colWorld.materials.size(); matIdx++)
            {
                ClipMaterial* clipMat = &clipMap->info.materials[matIdx];
                BSPMaterial bspMat = bsp->colWorld.materials.at(matIdx);

                clipMat->name = m_memory.Dup(bspMat.materialName.c_str());
                clipMat->contentFlags = bspMat.contentFlags;
                clipMat->surfaceFlags = bspMat.surfaceFlags;
            }

            return true;
        }

    public:
        explicit ClipMapLinkerImpl(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
            : m_memory(memory),
              m_search_path(searchPath),
              m_context(context)
        {
        }

        clipMap_t* linkClipMap(BSPData* bsp) override
        {
            clipMap_t* clipMap = m_memory.Alloc<clipMap_t>();
            clipMap->name = m_memory.Dup(bsp->bspName.c_str());

            clipMap->isInUse = true;
            clipMap->checksum = 0;
            clipMap->pInfo = nullptr;

            std::string mapEntsName = bsp->bspName;
            auto mapEntsAsset = m_context.LoadDependency<AssetMapEnts>(mapEntsName);
            assert(mapEntsAsset != nullptr);
            clipMap->mapEnts = mapEntsAsset->Asset();

            loadBoxData(clipMap);

            loadVisibility(clipMap);

            loadRopesAndConstraints(clipMap);

            loadDynEnts(clipMap);

            if (!loadXModelCollision(clipMap, bsp))
                return nullptr;

            if (!loadMaterials(clipMap, bsp))
                return nullptr;

            if (!loadWorldCollision(clipMap, bsp)) // requires materials
                return nullptr;

            // set all edges to walkable
            // might do weird stuff on walls, but from testing doesnt seem to do anything
            int walkableEdgeSize = (3 * clipMap->triCount + 31) / 32 * 4;
            clipMap->triEdgeIsWalkable = m_memory.Alloc<char>(walkableEdgeSize);
            memset(clipMap->triEdgeIsWalkable, 1, walkableEdgeSize * sizeof(char));

            return clipMap;
        }
    };

} // namespace

std::unique_ptr<ClipMapLinker> ClipMapLinker::Create(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)

{
    return std::make_unique<ClipMapLinkerImpl>(memory, searchPath, context);
}
