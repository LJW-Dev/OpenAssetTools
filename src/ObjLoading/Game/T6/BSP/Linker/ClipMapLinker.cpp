#include "ClipMapLinker.h"

#include "../BSPUtil.h"

namespace
{

} // namespace

namespace BSP
{
    ClipMapLinker::ClipMapLinker(MemoryManager& memory, ISearchPath& searchPath, AssetCreationContext& context)
        : m_memory(memory),
          m_search_path(searchPath),
          m_context(context)
    {
    }

    void ClipMapLinker::loadPlanes(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->info.planeCount = T5ClipMap->planeCount;
        T6ClipMap->info.planes = m_memory.Alloc<T6::cplane_s>(T5ClipMap->planeCount);

        static_assert(sizeof(T6::cplane_s) == sizeof(T5::cplane_s));

        memcpy(T6ClipMap->info.planes, T5ClipMap->planes, sizeof(T5::cplane_s) * T5ClipMap->planeCount);
    }

    void ClipMapLinker::loadMaterials(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->info.numMaterials = T5ClipMap->numMaterials;
        T6ClipMap->info.materials = m_memory.Alloc<T6::ClipMaterial>(T5ClipMap->numMaterials);

        for (unsigned int matIdx = 0; matIdx < T5ClipMap->numMaterials; matIdx++)
        {
            T5::dmaterial_t* T5material = &T5ClipMap->materials[matIdx];
            T6::ClipMaterial* T6material = &T6ClipMap->info.materials[matIdx];

            T6material->name = m_memory.Dup(T5material->material);
            T6material->surfaceFlags = T5material->surfaceFlags;
            T6material->contentFlags = T5material->contentFlags;
        }
    }

    void ClipMapLinker::loadBrushSides(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->info.numBrushSides = T5ClipMap->numBrushSides;
        T6ClipMap->info.brushsides = m_memory.Alloc<T6::cbrushside_t>(T5ClipMap->numBrushSides);

        for (unsigned int sideIdx = 0; sideIdx < T5ClipMap->numBrushSides; sideIdx++)
        {
            T5::cbrushside_t* T5BrushSide = &T5ClipMap->brushsides[sideIdx];
            T6::cbrushside_t* T6BrushSide = &T6ClipMap->info.brushsides[sideIdx];

            T6BrushSide->cflags = T5BrushSide->cflags;
            T6BrushSide->sflags = T5BrushSide->sflags;
            T6BrushSide->plane = m_memory.Alloc<T6::cplane_s>();

            static_assert(sizeof(T6::cplane_s) == sizeof(T5::cplane_s));
            memcpy(T6BrushSide->plane, T5BrushSide->plane, sizeof(T5::cplane_s));
        }
    }

    void ClipMapLinker::loadLeafBrushNodes(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->info.leafbrushNodesCount = T5ClipMap->leafbrushNodesCount;
        T6ClipMap->info.leafbrushNodes = m_memory.Alloc<T6::cLeafBrushNode_s>(T5ClipMap->leafbrushNodesCount);

        for (unsigned int nodeIdx = 0; nodeIdx < T5ClipMap->leafbrushNodesCount; nodeIdx++)
        {
            T5::cLeafBrushNode_s* T5BrushNode = &T5ClipMap->leafbrushNodes[nodeIdx];
            T6::cLeafBrushNode_s* T6BrushNode = &T6ClipMap->info.leafbrushNodes[nodeIdx];

            T6BrushNode->axis = T5BrushNode->axis;
            T6BrushNode->leafBrushCount = T5BrushNode->leafBrushCount;
            T6BrushNode->contents = T5BrushNode->contents;

            if (T6BrushNode->leafBrushCount > 0)
            {
                static_assert(sizeof(uint16_t) == sizeof(T6::LeafBrush));
                T6BrushNode->data.leaf.brushes = m_memory.Alloc<T6::LeafBrush>(T5BrushNode->leafBrushCount);
                memcpy(T6BrushNode->data.leaf.brushes, T5BrushNode->data.leaf.brushes, sizeof(uint16_t) * T5BrushNode->leafBrushCount);
            }
            else
            {
                T6BrushNode->data.children.dist = T5BrushNode->data.children.dist;
                T6BrushNode->data.children.range = T5BrushNode->data.children.range;
                T6BrushNode->data.children.childOffset[0] = T5BrushNode->data.children.childOffset[0];
                T6BrushNode->data.children.childOffset[1] = T5BrushNode->data.children.childOffset[1];
            }
        }
    }

    void ClipMapLinker::loadLeafBrushes(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->info.numLeafBrushes = T5ClipMap->numLeafBrushes;
        T6ClipMap->info.leafbrushes = m_memory.Alloc<T6::LeafBrush>(T5ClipMap->numLeafBrushes);

        static_assert(sizeof(uint16_t) == sizeof(T6::LeafBrush));

        memcpy(T6ClipMap->info.leafbrushes, T5ClipMap->leafbrushes, sizeof(uint16_t) * T5ClipMap->numLeafBrushes);
    }

    void ClipMapLinker::loadBrushVerts(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->info.numBrushVerts = T5ClipMap->numBrushVerts;
        T6ClipMap->info.brushVerts = m_memory.Alloc<T6::vec3_t>(T5ClipMap->numBrushVerts);

        static_assert(sizeof(T6::vec3_t) == sizeof(T5::vec3_t));

        memcpy(T6ClipMap->info.brushVerts, T5ClipMap->brushVerts, sizeof(T5::vec3_t) * T5ClipMap->numBrushVerts);
    }

    void ClipMapLinker::loadUinds(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->info.nuinds = T5ClipMap->nuinds;
        T6ClipMap->info.uinds = m_memory.Alloc<uint16_t>(T5ClipMap->nuinds);

        memcpy(T6ClipMap->info.uinds, T5ClipMap->uinds, sizeof(uint16_t) * T5ClipMap->nuinds);
    }

    void ClipMapLinker::loadBrushes(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->info.numBrushes = T5ClipMap->numBrushes;
        T6ClipMap->info.brushes = m_memory.Alloc<T6::cbrush_array_t>(T5ClipMap->numBrushes);

        for (unsigned int brushIdx = 0; brushIdx < T5ClipMap->numBrushes; brushIdx++)
        {
            T5::cbrush_t* T5Brush = &T5ClipMap->brushes[brushIdx];
            T6::cbrush_array_t* T6Brush = &T6ClipMap->info.brushes[brushIdx];

            T6Brush->mins.x = T5Brush->mins[0];
            T6Brush->mins.y = T5Brush->mins[1];
            T6Brush->mins.z = T5Brush->mins[2];
            T6Brush->contents = T5Brush->contents;
            T6Brush->maxs.x = T5Brush->maxs[0];
            T6Brush->maxs.y = T5Brush->maxs[1];
            T6Brush->maxs.z = T5Brush->maxs[2];

            T6Brush->axial_cflags[0][0] = T5Brush->axial_cflags[0][0];
            T6Brush->axial_cflags[0][1] = T5Brush->axial_cflags[0][1];
            T6Brush->axial_cflags[0][2] = T5Brush->axial_cflags[0][2];
            T6Brush->axial_cflags[1][0] = T5Brush->axial_cflags[1][0];
            T6Brush->axial_cflags[1][1] = T5Brush->axial_cflags[1][1];
            T6Brush->axial_cflags[1][2] = T5Brush->axial_cflags[1][2];
            T6Brush->axial_sflags[0][0] = T5Brush->axial_sflags[0][0];
            T6Brush->axial_sflags[0][1] = T5Brush->axial_sflags[0][1];
            T6Brush->axial_sflags[0][2] = T5Brush->axial_sflags[0][2];
            T6Brush->axial_sflags[1][0] = T5Brush->axial_sflags[1][0];
            T6Brush->axial_sflags[1][1] = T5Brush->axial_sflags[1][1];
            T6Brush->axial_sflags[1][2] = T5Brush->axial_sflags[1][2];

            static_assert(sizeof(T6::vec3_t) == sizeof(T5::vec3_t));
            T6Brush->numverts = T5Brush->numverts;
            T6Brush->verts = m_memory.Alloc<T6::vec3_t>(T5Brush->numverts);
            memcpy(T6Brush->verts, T5Brush->verts, sizeof(T5::vec3_t) * T5Brush->numverts);

            T6Brush->numsides = T5Brush->numsides;
            T6Brush->sides = m_memory.Alloc<T6::cbrushside_t>(T5Brush->numsides);
            for (unsigned int sideIdx = 0; sideIdx < T5Brush->numsides; sideIdx++)
            {
                T5::cbrushside_t* T5Side = &T5Brush->sides[sideIdx];
                T6::cbrushside_t* T6Side = &T6Brush->sides[sideIdx];

                T6Side->cflags = T5Side->cflags;
                T6Side->sflags = T5Side->sflags;

                T6Side->plane = m_memory.Alloc<T6::cplane_s>();
                static_assert(sizeof(T6::cplane_s) == sizeof(T5::cplane_s));
                memcpy(T6Side->plane, T5Side->plane, sizeof(T5::cplane_s));
            }
        }
    }

    bool ClipMapLinker::loadStaticModels(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->numStaticModels = T5ClipMap->numStaticModels;
        T6ClipMap->staticModelList = m_memory.Alloc<T6::cStaticModel_s>(T5ClipMap->numStaticModels);

        for (unsigned int modelIdx = 0; modelIdx < T5ClipMap->numStaticModels; modelIdx++)
        {
            T5::cStaticModel_s* T5model = &T5ClipMap->staticModelList[modelIdx];
            T6::cStaticModel_s* T6model = &T6ClipMap->staticModelList[modelIdx];

            T6model->writable.nextModelInWorldSector = T5model->writable.nextModelInWorldSector;
            T6model->origin.x = T5model->origin[0];
            T6model->origin.y = T5model->origin[1];
            T6model->origin.z = T5model->origin[2];
            T6model->invScaledAxis[0].x = T5model->invScaledAxis[0][0];
            T6model->invScaledAxis[0].y = T5model->invScaledAxis[0][1];
            T6model->invScaledAxis[0].z = T5model->invScaledAxis[0][2];
            T6model->invScaledAxis[1].x = T5model->invScaledAxis[1][0];
            T6model->invScaledAxis[1].y = T5model->invScaledAxis[1][1];
            T6model->invScaledAxis[1].z = T5model->invScaledAxis[1][2];
            T6model->invScaledAxis[2].x = T5model->invScaledAxis[2][0];
            T6model->invScaledAxis[2].y = T5model->invScaledAxis[2][1];
            T6model->invScaledAxis[2].z = T5model->invScaledAxis[2][2];
            T6model->absmin.x = T5model->absmin[0];
            T6model->absmin.y = T5model->absmin[1];
            T6model->absmin.z = T5model->absmin[2];
            T6model->absmax.x = T5model->absmax[0];
            T6model->absmax.y = T5model->absmax[1];
            T6model->absmax.z = T5model->absmax[2];

            auto T6XModel = m_context.LoadDependency<T6::AssetXModel>(T5model->xmodel->name);
            if (T6XModel == nullptr)
                return false;
            T6model->xmodel = T6XModel->Asset();

            // new in T6
            T6model->contents = T6model->xmodel->contents;
        }

        return true;
    }

    void ClipMapLinker::loadNodes(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->numNodes = T5ClipMap->numNodes;
        T6ClipMap->nodes = m_memory.Alloc<T6::cNode_t>(T5ClipMap->numNodes);

        for (unsigned int nodeIdx = 0; nodeIdx < T5ClipMap->numNodes; nodeIdx++)
        {
            T5::cNode_t* T5Node = &T5ClipMap->nodes[nodeIdx];
            T6::cNode_t* T6Node = &T6ClipMap->nodes[nodeIdx];

            T6Node->children[0] = T5Node->children[0];
            T6Node->children[1] = T5Node->children[1];

            static_assert(sizeof(T6::cplane_s) == sizeof(T5::cplane_s));
            memcpy(T6Node->plane, T5Node->plane, sizeof(T5::cplane_s));
        }
    }

    void ClipMapLinker::loadLeafs(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->numLeafs = T5ClipMap->numLeafs;
        T6ClipMap->leafs = m_memory.Alloc<T6::cLeaf_s>(T5ClipMap->numLeafs);

        for (unsigned int leafIdx = 0; leafIdx < T5ClipMap->numLeafs; leafIdx++)
        {
            T5::cLeaf_s* T5Leaf = &T5ClipMap->leafs[leafIdx];
            T6::cLeaf_s* T6Leaf = &T6ClipMap->leafs[leafIdx];

            T6Leaf->firstCollAabbIndex = T5Leaf->firstCollAabbIndex;
            T6Leaf->collAabbCount = T5Leaf->collAabbCount;
            T6Leaf->brushContents = T5Leaf->brushContents;
            T6Leaf->terrainContents = T5Leaf->terrainContents;
            T6Leaf->mins.x = T5Leaf->mins[0];
            T6Leaf->mins.y = T5Leaf->mins[1];
            T6Leaf->mins.z = T5Leaf->mins[2];
            T6Leaf->maxs.x = T5Leaf->maxs[0];
            T6Leaf->maxs.y = T5Leaf->maxs[1];
            T6Leaf->maxs.z = T5Leaf->maxs[2];
            T6Leaf->leafBrushNode = T5Leaf->leafBrushNode;
            T6Leaf->cluster = T5Leaf->cluster;
        }
    }

    void ClipMapLinker::loadVerts(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->vertCount = T5ClipMap->vertCount;
        T6ClipMap->verts = m_memory.Alloc<T6::vec3_t>(T5ClipMap->vertCount);

        static_assert(sizeof(T6::vec3_t) == sizeof(T5::vec3_t));

        memcpy(T6ClipMap->verts, T5ClipMap->verts, sizeof(T5::vec3_t) * T5ClipMap->vertCount);
    }

    void ClipMapLinker::loadTris(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->triCount = T5ClipMap->triCount;
        T6ClipMap->triIndices = reinterpret_cast<uint16_t(*)[3]>(m_memory.Alloc<uint16_t>(T5ClipMap->triCount * 3));

        memcpy(T6ClipMap->triIndices, T5ClipMap->triIndices, sizeof(uint16_t) * T5ClipMap->triCount * 3);
    }

    void ClipMapLinker::loadWalkableEdges(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        size_t walkableEdgeSize = (3 * T5ClipMap->triCount + 31) / 32 * 4;
        memcpy(T6ClipMap->triEdgeIsWalkable, T5ClipMap->triEdgeIsWalkable, sizeof(char) * walkableEdgeSize);
    }

    void ClipMapLinker::loadPartitions(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->partitionCount = T5ClipMap->partitionCount;
        T6ClipMap->partitions = m_memory.Alloc<T6::CollisionPartition>(T5ClipMap->partitionCount);

        for (unsigned int partIdx = 0; partIdx < T5ClipMap->partitionCount; partIdx++)
        {
            T5::CollisionPartition* T5Part = &T5ClipMap->partitions[partIdx];
            T6::CollisionPartition* T6Part = &T6ClipMap->partitions[partIdx];

            T6Part->triCount = T5Part->triCount;
            T6Part->firstTri = T5Part->firstTri;
            T6Part->nuinds = T5Part->nuinds;
            T6Part->fuind = T5Part->fuind;

            // removed from T5:
            // char borderCount;
            // CollisionBorder* borders;
        }
    }

    void ClipMapLinker::loadAaBbs(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->aabbTreeCount = T5ClipMap->aabbTreeCount;
        T6ClipMap->aabbTrees = m_memory.Alloc<T6::CollisionAabbTree>(T5ClipMap->aabbTreeCount);

        for (unsigned int aabbIdx = 0; aabbIdx < T5ClipMap->aabbTreeCount; aabbIdx++)
        {
            T5::CollisionAabbTree* T5aabb = &T5ClipMap->aabbTrees[aabbIdx];
            T6::CollisionAabbTree* T6aabb = &T6ClipMap->aabbTrees[aabbIdx];

            T6aabb->origin.x = T5aabb->origin[0];
            T6aabb->origin.y = T5aabb->origin[1];
            T6aabb->origin.z = T5aabb->origin[2];
            T6aabb->materialIndex = T5aabb->materialIndex;
            T6aabb->childCount = T5aabb->childCount;
            T6aabb->halfSize.x = T5aabb->halfSize[0];
            T6aabb->halfSize.y = T5aabb->halfSize[1];
            T6aabb->halfSize.z = T5aabb->halfSize[2];
            T6aabb->u.firstChildIndex = T5aabb->u.firstChildIndex;
        }
    }

    void ClipMapLinker::loadSubModels(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->numSubModels = T5ClipMap->numSubModels;
        T6ClipMap->cmodels = m_memory.Alloc<T6::cmodel_t>(T5ClipMap->numSubModels);

        for (unsigned int modelIdx = 0; modelIdx < T5ClipMap->numSubModels; modelIdx++)
        {
            T5::cmodel_t* T5model = &T5ClipMap->cmodels[modelIdx];
            T6::cmodel_t* T6model = &T6ClipMap->cmodels[modelIdx];

            T6model->mins.x = T5model->mins[0];
            T6model->mins.y = T5model->mins[1];
            T6model->mins.z = T5model->mins[2];
            T6model->maxs.x = T5model->maxs[0];
            T6model->maxs.y = T5model->maxs[1];
            T6model->maxs.z = T5model->maxs[2];
            T6model->radius = T5model->radius;
            T6model->leaf.firstCollAabbIndex = T5model->leaf.firstCollAabbIndex;
            T6model->leaf.collAabbCount = T5model->leaf.collAabbCount;
            T6model->leaf.brushContents = T5model->leaf.brushContents;
            T6model->leaf.terrainContents = T5model->leaf.terrainContents;
            T6model->leaf.mins.x = T5model->leaf.mins[0];
            T6model->leaf.mins.y = T5model->leaf.mins[1];
            T6model->leaf.mins.z = T5model->leaf.mins[2];
            T6model->leaf.maxs.x = T5model->leaf.maxs[0];
            T6model->leaf.maxs.y = T5model->leaf.maxs[1];
            T6model->leaf.maxs.z = T5model->leaf.maxs[2];
            T6model->leaf.leafBrushNode = T5model->leaf.leafBrushNode;
            T6model->leaf.cluster = T5model->leaf.cluster;

            // new in T6
            T6model->info = nullptr;
        }
    }

    void ClipMapLinker::loadClusters(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->numClusters = T5ClipMap->numClusters;
        T6ClipMap->vised = T5ClipMap->vised;
        T6ClipMap->clusterBytes = T5ClipMap->clusterBytes;
        T6ClipMap->visibility = m_memory.Alloc<char>(T5ClipMap->clusterBytes);
        memcpy(T6ClipMap->visibility, T5ClipMap->visibility, sizeof(char) * T5ClipMap->clusterBytes);
    }

    void ClipMapLinker::loadBoxHulls(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        T6ClipMap->box_brush = m_memory.Alloc<T6::cbrush_t>();

        T6ClipMap->box_brush->mins.x = T5ClipMap->box_brush->mins[0];
        T6ClipMap->box_brush->mins.y = T5ClipMap->box_brush->mins[1];
        T6ClipMap->box_brush->mins.z = T5ClipMap->box_brush->mins[2];
        T6ClipMap->box_brush->contents = T5ClipMap->box_brush->contents;
        T6ClipMap->box_brush->maxs.x = T5ClipMap->box_brush->maxs[0];
        T6ClipMap->box_brush->maxs.y = T5ClipMap->box_brush->maxs[1];
        T6ClipMap->box_brush->maxs.z = T5ClipMap->box_brush->maxs[2];

        T6ClipMap->box_brush->axial_cflags[0][0] = T5ClipMap->box_brush->axial_cflags[0][0];
        T6ClipMap->box_brush->axial_cflags[0][1] = T5ClipMap->box_brush->axial_cflags[0][1];
        T6ClipMap->box_brush->axial_cflags[0][2] = T5ClipMap->box_brush->axial_cflags[0][2];
        T6ClipMap->box_brush->axial_cflags[1][0] = T5ClipMap->box_brush->axial_cflags[1][0];
        T6ClipMap->box_brush->axial_cflags[1][1] = T5ClipMap->box_brush->axial_cflags[1][1];
        T6ClipMap->box_brush->axial_cflags[1][2] = T5ClipMap->box_brush->axial_cflags[1][2];
        T6ClipMap->box_brush->axial_sflags[0][0] = T5ClipMap->box_brush->axial_sflags[0][0];
        T6ClipMap->box_brush->axial_sflags[0][1] = T5ClipMap->box_brush->axial_sflags[0][1];
        T6ClipMap->box_brush->axial_sflags[0][2] = T5ClipMap->box_brush->axial_sflags[0][2];
        T6ClipMap->box_brush->axial_sflags[1][0] = T5ClipMap->box_brush->axial_sflags[1][0];
        T6ClipMap->box_brush->axial_sflags[1][1] = T5ClipMap->box_brush->axial_sflags[1][1];
        T6ClipMap->box_brush->axial_sflags[1][2] = T5ClipMap->box_brush->axial_sflags[1][2];

        static_assert(sizeof(T6::vec3_t) == sizeof(T5::vec3_t));
        T6ClipMap->box_brush->numverts = T5ClipMap->box_brush->numverts;
        T6ClipMap->box_brush->verts = m_memory.Alloc<T6::vec3_t>(T5ClipMap->box_brush->numverts);
        memcpy(T6ClipMap->box_brush->verts, T5ClipMap->box_brush->verts, sizeof(T5::vec3_t) * T5ClipMap->box_brush->numverts);

        T6ClipMap->box_brush->numsides = T5ClipMap->box_brush->numsides;
        T6ClipMap->box_brush->sides = m_memory.Alloc<T6::cbrushside_t>(T5ClipMap->box_brush->numsides);
        for (unsigned int sideIdx = 0; sideIdx < T5ClipMap->box_brush->numsides; sideIdx++)
        {
            T5::cbrushside_t* T5Side = &T5ClipMap->box_brush->sides[sideIdx];
            T6::cbrushside_t* T6Side = &T6ClipMap->box_brush->sides[sideIdx];

            T6Side->cflags = T5Side->cflags;
            T6Side->sflags = T5Side->sflags;

            T6Side->plane = m_memory.Alloc<T6::cplane_s>();
            static_assert(sizeof(T6::cplane_s) == sizeof(T5::cplane_s));
            memcpy(T6Side->plane, T5Side->plane, sizeof(T5::cplane_s));
        }

        T6ClipMap->box_model.mins.x = T5ClipMap->box_model.mins[0];
        T6ClipMap->box_model.mins.y = T5ClipMap->box_model.mins[1];
        T6ClipMap->box_model.mins.z = T5ClipMap->box_model.mins[2];
        T6ClipMap->box_model.maxs.x = T5ClipMap->box_model.maxs[0];
        T6ClipMap->box_model.maxs.y = T5ClipMap->box_model.maxs[1];
        T6ClipMap->box_model.maxs.z = T5ClipMap->box_model.maxs[2];
        T6ClipMap->box_model.radius = T5ClipMap->box_model.radius;
        T6ClipMap->box_model.leaf.firstCollAabbIndex = T5ClipMap->box_model.leaf.firstCollAabbIndex;
        T6ClipMap->box_model.leaf.collAabbCount = T5ClipMap->box_model.leaf.collAabbCount;
        T6ClipMap->box_model.leaf.brushContents = T5ClipMap->box_model.leaf.brushContents;
        T6ClipMap->box_model.leaf.terrainContents = T5ClipMap->box_model.leaf.terrainContents;
        T6ClipMap->box_model.leaf.mins.x = T5ClipMap->box_model.leaf.mins[0];
        T6ClipMap->box_model.leaf.mins.y = T5ClipMap->box_model.leaf.mins[1];
        T6ClipMap->box_model.leaf.mins.z = T5ClipMap->box_model.leaf.mins[2];
        T6ClipMap->box_model.leaf.maxs.x = T5ClipMap->box_model.leaf.maxs[0];
        T6ClipMap->box_model.leaf.maxs.y = T5ClipMap->box_model.leaf.maxs[1];
        T6ClipMap->box_model.leaf.maxs.z = T5ClipMap->box_model.leaf.maxs[2];
        T6ClipMap->box_model.leaf.leafBrushNode = T5ClipMap->box_model.leaf.leafBrushNode;
        T6ClipMap->box_model.leaf.cluster = T5ClipMap->box_model.leaf.cluster;

        // new in T6
        T6ClipMap->box_model.info = nullptr;
    }

    void ClipMapLinker::loadDynEnts(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        // TODO: unimportant and too much work to implement right now
        // Remove dynents

        int dynEntCount = 0;
        T6ClipMap->originalDynEntCount = dynEntCount;
        T6ClipMap->dynEntCount[0] = T6ClipMap->originalDynEntCount + 256; // the game allocs 256 empty dynents, as they may be used ingame
        T6ClipMap->dynEntCount[1] = 0;
        T6ClipMap->dynEntCount[2] = 0;
        T6ClipMap->dynEntCount[3] = 0;

        T6ClipMap->dynEntClientList[0] = m_memory.Alloc<T6::DynEntityClient>(T6ClipMap->dynEntCount[0]);
        T6ClipMap->dynEntClientList[1] = nullptr;

        T6ClipMap->dynEntServerList[0] = nullptr;
        T6ClipMap->dynEntServerList[1] = nullptr;

        T6ClipMap->dynEntCollList[0] = m_memory.Alloc<T6::DynEntityColl>(T6ClipMap->dynEntCount[0]);
        T6ClipMap->dynEntCollList[1] = nullptr;
        T6ClipMap->dynEntCollList[2] = nullptr;
        T6ClipMap->dynEntCollList[3] = nullptr;

        T6ClipMap->dynEntPoseList[0] = m_memory.Alloc<T6::DynEntityPose>(T6ClipMap->dynEntCount[0]);
        T6ClipMap->dynEntPoseList[1] = nullptr;

        T6ClipMap->dynEntDefList[0] = m_memory.Alloc<T6::DynEntityDef>(T6ClipMap->dynEntCount[0]);
        T6ClipMap->dynEntDefList[1] = nullptr;
    }

    void ClipMapLinker::loadConstraints(T5::clipMap_t* T5ClipMap, T6::clipMap_t* T6ClipMap)
    {
        // TODO: unimportant and too much work to implement right now
        // Remove constraints

        T6ClipMap->num_constraints = 0; // max 511
        T6ClipMap->constraints = nullptr;

        // The game allocates 32 empty ropes
        T6ClipMap->max_ropes = 32; // max 300
        T6ClipMap->ropes = m_memory.Alloc<T6::rope_t>(T6ClipMap->max_ropes);
    }

    bool ClipMapLinker::linkClipMap(ZoneAssetPools* T5AssetPool, std::string& bspName)
    {
        auto T5ClipMapAsset = T5AssetPool->GetAsset(T5::ASSET_TYPE_CLIPMAP_PVS, bspName);
        if (T5ClipMapAsset == nullptr)
        {
            con::error("Can't find T5 ComWorld asset.");
            return false;
        }
        T5::clipMap_t* T5ClipMap = static_cast<T5::clipMap_t*>(T5ClipMapAsset->m_ptr);
        T6::clipMap_t* T6ClipMap = m_memory.Alloc<T6::clipMap_t>();

        T6ClipMap->name = m_memory.Dup(T5ClipMap->name);
        T6ClipMap->isInUse = T5ClipMap->isInUse;
        T6ClipMap->checksum = T5ClipMap->checksum;

        T6ClipMap->info.brushBounds = nullptr;
        T6ClipMap->info.brushContents = nullptr;

        loadPlanes(T5ClipMap, T6ClipMap);
        loadMaterials(T5ClipMap, T6ClipMap);
        loadBrushSides(T5ClipMap, T6ClipMap);
        loadLeafBrushNodes(T5ClipMap, T6ClipMap);
        loadLeafBrushes(T5ClipMap, T6ClipMap);
        loadBrushVerts(T5ClipMap, T6ClipMap);
        loadUinds(T5ClipMap, T6ClipMap);
        loadBrushes(T5ClipMap, T6ClipMap);
        loadStaticModels(T5ClipMap, T6ClipMap);
        loadNodes(T5ClipMap, T6ClipMap);
        loadLeafs(T5ClipMap, T6ClipMap);
        loadVerts(T5ClipMap, T6ClipMap);
        loadWalkableEdges(T5ClipMap, T6ClipMap);
        loadPartitions(T5ClipMap, T6ClipMap);
        loadAaBbs(T5ClipMap, T6ClipMap);
        loadSubModels(T5ClipMap, T6ClipMap);
        loadClusters(T5ClipMap, T6ClipMap);
        loadBoxHulls(T5ClipMap, T6ClipMap);
        loadDynEnts(T5ClipMap, T6ClipMap);
        loadConstraints(T5ClipMap, T6ClipMap);

        auto T6MapEnts = m_context.LoadDependency<T6::AssetMapEnts>(bspName);
        if (T6MapEnts == nullptr)
            return false;
        T6ClipMap->mapEnts = T6MapEnts->Asset();

        // new in T6
        T6ClipMap->pInfo = nullptr;

        // removed from T5
        //  int borderCount;
        //  CollisionBorder* borders;

        m_context.AddAsset<T6::AssetClipMap>(T6ClipMap->name, T6ClipMap);

        return true;
    }
} // namespace BSP
