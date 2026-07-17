#ifndef OBJETO3D_H
#define OBJETO3D_H

#include <iostream>
#include <fstream>
#include <cmath>
#include <malloc.h> // Necessário para memalign() no PSP

// Headers nativos do PSP
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h> // Para limpeza de cache

using namespace std;

// Struct auxiliar temporária apenas para leitura do arquivo
typedef struct {
    float X, Y, Z;
    float U, V;
    
    void Set(float x, float y, float z, float u = 0.0f, float v = 0.0f) {
        X = x; Y = y; Z = z; U = u; V = v;
    }
} TPoint;

// A ESTRUTURA OFICIAL DE VÉRTICE PARA O PSP
// A ordem (Textura -> Normal -> Posição) é obrigatória!
struct Vertex3D {
    float u, v;       // Textura (8 bytes)
    float nx, ny, nz; // Normal para iluminação (12 bytes)
    float x, y, z;    // Posição 3D (12 bytes)
};                    // Total = 32 bytes (múltiplo perfeito de 16)

void ProdVetorial(TPoint v1, TPoint v2, TPoint &vresult) {
    vresult.X = v1.Y * v2.Z - (v1.Z * v2.Y);
    vresult.Y = v1.Z * v2.X - (v1.X * v2.Z);
    vresult.Z = v1.X * v2.Y - (v1.Y * v2.X);
}

void VetUnitario(TPoint &vet) {
    float modulo = sqrt(vet.X * vet.X + vet.Y * vet.Y + vet.Z * vet.Z);
    if (modulo == 0.0) return;
    vet.X /= modulo;
    vet.Y /= modulo;
    vet.Z /= modulo;
}

class Objeto3D {
    Vertex3D *vertices;    // Array otimizado para a GPU do PSP
    unsigned int nVertices;

public:
    Objeto3D() {
        nVertices = 0;
        vertices = NULL;
    }

    // Boa prática: liberar a memória RAM quando o objeto for destruído
    ~Objeto3D() {
        if (vertices != NULL) {
            free(vertices);
        }
    }

    unsigned int getNFaces() {
        return nVertices / 3;
    }

    void leObjeto(const char *Nome) {
        FILE *fp = fopen(Nome, "r");
        if (fp == NULL) {
            cout << "erro ao abrir o arquivo " << Nome << endl;
            return;
        }

        int numVerticesTemp = 0;
        if (fscanf(fp, "%d", &numVerticesTemp) != 1) return;

        TPoint* tempVertices = new TPoint[numVerticesTemp];
        for (int i = 0; i < numVerticesTemp; i++) {
            float x, y, z, u, v;
            fscanf(fp, "%f %f %f %f %f", &x, &y, &z, &u, &v);
            tempVertices[i].Set(x, y, z, u, v);
        }

        unsigned int nFaces;
        fscanf(fp, "%u", &nFaces);
        
        nVertices = nFaces * 3;

        // ALOCAÇÃO ALINHADA: Crucial para o PSP! Garante blocos de 16 bytes
        vertices = (Vertex3D*) memalign(16, nVertices * sizeof(Vertex3D));

        int vIndex = 0;
        for (unsigned int i = 0; i < nFaces; i++) {
            int id1, id2, id3;
            fscanf(fp, "%d %d %d", &id1, &id2, &id3);

            TPoint P1 = tempVertices[id1];
            TPoint P2 = tempVertices[id2];
            TPoint P3 = tempVertices[id3];

            TPoint v1, v2, normal;
            v1.Set(P2.X - P1.X, P2.Y - P1.Y, P2.Z - P1.Z);
            v2.Set(P3.X - P1.X, P3.Y - P1.Y, P3.Z - P1.Z);
            
            ProdVetorial(v1, v2, normal);
            VetUnitario(normal);

            // Popula o array do PSP. Vértice 1:
            vertices[vIndex].u = P1.U; vertices[vIndex].v = P1.V;
            vertices[vIndex].nx = normal.X; vertices[vIndex].ny = normal.Y; vertices[vIndex].nz = normal.Z;
            vertices[vIndex].x = P1.X; vertices[vIndex].y = P1.Y; vertices[vIndex].z = P1.Z;
            vIndex++;

            // Vértice 2:
            vertices[vIndex].u = P2.U; vertices[vIndex].v = P2.V;
            vertices[vIndex].nx = normal.X; vertices[vIndex].ny = normal.Y; vertices[vIndex].nz = normal.Z;
            vertices[vIndex].x = P2.X; vertices[vIndex].y = P2.Y; vertices[vIndex].z = P2.Z;
            vIndex++;

            // Vértice 3:
            vertices[vIndex].u = P3.U; vertices[vIndex].v = P3.V;
            vertices[vIndex].nx = normal.X; vertices[vIndex].ny = normal.Y; vertices[vIndex].nz = normal.Z;
            vertices[vIndex].x = P3.X; vertices[vIndex].y = P3.Y; vertices[vIndex].z = P3.Z;
            vIndex++;
        }

        delete[] tempVertices;
        fclose(fp);

        // FLUSH DO CACHE: Avisa o PSP que a memória RAM foi atualizada 
        // e a placa gráfica (GU) já pode ler esses vértices com segurança.
        sceKernelDcacheWritebackInvalidateRange(vertices, nVertices * sizeof(Vertex3D));

        cout << "Modelo " << Nome << " carregado para PSP (" << nFaces << " faces)" << endl;
    }

    void desenha() {
        if (nVertices == 0 || vertices == NULL) return;
        
        // As flags que indicam a estrutura que montamos (Textura, Normal, Posição 3D)
        int format = GU_TEXTURE_32BITF | GU_NORMAL_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D;
        
        // Desenha toda a malha 3D de uma única vez!
        sceGumDrawArray(GU_TRIANGLES, format, nVertices, 0, vertices);
    }
};

#endif