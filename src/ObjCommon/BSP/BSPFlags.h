#pragma once

namespace T6
{
    namespace BSPFlags
    {
        enum SurfaceType
        {
            BSP_SURF_TYPE_BARK,
            BSP_SURF_TYPE_BRICK,
            BSP_SURF_TYPE_CARPET,
            BSP_SURF_TYPE_CLOTH,
            BSP_SURF_TYPE_CONCRETE,
            BSP_SURF_TYPE_DIRT,
            BSP_SURF_TYPE_FLESH,
            BSP_SURF_TYPE_FOLIAGE,
            BSP_SURF_TYPE_GLASS,
            BSP_SURF_TYPE_GRASS,
            BSP_SURF_TYPE_GRAVEL,
            BSP_SURF_TYPE_ICE,
            BSP_SURF_TYPE_METAL,
            BSP_SURF_TYPE_MUD,
            BSP_SURF_TYPE_PAPER,
            BSP_SURF_TYPE_PLASTER,
            BSP_SURF_TYPE_ROCK,
            BSP_SURF_TYPE_SAND,
            BSP_SURF_TYPE_SNOW,
            BSP_SURF_TYPE_WATER,
            BSP_SURF_TYPE_WOOD,
            BSP_SURF_TYPE_ASPHALT,
            BSP_SURF_TYPE_CERAMIC,
            BSP_SURF_TYPE_PLASTIC,
            BSP_SURF_TYPE_RUBBER,
            BSP_SURF_TYPE_CUSHION,
            BSP_SURF_TYPE_FRUIT,
            BSP_SURF_TYPE_PAINTEDMETAL,
            BSP_SURF_TYPE_PLAYER,
            BSP_SURF_TYPE_TALLGRASS,
            BSP_SURF_TYPE_RIOTSHIELD,
            BSP_SURF_TYPE_OPAQUEGLASS,
            BSP_SURF_TYPE_CLIPMISSILE,
            BSP_SURF_TYPE_AI_NOSIGHT,
            BSP_SURF_TYPE_CLIPSHOT,
            BSP_SURF_TYPE_PLAYERCLIP,
            BSP_SURF_TYPE_MONSTERCLIP,
            BSP_SURF_TYPE_VEHICLECLIP,
            BSP_SURF_TYPE_ITEMCLIP,
            BSP_SURF_TYPE_NODROP,
            BSP_SURF_TYPE_NONSOLID,
            BSP_SURF_TYPE_DETAIL,
            BSP_SURF_TYPE_STRUCTURAL,
            BSP_SURF_TYPE_PORTAL,
            BSP_SURF_TYPE_CANSHOOTCLIP,
            BSP_SURF_TYPE_ORIGIN,
            BSP_SURF_TYPE_SKY,
            BSP_SURF_TYPE_NOCASTSHADOW,
            BSP_SURF_TYPE_ONLYCASTSHADOW,
            BSP_SURF_TYPE_PHYSICSGEOM,
            BSP_SURF_TYPE_LIGHTPORTAL,
            BSP_SURF_TYPE_CAULK,
            BSP_SURF_TYPE_AREALIGHT,
            BSP_SURF_TYPE_SLICK,
            BSP_SURF_TYPE_NOIMPACT,
            BSP_SURF_TYPE_NOMARKS,
            BSP_SURF_TYPE_NOPENETRATE,
            BSP_SURF_TYPE_LADDER,
            BSP_SURF_TYPE_NODAMAGE,
            BSP_SURF_TYPE_MANTLEON,
            BSP_SURF_TYPE_MANTLEOVER,
            BSP_SURF_TYPE_MOUNT,
            BSP_SURF_TYPE_NOSTEPS,
            BSP_SURF_TYPE_NODRAW,
            BSP_SURF_TYPE_NORECEIVEDYNAMICSHADOW,
            BSP_SURF_TYPE_NODLIGHT,
            BSP_SURF_TYPE_COUNT
        };

        struct s_SurfaceTypeFlags
        {
            int surfaceFlags;
            int contentFlags;
        };

        constexpr s_SurfaceTypeFlags surfaceTypeToFlagMap[BSP_SURF_TYPE_COUNT] = {
            {0x100000,   0         }, // bark
            {0x200000,   0         }, // brick
            {0x300000,   0         }, // carpet
            {0x400000,   0         }, // cloth
            {0x500000,   0         }, // concrete
            {0x600000,   0         }, // dirt
            {0x700000,   0         }, // flesh
            {0x800000,   2         }, // foliage
            {0x900000,   0x10      }, // glass
            {0x0A00000,  0         }, // grass
            {0x0B00000,  0         }, // gravel
            {0x0C00000,  0         }, // ice
            {0x0D00000,  0         }, // metal
            {0x0E00000,  0         }, // mud
            {0x0F00000,  0         }, // paper
            {0x1000000,  0         }, // plaster
            {0x1100000,  0         }, // rock
            {0x1200000,  0         }, // sand
            {0x1300000,  0         }, // snow
            {0x1400000,  0x20      }, // water
            {0x1500000,  0         }, // wood
            {0x1600000,  0         }, // asphalt
            {0x1700000,  0         }, // ceramic
            {0x1800000,  0         }, // plastic
            {0x1900000,  0         }, // rubber
            {0x1A00000,  0         }, // cushion
            {0x1B00000,  0         }, // fruit
            {0x1C00000,  0         }, // paintedmetal
            {0x1D00000,  0         }, // player
            {0x1E00000,  0         }, // tallgrass
            {0x1F00000,  0         }, // riotshield
            {0x900000,   0         }, // opaqueglass
            {0,          0x80      }, // clipmissile
            {0,          0x1000    }, // ai_nosight
            {0,          0x2000    }, // clipshot
            {0,          0x10000   }, // playerclip
            {0,          0x20000   }, // monsterclip
            {0,          0x200     }, // vehicleclip
            {0,          0x400     }, // itemclip
            {0,          0x80000000}, // noDrop
            {0x4000,     0         }, // nonSolid
            {0,          0x8000000 }, // detail
            {0,          0x10000000}, // structural
            {0x80000000, 0         }, // portal
            {0,          0x40      }, // canShootClip
            {0,          0         }, // origin
            {4,          0x800     }, // sky
            {0x40000,    0         }, // noCastShadow
            {0x80000,    0         }, // onlyCastShadow
            {0,          0         }, // physicsGeom
            {0,          0         }, // lightPortal
            {0x1000,     0         }, // caulk
            {0x8000,     0         }, // areaLight
            {2,          0         }, // slick
            {0x10,       0         }, // noImpact
            {0x20,       0         }, // noMarks
            {0x100,      0         }, // noPenetrate
            {8,          0         }, // ladder
            {1,          0         }, // noDamage
            {0x4000000,  0x1000000 }, // mantleOn
            {0x8000000,  0x1000000 }, // mantleOver
            {0x10000000, 0x1000000 }, // mount
            {0x2000,     0         }, // noSteps
            {0x80,       0         }, // noDraw
            {0x800,      0         }, // noReceiveDynamicShadow
            {0x20000,    0         }  // noDlight
        };

        constexpr SurfaceType materialTypes[] = {
            BSP_SURF_TYPE_BARK,       BSP_SURF_TYPE_BRICK,       BSP_SURF_TYPE_CARPET,       BSP_SURF_TYPE_CLOTH,   BSP_SURF_TYPE_CONCRETE,
            BSP_SURF_TYPE_DIRT,       BSP_SURF_TYPE_FLESH,       BSP_SURF_TYPE_FOLIAGE,      BSP_SURF_TYPE_GLASS,   BSP_SURF_TYPE_GRASS,
            BSP_SURF_TYPE_GRAVEL,     BSP_SURF_TYPE_ICE,         BSP_SURF_TYPE_METAL,        BSP_SURF_TYPE_MUD,     BSP_SURF_TYPE_PAPER,
            BSP_SURF_TYPE_PLASTER,    BSP_SURF_TYPE_ROCK,        BSP_SURF_TYPE_SAND,         BSP_SURF_TYPE_SNOW,    BSP_SURF_TYPE_WATER,
            BSP_SURF_TYPE_WOOD,       BSP_SURF_TYPE_ASPHALT,     BSP_SURF_TYPE_CERAMIC,      BSP_SURF_TYPE_PLASTIC, BSP_SURF_TYPE_RUBBER,
            BSP_SURF_TYPE_CUSHION,    BSP_SURF_TYPE_FRUIT,       BSP_SURF_TYPE_PAINTEDMETAL, BSP_SURF_TYPE_PLAYER,  BSP_SURF_TYPE_TALLGRASS,
            BSP_SURF_TYPE_RIOTSHIELD, BSP_SURF_TYPE_OPAQUEGLASS,
        };

        constexpr SurfaceType materialFlags[] = {
            BSP_SURF_TYPE_CLIPMISSILE,    BSP_SURF_TYPE_AI_NOSIGHT,  BSP_SURF_TYPE_CLIPSHOT,     BSP_SURF_TYPE_PLAYERCLIP, BSP_SURF_TYPE_MONSTERCLIP,
            BSP_SURF_TYPE_VEHICLECLIP,    BSP_SURF_TYPE_ITEMCLIP,    BSP_SURF_TYPE_NODROP,       BSP_SURF_TYPE_NONSOLID,   BSP_SURF_TYPE_DETAIL,
            BSP_SURF_TYPE_STRUCTURAL,     BSP_SURF_TYPE_PORTAL,      BSP_SURF_TYPE_CANSHOOTCLIP, BSP_SURF_TYPE_SKY,        BSP_SURF_TYPE_NOCASTSHADOW,
            BSP_SURF_TYPE_ONLYCASTSHADOW, BSP_SURF_TYPE_CAULK,       BSP_SURF_TYPE_AREALIGHT,    BSP_SURF_TYPE_SLICK,      BSP_SURF_TYPE_NOIMPACT,
            BSP_SURF_TYPE_NOMARKS,        BSP_SURF_TYPE_NOPENETRATE, BSP_SURF_TYPE_LADDER,       BSP_SURF_TYPE_NODAMAGE,   BSP_SURF_TYPE_MANTLEON,
            BSP_SURF_TYPE_MANTLEOVER,     BSP_SURF_TYPE_MOUNT,       BSP_SURF_TYPE_NOSTEPS,      BSP_SURF_TYPE_NODRAW,     BSP_SURF_TYPE_NORECEIVEDYNAMICSHADOW,
            BSP_SURF_TYPE_NODLIGHT,
        };

        constexpr const char* surfaceTypeToNameMap[BSP_SURF_TYPE_COUNT] = {"bark",
                                                                           "brick",
                                                                           "carpet",
                                                                           "cloth",
                                                                           "concrete",
                                                                           "dirt",
                                                                           "flesh",
                                                                           "foliage",
                                                                           "glass",
                                                                           "grass",
                                                                           "gravel",
                                                                           "ice",
                                                                           "metal",
                                                                           "mud",
                                                                           "paper",
                                                                           "plaster",
                                                                           "rock",
                                                                           "sand",
                                                                           "snow",
                                                                           "water",
                                                                           "wood",
                                                                           "asphalt",
                                                                           "ceramic",
                                                                           "plastic",
                                                                           "rubber",
                                                                           "cushion",
                                                                           "fruit",
                                                                           "paintedmetal",
                                                                           "player",
                                                                           "tallgrass",
                                                                           "riotshield",
                                                                           "opaqueglass",
                                                                           "clipmissile",
                                                                           "ai_nosight",
                                                                           "clipshot",
                                                                           "playerclip",
                                                                           "monsterclip",
                                                                           "vehicleclip",
                                                                           "itemclip",
                                                                           "nodrop",
                                                                           "nonsolid",
                                                                           "detail",
                                                                           "structural",
                                                                           "portal",
                                                                           "canshootclip",
                                                                           "origin",
                                                                           "sky",
                                                                           "nocastshadow",
                                                                           "onlycastshadow",
                                                                           "physicsgeom",
                                                                           "lightportal",
                                                                           "caulk",
                                                                           "arealight",
                                                                           "slick",
                                                                           "noimpact",
                                                                           "nomarks",
                                                                           "nopenetrate",
                                                                           "ladder",
                                                                           "nodamage",
                                                                           "mantleon",
                                                                           "mantleover",
                                                                           "mount",
                                                                           "nosteps",
                                                                           "nodraw",
                                                                           "noreceivedynamicshadow",
                                                                           "nodlight"};
    } // namespace BSPFlags
} // namespace T6
