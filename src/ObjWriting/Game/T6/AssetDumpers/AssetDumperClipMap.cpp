#include <nlohmann/json.hpp>

#include "AssetDumperClipMap.h"

using namespace T6;

bool AssetDumperClipMap::ShouldDump(XAssetInfo<clipMap_t>* asset)
{
    return true;
}

const char* cm_emptyString = "";

const char* cm_sanitiseJsonString(const char* str)
{
    if (str == NULL)
        return cm_emptyString;
    else
        return str;
}

void AssetDumperClipMap::DumpAsset(AssetDumpingContext& context, XAssetInfo<clipMap_t>* asset)
{
    const auto* clipMap = asset->Asset();
    const auto assetFile = context.OpenAssetFile("bsp/clipmap.json");

    if (!assetFile)
        return;

    auto& stream = *assetFile;

    nlohmann::json j;

    j["name"] = clipMap->name;
    j["isInUse"] = clipMap->isInUse;
    
    j["info"]["planeCount"] = clipMap->info.planeCount;
    auto infoPlanes = nlohmann::json::array();
    for (int i = 0; i < clipMap->info.planeCount; i++)
    {
        auto currentInfoPlane = clipMap->info.planes[i];
        infoPlanes.push_back({
            {"normal",   {{"x", currentInfoPlane.normal.x}, {"y", currentInfoPlane.normal.y}, {"z", currentInfoPlane.normal.z}}},
            {"dist",     currentInfoPlane.dist                                                                                 },
            {"type",     currentInfoPlane.type                                                                                 },
            {"signbits", currentInfoPlane.signbits                                                                             },
            {"pad",      {{"0", currentInfoPlane.pad[0]}, {"1", currentInfoPlane.pad[1]}}                                      },
        });
    }
    j["info"]["planes"] = infoPlanes;

    j["info"]["numMaterials"] = clipMap->info.numMaterials;
    auto infoMaterials = nlohmann::json::array();
    for (unsigned int i = 0; i < clipMap->info.numMaterials; i++)
    {
        auto currentMaterial = clipMap->info.materials[i];
        infoMaterials.push_back({
            {"name",         cm_sanitiseJsonString(currentMaterial.name)},
            {"surfaceFlags", currentMaterial.surfaceFlags },
            {"contentFlags", currentMaterial.contentFlags},
        });
    }
    j["info"]["materials"] = infoMaterials;
    
    j["info"]["numBrushSides"] = clipMap->info.numBrushSides;
    auto infoBrushSides = nlohmann::json::array();
    for (unsigned int i = 0; i < clipMap->info.numBrushSides; i++)
    {
        auto currentBrushSide = clipMap->info.brushsides[i];

        // find the plane that is referenced
        int planeIndex = -1;
        for (int k = 0; k < clipMap->info.planeCount; k++)
        {
            if (currentBrushSide.plane == &clipMap->info.planes[k])
            {
                planeIndex = k;
                break;
            }
        }
        assert(planeIndex != -1);
        
        infoBrushSides.push_back({
            {"planeReferenceIndex",  planeIndex            },
            {"cflags",              currentBrushSide.cflags},
            {"sflags",              currentBrushSide.sflags},
        });
    }
    j["info"]["brushsides"] = infoBrushSides;
    
    j["info"]["leafbrushNodesCount"] = clipMap->info.leafbrushNodesCount;
    auto leafBrushNodes = nlohmann::json::array();
    for (unsigned int i = 0; i < clipMap->info.leafbrushNodesCount; i++)
    {
        auto currLeafBrushNode = clipMap->info.leafbrushNodes[i];

        auto leafBrushNodeData = nlohmann::json::object();
        if (currLeafBrushNode.leafBrushCount > 0)
        {
            // for some reason the leaf brushes info array size isn't correct and this method fails on later values within currLeafBrushNode.
            // just write directly to JSON as a workaround right now
            // find the brush that is referenced
            //int brushIndex = -1;
            //for (unsigned int k = 0; k < clipMap->info.numLeafBrushes; k++)
            //{
            //    if (currLeafBrushNode.data.leaf.brushes == &clipMap->info.leafbrushes[k])
            //    {
            //        brushIndex = k;
            //        break;
            //    }
            //}
            auto leafBrushArray = nlohmann::json::array();
            for (int k = 0; k < currLeafBrushNode.leafBrushCount; k++)
                leafBrushArray.push_back(currLeafBrushNode.data.leaf.brushes[k]);
            
            leafBrushNodeData = leafBrushArray;
        }
        else
        {
            leafBrushNodeData = {
                {"dist", currLeafBrushNode.data.children.dist},
                {"range", currLeafBrushNode.data.children.range},
                {"childOffset0", currLeafBrushNode.data.children.childOffset[0]},
                {"childOffset1", currLeafBrushNode.data.children.childOffset[1]},
            };
        }


        leafBrushNodes.push_back({
            {"axis", currLeafBrushNode.axis},
            {"leafBrushCount", currLeafBrushNode.leafBrushCount},
            {"contents", currLeafBrushNode.contents},
            {"data", leafBrushNodeData},
        });
    }
    j["info"]["leafbrushNodes"] = leafBrushNodes;

    j["info"]["numLeafBrushes"] = clipMap->info.numLeafBrushes;
    auto leafbrushes = nlohmann::json::array();
    for (unsigned int i = 0; i < clipMap->info.numLeafBrushes; i++)
        leafbrushes.push_back(clipMap->info.leafbrushes[i]);
    j["info"]["leafbrushes"] = leafbrushes;

    j["info"]["numBrushVerts"] = clipMap->info.numBrushVerts;
    auto brushVerts = nlohmann::json::array();
    for (unsigned int i = 0; i < clipMap->info.numBrushVerts; i++)
        brushVerts.push_back({
            {"x", clipMap->info.brushVerts[i].x},
            {"y", clipMap->info.brushVerts[i].y},
            {"z", clipMap->info.brushVerts[i].z}
        });
    j["info"]["brushVerts"] = brushVerts;

    j["info"]["nuinds"] = clipMap->info.nuinds;
    auto uinds = nlohmann::json::array();
    for (unsigned int i = 0; i < clipMap->info.nuinds; i++)
        uinds.push_back(clipMap->info.uinds[i]);
    j["info"]["uinds"] = uinds;

    j["info"]["numBrushes"] = clipMap->info.numBrushes;
    auto infoBrushes = nlohmann::json::array();
    auto infoBrushBounds = nlohmann::json::array();
    auto infoBrushContents = nlohmann::json::array();
    for (int i = 0; i < clipMap->info.numBrushes; i++)
    {
        if (clipMap->info.brushes)
        {
            auto currentBrush = clipMap->info.brushes[i];

            // find the start of the array of sides that is referenced
            int sideReferenceIndex = -1;
            if (currentBrush.sides)
            {
                for (unsigned int k = 0; k < clipMap->info.numBrushSides; k++)
                {
                    if (currentBrush.sides == &clipMap->info.brushsides[k])
                    {
                        sideReferenceIndex = k;
                        break;
                    }
                }
                assert(sideReferenceIndex != -1);
            }

            //  find the start of the array of verts that is referenced
            int vertReferenceIndex = -1;
            if (currentBrush.verts)
            {
                for (unsigned int k = 0; k < clipMap->info.numBrushVerts; k++)
                {
                    if (currentBrush.verts == &clipMap->info.brushVerts[k])
                    {
                        vertReferenceIndex = k;
                        break;
                    }
                }
                assert(vertReferenceIndex != -1);
            }

            infoBrushes.push_back({
                {"contents",       clipMap->info.brushes[i].contents                                                                                       },
                {"numsides",       clipMap->info.brushes[i].numsides                                                                                       },
                {"numverts",       clipMap->info.brushes[i].numverts                                                                                       },
                {"mins",           {{"x", clipMap->info.brushes[i].mins.x}, {"y", clipMap->info.brushes[i].mins.y}, {"z", clipMap->info.brushes[i].mins.z}}},
                {"maxs",           {{"x", clipMap->info.brushes[i].maxs.x}, {"y", clipMap->info.brushes[i].maxs.y}, {"z", clipMap->info.brushes[i].maxs.z}}},
                {"axial_cflags00", clipMap->info.brushes[i].axial_cflags[0][0]                                                                             },
                {"axial_cflags01", clipMap->info.brushes[i].axial_cflags[0][1]                                                                             },
                {"axial_cflags02", clipMap->info.brushes[i].axial_cflags[0][2]                                                                             },
                {"axial_cflags10", clipMap->info.brushes[i].axial_cflags[1][0]                                                                             },
                {"axial_cflags11", clipMap->info.brushes[i].axial_cflags[1][1]                                                                             },
                {"axial_cflags12", clipMap->info.brushes[i].axial_cflags[1][2]                                                                             },
                {"axial_sflags00", clipMap->info.brushes[i].axial_sflags[0][0]                                                                             },
                {"axial_sflags01", clipMap->info.brushes[i].axial_sflags[0][1]                                                                             },
                {"axial_sflags02", clipMap->info.brushes[i].axial_sflags[0][2]                                                                             },
                {"axial_sflags10", clipMap->info.brushes[i].axial_sflags[1][0]                                                                             },
                {"axial_sflags11", clipMap->info.brushes[i].axial_sflags[1][1]                                                                             },
                {"axial_sflags12", clipMap->info.brushes[i].axial_sflags[1][2]                                                                             },
                {"sidesArrayIndexStart", sideReferenceIndex                                                                                                       },
                {"vertsArrayIndexStart", vertReferenceIndex                                                                                                      }
            });
        }

        if (clipMap->info.brushBounds)
            infoBrushBounds.push_back({
                {"midpoint", {
                    {"x", clipMap->info.brushBounds[i].midPoint.x}, 
                    {"y", clipMap->info.brushBounds[i].midPoint.y}, 
                    {"z", clipMap->info.brushBounds[i].midPoint.z}}
                },
                {"halfSize", {
                    {"x", clipMap->info.brushBounds[i].halfSize.x}, 
                    {"y", clipMap->info.brushBounds[i].halfSize.y}, 
                    {"z", clipMap->info.brushBounds[i].halfSize.z}}
                }
            });

        if (clipMap->info.brushContents)
            infoBrushContents.push_back(clipMap->info.brushContents[i]);
    }
    j["info"]["brushes"] = infoBrushes;
    j["info"]["brushBounds"] = infoBrushBounds;
    j["info"]["brushContents"] = infoBrushContents;
    

    assert(clipMap->pInfo == nullptr);
    j["pInfo"] = 0;
    
    j["numStaticModels"] = clipMap->numStaticModels;
    auto staticModelList = nlohmann::json::array();
    for (unsigned int i = 0; i < clipMap->numStaticModels; i++)
    {
        staticModelList.push_back({
            {"nextModelInWorldSector", clipMap->staticModelList[i].writable.nextModelInWorldSector},
            {"xmodel", cm_sanitiseJsonString(clipMap->staticModelList[i].xmodel->name)},
            {"contents", clipMap->staticModelList[i].contents},
            {"origin", {{"x", clipMap->staticModelList[i].origin.x}, {"y", clipMap->staticModelList[i].origin.y}, {"z", clipMap->staticModelList[i].origin.z}}},
            {"invScaledAxis0", {{"x", clipMap->staticModelList[i].invScaledAxis[0].x}, {"y", clipMap->staticModelList[i].invScaledAxis[0].y}, {"z", clipMap->staticModelList[i].invScaledAxis[0].z}}},
            {"invScaledAxis1", {{"x", clipMap->staticModelList[i].invScaledAxis[1].x}, {"y", clipMap->staticModelList[i].invScaledAxis[1].y}, {"z", clipMap->staticModelList[i].invScaledAxis[1].z}}},
            {"invScaledAxis2", {{"x", clipMap->staticModelList[i].invScaledAxis[2].x}, {"y", clipMap->staticModelList[i].invScaledAxis[2].y}, {"z", clipMap->staticModelList[i].invScaledAxis[2].z}}},
            {"absmin", {{"x", clipMap->staticModelList[i].absmin.x}, {"y", clipMap->staticModelList[i].absmin.y}, {"z", clipMap->staticModelList[i].absmin.z}}},
            {"absmax", {{"x", clipMap->staticModelList[i].absmax.x}, {"y", clipMap->staticModelList[i].absmax.y}, {"z", clipMap->staticModelList[i].absmax.z}}},
        });
    }
    j["staticModelList"] = staticModelList;

    auto cNodes = nlohmann::json::array();
    j["numNodes"] = clipMap->numNodes;
    for (unsigned int i = 0; i < clipMap->numNodes; i++)
    {
        // find the plane that is referenced
        int cPlaneIndex = -1;
        for (int k = 0; k < clipMap->info.planeCount; k++)
        {
            if (clipMap->nodes[i].plane == &clipMap->info.planes[k])
            {
                cPlaneIndex = k;
                break;
            }
        }
        assert(cPlaneIndex != -1);

        cNodes.push_back({
            {"plane",     cPlaneIndex                  },
            {"children0", clipMap->nodes[i].children[0]},
            {"children1", clipMap->nodes[i].children[1]},
        });
    }
    j["nodes"] = cNodes;

    j["numLeafs"] = clipMap->numLeafs;
    auto cLeafs = nlohmann::json::array();
    for (unsigned int i = 0; i < clipMap->numLeafs; i++)
    {
        cLeafs.push_back({
            {"firstCollAabbIndex", clipMap->leafs[i].firstCollAabbIndex},
            {"collAabbCount",                   clipMap->leafs[i].collAabbCount     },
            {"brushContents",                   clipMap->leafs[i].brushContents     },
            {"terrainContents",                   clipMap->leafs[i].terrainContents   },
            {"maxs", {{"x", clipMap->leafs[i].maxs.x}, {"y", clipMap->leafs[i].maxs.y}, {"z", clipMap->leafs[i].maxs.z}}},
            {"mins", {{"x", clipMap->leafs[i].mins.x}, {"y", clipMap->leafs[i].mins.y}, {"z", clipMap->leafs[i].mins.z}}},
            {"leafBrushNode",      clipMap->leafs[i].leafBrushNode     },
            {"cluster",            clipMap->leafs[i].cluster           },
        });
    }
    j["leafs"] = cLeafs;

    j["vertCount"] = clipMap->vertCount;
    auto clipVerts = nlohmann::json::array();
    for (unsigned int i = 0; i < clipMap->vertCount; i++)
        clipVerts.push_back({
            {"x", clipMap->verts[i].x},
            {"y", clipMap->verts[i].y},
            {"z", clipMap->verts[i].z}
        });
    j["verts"] = clipVerts;

    j["triCount"] = clipMap->triCount;
    auto triIndices = nlohmann::json::array();
    for (int i = 0; i < clipMap->triCount; i++)
        triIndices.push_back({
            {"0", clipMap->triIndices[i][0]},
            {"1", clipMap->triIndices[i][1]},
            {"2", clipMap->triIndices[i][2]}
        });
    j["triIndices"] = triIndices;
    

    auto triEdgeIsWalkable = nlohmann::json::array();
    for (int i = 0; i < (3 * clipMap->triCount + 31) / 32 * 4; i++)
        triEdgeIsWalkable.push_back(clipMap->triEdgeIsWalkable[i]);
    j["triEdgeIsWalkable"] = triEdgeIsWalkable;

    j["partitionCount"] = clipMap->partitionCount;
    auto partitions = nlohmann::json::array();
    for (int i = 0; i < clipMap->partitionCount; i++)
    {
        partitions.push_back({
            {"triCount", clipMap->partitions[i].triCount},
            {"firstTri", clipMap->partitions[i].firstTri},
            {"nuinds",   clipMap->partitions[i].nuinds  },
            {"fuind",    clipMap->partitions[i].fuind   },
        });
    }
    j["partitions"] = partitions;
    
    j["aabbTreeCount"] = clipMap->aabbTreeCount;
    auto aabbTrees = nlohmann::json::array();
    for (int i = 0; i < clipMap->aabbTreeCount; i++)
    {
        aabbTrees.push_back({
            {"origin",        {{"x", clipMap->aabbTrees[i].origin.x}, {"y", clipMap->aabbTrees[i].origin.y}, {"z", clipMap->aabbTrees[i].origin.z}}      },
            {"materialIndex", clipMap->aabbTrees[i].materialIndex                                                                                        },
            {"childCount",    clipMap->aabbTrees[i].childCount                                                                                           },
            {"halfSize",      {{"x", clipMap->aabbTrees[i].halfSize.x}, {"y", clipMap->aabbTrees[i].halfSize.y}, {"z", clipMap->aabbTrees[i].halfSize.z}}},
            {"u",             clipMap->aabbTrees[i].u.firstChildIndex                                                                                    },
        });
    }
    j["aabbTrees"] = aabbTrees;

    j["numSubModels"] = clipMap->numSubModels;
    auto cmodels = nlohmann::json::array();
    for (unsigned int i = 0; i < clipMap->numSubModels; i++)
    {
        assert(clipMap->cmodels[i].info == clipMap->pInfo);

        cmodels.push_back({
            {"maxs", {{"x", clipMap->cmodels[i].maxs.x}, {"y", clipMap->cmodels[i].maxs.y}, {"z", clipMap->cmodels[i].maxs.z}}},
            {"mins", {{"x", clipMap->cmodels[i].mins.x}, {"y", clipMap->cmodels[i].mins.y}, {"z", clipMap->cmodels[i].mins.z}}},
            {"radius", clipMap->cmodels[i].radius                                                                             },
            {"leaf", {
                 {"firstCollAabbIndex", clipMap->cmodels[i].leaf.firstCollAabbIndex},
                 {"collAabbCount", clipMap->cmodels[i].leaf.collAabbCount},
                 {"brushContents", clipMap->cmodels[i].leaf.brushContents},
                 {"terrainContents", clipMap->cmodels[i].leaf.terrainContents},
                 {"maxs", {{"x", clipMap->cmodels[i].leaf.maxs.x}, {"y", clipMap->cmodels[i].leaf.maxs.y}, {"z", clipMap->cmodels[i].leaf.maxs.z}}},
                 {"mins", {{"x", clipMap->cmodels[i].leaf.mins.x}, {"y", clipMap->cmodels[i].leaf.mins.y}, {"z", clipMap->cmodels[i].leaf.mins.z}}},
                 {"leafBrushNode", clipMap->cmodels[i].leaf.leafBrushNode},
                 {"cluster", clipMap->cmodels[i].leaf.cluster},
            }},
        });
    }
    j["cmodels"] = cmodels;
    

    j["numClusters"] = clipMap->numClusters;
    j["clusterBytes"] = clipMap->clusterBytes;
    j["vised"] = clipMap->vised;
    auto visibility = nlohmann::json::array();
    for (int i = 0; i < clipMap->numClusters * clipMap->clusterBytes; i++)
    {
        visibility.push_back(clipMap->visibility[i]);
    }
    j["visibility"] = visibility;

    // map ents is part of the d3dbsp and has no unique name
    j["mapEnts"] = clipMap->name;

    j["box_brush"]["contents"] = clipMap->box_brush->contents;
    j["box_brush"]["numsides"] = clipMap->box_brush->numsides;
    j["box_brush"]["numverts"] = clipMap->box_brush->numverts;
    j["box_brush"]["mins"] = {
        {"x", clipMap->box_brush->mins.x},
        {"y", clipMap->box_brush->mins.y},
        {"z", clipMap->box_brush->mins.z}
    };
    j["box_brush"]["maxs"] = {
        {"x", clipMap->box_brush->maxs.x},
        {"y", clipMap->box_brush->maxs.y},
        {"z", clipMap->box_brush->maxs.z}
    };
    j["box_brush"]["axial_cflags00"] = clipMap->box_brush->axial_cflags[0][0];
    j["box_brush"]["axial_cflags01"] = clipMap->box_brush->axial_cflags[0][1];
    j["box_brush"]["axial_cflags02"] = clipMap->box_brush->axial_cflags[0][2];
    j["box_brush"]["axial_cflags10"] = clipMap->box_brush->axial_cflags[1][0];
    j["box_brush"]["axial_cflags11"] = clipMap->box_brush->axial_cflags[1][1];
    j["box_brush"]["axial_cflags12"] = clipMap->box_brush->axial_cflags[1][2];
    j["box_brush"]["axial_sflags00"] = clipMap->box_brush->axial_sflags[0][0];
    j["box_brush"]["axial_sflags01"] = clipMap->box_brush->axial_sflags[0][1];
    j["box_brush"]["axial_sflags02"] = clipMap->box_brush->axial_sflags[0][2];
    j["box_brush"]["axial_sflags10"] = clipMap->box_brush->axial_sflags[1][0];
    j["box_brush"]["axial_sflags11"] = clipMap->box_brush->axial_sflags[1][1];
    j["box_brush"]["axial_sflags12"] = clipMap->box_brush->axial_sflags[1][2];

    assert(clipMap->box_brush->numsides == 0);
    assert(clipMap->box_brush->numverts == 0);
    j["box_brush"]["sides"] = nlohmann::json::array();
    j["box_brush"]["verts"] = nlohmann::json::array();

    j["box_model"]["mins"] = {
        {"x", clipMap->box_model.mins.x},
        {"y", clipMap->box_model.mins.y},
        {"z", clipMap->box_model.mins.z}
    };
    j["box_model"]["maxs"] = {
        {"x", clipMap->box_model.maxs.x},
        {"y", clipMap->box_model.maxs.y},
        {"z", clipMap->box_model.maxs.z}
    };
    j["box_model"]["radius"] = clipMap->box_model.radius;
    j["box_model"]["info"] = 0;
    assert(clipMap->box_model.info == clipMap->pInfo);

    j["box_model"]["leaf"]["firstCollAabbIndex"] = clipMap->box_model.leaf.firstCollAabbIndex;
    j["box_model"]["leaf"]["collAabbCount"] = clipMap->box_model.leaf.collAabbCount;
    j["box_model"]["leaf"]["brushContents"] = clipMap->box_model.leaf.brushContents;
    j["box_model"]["leaf"]["terrainContents"] = clipMap->box_model.leaf.terrainContents;
    j["box_model"]["leaf"]["mins"] = {
        {"x", clipMap->box_model.leaf.mins.x},
        {"y", clipMap->box_model.leaf.mins.y},
        {"z", clipMap->box_model.leaf.mins.z}
    };
    j["box_model"]["leaf"]["maxs"] = {
        {"x", clipMap->box_model.leaf.maxs.x},
        {"y", clipMap->box_model.leaf.maxs.y},
        {"z", clipMap->box_model.leaf.maxs.z}
    };
    j["box_model"]["leaf"]["leafBrushNode"] = clipMap->box_model.leaf.leafBrushNode;
    j["box_model"]["leaf"]["cluster"] = clipMap->box_model.leaf.cluster;

    j["originalDynEntCount"] = clipMap->originalDynEntCount;
    j["dynEntCount0"] = clipMap->dynEntCount[0];
    j["dynEntCount1"] = clipMap->dynEntCount[1];
    j["dynEntCount2"] = clipMap->dynEntCount[2];
    j["dynEntCount3"] = clipMap->dynEntCount[3];
    
    auto dynEntDefList0 = nlohmann::json::array();
    for (int i = 0; i < clipMap->dynEntCount[0]; i++)
    {
        auto currDynEntDef = clipMap->dynEntDefList[0][i];

        const char* xModel = "";
        if (currDynEntDef.xModel)
            xModel = cm_sanitiseJsonString(currDynEntDef.xModel->name);

        const char* destroyedxModel = "";
        if (currDynEntDef.destroyedxModel)
            destroyedxModel = cm_sanitiseJsonString(currDynEntDef.destroyedxModel->name);

        const char* destroyFx = "";
        if (currDynEntDef.destroyFx)
            destroyFx = cm_sanitiseJsonString(currDynEntDef.destroyFx->name);

        const char* physPreset = "";
        if (currDynEntDef.physPreset)
            physPreset = cm_sanitiseJsonString(currDynEntDef.physPreset->name);

        auto destroyPieces = nlohmann::json::object();
        if (currDynEntDef.destroyPieces)
        {
            auto destroyPieceParts = nlohmann::json::array();
            for (int k = 0; k < currDynEntDef.destroyPieces->numpieces; k++)
            {
                destroyPieceParts.push_back({
                    {"model",  cm_sanitiseJsonString(currDynEntDef.destroyPieces->pieces[i].model->name)},
                    {"offset",
                     {
                         {"x", currDynEntDef.destroyPieces->pieces[i].offset.x},
                         {"y", currDynEntDef.destroyPieces->pieces[i].offset.y},
                         {"z", currDynEntDef.destroyPieces->pieces[i].offset.z},
                     }                                                                                  },
                });
            }
            destroyPieces = nlohmann::json{
                {"name",      cm_sanitiseJsonString(currDynEntDef.destroyPieces->name)},
                {"numpieces", currDynEntDef.destroyPieces->numpieces                  },
                {"pieces",    destroyPieceParts                                       },
            };
        }
        
        dynEntDefList0.push_back({
            {"type",                currDynEntDef.type},
            {"quat", {
                {"x",               currDynEntDef.pose.quat.x},
                {"y",               currDynEntDef.pose.quat.y},
                {"z",               currDynEntDef.pose.quat.z},
                {"w",               currDynEntDef.pose.quat.w}}},
            {"origin", {
                {"x",               currDynEntDef.pose.origin.x},
                {"y",               currDynEntDef.pose.origin.y},
                {"z",               currDynEntDef.pose.origin.z}}},
            {"xModel",              xModel},
            {"destroyedxModel",     destroyedxModel},
            {"brushModel",          currDynEntDef.brushModel},
            {"physicsBrushModel",   currDynEntDef.physicsBrushModel},
            {"destroyFx",           destroyFx},
            {"destroySound",        currDynEntDef.destroySound},
            {"destroyPieces",       destroyPieces},
            {"physPreset",          physPreset},
            {"physConstraints0",    currDynEntDef.physConstraints[0]},
            {"physConstraints1",    currDynEntDef.physConstraints[1]},
            {"physConstraints2",    currDynEntDef.physConstraints[2]},
            {"physConstraints3",    currDynEntDef.physConstraints[3]},
            {"health",              currDynEntDef.health},
            {"flags",               currDynEntDef.flags},
            {"contents",            currDynEntDef.contents},
            {"targetname",          asset->m_zone->m_script_strings[currDynEntDef.targetname]},
            {"target",              asset->m_zone->m_script_strings[currDynEntDef.target]}
        });
    }
    j["dynEntDefList0"] = dynEntDefList0;


    auto dynEntDefList1 = nlohmann::json::array();
    for (int i = 0; i < clipMap->dynEntCount[1]; i++)
    {
        auto currDynEntDef = clipMap->dynEntDefList[1][i];

        const char* xModel = "";
        if (currDynEntDef.xModel)
            xModel = cm_sanitiseJsonString(currDynEntDef.xModel->name);

        const char* destroyedxModel = "";
        if (currDynEntDef.destroyedxModel)
            destroyedxModel = cm_sanitiseJsonString(currDynEntDef.destroyedxModel->name);

        const char* destroyFx = "";
        if (currDynEntDef.destroyFx)
            destroyFx = cm_sanitiseJsonString(currDynEntDef.destroyFx->name);

        const char* physPreset = "";
        if (currDynEntDef.physPreset)
            physPreset = cm_sanitiseJsonString(currDynEntDef.physPreset->name);

        auto destroyPieces = nlohmann::json::object();
        if (currDynEntDef.destroyPieces)
        {
            auto destroyPieceParts = nlohmann::json::array();
            for (int k = 0; k < currDynEntDef.destroyPieces->numpieces; k++)
            {
                destroyPieceParts.push_back({
                    {"model",  cm_sanitiseJsonString(currDynEntDef.destroyPieces->pieces[i].model->name)},
                    {"offset",
                     {
                         {"x", currDynEntDef.destroyPieces->pieces[i].offset.x},
                         {"y", currDynEntDef.destroyPieces->pieces[i].offset.y},
                         {"z", currDynEntDef.destroyPieces->pieces[i].offset.z},
                     }                                                                                  },
                });
            }
            destroyPieces = nlohmann::json{
                {"name",      cm_sanitiseJsonString(currDynEntDef.destroyPieces->name)},
                {"numpieces", currDynEntDef.destroyPieces->numpieces                  },
                {"pieces",    destroyPieceParts                                       },
            };
        }

        dynEntDefList1.push_back({
            {"type",              currDynEntDef.type                                                                                                                      },
            {"quat",              {{"x", currDynEntDef.pose.quat.x}, {"y", currDynEntDef.pose.quat.y}, {"z", currDynEntDef.pose.quat.z}, {"w", currDynEntDef.pose.quat.w}}},
            {"origin",            {{"x", currDynEntDef.pose.origin.x}, {"y", currDynEntDef.pose.origin.y}, {"z", currDynEntDef.pose.origin.z}}                            },
            {"xModel",            xModel                                                                                                                                  },
            {"destroyedxModel",   destroyedxModel                                                                                                                         },
            {"brushModel",        currDynEntDef.brushModel                                                                                                                },
            {"physicsBrushModel", currDynEntDef.physicsBrushModel                                                                                                         },
            {"destroyFx",         destroyFx                                                                                                                               },
            {"destroySound",      currDynEntDef.destroySound                                                                                                              },
            {"destroyPieces",     destroyPieces                                                                                                                           },
            {"physPreset",        physPreset                                                                                                                              },
            {"physConstraints0",  currDynEntDef.physConstraints[0]                                                                                                        },
            {"physConstraints1",  currDynEntDef.physConstraints[1]                                                                                                        },
            {"physConstraints2",  currDynEntDef.physConstraints[2]                                                                                                        },
            {"physConstraints3",  currDynEntDef.physConstraints[3]                                                                                                        },
            {"health",            currDynEntDef.health                                                                                                                    },
            {"flags",             currDynEntDef.flags                                                                                                                     },
            {"contents",          currDynEntDef.contents                                                                                                                  },
            {"targetname",        currDynEntDef.targetname                                                                                                                },
            {"target",            currDynEntDef.target                                                                                                                    }
        });
    }
    j["dynEntDefList1"] = dynEntDefList1;



    auto dynEntPoseList0 = nlohmann::json::array();
    for (int i = 0; i < clipMap->dynEntCount[0]; i++)
    {
        dynEntPoseList0.push_back({
            {"quat", {
                {"x", clipMap->dynEntPoseList[0][i].pose.quat.x},
                {"y", clipMap->dynEntPoseList[0][i].pose.quat.y},
                {"z", clipMap->dynEntPoseList[0][i].pose.quat.z},
                {"w", clipMap->dynEntPoseList[0][i].pose.quat.w}}},
            {"origin", {
                {"x",  clipMap->dynEntPoseList[0][i].pose.origin.x},
                {"y",  clipMap->dynEntPoseList[0][i].pose.origin.y},
                {"z",  clipMap->dynEntPoseList[0][i].pose.origin.z}}},
            {"radius", clipMap->dynEntPoseList[0][i].radius}
        });
    }
    j["dynEntPoseList0"] = dynEntPoseList0;

    auto dynEntPoseList1 = nlohmann::json::array();
    for (int i = 0; i < clipMap->dynEntCount[1]; i++)
    {
        dynEntPoseList1.push_back({
            {"quat",
             {{"x", clipMap->dynEntPoseList[1][i].pose.quat.x},
              {"y", clipMap->dynEntPoseList[1][i].pose.quat.y},
              {"z", clipMap->dynEntPoseList[1][i].pose.quat.z},
              {"w", clipMap->dynEntPoseList[1][i].pose.quat.w}}  },
            {"origin",
             {{"x", clipMap->dynEntPoseList[1][i].pose.origin.x},
              {"y", clipMap->dynEntPoseList[1][i].pose.origin.y},
              {"z", clipMap->dynEntPoseList[1][i].pose.origin.z}}},
            {"radius", clipMap->dynEntPoseList[1][i].radius      }
        });
    }
    j["dynEntPoseList1"] = dynEntPoseList1;



    auto dynEntClientList0 = nlohmann::json::array();
    for (int i = 0; i < clipMap->dynEntCount[0]; i++)
    {
        auto currentDynEntClient = clipMap->dynEntClientList[0][i];

        dynEntClientList0.push_back({
            {"physObjId", currentDynEntClient.physObjId},
            {"flags",  currentDynEntClient.flags},
            {"lightingHandle",     currentDynEntClient.lightingHandle},
            {"health", currentDynEntClient.health},
            {"burnTime",         currentDynEntClient.burnTime      },
            {"fadeTime",         currentDynEntClient.fadeTime      },
            {"physicsStartTime",         currentDynEntClient.physicsStartTime},
        });
    }
    j["dynEntClientList0"] = dynEntClientList0;

    auto dynEntClientList1 = nlohmann::json::array();
    for (int i = 0; i < clipMap->dynEntCount[1]; i++)
    {
        auto currentDynEntClient = clipMap->dynEntClientList[1][i];

        dynEntClientList1.push_back({
            {"physObjId",        currentDynEntClient.physObjId       },
            {"flags",            currentDynEntClient.flags           },
            {"lightingHandle",   currentDynEntClient.lightingHandle  },
            {"health",           currentDynEntClient.health          },
            {"burnTime",         currentDynEntClient.burnTime        },
            {"fadeTime",         currentDynEntClient.fadeTime        },
            {"physicsStartTime", currentDynEntClient.physicsStartTime},
        });
    }
    j["dynEntClientList1"] = dynEntClientList1;



    auto dynEntServerList0 = nlohmann::json::array();
    for (int i = 0; i < clipMap->dynEntCount[2]; i++)
    {
        auto currentDynEntServer = clipMap->dynEntServerList[0][i];

        dynEntServerList0.push_back({
            {"flags",  currentDynEntServer.flags },
            {"health", currentDynEntServer.health}
        });
    }
    j["dynEntServerList0"] = dynEntServerList0;

    auto dynEntServerList1 = nlohmann::json::array();
    for (int i = 0; i < clipMap->dynEntCount[3]; i++)
    {
        auto currentDynEntServer = clipMap->dynEntServerList[1][i];

        dynEntServerList1.push_back({
            {"flags", currentDynEntServer.flags},
            {"health", currentDynEntServer.health}
        });
    }
    j["dynEntServerList1"] = dynEntServerList1;



    auto dynEntCollList0 = nlohmann::json::array();
    for (int i = 0; i < clipMap->dynEntCount[0]; i++)
    {
        auto currentDynEntColl = clipMap->dynEntCollList[0][i];

        dynEntCollList0.push_back({
            {"sector", currentDynEntColl.sector},
            {"nextEntInSector", currentDynEntColl.nextEntInSector},
            {"linkMins", {
                {"x", currentDynEntColl.linkMins.x},
                {"y", currentDynEntColl.linkMins.y},
                {"z", currentDynEntColl.linkMins.z}}},
            {"linkMaxs", {
                {"x", currentDynEntColl.linkMaxs.x},
                {"y", currentDynEntColl.linkMaxs.y},
                {"z", currentDynEntColl.linkMaxs.z}}},
            {"contents", currentDynEntColl.contents},
        });
    }
    j["dynEntCollList0"] = dynEntCollList0;

    auto dynEntCollList1 = nlohmann::json::array();
    for (int i = 0; i < clipMap->dynEntCount[1]; i++)
    {
        auto currentDynEntColl = clipMap->dynEntCollList[1][i];

        dynEntCollList1.push_back({
            {"sector",          currentDynEntColl.sector                                                                                       },
            {"nextEntInSector", currentDynEntColl.nextEntInSector                                                                              },
            {"linkMins",        {{"x", currentDynEntColl.linkMins.x}, {"y", currentDynEntColl.linkMins.y}, {"z", currentDynEntColl.linkMins.z}}},
            {"linkMaxs",        {{"x", currentDynEntColl.linkMaxs.x}, {"y", currentDynEntColl.linkMaxs.y}, {"z", currentDynEntColl.linkMaxs.z}}},
            {"contents",        currentDynEntColl.contents                                                                                     },
        });
    }
    j["dynEntCollList1"] = dynEntCollList1;
    
    auto dynEntCollList2 = nlohmann::json::array();
    for (int i = 0; i < clipMap->dynEntCount[2]; i++)
    {
        auto currentDynEntColl = clipMap->dynEntCollList[2][i];

        dynEntCollList2.push_back({
            {"sector",          currentDynEntColl.sector                                                                                       },
            {"nextEntInSector", currentDynEntColl.nextEntInSector                                                                              },
            {"linkMins",        {{"x", currentDynEntColl.linkMins.x}, {"y", currentDynEntColl.linkMins.y}, {"z", currentDynEntColl.linkMins.z}}},
            {"linkMaxs",        {{"x", currentDynEntColl.linkMaxs.x}, {"y", currentDynEntColl.linkMaxs.y}, {"z", currentDynEntColl.linkMaxs.z}}},
            {"contents",        currentDynEntColl.contents                                                                                     },
        });
    }
    j["dynEntCollList2"] = dynEntCollList2;

    auto dynEntCollList3 = nlohmann::json::array();
    for (int i = 0; i < clipMap->dynEntCount[3]; i++)
    {
        auto currentDynEntColl = clipMap->dynEntCollList[3][i];

        dynEntCollList3.push_back({
            {"sector",          currentDynEntColl.sector                                                                                       },
            {"nextEntInSector", currentDynEntColl.nextEntInSector                                                                              },
            {"linkMins",        {{"x", currentDynEntColl.linkMins.x}, {"y", currentDynEntColl.linkMins.y}, {"z", currentDynEntColl.linkMins.z}}},
            {"linkMaxs",        {{"x", currentDynEntColl.linkMaxs.x}, {"y", currentDynEntColl.linkMaxs.y}, {"z", currentDynEntColl.linkMaxs.z}}},
            {"contents",        currentDynEntColl.contents                                                                                     },
        });
    }
    j["dynEntCollList3"] = dynEntCollList3;
    
    j["num_constraints"] = clipMap->num_constraints;
    auto constraints = nlohmann::json::array();
    for (int i = 0; i < clipMap->num_constraints; i++)
    {
        auto currentConstraint = clipMap->constraints[i];

        const char* matName = "";
        if (currentConstraint.material)
            matName = cm_sanitiseJsonString(currentConstraint.material->info.name);

        constraints.push_back({
            {"targetname",         currentConstraint.targetname        },
            {"type",               currentConstraint.type              },
            {"attach_point_type1", currentConstraint.attach_point_type1},
            {"target_index1",      currentConstraint.target_index1     },
            {"target_ent1",        currentConstraint.target_ent1       },
            {"target_bone1",       cm_sanitiseJsonString(currentConstraint.target_bone1)                                                      },
            {"attach_point_type2", currentConstraint.attach_point_type2},
            {"target_index2",      currentConstraint.target_index2     },
            {"target_ent2",        currentConstraint.target_ent2       },
            {"target_bone2",       cm_sanitiseJsonString(currentConstraint.target_bone2)                                                      },

            {"offset",             {{"x", currentConstraint.offset.x}, {"y", currentConstraint.offset.y}, {"z", currentConstraint.offset.z}}},

            {"pos",                {{"x", currentConstraint.pos.x}, {"y", currentConstraint.pos.y}, {"z", currentConstraint.pos.z}}         },

            {"pos2",               {{"x", currentConstraint.pos2.x}, {"y", currentConstraint.pos2.y}, {"z", currentConstraint.pos2.z}}      },

            {"dir",                {{"x", currentConstraint.dir.x}, {"y", currentConstraint.dir.y}, {"z", currentConstraint.dir.z}}         },

            {"flags",              currentConstraint.flags             },
            {"timeout",            currentConstraint.timeout           },
            {"min_health",         currentConstraint.min_health        },
            {"max_health",         currentConstraint.max_health        },
            {"distance",           currentConstraint.distance          },
            {"damp",               currentConstraint.damp              },
            {"power",              currentConstraint.power             },

            {"scale",              {{"x", currentConstraint.scale.x}, {"y", currentConstraint.scale.y}, {"z", currentConstraint.scale.z}}   },

            {"spin_scale",         currentConstraint.spin_scale        },
            {"minAngle",           currentConstraint.minAngle          },
            {"maxAngle",           currentConstraint.maxAngle          },

            {"material",           matName                             },

            {"constraintHandle",   currentConstraint.constraintHandle  },
            {"rope_index",         currentConstraint.rope_index        },
            {"centity_num0",       currentConstraint.centity_num[0]    },
            {"centity_num1",       currentConstraint.centity_num[1]    },
            {"centity_num2",       currentConstraint.centity_num[2]    },
            {"centity_num3",       currentConstraint.centity_num[3]    },
        });
    }
    j["constraints"] = constraints;

    // ropes have the last 32 spots empty for ropes being added during the game
    // It is empty and data is generated during map loading from the constraints (above)
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    j["max_ropes"] = clipMap->max_ropes;
    printf("%i\n", clipMap->max_ropes);
    //assert(clipMap->num_constraints + 32 == clipMap->max_ropes);
    //j["max_ropes"] = clipMap->num_constraints + 32;
    j["ropes"] = nlohmann::json::array();

    j["checksum"] = clipMap->checksum;

    std::string jsonString = j.dump(4);
    stream.write(jsonString.c_str(), jsonString.size());
}
