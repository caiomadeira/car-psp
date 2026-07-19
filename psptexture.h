#ifndef PSP_TEXTURE_H
#define PSP_TEXTURE_H

#include "common.h"
#include "texture.h"

enum TextureID {
    TEX_NONE = 0,
    TEX_ROAD_CROSS, // 1
    TEX_ROAD_DL,    // 2
    TEX_ROAD_DLR,   // 3
    TEX_ROAD_DR,    // 4
    TEX_ROAD_LR,    // 5
    TEX_SIDEWALK, // 6 
    TEX_ROAD_UD,    // 7
    TEX_ROAD_UDL,   // 8
    TEX_ROAD_UDR,   // 9
    TEX_ROAD_UL,    // 10
    TEX_ROAD_ULR,   // 11
    TEX_ROAD_UR,    // 12
    TEX_SAND,   // 13
    TEX_SKY,        // 14
    TEX_SIDEWALK_BEACH,      // 15
    MAX_TEXTURES    // 16
};
extern PspTexture environment_textures[MAX_TEXTURES];

void LoadAllEnvironmentTextures();

#endif