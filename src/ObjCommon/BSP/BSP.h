#pragma once

#include "BSPFlags.h"
#include "Game/T6/T6.h"
#include "Utils/Logging/Log.h"

#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace T6
{
    namespace BSP
    {
        struct BSPVertex
        {
            vec3_t pos;
            vec4_t color;
            vec2_t texCoord;
            vec3_t normal;
            vec3_t tangent;
            vec3_t binormal;
        };

        enum BSPMaterialType
        {
            MATERIAL_TYPE_COLOUR,
            MATERIAL_TYPE_TEXTURE
        };

        struct BSPMaterial
        {
            std::string materialName;
            BSPMaterialType materialType;
            vec4_t materialColour;

            int surfaceFlags;
            int contentFlags;
        };

        struct BSPSurface
        {
            bool isLocalCoords;
            vec3_t origin;
            size_t materialIndex;
            size_t vertexCount;
            size_t triCount;
            size_t indexOfFirstVertex;
            size_t indexOfFirstIndex;
        };

        struct BSPXModel
        {
            std::string name;

            vec3_t origin;
            vec4_t rotationQuaternion;
            vec3_t scale;

            bool areBoundsValid;
            vec3_t mins;
            vec3_t maxs;

            bool doesCastShadow;
        };

        struct BSPWorld
        {
            std::vector<BSPSurface> surfaces;
            std::vector<BSPVertex> vertices;
            std::vector<uint16_t> indices;
            std::vector<BSPMaterial> materials;
            std::vector<BSPXModel> xmodels;
        };

        enum BSPLightType
        {
            LIGHT_TYPE_DIRECTIONAL,
            LIGHT_TYPE_SPOT
        };

        struct BSPLight
        {
            BSPLightType type;
            vec3_t colour;
            float range;
            float intensity;

            vec3_t pos;
            vec3_t direction;

            // angle is in radians. only used on spot lights
            float innerConeAngle;
            float outerConeAngle;
        };

        struct BSPSpawnPoint
        {
            vec3_t origin;
            vec3_t forward;
            std::string spawnpointGroupName;
        };

        struct BSPZoneZM
        {
            vec3_t origin;
            std::string zoneName;
            std::string spawnerGroupName;
            std::string spawnpointGroupName;
            size_t modelIndex;
        };

        struct BSPZSpawnerZM
        {
            std::string spawnerGroupName;
            vec3_t origin;
            vec3_t forward;
        };

        constexpr const char* bspEntityTypeNames[] = {"Weapons",
                                                      "Volumes",
                                                      "Triggers",
                                                      "Pathnodes",
                                                      "Lights",
                                                      "Structs",
                                                      "Vehicles",
                                                      "Models",
                                                      "Brushmodels",
                                                      "ZBarriers",
                                                      "Points",
                                                      "Actors",
                                                      "Glass",
                                                      "Ropes",
                                                      "Other"};

        enum bspEntityType
        {
            ET_WEAPON,
            ET_VOLUME,
            ET_TRIGGER,
            ET_PATHNODE,
            ET_LIGHT,
            ET_STRUCT,
            ET_VEHICLE,
            ET_MODEL,
            ET_BRUSHMODEL,
            ET_ZBARRIER,
            ET_POINT,
            ET_ACTOR,
            ET_GLASS,
            ET_ROPE,
            ET_OTHER,
            ET_COUNT
        };

        enum bspModelSurfType
        {
            MST_NONE,
            MST_BRUSH,
            MST_TERRAIN,
            MST_BOTH
        };

        enum bspModelSurfSide
        {
            MSS_NONE,
            MSS_GFX,
            MSS_COL,
            MSS_BOTH
        };

        struct BSPModel
        {
            bspModelSurfSide surfaceSide;
            bspModelSurfType surfaceType;
            size_t gfxSurfaceIndex;
            size_t gfxSurfaceCount;
            size_t colBrushSurfaceIndex;
            size_t colBrushSurfaceCount;
            size_t colTerrainSurfaceIndex;
            size_t colTerrainSurfaceCount;
        };

        struct BSPEntityEntry
        {
            std::string key;
            std::string value;
        };

        struct BSPEntity
        {
            size_t uniqueEntityNumber;

            vec3_t origin;
            vec4_t rotationQuaternion;
            bool hasModel;
            size_t modelIndex;

            bspEntityType type;
            std::string classname;
            std::vector<BSPEntityEntry> entries;
        };

        struct BSPData
        {
            std::string name;
            std::string bspName;
            bool isZombiesMap;
            std::string skyboxName;

            size_t staticSurfaceStart;
            size_t staticSurfaceCount;
            // below are static surfaces only
            size_t litOpaqueSurfaceStart;
            size_t litOpaqueSurfaceCount;
            size_t litTransparentSurfaceStart;
            size_t litTransparentSurfaceCount;
            size_t emissiveOpaqueSurfaceStart;
            size_t emissiveOpaqueSurfaceCount;
            size_t emissiveTransparentSurfaceStart;
            size_t emissiveTransparentSurfaceCount;

            BSPWorld gfxWorld;

            size_t staticTerrainSurfaceStart;
            size_t staticTerrainSurfaceCount;
            size_t staticBrushSurfaceStart;
            size_t staticBrushSurfaceCount;
            BSPWorld colWorld;

            bool hasSunlightBeenSet;
            BSPLight sunlight;
            std::vector<BSPLight> lights;

            std::vector<BSPSpawnPoint> spawnpoints;
            std::vector<BSPZoneZM> zm_zones;
            std::vector<BSPZSpawnerZM> zm_spawners;

            bool containsWorldspawn;
            bool containsIntermssion;
            std::vector<BSPEntity> entities;
            BSPEntity worldspawn;

            std::vector<BSPModel> models;
        };

        enum BSPDefaultLights
        {
            EMPTY_LIGHT_INDEX = 0,
            SUN_LIGHT_INDEX = 1,
            BSP_DEFAULT_LIGHT_COUNT = 2
        };
    } // namespace BSP

} // namespace T6
