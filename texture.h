#ifndef TEXTURE_H
#define TEXTURE_H

#include <pspgu.h>
#include <psputils.h>
#include <stdio.h>
#include <malloc.h>

/*
A struct precisa guardar niveis.
unsigned int* data[8]; um ponteiro por nivel (ver a documentacao). A GE aceita no  maximo 8: 0..7)
int nLevels; qtds niveis existem.
Isso quebra os acessos direitos a tex.data e viram tex.data[0].
*/

struct PspTexture
{
    unsigned int* data[8];
    int width;
    int height;
    int nLevels;
};

static void GenerateMipmaps(PspTexture& tex) {
    int w = tex.width;
    int h = tex.height;
    while(w > 4 && h > 4 && tex.nLevels < 8) 
    {
        int nw = w >> 1;
        int nh = h >> 1;
        unsigned int* dst = (unsigned int*)memalign(16, (size_t)nw * nh * 4);
        if (!dst) break;

        const unsigned int* src = tex.data[tex.nLevels - 1];

        for (int y = 0; y < nh; y++)
        for (int x = 0; x < nw; x++)
        {
            unsigned int a = src[(y*2  )*w + (x*2  )];
            unsigned int b = src[(y*2  )*w + (x*2+1)];
            unsigned int c = src[(y*2+1)*w + (x*2  )];
            unsigned int d = src[(y*2+1)*w + (x*2+1)];

            unsigned int out = 0;
            for (int ch = 0; ch < 4; ch++)      // R, G, B, A separados
            {
                int sh = ch * 8;
                int soma = ((a>>sh)&0xFF) + ((b>>sh)&0xFF)
                         + ((c>>sh)&0xFF) + ((d>>sh)&0xFF);
                out |= ((unsigned int)(soma / 4) & 0xFF) << sh;
            }
            dst[y*nw + x] = out;
        }

        tex.data[tex.nLevels++] = dst;
        w = nw; h = nh;
    }
}


// Carrega o .raw gerado pelo script Python.
// Retorna true em sucesso.
inline bool LoadTexturePsp(const char* filename, PspTexture& tex, bool comMipmap = true)
{
    FILE* f = fopen(filename, "rb");
    if (!f) return false;

    unsigned int w, h;
    if (fread(&w,4,1,f) != 1 || fread(&h,4,1,f) != 1) { fclose(f); return false; }

    if (!w || !h || (w & (w-1)) || (h & (h-1)) || w > 512 || h > 512) {
        printf("TEXTURA INVALIDA %s: %ux%u\n", filename, w, h);
        fclose(f); return false;
    }

    size_t size = (size_t)w * h * 4;
    tex.data[0] = (unsigned int*)memalign(16, size);
    if (!tex.data[0]) { fclose(f); return false; }
    if (fread(tex.data[0], 1, size, f) != size) { fclose(f); return false; }
    fclose(f);

    tex.width = (int)w; tex.height = (int)h; tex.nLevels = 1;
    if (comMipmap) GenerateMipmaps(tex);  

    sceKernelDcacheWritebackInvalidateAll();
    return true;
}

// Ativa a textura pra ser usada no proximo desenho
inline void UseTexturePsp(const PspTexture& tex)
{
    sceGuEnable(GU_TEXTURE_2D);

    sceGuTexMode(GU_PSM_8888, tex.nLevels - 1, 0, 0);
    sceGuTexLevelMode(GU_TEXTURE_AUTO, 0.0f);          

    int w = tex.width, h = tex.height;
    for (int i = 0; i < tex.nLevels; i++) {
        sceGuTexImage(i, w, h, w, tex.data[i]);
        w >>= 1; h >>= 1;
    }

    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexFilter(GU_LINEAR_MIPMAP_LINEAR, GU_LINEAR);
    sceGuTexWrap(GU_REPEAT, GU_REPEAT);

    sceGuEnable(GU_ALPHA_TEST);
    sceGuAlphaFunc(GU_GREATER, 0, 0xff);
}

inline void DisableTexturePsp()
{
    sceGuDisable(GU_TEXTURE_2D);
    sceGuDisable(GU_ALPHA_TEST);
}

inline void CriarTexturaBranca(PspTexture& tex){
    tex.width = 2;
    tex.height = 2;
    tex.nLevels = 1; // Avisa que só tem 1 nível
    
    // Aloca a memória para o Nível 0
    tex.data[0] = (unsigned int*)memalign(16, 2 * 2 * 4);
    
    // Ponteiro auxiliar aponta para o Nível 0
    unsigned int* pixels = (unsigned int*)tex.data[0];
    
    for (int i = 0; i < 4; i++) {
        pixels[i] = 0xFFFFFFFF;
    }
}

#endif