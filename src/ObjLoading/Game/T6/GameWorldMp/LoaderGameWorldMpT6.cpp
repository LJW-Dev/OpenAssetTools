#include <nlohmann/json.hpp>

#include "LoaderGameWorldMpT6.h"

#include "Game/T6/T6.h"

#include <cstring>

using namespace T6;

namespace
{
    class GameWorldMpLoader final : public AssetCreator<AssetGameWorldMp>
    {
    public:
        GameWorldMpLoader(MemoryManager& memory, ISearchPath& searchPath, Zone& zone)
            : m_memory(memory),
              m_search_path(searchPath),
              m_zone(zone)
        {
        }

        AssetCreationResult CreateAsset(const std::string& assetName, AssetCreationContext& context) override
        {
            const auto file = m_search_path.Open("bsp/gameworldmp.json");
            if (!file.IsOpen())
                return AssetCreationResult::NoAction();

            nlohmann::json j = nlohmann::json::parse(*file.m_stream);
            
            auto* gameWorld = m_memory.Alloc<GameWorldMp>();

            gameWorld->name = m_memory.Dup(assetName.c_str());
            gameWorld->path.nodeCount = j["path"]["nodeCount"];
            gameWorld->path.originalNodeCount = j["path"]["originalNodeCount"];
            gameWorld->path.visBytes = j["path"]["visBytes"];
            gameWorld->path.smoothBytes = j["path"]["smoothBytes"];
            gameWorld->path.nodeTreeCount = j["path"]["nodeTreeCount"];

            gameWorld->path.nodes = m_memory.Alloc<pathnode_t>(gameWorld->path.nodeCount + 128);
            for (unsigned int i = 0; i < gameWorld->path.nodeCount + 128; i++)
            {
                auto currentNode = &gameWorld->path.nodes[i];
                auto currentNodeData = j["path"]["nodes"][i];

                currentNode->constant.type = currentNodeData["constant"]["type"];
                currentNode->constant.spawnflags = currentNodeData["constant"]["spawnflags"];
                
                //currentNode->constant.targetname = currentNodeData["constant"]["targetname"];
                //currentNode->constant.script_linkName = currentNodeData["constant"]["script_linkName"];
                //currentNode->constant.script_noteworthy = currentNodeData["constant"]["script_noteworthy"];
                //currentNode->constant.target = currentNodeData["constant"]["target"];
                //currentNode->constant.animscript = currentNodeData["constant"]["animscript"];
                
                currentNode->constant.targetname = m_zone.m_script_strings.AddOrGetScriptString(currentNodeData["constant"]["targetname"]);
                currentNode->constant.script_linkName = m_zone.m_script_strings.AddOrGetScriptString(currentNodeData["constant"]["script_linkName"]);
                currentNode->constant.script_noteworthy = m_zone.m_script_strings.AddOrGetScriptString(currentNodeData["constant"]["script_noteworthy"]);
                currentNode->constant.target = m_zone.m_script_strings.AddOrGetScriptString(currentNodeData["constant"]["target"]);
                currentNode->constant.animscript = m_zone.m_script_strings.AddOrGetScriptString(currentNodeData["constant"]["animscript"]);
                
                currentNode->constant.animscriptfunc = currentNodeData["constant"]["animscriptfunc"];
                currentNode->constant.vOrigin.x = currentNodeData["constant"]["vOrigin"]["x"];
                currentNode->constant.vOrigin.y = currentNodeData["constant"]["vOrigin"]["y"];
                currentNode->constant.vOrigin.z = currentNodeData["constant"]["vOrigin"]["z"];
                currentNode->constant.forward.x = currentNodeData["constant"]["forward"]["x"];
                currentNode->constant.forward.y = currentNodeData["constant"]["forward"]["y"];
                currentNode->constant.fAngle = currentNodeData["constant"]["fAngle"];
                currentNode->constant.fRadius = currentNodeData["constant"]["fRadius"];
                currentNode->constant.minUseDistSq = currentNodeData["constant"]["minUseDistSq"];
                currentNode->constant.wOverlapNode[0] = currentNodeData["constant"]["wOverlapNode0"];
                currentNode->constant.wOverlapNode[1] = currentNodeData["constant"]["wOverlapNode1"];
                currentNode->constant.totalLinkCount = currentNodeData["constant"]["totalLinkCount"];

                currentNode->constant.Links = m_memory.Alloc<pathlink_s>(currentNode->constant.totalLinkCount);
                for (int k = 0; k < currentNode->constant.totalLinkCount; k++)
                {
                    auto currentLink = &currentNode->constant.Links[k];
                    auto currentLinkData = currentNodeData["constant"]["Links"][k];
                    
                    currentLink->fDist = currentLinkData["fDist"];
                    currentLink->nodeNum = currentLinkData["nodeNum"];
                    currentLink->disconnectCount = static_cast<unsigned char>(currentLinkData["disconnectCount"]);
                    currentLink->negotiationLink = static_cast<unsigned char>(currentLinkData["negotiationLink"]);
                    currentLink->flags = static_cast<unsigned char>(currentLinkData["flags"]);
                    currentLink->ubBadPlaceCount[0] = static_cast<unsigned char>(currentLinkData["ubBadPlaceCount0"]);
                    currentLink->ubBadPlaceCount[1] = static_cast<unsigned char>(currentLinkData["ubBadPlaceCount1"]);
                    currentLink->ubBadPlaceCount[2] = static_cast<unsigned char>(currentLinkData["ubBadPlaceCount2"]);
                    currentLink->ubBadPlaceCount[3] = static_cast<unsigned char>(currentLinkData["ubBadPlaceCount3"]);
                    currentLink->ubBadPlaceCount[4] = static_cast<unsigned char>(currentLinkData["ubBadPlaceCount4"]);
                }

                currentNode->dynamic.pOwner.number = currentNodeData["dynamic"]["pOwner_number"];
                currentNode->dynamic.pOwner.infoIndex = currentNodeData["dynamic"]["pOwner_infoIndex"];
                currentNode->dynamic.iFreeTime = currentNodeData["dynamic"]["iFreeTime"];
                currentNode->dynamic.iFreeTime = currentNodeData["dynamic"]["iFreeTime"];
                currentNode->dynamic.iValidTime[0] = currentNodeData["dynamic"]["iValidTime0"];
                currentNode->dynamic.iValidTime[1] = currentNodeData["dynamic"]["iValidTime1"];
                currentNode->dynamic.iValidTime[2] = currentNodeData["dynamic"]["iValidTime2"];
                currentNode->dynamic.dangerousNodeTime[0] = currentNodeData["dynamic"]["dangerousNodeTime0"];
                currentNode->dynamic.dangerousNodeTime[1] = currentNodeData["dynamic"]["dangerousNodeTime1"];
                currentNode->dynamic.dangerousNodeTime[2] = currentNodeData["dynamic"]["dangerousNodeTime2"];
                currentNode->dynamic.inPlayerLOSTime = currentNodeData["dynamic"]["inPlayerLOSTime"];
                currentNode->dynamic.wLinkCount = currentNodeData["dynamic"]["wLinkCount"];
                currentNode->dynamic.wOverlapCount = currentNodeData["dynamic"]["wOverlapCount"];
                currentNode->dynamic.turretEntNumber = currentNodeData["dynamic"]["turretEntNumber"];
                currentNode->dynamic.userCount = currentNodeData["dynamic"]["userCount"];
                currentNode->dynamic.hasBadPlaceLink = currentNodeData["dynamic"]["dynamic"];

                currentNode->transient.iSearchFrame = currentNodeData["transient"]["iSearchFrame"];
                currentNode->transient.fCost = currentNodeData["transient"]["fCost"];
                currentNode->transient.fHeuristic = currentNodeData["transient"]["fHeuristic"];
                currentNode->transient.linkIndex = currentNodeData["transient"]["linkIndex"];
                currentNode->transient.pNextOpen = nullptr;
                currentNode->transient.pPrevOpen = nullptr;
                currentNode->transient.pParent = nullptr;
            }

            gameWorld->path.basenodes = m_memory.Alloc<pathbasenode_t>(gameWorld->path.nodeCount + 128);
            auto baseNodes = nlohmann::json::array();
            for (unsigned int i = 0; i < gameWorld->path.nodeCount + 128; i++)
            {
                auto currentBaseNode = &gameWorld->path.basenodes[i];
                auto currentBaseNodeData = j["path"]["basenodes"][i];

                currentBaseNode->type = currentBaseNodeData["type"];
                currentBaseNode->vOrigin.x = currentBaseNodeData["vOrigin"]["x"];
                currentBaseNode->vOrigin.y = currentBaseNodeData["vOrigin"]["y"];
                currentBaseNode->vOrigin.z = currentBaseNodeData["vOrigin"]["z"];
            }

            auto pathVis_Filename = j["path"]["pathVis_Filename"];
            if (gameWorld->path.visBytes > 0)
            {
                gameWorld->path.pathVis = m_memory.Alloc<char>(gameWorld->path.visBytes);
                auto pathVisFile = m_search_path.Open(pathVis_Filename);
                if (pathVisFile.IsOpen())
                {
                    auto& pathVisStream = *pathVisFile.m_stream;
                    pathVisStream.read(gameWorld->path.pathVis, gameWorld->path.visBytes);
                }
            }
            else
                gameWorld->path.pathVis = nullptr;

            auto smoothCache_Filename = j["path"]["smoothCache_Filename"];
            if (gameWorld->path.smoothBytes > 0)
            {
                gameWorld->path.smoothCache = m_memory.Alloc<char>(gameWorld->path.smoothBytes);
                auto smoothCacheFile = m_search_path.Open(smoothCache_Filename);
                if (smoothCacheFile.IsOpen())
                {
                    auto& smoothCacheStream = *smoothCacheFile.m_stream;
                    smoothCacheStream.read(gameWorld->path.smoothCache, gameWorld->path.smoothBytes);
                }
            }
            else
                 gameWorld->path.smoothCache= nullptr;

            if (gameWorld->path.nodeTreeCount > 0)
                 gameWorld->path.nodeTree = m_memory.Alloc<pathnode_tree_t>(gameWorld->path.nodeTreeCount);
            else
                 gameWorld->path.nodeTree = nullptr;
            for (int i = 0; i < gameWorld->path.nodeTreeCount; i++)
            {
                auto currentNodeTree = &gameWorld->path.nodeTree[i];
                auto currentNodeTreeData = j["path"]["nodeTree"][i];

                currentNodeTree->axis = currentNodeTreeData["axis"];
                currentNodeTree->dist = currentNodeTreeData["dist"];

                if (currentNodeTree->axis < 0)
                {
                    currentNodeTree->u.s.nodeCount = currentNodeTreeData["u"]["nodeCount"];

                    currentNodeTree->u.s.nodes = m_memory.Alloc<uint16_t>(currentNodeTree->u.s.nodeCount);
                    for (int k = 0; k < currentNodeTree->u.s.nodeCount; k++)
                        currentNodeTree->u.s.nodes[k] = currentNodeTreeData["u"]["nodes"][k];
                }
                else
                {
                    unsigned int child0Index = currentNodeTreeData["u"]["child0ReferenceIndex"];
                    unsigned int child1Index = currentNodeTreeData["u"]["child1ReferenceIndex"];

                    currentNodeTree->u.child[0] = &gameWorld->path.nodeTree[child0Index];
                    currentNodeTree->u.child[1] = &gameWorld->path.nodeTree[child1Index];
                }
            }

            return AssetCreationResult::Success(context.AddAsset<AssetGameWorldMp>(assetName, gameWorld));
        }

    private:
        MemoryManager& m_memory;
        ISearchPath& m_search_path;
        Zone& m_zone;
    };
} // namespace

namespace T6
{
    std::unique_ptr<AssetCreator<AssetGameWorldMp>> CreateGameWorldMpLoader(MemoryManager& memory, ISearchPath& searchPath, Zone& zone)
    {
        return std::make_unique<GameWorldMpLoader>(memory, searchPath, zone);
    }
} // namespace T6
