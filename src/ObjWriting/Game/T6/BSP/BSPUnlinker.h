#pragma once

#include "BSP/BSP.h"
#include "Game/T6/T6.h"

#include <string>
#include <vector>

namespace T6
{
    namespace BSP
    {
        struct BSPAssetPtrs
        {
            const T6::MapEnts* mapEnts;
            const T6::GameWorldMp* gameWorldMp;
            const T6::ComWorld* comworld;
            const T6::GfxWorld* gfxworld;
            const T6::clipMap_t* clipmap;
            const T6::SkinnedVertsDef* skinnedverts;
            std::vector<T6::GfxLightDef*> lightDefs;
        };

        void dumpBSPData(BSPData& dumpData, std::string zoneName, BSPAssetPtrs& assetPtrs);
    } // namespace BSP
} // namespace T6
