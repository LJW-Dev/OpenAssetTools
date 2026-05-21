#include "ClipMapLinker.h"

#include "BSP/BSPUtil.h"
#include "BSPCalculation.h"

#include <limits>
#include <map>

using namespace T6;
using namespace BSP;

namespace
{
    struct ColSurface
    {
        size_t materialIndex;
        size_t partitionCount;
        size_t partitionStartIndex;
    };

    struct ColTerrainTri
    {
        size_t materialIndex;
        size_t indexStartIndex;
    };

    struct ColBrush
    {
        size_t materialIndex;
        size_t brushVertCount;
        size_t brushVertStartIndex;
    };

    struct ColModel
    {
        bspModelSurfType type;
        size_t colBrushIndex;
        size_t colBrushCount;
        size_t colTerrainTriIndex;
        size_t colTerrainTriCount;
    };

    struct CollisionData
    {
        size_t staticTerrainTriCount; // static terrain always starts at 0
        std::vector<ColTerrainTri> terrainTriVec;
        std::vector<uint16_t> terrainIndices;
        std::vector<vec3_t> terrainVerts;

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
        std::vector<cbrush_array_t> brushVec;
        std::vector<CollisionAabbTree> AABBTreeVec;
        std::vector<cmodel_t> modelVec;
    };

    constexpr size_t MAX_AABB_TREE_CHILDREN = 128;

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
            assert(clipMap->box_model.leaf.mins.x == std::numeric_limits<float>::max());
            assert(clipMap->box_model.leaf.maxs.x == std::numeric_limits<float>::min());

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
                                    std::vector<size_t>& triangles,
                                    size_t* out_parentCount,
                                    size_t* out_parentStartIndex,
                                    vec3_t* out_mins,
                                    vec3_t* out_maxs,
                                    int* out_treeContents)
        {
            // partitions have the same index as the collisiondata triangles
            size_t partitionCount = triangles.size();
            assert(partitionCount > 0);

            std::map<size_t, std::vector<size_t>> uniqueMaterials;
            for (size_t partitionIdx = 0; partitionIdx < partitionCount; partitionIdx++)
            {
                size_t materialIndex = data.terrainTriVec.at(triangles.at(partitionIdx)).materialIndex;
                if (!uniqueMaterials.contains(materialIndex))
                    uniqueMaterials[materialIndex] = std::vector<size_t>();
                uniqueMaterials.at(materialIndex).emplace_back(partitionIdx);
            }

            // BO2 has a maximum limit of 128 children per AABB tree (essentially),
            // so this is fixed by adding multiple parent AABB trees that hold 128 children each
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
                size_t matPartCount = matData.second.size();
                size_t parentCount = matPartCount / MAX_AABB_TREE_CHILDREN;
                size_t remainder = matPartCount % MAX_AABB_TREE_CHILDREN;
                if (remainder > 0)
                    parentCount++;

                size_t unaddedObjectCount = matPartCount;
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
                        size_t triIndexStartIndex = data.terrainTriVec.at(partitionIndex).indexStartIndex;
                        vec3_t childMins;
                        vec3_t childMaxs;
                        for (size_t indexIdx = 0; indexIdx < 3; indexIdx++)
                        {
                            vec3_t& vert = data.terrainVerts.at(data.terrainIndices.at(triIndexStartIndex + indexIdx));
                            if (indexIdx == 0)
                            {
                                childMins = vert;
                                childMaxs = vert;
                            }
                            else
                                BSPUtil::updateAABBWithPoint(vert, childMins, childMaxs);
                        }

                        CollisionAabbTree childAABBTree;
                        childAABBTree.materialIndex = static_cast<uint16_t>(matData.first);
                        childAABBTree.childCount = 0;
                        childAABBTree.u.partitionIndex = static_cast<int>(partitionIndex);
                        childAABBTree.origin = BSPUtil::calcMiddleOfAABB(childMins, childMaxs);
                        childAABBTree.halfSize = BSPUtil::calcHalfSizeOfAABB(childMins, childMaxs);
                        output.AABBTreeVec.emplace_back(childAABBTree);

                        // update the parent AABB with the child AABB
                        if (objectIdx == 0)
                        {
                            parentMins = childMins;
                            parentMaxs = childMaxs;
                        }
                        else
                            BSPUtil::updateAABB(childMins, childMaxs, parentMins, parentMaxs);
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

                vec3_t brushMins{};
                vec3_t brushMaxs{};
                assert(colBrush.brushVertCount != 0);
                for (size_t vertIdx = 0; vertIdx < colBrush.brushVertCount; vertIdx++)
                {
                    vec3_t& vertex = data.brushVerts.at(vertIdx);
                    if (vertIdx == 0)
                    {
                        brushMins = vertex;
                        brushMaxs = vertex;
                    }
                    else
                        BSPUtil::updateAABBWithPoint(vertex, brushMins, brushMaxs);
                }
                if (brushIdx == 0)
                {
                    totalMins = brushMins;
                    totalMaxs = brushMaxs;
                }
                else
                    BSPUtil::updateAABB(brushMins, brushMaxs, totalMins, totalMaxs);

                int brushSurfaceFlags = bsp->colWorld.materials.at(colBrush.materialIndex).surfaceFlags;
                int brushContentFlags = bsp->colWorld.materials.at(colBrush.materialIndex).contentFlags;
                totalBrushContents |= brushContentFlags;
                cbrush_array_t outputBrush{};
                outputBrush.numverts = static_cast<unsigned int>(colBrush.brushVertCount);
                outputBrush.verts = m_memory.Alloc<vec3_t>(colBrush.brushVertCount);
                for (size_t brushVertidx = 0; brushVertidx < colBrush.brushVertCount; brushVertidx++)
                    outputBrush.verts[brushVertidx] = data.brushVerts.at(colBrush.brushVertStartIndex + brushVertidx);
                outputBrush.contents = brushContentFlags;
                outputBrush.mins = brushMins;
                outputBrush.maxs = brushMaxs;
                outputBrush.axial_cflags[0][0] = brushContentFlags;
                outputBrush.axial_cflags[0][1] = brushContentFlags;
                outputBrush.axial_cflags[0][2] = brushContentFlags;
                outputBrush.axial_cflags[1][0] = brushContentFlags;
                outputBrush.axial_cflags[1][1] = brushContentFlags;
                outputBrush.axial_cflags[1][2] = brushContentFlags;
                outputBrush.axial_sflags[0][0] = brushSurfaceFlags;
                outputBrush.axial_sflags[0][1] = brushSurfaceFlags;
                outputBrush.axial_sflags[0][2] = brushSurfaceFlags;
                outputBrush.axial_sflags[1][0] = brushSurfaceFlags;
                outputBrush.axial_sflags[1][1] = brushSurfaceFlags;
                outputBrush.axial_sflags[1][2] = brushSurfaceFlags;
                output.brushVec.emplace_back(outputBrush);
            }

            cLeafBrushNode_s brushNode{};
            brushNode.axis = 0;
            brushNode.contents = totalBrushContents;
            brushNode.leafBrushCount = static_cast<int16_t>(brushes.size());
            brushNode.data.leaf.brushes = m_memory.Alloc<LeafBrush>(brushes.size());
            size_t brushStartIdx = output.brushVec.size();
            for (size_t brushIdx = 0; brushIdx < brushes.size(); brushIdx++)
                brushNode.data.leaf.brushes[0] = static_cast<unsigned short>(brushStartIdx + brushIdx);
            size_t brushNodeIdx = output.brushNodeVec.size();
            output.brushNodeVec.emplace_back(brushNode);

            *out_brushContents = totalBrushContents;
            *out_mins = totalMins;
            *out_maxs = totalMaxs;

            return brushNodeIdx;
        }

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
                    node.children[0] = -1; // index first leaf
                    node.children[1] = -1; // index first leaf
                    output.nodeVec.emplace_back(node);
                }

                cLeaf_s leaf{};
                if (tree->leaf->getObjectCount() > 0)
                {
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
                }
                uint16_t leafIndex = static_cast<uint16_t>(output.leafVec.size());
                output.leafVec.emplace_back(leaf);
                return -1 - leafIndex;
            }
            else
            {
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
            vec3_t worldMins = data.terrainVerts.at(0);
            vec3_t worldMaxs = data.terrainVerts.at(0);
            for (vec3_t& vertex : data.terrainVerts)
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
                    std::vector<size_t> triangles;
                    for (size_t triIdx = 0; triIdx < model.colTerrainTriCount; triIdx++)
                        triangles.emplace_back(model.colTerrainTriIndex + triIdx);
                    addAABBTreeFromTerrain(
                        bsp, output, data, triangles, &parentCount, &parentStartIndex, &modelMins, &modelMaxs, &outputModel.leaf.terrainContents);
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

        void loadCollisionData(CollisionData& data, BSPData* bsp)
        {
            std::vector<size_t> tempTerrainIndexBuffer;
            assert(data.terrainTriVec.size() == 0);
            for (size_t surfIdx = 0; surfIdx < bsp->staticTerrainSurfaceCount; surfIdx++)
            {
                BSPSurface surface = bsp->colWorld.surfaces.at(bsp->staticTerrainSurfaceStart + surfIdx);
                for (size_t triIdx = 0; triIdx < surface.triCount; triIdx++)
                {
                    ColTerrainTri tri{};
                    tri.materialIndex = surface.materialIndex;
                    tri.indexStartIndex = tempTerrainIndexBuffer.size();
                    data.terrainTriVec.emplace_back(tri);

                    size_t index0 = bsp->colWorld.indices.at(surface.indexOfFirstIndex + (triIdx * 3));
                    size_t index1 = bsp->colWorld.indices.at(surface.indexOfFirstIndex + (triIdx * 3) + 1);
                    size_t index2 = bsp->colWorld.indices.at(surface.indexOfFirstIndex + (triIdx * 3) + 2);
                    tempTerrainIndexBuffer.emplace_back(data.terrainVerts.size() + index0); // indices cover the entire vert buffer
                    tempTerrainIndexBuffer.emplace_back(data.terrainVerts.size() + index1); // indices cover the entire vert buffer
                    tempTerrainIndexBuffer.emplace_back(data.terrainVerts.size() + index2); // indices cover the entire vert buffer
                }

                for (size_t vertIdx = 0; vertIdx < surface.vertexCount; vertIdx++)
                    data.terrainVerts.emplace_back(bsp->colWorld.vertices.at(surface.indexOfFirstVertex + vertIdx).pos);
            }
            data.staticTerrainTriCount = data.terrainTriVec.size();

            assert(data.brushVec.size() == 0);
            for (size_t surfIdx = 0; surfIdx < bsp->staticBrushSurfaceCount; surfIdx++)
            {
                ColBrush brush{};
                BSPSurface surface = bsp->colWorld.surfaces.at(bsp->staticBrushSurfaceStart + surfIdx);

                brush.materialIndex = surface.materialIndex;
                brush.brushVertStartIndex = data.brushVerts.size();
                brush.brushVertCount = surface.vertexCount;

                for (size_t vertIdx = 0; vertIdx < surface.vertexCount; vertIdx++)
                    data.brushVerts.emplace_back(bsp->colWorld.vertices.at(surface.indexOfFirstVertex + vertIdx).pos);

                data.brushVec.emplace_back(brush);
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
                    colModel.colTerrainTriIndex = data.terrainTriVec.size();
                    for (size_t surfIdx = 0; surfIdx < model.colTerrainSurfaceCount; surfIdx++)
                    {
                        BSPSurface surface = bsp->colWorld.surfaces.at(model.colTerrainSurfaceIndex + surfIdx);
                        for (size_t triIdx = 0; triIdx < surface.triCount * 3; triIdx++)
                        {
                            ColTerrainTri tri{};
                            tri.materialIndex = surface.materialIndex;
                            tri.indexStartIndex = tempTerrainIndexBuffer.size();
                            data.terrainTriVec.emplace_back(tri);

                            size_t index0 = bsp->colWorld.indices.at(surface.indexOfFirstIndex + (triIdx * 3));
                            size_t index1 = bsp->colWorld.indices.at(surface.indexOfFirstIndex + (triIdx * 3) + 1);
                            size_t index2 = bsp->colWorld.indices.at(surface.indexOfFirstIndex + (triIdx * 3) + 2);
                            tempTerrainIndexBuffer.emplace_back(data.terrainVerts.size() + index0); // indices cover the entire vert buffer
                            tempTerrainIndexBuffer.emplace_back(data.terrainVerts.size() + index1); // indices cover the entire vert buffer
                            tempTerrainIndexBuffer.emplace_back(data.terrainVerts.size() + index2); // indices cover the entire vert buffer
                        }

                        for (size_t vertIdx = 0; vertIdx < surface.vertexCount; vertIdx++)
                            data.terrainVerts.emplace_back(bsp->colWorld.vertices.at(surface.indexOfFirstVertex + vertIdx).pos);
                    }
                    colModel.colTerrainTriCount = data.terrainTriVec.size() - colModel.colTerrainTriIndex;
                }

                if (colModel.type == MST_BRUSH || colModel.type == MST_BOTH)
                {
                    colModel.colBrushIndex = data.brushVec.size();
                    colModel.colBrushCount = model.colBrushSurfaceCount;
                    for (size_t surfIdx = 0; surfIdx < model.colBrushSurfaceCount; surfIdx++)
                    {
                        ColBrush brush{};
                        BSPSurface surface = bsp->colWorld.surfaces.at(model.colBrushSurfaceIndex + surfIdx);

                        brush.materialIndex = surface.materialIndex;
                        brush.brushVertStartIndex = data.brushVerts.size();
                        brush.brushVertCount = surface.vertexCount;

                        for (size_t vertIdx = 0; vertIdx < surface.vertexCount; vertIdx++)
                            data.brushVerts.emplace_back(bsp->colWorld.vertices.at(surface.indexOfFirstVertex + vertIdx).pos);

                        data.brushVec.emplace_back(brush);
                    }
                }

                data.models.emplace_back(colModel);
            }

            // TODO: simplify vertices and update index buffer
            for (size_t idx : tempTerrainIndexBuffer)
            {
                assert(idx < 0xFFFF);
                data.terrainIndices.emplace_back(static_cast<uint16_t>(idx));
            }
        }

        std::unique_ptr<BSPTree> createBSPTree(CollisionData& data)
        {
            vec3_t worldMins = data.terrainVerts.at(0);
            vec3_t worldMaxs = data.terrainVerts.at(0);
            for (vec3_t& vertex : data.terrainVerts)
                BSPUtil::updateAABBWithPoint(vertex, worldMins, worldMaxs);
            for (vec3_t& vertex : data.brushVerts)
                BSPUtil::updateAABBWithPoint(vertex, worldMins, worldMaxs);

            std::unique_ptr<BSPTree> tree = std::make_unique<BSPTree>(worldMins.x, worldMins.y, worldMins.z, worldMaxs.x, worldMaxs.y, worldMaxs.z, 0);

            for (size_t triIdx = 0; triIdx < data.staticTerrainTriCount; triIdx++)
            {
                ColTerrainTri& tri = data.terrainTriVec.at(triIdx);
                vec3_t triMins;
                vec3_t triMaxs;
                for (size_t indexIdx = 0; indexIdx < 3; indexIdx++)
                {
                    uint16_t index = data.terrainIndices.at(tri.indexStartIndex + indexIdx);
                    vec3_t& vert = data.terrainVerts.at(index);
                    if (indexIdx == 0)
                    {
                        triMins = vert;
                        triMaxs = vert;
                    }
                    else
                        BSPUtil::updateAABBWithPoint(vert, triMins, triMaxs);
                }
                std::shared_ptr<BSPObject> object =
                    std::make_shared<BSPObject>(triMins.x, triMins.y, triMins.z, triMaxs.x, triMaxs.y, triMaxs.z, false, triIdx);
                tree->addObjectToTree(std::move(object));
            }

            for (size_t brushIdx = 0; brushIdx < data.staticBrushCount; brushIdx++)
            {
                ColBrush& brush = data.brushVec.at(brushIdx);
                vec3_t brushMins;
                vec3_t brushMaxs;
                for (size_t vertIdx = 0; vertIdx < brush.brushVertCount; vertIdx++)
                {
                    vec3_t& vert = data.brushVerts.at(vertIdx);
                    if (vertIdx == 0)
                    {
                        brushMins = vert;
                        brushMaxs = vert;
                    }
                    else
                        BSPUtil::updateAABBWithPoint(vert, brushMins, brushMaxs);
                }
                std::shared_ptr<BSPObject> object =
                    std::make_shared<BSPObject>(brushMins.x, brushMins.y, brushMins.z, brushMaxs.x, brushMaxs.y, brushMaxs.z, true, brushIdx);
                tree->addObjectToTree(std::move(object));
            }

            return tree;
        }

        bool loadWorldCollision(clipMap_t* clipMap, BSPData* bsp)
        {
            CollisionData data{};
            loadCollisionData(data, bsp);

            std::unique_ptr<BSPTree> tree = createBSPTree(data);
            if (tree == nullptr)
                return false;

            CollisionOutput output{};
            cLeafBrushNode_s tempNode{};
            output.brushNodeVec.emplace_back(tempNode); // first brush node is always empty

            loadBSPNode(bsp, tree.get(), true, output, data);

            loadModelCollision(bsp, data, output);

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
            if (output.brushVec.size() > 0xFFFF)
            {
                con::error("exceeded 0xFFFF brushes in clipmap");
                return false;
            }

            // unused brush data
            clipMap->info.planeCount = 0;
            clipMap->info.planes = nullptr;
            clipMap->info.numBrushSides = 0;
            clipMap->info.brushsides = nullptr;
            clipMap->info.numBrushVerts = 0;
            clipMap->info.brushVerts = nullptr;
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

            clipMap->info.numBrushes = static_cast<uint16_t>(output.brushVec.size());
            clipMap->info.brushes = m_memory.Alloc<cbrush_array_t>(output.brushVec.size());
            memcpy(clipMap->info.brushes, output.brushVec.data(), sizeof(cbrush_array_t) * output.brushVec.size());

            clipMap->numSubModels = static_cast<unsigned int>(output.modelVec.size());
            clipMap->cmodels = m_memory.Alloc<cmodel_t>(output.modelVec.size());
            memcpy(clipMap->cmodels, output.modelVec.data(), sizeof(cmodel_t) * output.modelVec.size());

            clipMap->partitionCount = static_cast<int>(data.terrainTriVec.size());
            clipMap->partitions = m_memory.Alloc<CollisionPartition>(data.terrainTriVec.size());
            clipMap->info.nuinds = static_cast<int>(data.terrainTriVec.size() * 3);
            clipMap->info.uinds = m_memory.Alloc<uint16_t>(data.terrainTriVec.size() * 3);
            for (size_t partIdx = 0; partIdx < data.terrainTriVec.size(); partIdx++)
            {
                clipMap->partitions[partIdx].triCount = 1;
                assert(data.terrainTriVec.at(partIdx).indexStartIndex % 3 == 0);
                clipMap->partitions[partIdx].firstTri = static_cast<int>(data.terrainTriVec.at(partIdx).indexStartIndex / 3);
                clipMap->partitions[partIdx].nuinds = 3;
                clipMap->partitions[partIdx].fuind = static_cast<int>(partIdx * 3);
            }

            clipMap->vertCount = static_cast<unsigned int>(data.terrainVerts.size());
            clipMap->verts = m_memory.Alloc<vec3_t>(data.terrainVerts.size());
            memcpy(clipMap->verts, data.terrainVerts.data(), sizeof(vec3_t) * data.terrainVerts.size());

            assert(data.terrainIndices.size() % 3 == 0);
            clipMap->triCount = static_cast<unsigned int>(data.terrainIndices.size() / 3);
            uint16_t* indices = m_memory.Alloc<uint16_t>(data.terrainIndices.size());
            memcpy(indices, data.terrainIndices.data(), sizeof(uint16_t) * data.terrainIndices.size());
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
