#pragma once

#include "BSPUtil.h"

#include <cmath>
#include <format>
#include <numbers>

using namespace T6;
using namespace BSP;

std::string BSPUtil::getFileNameForBSPAsset(const std::string& assetName)
{
    return std::format("BSP/{}", assetName);
}

void BSPUtil::updateAABB(vec3_t& newAABBMins, vec3_t& newAABBMaxs, vec3_t& AABBMins, vec3_t& AABBMaxs)
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

void BSPUtil::updateAABBWithPoint(vec3_t& point, vec3_t& AABBMins, vec3_t& AABBMaxs)
{
    if (AABBMins.x > point.x)
        AABBMins.x = point.x;

    if (point.x > AABBMaxs.x)
        AABBMaxs.x = point.x;

    if (AABBMins.y > point.y)
        AABBMins.y = point.y;

    if (point.y > AABBMaxs.y)
        AABBMaxs.y = point.y;

    if (AABBMins.z > point.z)
        AABBMins.z = point.z;

    if (point.z > AABBMaxs.z)
        AABBMaxs.z = point.z;
}

vec3_t BSPUtil::calcMiddleOfAABB(vec3_t& mins, vec3_t& maxs)
{
    vec3_t result;
    result.x = (mins.x + maxs.x) * 0.5f;
    result.y = (mins.y + maxs.y) * 0.5f;
    result.z = (mins.z + maxs.z) * 0.5f;
    return result;
}

vec3_t BSPUtil::calcHalfSizeOfAABB(vec3_t& mins, vec3_t& maxs)
{
    vec3_t result;
    result.x = (maxs.x - mins.x) * 0.5f;
    result.y = (maxs.y - mins.y) * 0.5f;
    result.z = (maxs.z - mins.z) * 0.5f;
    return result;
}

size_t BSPUtil::allignBy128(size_t size)
{
    return ((size + 127) & 0xFFFFFF80);
}

float BSPUtil::distBetweenPoints(vec3_t& p1, vec3_t& p2)
{
    float x = p2.x - p1.x;
    float y = p2.y - p1.y;
    float z = p2.z - p1.z;
    return sqrtf((x * x) + (y * y) + (z * z));
}

void BSPUtil::calculateXmodelGfxBounds(XModel* xmodel, vec3_t axis[3], vec3_t& out_mins, vec3_t& out_maxs)
{
    out_mins.x = 0.0f;
    out_mins.y = 0.0f;
    out_mins.z = 0.0f;
    out_maxs.x = 0.0f;
    out_maxs.y = 0.0f;
    out_maxs.z = 0.0f;

    for (auto surfaceIndex = 0u; surfaceIndex < xmodel->lodInfo[0].numsurfs; surfaceIndex++)
    {
        const auto& surface = xmodel->surfs[surfaceIndex + xmodel->lodInfo[0].surfIndex];

        if (!surface.verts0)
            continue;

        for (auto vertIndex = 0u; vertIndex < surface.vertCount; vertIndex++)
        {
            const auto& vertex = surface.verts0[vertIndex].xyz;

            vec3_t rotatedVert;
            rotatedVert.x = (vertex.x * axis[0].x) + (vertex.y * axis[1].x) + (vertex.z * axis[2].x);
            rotatedVert.y = (vertex.x * axis[0].y) + (vertex.y * axis[1].y) + (vertex.z * axis[2].y);
            rotatedVert.z = (vertex.x * axis[0].z) + (vertex.y * axis[1].z) + (vertex.z * axis[2].z);

            if (vertIndex == 0 && surfaceIndex == 0)
            {
                out_mins = rotatedVert;
                out_maxs = rotatedVert;
            }
            else
                BSPUtil::updateAABBWithPoint(rotatedVert, out_mins, out_maxs);
        }
    }
}

void BSPUtil::calculateXmodelColBounds(XModel* xmodel, vec3_t axis[3], vec3_t& out_mins, vec3_t& out_maxs)
{
    out_mins.x = 0.0f;
    out_mins.y = 0.0f;
    out_mins.z = 0.0f;
    out_maxs.x = 0.0f;
    out_maxs.y = 0.0f;
    out_maxs.z = 0.0f;

    if (xmodel->numCollSurfs == 0)
        return;

    for (int surfIdx = 0; surfIdx < xmodel->numCollSurfs; surfIdx++)
    {
        auto& surface = xmodel->collSurfs[surfIdx];
        for (size_t vertIndex = 0u; vertIndex < 8; vertIndex++)
        {
            vec3_t vert;
            if ((vertIndex & 1) != 0)
                vert.x = surface.mins.x;
            else
                vert.x = surface.maxs.x;
            if ((vertIndex & 2) != 0)
                vert.y = surface.mins.y;
            else
                vert.y = surface.maxs.y;
            if ((vertIndex & 4) != 0)
                vert.z = surface.mins.z;
            else
                vert.z = surface.maxs.z;

            vec3_t rotatedVert;
            rotatedVert.x = (vert.x * axis[0].x) + (vert.y * axis[1].x) + (vert.z * axis[2].x);
            rotatedVert.y = (vert.x * axis[0].y) + (vert.y * axis[1].y) + (vert.z * axis[2].y);
            rotatedVert.z = (vert.x * axis[0].z) + (vert.y * axis[1].z) + (vert.z * axis[2].z);

            if (vertIndex == 0 && surfIdx == 0)
            {
                out_mins = rotatedVert;
                out_maxs = rotatedVert;
            }
            else
                BSPUtil::updateAABBWithPoint(rotatedVert, out_mins, out_maxs);
        }
    }
}

void BSPUtil::convertAnglesToAxis(vec3_t* angles, vec3_t axis[3])
{
    float cosX = cos(angles->x * (std::numbers::pi_v<float> / 180.0f));
    float sinX = sin(angles->x * (std::numbers::pi_v<float> / 180.0f));
    float cosY = cos(angles->y * (std::numbers::pi_v<float> / 180.0f));
    float sinY = sin(angles->y * (std::numbers::pi_v<float> / 180.0f));
    float cosZ = cos(angles->z * (std::numbers::pi_v<float> / 180.0f));
    float sinZ = sin(angles->z * (std::numbers::pi_v<float> / 180.0f));

    axis[0].x = cosX * cosY;
    axis[0].y = cosX * sinY;
    axis[0].z = -sinX;
    axis[1].x = ((sinZ * sinX) * cosY) - (cosZ * sinY);
    axis[1].y = ((sinZ * sinX) * sinY) + (cosZ * cosY);
    axis[1].z = sinZ * cosX;
    axis[2].x = ((cosZ * sinX) * cosY) + (sinZ * sinY);
    axis[2].y = ((cosZ * sinX) * sinY) - (sinZ * cosY);
    axis[2].z = cosZ * cosX;
}

inline float BSPUtil::lengthSquaredOfQuat(float quat[4])
{
    return quat[0] * quat[0] + quat[1] * quat[1] + quat[2] * quat[2] + quat[3] * quat[3];
}

vec4_t BSPUtil::convertAxisToQuat(vec3_t axis[3])
{
    float possibleQuats[4][4];

    float maxXX = axis[0].x;
    float matXY = axis[0].y;
    float matXZ = axis[0].z;
    float matYX = axis[1].x;
    float matYY = axis[1].y;
    float matYZ = axis[1].z;
    float matZX = axis[2].x;
    float matZY = axis[2].y;
    float matZZ = axis[2].z;

    float YZminZY = matYZ - matZY;
    float ZXminXZ = matZX - matXZ;
    float XYminYX = matXY - matYX;
    float ZYplusYZ = matZY + matYZ;
    float XZplusZX = matXZ + matZX;
    float YXplusXY = matYX + matXY;

    char axisToUse = 0;
    possibleQuats[0][0] = YZminZY;
    possibleQuats[0][1] = ZXminXZ;
    possibleQuats[0][2] = XYminYX;
    possibleQuats[0][3] = (maxXX + matYY + matZZ) + 1.0f;
    float lengthSquared = lengthSquaredOfQuat(possibleQuats[0]);
    if (lengthSquared < 1.0f)
    {
        axisToUse = 1;
        possibleQuats[1][0] = XZplusZX;
        possibleQuats[1][1] = ZYplusYZ;
        possibleQuats[1][2] = (matZZ - matYY - maxXX) + 1.0f;
        possibleQuats[1][3] = XYminYX;
        lengthSquared = lengthSquaredOfQuat(possibleQuats[1]);
        if (lengthSquared < 1.0f)
        {
            axisToUse = 2;
            possibleQuats[2][0] = (maxXX - matYY - matZZ) + 1.0f;
            possibleQuats[2][1] = YXplusXY;
            possibleQuats[2][2] = XZplusZX;
            possibleQuats[2][3] = YZminZY;
            lengthSquared = lengthSquaredOfQuat(possibleQuats[2]);
            if (lengthSquared < 1.0f)
            {
                axisToUse = 3;
                possibleQuats[3][0] = YXplusXY;
                possibleQuats[3][1] = (matYY - maxXX - matZZ) + 1.0f;
                possibleQuats[3][2] = ZYplusYZ;
                possibleQuats[3][3] = ZXminXZ;
                lengthSquared = lengthSquaredOfQuat(possibleQuats[3]);
                if (lengthSquared < 1.0f)
                    con::warn("Axis to quatrnion: bad axis.");
            }
        }
    }

    if (lengthSquared == 0.0f)
    {
        con::warn("Axis to quatrnion: bad length.");
        lengthSquared = 1.0f;
    }

    vec4_t quaternion;
    float inverseLength = 1.0f / sqrtf(lengthSquared);
    quaternion.x = possibleQuats[axisToUse][0] * inverseLength;
    quaternion.y = possibleQuats[axisToUse][1] * inverseLength;
    quaternion.z = possibleQuats[axisToUse][2] * inverseLength;
    quaternion.w = possibleQuats[axisToUse][3] * inverseLength;
    return quaternion;
}

vec4_t BSPUtil::convertAnglesToQuat(vec3_t& angles)
{
    vec3_t axis[3];
    convertAnglesToAxis(&angles, axis);
    return convertAxisToQuat(axis);
}

vec3_t BSPUtil::convertAnglesToForward(vec3_t& angles)
{
    float xRad = angles.x * (std::numbers::pi_v<float> / 180.0f);
    float yRad = angles.y * (std::numbers::pi_v<float> / 180.0f);

    float sinX = sinf(xRad);
    float sinY = sinf(yRad);
    float cosX = cosf(xRad);
    float cosY = cosf(yRad);
    vec3_t result{};
    result.x = cosX * cosY;
    result.y = cosX * sinY;
    result.z = -sinX;
    return result;
}

void BSPUtil::convertQuaternionToAxis(vec4_t* quat, vec3_t axis[3])
{
    float quatX = quat->v[0];
    float quatY = quat->v[1];
    float quatZ = quat->v[2];
    float quatW = quat->v[3];

    float xx = (quatX * 2.0f) * quatX;
    float xy = (quatX * 2.0f) * quatY;
    float xz = (quatX * 2.0f) * quatZ;
    float xw = (quatX * 2.0f) * quatW;

    float yy = (quatY * 2.0f) * quatY;
    float yz = (quatY * 2.0f) * quatZ;
    float yw = (quatY * 2.0f) * quatW;

    float zz = (quatZ * 2.0f) * quatZ;
    float zw = (quatZ * 2.0f) * quatW;

    axis->x = 1.0f - (zz + yy);
    axis->y = zw + xy;
    axis->z = xz - yw;

    axis[1].x = xy - zw;
    axis[1].y = 1.0f - (zz + xx);
    axis[1].z = yz + xw;

    axis[2].x = yw + xz;
    axis[2].y = yz - xw;
    axis[2].z = 1.0f - (yy + xx);
}

vec3_t BSPUtil::convertQuaternionToForwardVector(vec4_t* quat)
{
    float quatX = quat->v[0];
    float quatY = quat->v[1];
    float quatZ = quat->v[2];
    float quatW = quat->v[3];

    vec3_t result{};
    result.x = 1.0f - (((quatY * quatY) + (quatZ * quatZ)) * 2.0f);
    result.y = ((quatX * quatY) + (quatW * quatZ)) * 2.0f;
    result.z = ((quatX * quatZ) - (quatW * quatY)) * 2.0f;
    return result;
}

vec3_t BSPUtil::convertForwardVectorToViewAngles(vec3_t& forwardVec)
{
    vec3_t viewAngles;

    if (forwardVec.x == 0.0f && forwardVec.y == 0.0f)
    {
        if (-forwardVec.z < 0.0f)
            viewAngles.x = 270.0f;
        else
            viewAngles.x = 90.0f;
        viewAngles.y = 0.0f;
        viewAngles.z = 0.0f;
        return viewAngles;
    }

    float xAndYDist = sqrtf((forwardVec.x * forwardVec.x) + (forwardVec.y * forwardVec.y));
    float atanXRadians = atan2f(forwardVec.z, xAndYDist);
    float atanXDegrees = atanXRadians * (-180.0f / std::numbers::pi_v<float>);
    if (atanXDegrees < 0.0f)
        atanXDegrees += 360.0f;
    viewAngles.x = atanXDegrees;

    float atanYRadians = atan2f(forwardVec.y, forwardVec.x);
    float atanYDegrees = atanYRadians * (180.0f / std::numbers::pi_v<float>);
    if (atanYDegrees < 0.0f)
        atanYDegrees += 360.0f;
    viewAngles.y = atanYDegrees;

    viewAngles.z = 0.0f;

    return viewAngles;
}

float BSPUtil::getPitchFromVector(vec3_t& vector)
{
    if (vector.x == 0.0f && vector.y == 0.0f)
    {
        if (-vector.z < 0.0f)
            return -90.0f;
        else
            return 90.0f;
    }

    float xAndYDist = sqrtf((vector.x * vector.x) + (vector.y * vector.y));
    float pitchRadians = atan2f(vector.z, xAndYDist);
    float pitchDegrees = pitchRadians * (-180.0f / std::numbers::pi_v<float>);
    return pitchDegrees;
}

vec3_t BSPUtil::convertAxisToAngles(vec3_t axis[3])
{
    vec3_t tempAngles = convertForwardVectorToViewAngles(axis[0]);
    float xRadiansNeg = -tempAngles.x * (std::numbers::pi_v<float> / 180.0f);
    float yRadiansNeg = -tempAngles.y * (std::numbers::pi_v<float> / 180.0f);
    float cosX = cos(xRadiansNeg);
    float sinX = sin(xRadiansNeg);
    float cosY = cos(yRadiansNeg);
    float sinY = sin(yRadiansNeg);

    vec3_t tempVec;
    float tempFloat = (axis[1].x * cosY) - (axis[1].y * sinY);
    tempVec.x = (axis[1].z * sinX) + (tempFloat * cosX);
    tempVec.y = (axis[1].x * sinY) + (axis[1].y * cosY);
    tempVec.z = (axis[1].z * cosX) - (tempFloat * sinX);
    float pitch = getPitchFromVector(tempVec);
    if (tempVec.y >= 0.0f)
        tempAngles.z = -pitch;
    else if (pitch >= 0.0f)
        tempAngles.z = pitch - 180.0f;
    else
        tempAngles.z = pitch + 180.0f;
    return tempAngles;
}

vec3_t BSPUtil::convertQuatToAngles(vec4_t* quat)
{
    vec3_t axis[3]{};
    convertQuaternionToAxis(quat, axis);
    return convertAxisToAngles(axis);
}

void BSPUtil::matrixTranspose3x3(const vec3_t* in, vec3_t* out)
{
    out[0].x = in[0].x;
    out[0].y = in[1].x;
    out[0].z = in[2].x;
    out[1].x = in[0].y;
    out[1].y = in[1].y;
    out[1].z = in[2].y;
    out[2].x = in[0].z;
    out[2].y = in[1].z;
    out[2].z = in[2].z;
}

vec3_t BSPUtil::convertStringToVec3(const char* str)
{
    std::string v1Str = str;

    int nextValIndex = 0;
    while (v1Str[nextValIndex] != ' ')
        nextValIndex++;
    nextValIndex++; // skip past space
    std::string v2Str = &v1Str[nextValIndex];

    nextValIndex = 0;
    while (v2Str[nextValIndex] != ' ')
        nextValIndex++;
    nextValIndex++; // skip past space
    std::string v3Str = &v2Str[nextValIndex];

    vec3_t result;
    result.x = static_cast<float>(atof(v1Str.c_str()));
    result.y = static_cast<float>(atof(v2Str.c_str()));
    result.z = static_cast<float>(atof(v3Str.c_str()));
    return result;
}

std::string BSPUtil::convertVec3ToString(vec3_t& vec)
{
    std::string result = std::format("{} {} {}", roundf(vec.x), roundf(vec.y), roundf(vec.z));
    return result;
}

// return true if inFlags contains test_flag
bool BSPUtil::flagsMatchExact(int test_flag, int inFlags)
{
    return (test_flag & inFlags) == test_flag;
}

// return true if any flags are the same between flag1 and flag2
bool BSPUtil::flagsMatchAny(int flag1, int flag2)
{
    return (flag1 & flag2) != 0;
}
