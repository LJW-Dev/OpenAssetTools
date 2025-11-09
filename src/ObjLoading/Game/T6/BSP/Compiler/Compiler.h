#pragma once

#include "../BSPUtil.h"
#include "Game/T5/T5.h"
#include "Game/T6/T6.h"
#include "Utils/Logging/Log.h"

namespace BSP
{
    namespace CBSPGameConstants
    {
        constexpr unsigned int MAX_COLLISION_VERTS = UINT16_MAX;

        constexpr size_t MAX_AABB_TREE_CHILDREN = 128;

        enum BSPDefaultLights
        {
            STATIC_LIGHT_INDEX = 0,
            SUN_LIGHT_INDEX = 1,
            BSP_DEFAULT_LIGHT_COUNT = 2
        };

        inline const char* DEFENDER_SPAWN_POINT_NAMES[] = {"mp_ctf_spawn_allies",
                                                           "mp_ctf_spawn_allies_start",
                                                           "mp_sd_spawn_defender",
                                                           "mp_dom_spawn_allies_start",
                                                           "mp_dem_spawn_defender_start",
                                                           "mp_dem_spawn_defenderOT_start",
                                                           "mp_dem_spawn_defender",
                                                           "mp_tdm_spawn_allies_start",
                                                           "mp_tdm_spawn_team1_start",
                                                           "mp_tdm_spawn_team2_start",
                                                           "mp_tdm_spawn_team3_start"};

        inline const char* ATTACKER_SPAWN_POINT_NAMES[] = {"mp_ctf_spawn_axis",
                                                           "mp_ctf_spawn_axis_start",
                                                           "mp_sd_spawn_attacker",
                                                           "mp_dom_spawn_axis_start",
                                                           "mp_dem_spawn_attacker_start",
                                                           "mp_dem_spawn_attackerOT_start",
                                                           "mp_dem_spawn_defender",
                                                           "mp_tdm_spawn_axis_start",
                                                           "mp_tdm_spawn_team4_start",
                                                           "mp_tdm_spawn_team5_start",
                                                           "mp_tdm_spawn_team6_start"};

        inline const char* FFA_SPAWN_POINT_NAMES[] = {"mp_tdm_spawn", "mp_dm_spawn", "mp_dom_spawn"};
    } // namespace CBSPGameConstants

    // BSPLinkingConstants:
    // These values are BSP linking constants that are required for the link to be successful
    namespace CBSPLinkingConstants
    {
        constexpr const char* MISSING_IMAGE_NAME = ",mc/lambert1";
        constexpr const char* COLOR_ONLY_IMAGE_NAME = ",white";

        constexpr const char* DEAFULT_LIGHTDEF_IMAGE = "whitesquare"; // requires adding this asset to the zone folder

        constexpr const char* DEFAULT_SPAWN_POINT_STRING = R"({
    "attackers": [
		{
			"origin": "200 200 200",
			"angles": "0 0 0"
		}
    ],
	"defenders": [
		{
			"origin": "200 200 200",
			"angles": "0 0 0"
		}
    ],
	"FFA": [
		{
			"origin": "200 200 200",
			"angles": "0 0 0"
		}
	]
    })";

        constexpr const char* DEFAULT_MAP_ENTS_STRING = R"({
    "entities": [
        {
            "classname": "worldspawn"
        },
        {
            "angles": "0 0 0",
            "classname": "info_player_start",
            "origin": "0 0 0"
        },
        {
            "angles": "0 0 0",
            "classname": "mp_global_intermission",
            "origin": "0 0 0"
        }
    ]
    })";
    } // namespace CBSPLinkingConstants

    // BSPEditableConstants:
    // These values are BSP constants that can be edited and may not break the linker/game if changed
    namespace CBSPEditableConstants
    {
        // Default xmodel values
        // Unused as there is no support for xmodels right now
        constexpr float DEFAULT_SMODEL_CULL_DIST = 10000.0f;
        constexpr int DEFAULT_SMODEL_FLAGS = T6::STATIC_MODEL_FLAG_NO_SHADOW;
        constexpr int DEFAULT_SMODEL_LIGHT = 1;
        constexpr int DEFAULT_SMODEL_REFLECTION_PROBE = 0;

        // Default surface values
        constexpr int DEFAULT_SURFACE_LIGHT = CBSPGameConstants::SUN_LIGHT_INDEX;
        constexpr int DEFAULT_SURFACE_LIGHTMAP = 0;
        constexpr int DEFAULT_SURFACE_REFLECTION_PROBE = 0;
        constexpr int DEFAULT_SURFACE_FLAGS = (T6::GFX_SURFACE_CASTS_SUN_SHADOW | T6::GFX_SURFACE_CASTS_SHADOW);

        // material flags determine the features of a surface
        // unsure which flag type changes what right now
        // -1 results in: no running, water splashes all the time, low friction, slanted angles make you slide very fast
        // 1 results in: normal surface features, grenades work, seems normal
        constexpr int MATERIAL_SURFACE_FLAGS = 1;
        constexpr int MATERIAL_CONTENT_FLAGS = 1;

        // terrain/world flags: does not change the type of terrain or what features they have
        // from testing, as long at it isn't 0 things will work correctly
        constexpr int LEAF_TERRAIN_CONTENTS = 1;
        constexpr int WORLD_TERRAIN_CONTENTS = 1;

        // lightgrid (global) lighting colour
        // since lightgrids are not well understood, this colour is used for the R, G and B values right now
        constexpr unsigned char LIGHTGRID_COLOUR = 255;

        // Sunlight values
        constexpr T6::vec4_t SUNLIGHT_COLOR = {0.75f, 0.75f, 0.75f, 1.0f};
        constexpr T6::vec3_t SUNLIGHT_DIRECTION = {0.0f, 0.0f, 0.0f};
    }; // namespace CBSPEditableConstants
} // namespace BSP
