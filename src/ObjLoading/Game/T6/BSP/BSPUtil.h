#pragma once

#include "BSP.h"

namespace BSP
{
    class BSPUtil
    {
    public:
        static float distBetweenPoints(T6::vec3_t& p1, T6::vec3_t& p2);
        static std::string getFileNameForBSPAsset(std::string& assetName);
        static void updateAABB(T6::vec3_t& newAABBMins, T6::vec3_t& newAABBMaxs, T6::vec3_t& AABBMins, T6::vec3_t& AABBMaxs);
        static size_t allignBy128(size_t size);
    };
} // namespace BSP
