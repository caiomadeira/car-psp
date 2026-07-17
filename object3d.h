#ifndef OBJETO3D_H
#define OBJETO3D_H

#include <pspgu.h>
#include <pspgum.h>
#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include "texture.h"

// ORDEM IMPORTA para o GU: textura, cor, normal, posicao
struct TriVertex
{
    float u, v;
    unsigned int color;
    float nx, ny, nz;
    float x, y, z;
};

struct TPoint3
{
    float X, Y, Z, U, V;
};

static void ProdVetorial(TPoint3 v1, TPoint3 v2, TPoint3 &r)
{
    r.X = v1.Y * v2.Z - v1.Z * v2.Y;
    r.Y = v1.Z * v2.X - v1.X * v2.Z;
    r.Z = v1.X * v2.Y - v1.Y * v2.X;
}

static void VetUnitario(TPoint3 &v)
{
    float mod = sqrtf(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
    if (mod == 0.0f) return;
    v.X /= mod; v.Y /= mod; v.Z /= mod;
}

class Objeto3D
{
    TriVertex* verts;
    unsigned int nFaces;
    PspTexture textura;
    bool temTextura;

    // NOVO: bounding box do modelo em espaço local
    float minX, maxX, minY, maxY, minZ, maxZ;

public:
    Objeto3D() : verts(NULL), nFaces(0), temTextura(false),
                 minX(0), maxX(0), minY(0), maxY(0), minZ(0), maxZ(0) {}

    unsigned int getNFaces() { return nFaces; }

    // NOVO: getters de footprint
    float getSizeX() { return maxX - minX; }
    float getSizeZ() { return maxZ - minZ; }
    float getCenterX() { return (minX + maxX) * 0.5f; }
    float getCenterZ() { return (minZ + maxZ) * 0.5f; }

    bool carregarTextura(const char* arquivoRaw)
    {
        temTextura = LoadTexturePsp(arquivoRaw, textura);
        return temTextura;
    }

    // color: usado como tint/cor base (multiplica com a textura via GU_TFX_MODULATE).
    // Se nao tiver textura carregada, funciona como antes (cor solida).
    bool leObjeto(const char* nome, unsigned int color)
    {
        FILE* fp = fopen(nome, "r");
        if (!fp) return false;

        int numVertices = 0;
        if (fscanf(fp, "%d", &numVertices) != 1) { fclose(fp); return false; }

        TPoint3* tempVertices = new TPoint3[numVertices];

        // NOVO: inicializa bbox com extremos invertidos
        minX = minY = minZ =  1e9f;
        maxX = maxY = maxZ = -1e9f;

        for (int i = 0; i < numVertices; i++)
        {
            float x, y, z, u, v;
            fscanf(fp, "%f %f %f %f %f", &x, &y, &z, &u, &v);
            tempVertices[i] = { x, y, z, u, v };

            // NOVO: atualiza bbox
            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
            if (y < minY) minY = y;
            if (y > maxY) maxY = y;
            if (z < minZ) minZ = z;
            if (z > maxZ) maxZ = z;
        }

        if (fscanf(fp, "%u", &nFaces) != 1)
        {
            delete[] tempVertices;
            fclose(fp);
            return false;
        }

        verts = (TriVertex*)memalign(16, sizeof(TriVertex) * nFaces * 3);

        for (unsigned int i = 0; i < nFaces; i++)
        {
            int id1, id2, id3;
            fscanf(fp, "%d %d %d", &id1, &id2, &id3);

            TPoint3 p1 = tempVertices[id1];
            TPoint3 p2 = tempVertices[id2];
            TPoint3 p3 = tempVertices[id3];

            TPoint3 v1 = { p2.X - p1.X, p2.Y - p1.Y, p2.Z - p1.Z, 0, 0 };
            TPoint3 v2 = { p3.X - p1.X, p3.Y - p1.Y, p3.Z - p1.Z, 0, 0 };
            TPoint3 normal;
            ProdVetorial(v1, v2, normal);
            VetUnitario(normal);

            TriVertex* v = &verts[i * 3];

            v[0].u = p1.U; v[0].v = p1.V;
            v[0].color = color;
            v[0].nx = normal.X; v[0].ny = normal.Y; v[0].nz = normal.Z;
            v[0].x = p1.X; v[0].y = p1.Y; v[0].z = p1.Z;

            v[1].u = p2.U; v[1].v = p2.V;
            v[1].color = color;
            v[1].nx = normal.X; v[1].ny = normal.Y; v[1].nz = normal.Z;
            v[1].x = p2.X; v[1].y = p2.Y; v[1].z = p2.Z;

            v[2].u = p3.U; v[2].v = p3.V;
            v[2].color = color;
            v[2].nx = normal.X; v[2].ny = normal.Y; v[2].nz = normal.Z;
            v[2].x = p3.X; v[2].y = p3.Y; v[2].z = p3.Z;
        }

        delete[] tempVertices;
        fclose(fp);
        
        if (!temTextura)
        {
            CriarTexturaBranca(textura);
            temTextura = true;
        }
        return true;
    }

    void desenha()
    {
        if (nFaces == 0 || verts == NULL) return;

        sceGuDisable(GU_CULL_FACE);   // <-- modelos .tri tem winding inconsistente

        UseTexturePsp(textura);
        int vtype = GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_NORMAL_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D;
        sceGumDrawArray(GU_TRIANGLES, vtype, nFaces * 3, NULL, verts);
        DisableTexturePsp();

        sceGuEnable(GU_CULL_FACE);    // reabilita pro resto (chao, etc)
    }
};

#endif