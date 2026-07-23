#pragma once

#include "BSP.h"

namespace T6
{
    namespace BSP
    {
        class BSPUtil
        {
        public:
            static std::string getFileNameForBSPAsset(const std::string& assetName);
            static void updateAABB(vec3_t& newAABBMins, vec3_t& newAABBMaxs, vec3_t& AABBMins, vec3_t& AABBMaxs);
            static void updateAABBWithPoint(vec3_t& point, vec3_t& AABBMins, vec3_t& AABBMaxs);
            static vec3_t calcMiddleOfAABB(vec3_t& mins, vec3_t& maxs);
            static vec3_t calcHalfSizeOfAABB(vec3_t& mins, vec3_t& maxs);
            static size_t allignBy128(size_t size);
            static float distBetweenPoints(vec3_t& p1, vec3_t& p2);
            static void calculateXmodelGfxBounds(XModel* xmodel, vec3_t axis[3], vec3_t& out_mins, vec3_t& out_maxs);
            static void calculateXmodelColBounds(XModel* xmodel, vec3_t axis[3], vec3_t& out_mins, vec3_t& out_maxs);
            static void convertAnglesToAxis(vec3_t* angles, vec3_t axis[3]);
            static vec3_t convertAnglesToForward(vec3_t& angles);
            static void convertQuaternionToAxis(vec4_t* quat, vec3_t axis[3]);
            static vec3_t convertQuaternionToForwardVector(vec4_t* quat);
            static vec3_t convertForwardVectorToViewAngles(vec3_t& forwardVec);
            static float getPitchFromVector(vec3_t& vector);
            static vec3_t convertAxisToAngles(vec3_t axis[3]);
            static vec3_t convertQuatToAngles(vec4_t* quat);
            static void matrixTranspose3x3(const vec3_t* in, vec3_t* out);
            static vec3_t convertStringToVec3(const char* str);
            static std::string convertVec3ToString(vec3_t& vec);

            static vec4_t convertAnglesToQuat(vec3_t& angles);
            static vec4_t convertAxisToQuat(vec3_t axis[3]);
            static inline float lengthSquaredOfQuat(float quat[4]);

            static bool flagsMatchExact(int test_flag, int inFlags);
            static bool flagsMatchAny(int flag1, int flag2);
        };
    } // namespace BSP

} // namespace T6
