#pragma once

#include "BSPUtil.h"

#include <cmath>
#include <format>

namespace BSP
{
    float BSPUtil::distBetweenPoints(T6::vec3_t& p1, T6::vec3_t& p2)
    {
        float x = p2.x - p1.x;
        float y = p2.y - p1.y;
        float z = p2.z - p1.z;
        return sqrtf((x * x) + (y * y) + (z * z));
    }

    std::string BSPUtil::getFileNameForBSPAsset(std::string& assetName)
    {
        return std::format("BSP/{}", assetName);
    }

    void BSPUtil::updateAABB(T6::vec3_t& newAABBMins, T6::vec3_t& newAABBMaxs, T6::vec3_t& AABBMins, T6::vec3_t& AABBMaxs)
    {
        if (AABBMins.x > newAABBMins.x)
            AABBMins.x = newAABBMins.x;

        if (newAABBMaxs.x > AABBMaxs.x)
            AABBMaxs.x = newAABBMaxs.x;

        if (AABBMins.y > newAABBMins.y)
            AABBMins.y = newAABBMins.y;

        if (newAABBMaxs.y > AABBMaxs.y)
            AABBMaxs.y = newAABBMaxs.y;

        if (AABBMins.z > newAABBMins.z)
            AABBMins.z = newAABBMins.z;

        if (newAABBMaxs.z > AABBMaxs.z)
            AABBMaxs.z = newAABBMaxs.z;
    }

    size_t BSPUtil::allignBy128(size_t size)
    {
        return ((size + 127) & 0xFFFFFF80);
    }
} // namespace BSP
