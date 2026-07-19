#include "psptexture.h"
#include <stdlib.h>
#include <stdio.h> // Adicionado para o sprintf

PspTexture environment_textures[MAX_TEXTURES];

void LoadAllEnvironmentTextures() {
    const char* roadTexNames[] = {
        "", "CROSS", "DL", "DLR", "DR", "LR", "None", 
        "UD", "UDL", "UDR", "UL", "ULR", "UR" };
        
    for(int i = 1; i <= 12; i++) {
        char path[64];
        sprintf(path, "assets/tex/%s.raw", roadTexNames[i]);
        
        // Tenta carregar a rua, se não achar, ignora e segue a vida
        LoadTexturePsp(path, environment_textures[i]); 
    }
    
    // As texturas fora das ruas (carregadas apenas UMA vez para poupar VRAM)
    LoadTexturePsp("assets/tex/sidewalk.raw", environment_textures[TEX_SIDEWALK]);
    LoadTexturePsp("assets/tex/sidewalk.raw", environment_textures[TEX_SIDEWALK]);
    // LoadTexturePsp("assets/tex/sand.raw", environment_textures[TEX_SAND]);
    // LoadTexturePsp("assets/tex/sky.raw", environment_textures[TEX_SKY]);
}