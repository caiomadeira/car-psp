#include "psptexture.h"
#include <stdlib.h>
#include <stdio.h> // Adicionado para o sprintf

static PspTexture textures[NUM_ENV_TEX];
static int mapCodeToSlot[MAX_MAP_CODE + 1];
static bool loaded[NUM_ENV_TEX];

int LoadAllEnvironmentTextures() {
    int success = 0;

    // zerando a tabela d eocnversao
    for(int c = 0; c <= MAX_MAP_CODE; c++) {
        mapCodeToSlot[c] = -1;
    }

    // percorre a tabela carregando e registrando
    for(int slot = 0; slot < NUM_ENV_TEX; slot++) {
        const TexDef& def = TEX_TABLE[slot];

        // monta o caminho
        char path[96];
        snprintf(path, sizeof(path), "assets/tex/%s.raw", def.file);
        loaded[slot] = LoadTexturePsp(path, textures[slot], def.mipmap);
        if (loaded[slot]) {
            success++;
            if (def.mapCode >= 1 && def.mapCode <= MAX_MAP_CODE) {
                mapCodeToSlot[def.mapCode] = slot;
            } else {
                printf("TEX_TABLE[%d]: mapCode %d fora de 1..%d, ignorado\n",
                       slot, def.mapCode, MAX_MAP_CODE);
            }
        } else {
            // falha de load n crash. o tile so n desenha
            printf("AVISO: nao carregou %s\n", path);
        }
    }
    return success;
}


int TexSlotFromMapCode(int mapCode) {
    // um sanity check no mapa.txt por ser editado a mao ne eh sucetivel a erro.
    if (mapCode < 0 || mapCode > MAX_MAP_CODE) return -1;
    return mapCodeToSlot[mapCode];
}

PspTexture& EnvTexture(int slot) {
    if (slot < 0) slot = 0;
    if (slot >= NUM_ENV_TEX) slot = NUM_ENV_TEX - 1;
    return textures[slot];
}