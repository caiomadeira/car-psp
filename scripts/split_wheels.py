#!/usr/bin/env python3
"""
split_wheels.py - separa um .tri que contem as 4 rodas juntas em arquivos
individuais, cada um CENTRADO na propria origem.

Por que: para esterçar as rodas dianteiras (modelo de bicicleta), cada roda
precisa ser um modelo proprio, girando em torno do proprio eixo. Um .tri com
as 4 juntas so permite rodar o conjunto inteiro.

Saida:
  - wheel.tri            : UMA roda, centrada na origem (desenhada 4x no jogo)
  - relatorio das posicoes de cada roda, para preencher a tabela WHEELS[] no C

Uso:
  python split_wheels.py uno_tiles.tri saida/
"""

import sys, os


def ler_tri(path):
    with open(path) as f:
        n = int(f.readline().split()[0])
        V = []
        for _ in range(n):
            t = f.readline().split()
            V.append(tuple(float(c) for c in t[:5]) if len(t) >= 5
                     else (float(t[0]), float(t[1]), float(t[2]), 0.0, 0.0))
        nf = int(f.readline().split()[0])
        F = [tuple(int(c) for c in f.readline().split()[:3]) for _ in range(nf)]
    return V, F


def escrever_tri(path, V, F):
    with open(path, "w") as f:
        f.write("%d\n" % len(V))
        for v in V:
            f.write("%.6f %.6f %.6f %.6f %.6f\n" % v)
        f.write("%d\n" % len(F))
        for a, b, c in F:
            f.write("%d %d %d\n" % (a, b, c))


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    src, outdir = sys.argv[1], sys.argv[2]
    os.makedirs(outdir, exist_ok=True)

    V, F = ler_tri(src)

    # centro geral do modelo: divisor dos quadrantes
    xs = [v[0] for v in V]; zs = [v[2] for v in V]
    mx = (min(xs) + max(xs)) / 2.0
    mz = (min(zs) + max(zs)) / 2.0

    # separa as FACES em 4 grupos pelo centroide
    grupos = {}
    for (a, b, c) in F:
        cx = (V[a][0] + V[b][0] + V[c][0]) / 3.0
        cz = (V[a][2] + V[b][2] + V[c][2]) / 3.0
        chave = ('D' if cz > mz else 'T',      # Dianteira / Traseira
                 'D' if cx > mx else 'E')      # Direita   / Esquerda
        grupos.setdefault(chave, []).append((a, b, c))

    nomes = {('D','E'): 'dianteira_esquerda', ('D','D'): 'dianteira_direita',
             ('T','E'): 'traseira_esquerda',  ('T','D'): 'traseira_direita'}

    print("centro do modelo: X=%+.4f  Z=%+.4f" % (mx, mz))
    print()
    print("%-22s %6s %6s   centro local (X, Y, Z)" % ("roda", "vert", "faces"))

    posicoes = {}
    maior = None

    for chave, faces in sorted(grupos.items()):
        # reindexa: so os vertices usados por este grupo
        usados = sorted({i for f3 in faces for i in f3})
        remap = {vi: k for k, vi in enumerate(usados)}
        VV = [V[i] for i in usados]
        FF = [(remap[a], remap[b], remap[c]) for (a, b, c) in faces]

        # centro desta roda
        gx = [v[0] for v in VV]; gy = [v[1] for v in VV]; gz = [v[2] for v in VV]
        cx = (min(gx) + max(gx)) / 2.0
        cy = (min(gy) + max(gy)) / 2.0
        cz = (min(gz) + max(gz)) / 2.0

        # move para a origem: o giro passa a ser em torno do proprio eixo
        VC = [(v[0]-cx, v[1]-cy, v[2]-cz, v[3], v[4]) for v in VV]

        nome = nomes[chave]
        escrever_tri(os.path.join(outdir, "wheel_%s.tri" % nome), VC, FF)
        posicoes[nome] = (cx, cy, cz)

        raio = (max(gy) - min(gy)) / 2.0
        print("%-22s %6d %6d   (%+.4f, %+.4f, %+.4f)  raio=%.4f"
              % (nome, len(VV), len(FF), cx, cy, cz, raio))

        if maior is None or len(FF) > maior[1]:
            maior = (VC, len(FF), FF)

    # UMA roda generica, para desenhar 4x (economiza RAM)
    escrever_tri(os.path.join(outdir, "wheel.tri"), maior[0], maior[2])
    print()
    print("wheel.tri  (roda unica, %d faces) -- desenhe 4x no jogo" % maior[1])

    # tabela pronta para colar no C
    print()
    print("/* cole no main.cpp -- posicoes em ESPACO DE MODELO */")
    print("static const struct { float x, y, z; int steers; } WHEELS[4] = {")
    for nome in ['dianteira_esquerda','dianteira_direita',
                 'traseira_esquerda','traseira_direita']:
        x, y, z = posicoes[nome]
        st = 1 if nome.startswith('dianteira') else 0
        print("    { %+.4ff, %+.4ff, %+.4ff, %d },   /* %s */" % (x, y, z, st, nome))
    print("};")

    # entre-eixos e raio, para o modelo de bicicleta
    zd = (posicoes['dianteira_esquerda'][2] + posicoes['dianteira_direita'][2]) / 2
    zt = (posicoes['traseira_esquerda'][2]  + posicoes['traseira_direita'][2])  / 2
    xe = (posicoes['dianteira_esquerda'][0] + posicoes['traseira_esquerda'][0]) / 2
    xd = (posicoes['dianteira_direita'][0]  + posicoes['traseira_direita'][0])  / 2
    print()
    print("entre-eixos (L) no modelo : %.4f" % abs(zd - zt))
    print("bitola      (B) no modelo : %.4f" % abs(xd - xe))
    return 0


if __name__ == "__main__":
    sys.exit(main())