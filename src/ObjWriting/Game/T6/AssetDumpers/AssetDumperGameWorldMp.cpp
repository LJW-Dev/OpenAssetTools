#include <nlohmann/json.hpp>

#include "AssetDumperGameWorldMp.h"

using namespace T6;

bool AssetDumperGameWorldMp::ShouldDump(XAssetInfo<GameWorldMp>* asset)
{
    return true;
}

const char* emptyString = "";
const char* sanitiseJsonString(const char* str)
{
    if (str == NULL)
        return emptyString;
    else
        return str;
}

void AssetDumperGameWorldMp::DumpAsset(AssetDumpingContext& context, XAssetInfo<GameWorldMp>* asset)
{
    const auto* gameWorld = asset->Asset();
    const auto assetFile = context.OpenAssetFile("bsp/gameworldmp.json");

    if (!assetFile)
        return;

    auto& stream = *assetFile;

    nlohmann::json j;

    j["name"] = sanitiseJsonString(gameWorld->name);
    j["path"]["nodeCount"] = gameWorld->path.nodeCount;
    j["path"]["originalNodeCount"] = gameWorld->path.originalNodeCount;
    auto pathNodes = nlohmann::json::array();
    for (unsigned int i = 0; i < gameWorld->path.nodeCount + 128; i++)
    {
        auto currentNode = gameWorld->path.nodes[i];

        assert(currentNode.transient.pNextOpen == nullptr);
        assert(currentNode.transient.pPrevOpen == nullptr);
        assert(currentNode.transient.pParent == nullptr);

        auto constantLinks = nlohmann::json::array();
        for (int k = 0; k < currentNode.constant.totalLinkCount; k++)
        {
            auto currentLink = currentNode.constant.Links[k];
            
            constantLinks.push_back({
                {"fDist", currentLink.fDist},
                {"nodeNum", currentLink.nodeNum},
                {"disconnectCount", currentLink.disconnectCount},
                {"negotiationLink", currentLink.negotiationLink},
                {"flags",           currentLink.flags          },
                {"ubBadPlaceCount0", currentLink.ubBadPlaceCount[0]},
                {"ubBadPlaceCount1", currentLink.ubBadPlaceCount[1]},
                {"ubBadPlaceCount2", currentLink.ubBadPlaceCount[2]},
                {"ubBadPlaceCount3", currentLink.ubBadPlaceCount[3]},
                {"ubBadPlaceCount4", currentLink.ubBadPlaceCount[4]},
            });
        }
        
        pathNodes.push_back({
            {"constant", {
                 {"type", currentNode.constant.type},
                 {"spawnflags", currentNode.constant.spawnflags},
                 {"targetname", asset->m_zone->m_script_strings[currentNode.constant.targetname]},
                 {"script_linkName",  asset->m_zone->m_script_strings[currentNode.constant.script_linkName]},
                 {"script_noteworthy",  asset->m_zone->m_script_strings[currentNode.constant.script_noteworthy]},
                 {"target",  asset->m_zone->m_script_strings[currentNode.constant.target]},
                 {"animscript",  asset->m_zone->m_script_strings[currentNode.constant.animscript]},
                 {"animscriptfunc", currentNode.constant.animscriptfunc},
                 {"vOrigin", {{"x", currentNode.constant.vOrigin.x}, {"y", currentNode.constant.vOrigin.y}, {"z", currentNode.constant.vOrigin.z}}},
                 {"fAngle", currentNode.constant.fAngle},
                 {"forward", {{"x", currentNode.constant.forward.x}, {"y", currentNode.constant.forward.y}}},
                 {"fRadius", currentNode.constant.fRadius},
                 {"minUseDistSq", currentNode.constant.minUseDistSq},
                 {"wOverlapNode0", currentNode.constant.wOverlapNode[0]},
                 {"wOverlapNode1", currentNode.constant.wOverlapNode[1]},
                 {"totalLinkCount", currentNode.constant.totalLinkCount},
                 {"Links", constantLinks},
            }},
            {"dynamic", {
                 {"pOwner_number", currentNode.dynamic.pOwner.number},
                 {"pOwner_infoIndex", currentNode.dynamic.pOwner.infoIndex},
                 {"iFreeTime", currentNode.dynamic.iFreeTime},
                 {"iFreeTime", currentNode.dynamic.iFreeTime},
                 {"iValidTime0", currentNode.dynamic.iValidTime[0]},
                 {"iValidTime1", currentNode.dynamic.iValidTime[1]},
                 {"iValidTime2", currentNode.dynamic.iValidTime[2]},
                 {"dangerousNodeTime0", currentNode.dynamic.dangerousNodeTime[0]},
                 {"dangerousNodeTime1", currentNode.dynamic.dangerousNodeTime[1]},
                 {"dangerousNodeTime2", currentNode.dynamic.dangerousNodeTime[2]},
                 {"inPlayerLOSTime", currentNode.dynamic.inPlayerLOSTime},
                 {"wLinkCount", currentNode.dynamic.wLinkCount},
                 {"wOverlapCount", currentNode.dynamic.wOverlapCount},
                 {"turretEntNumber", currentNode.dynamic.turretEntNumber},
                 {"userCount", currentNode.dynamic.userCount},
                 {"dynamic", currentNode.dynamic.hasBadPlaceLink},
            }},
            {"transient", {
                 {"iSearchFrame", currentNode.transient.iSearchFrame},
                 {"fCost", currentNode.transient.fCost},
                 {"fHeuristic", currentNode.transient.fHeuristic},
                 {"linkIndex", currentNode.transient.linkIndex},
            }},
        });
    }
    j["path"]["nodes"] = pathNodes;

    auto baseNodes = nlohmann::json::array();
    for (unsigned int i = 0; i < gameWorld->path.nodeCount + 128; i++)
    {
        auto currentBaseNode = gameWorld->path.basenodes[i];

        baseNodes.push_back({
            {"type", currentBaseNode.type},
            {"vOrigin", {{"x", currentBaseNode.vOrigin.x}, {"y", currentBaseNode.vOrigin.y}, {"z", currentBaseNode.vOrigin.z}}},
        });
    }
    j["path"]["basenodes"] = baseNodes;

    j["path"]["visBytes"] = gameWorld->path.visBytes;
    j["path"]["pathVis_Filename"] = "bsp/gameworldmp/pathVis";
    if (gameWorld->path.visBytes > 0)
    {
        assert(gameWorld->path.pathVis);
        auto pathVisFile = context.OpenAssetFile("bsp/gameworldmp/pathVis");
        if (pathVisFile)
        {
            auto& pathVisStream = *pathVisFile;
            pathVisStream.write(gameWorld->path.pathVis, gameWorld->path.visBytes);
        }
    }

    j["path"]["smoothBytes"] = gameWorld->path.smoothBytes;
    j["path"]["smoothCache_Filename"] = "bsp/gameworldmp/smoothCache";
    if (gameWorld->path.smoothBytes > 0)
    {
        assert(gameWorld->path.smoothCache);
        auto smoothCacheFile = context.OpenAssetFile("bsp/gameworldmp/smoothCache");
        if (smoothCacheFile)
        {
            auto& smoothCacheStream = *smoothCacheFile;
            smoothCacheStream.write(gameWorld->path.smoothCache, gameWorld->path.smoothBytes);
        }
    }

    j["path"]["nodeTreeCount"] = gameWorld->path.nodeTreeCount;
    auto nodeTree = nlohmann::json::array();
    for (int i = 0; i < gameWorld->path.nodeTreeCount; i++)
    {
        auto currentNodeTree = gameWorld->path.nodeTree[i];

        auto treeInfo = nlohmann::json::object();
        if (currentNodeTree.axis < 0)
        {
            treeInfo["nodeCount"] = currentNodeTree.u.s.nodeCount;
            auto treeInfoNodes = nlohmann::json::array();
            for (int k = 0; k < currentNodeTree.u.s.nodeCount; k++)
                treeInfoNodes.push_back(currentNodeTree.u.s.nodes[k]);
            treeInfo["nodes"] = treeInfoNodes;
        }
        else
        {
            // find the node tree child that is referenced
            int child0ReferenceIndex = -1;
            int child1ReferenceIndex = -1;
            for (int k = 0; k < gameWorld->path.nodeTreeCount; k++)
            {
                if (&gameWorld->path.nodeTree[k] == currentNodeTree.u.child[0])
                {
                    child0ReferenceIndex = k;
                }
                if (&gameWorld->path.nodeTree[k] == currentNodeTree.u.child[1])
                {
                    child1ReferenceIndex = k;
                }
            }
            assert(child0ReferenceIndex != -1);
            assert(child1ReferenceIndex != -1);

            treeInfo["child0ReferenceIndex"] = child0ReferenceIndex;
            treeInfo["child1ReferenceIndex"] = child1ReferenceIndex;
        }

        nodeTree.push_back({
            {"axis", currentNodeTree.axis},
            {"dist", currentNodeTree.dist},
            {"u",    treeInfo            },
        });
    }
    j["path"]["nodeTree"] = nodeTree;

    std::string jsonString = j.dump(4);
    stream.write(jsonString.c_str(), jsonString.size());
}
