#!/usr/bin/env python3
"""
makefont.py - gera um atlas de fonte para o PSP a partir de um .ttf.

Produz um .raw (o mesmo formato que LoadTexturePsp le) com 16x16 = 256
caracteres ASCII numa grade. Cada celula tem CELL x CELL pixels.

O codigo C recorta a celula do caractere 'c' assim:
    coluna = c % 16;  linha = c / 16;
    u0 = coluna * CELL;  v0 = linha * CELL;

Uso:
    python makefont.py Poppins-Bold.ttf fonte.raw
    python makefont.py Poppins-Bold.ttf fonte.raw --cell 16 --size 14

Saida: atlas 256x256 (16 celulas de 16px). Potencia de 2, cabe no PSP.
"""

import sys
import struct
import argparse
from PIL import Image, ImageFont, ImageDraw


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("ttf")
    ap.add_argument("saida")
    ap.add_argument("--cell", type=int, default=16, help="tamanho da celula em px (default 16)")
    ap.add_argument("--size", type=int, default=0, help="tamanho da fonte (default: cell-2)")
    ap.add_argument("--preview", help="salva um PNG para conferir")
    args = ap.parse_args()

    cell = args.cell
    fontsize = args.size if args.size else cell - 2
    atlas = cell * 16   # 16 colunas -> largura total

    if atlas & (atlas - 1):
        print(f"ERRO: atlas {atlas}x{atlas} nao e potencia de 2. Ajuste --cell.")
        return 1
    if atlas > 512:
        print(f"ERRO: atlas {atlas}x{atlas} passa de 512. Reduza --cell.")
        return 1

    font = ImageFont.truetype(args.ttf, fontsize)
    img = Image.new("RGBA", (atlas, atlas), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    for code in range(32, 127):        # ASCII imprimivel
        col = code % 16
        row = code // 16
        ch = chr(code)
        # centraliza o glifo na celula
        bbox = draw.textbbox((0, 0), ch, font=font)
        gw = bbox[2] - bbox[0]
        gh = bbox[3] - bbox[1]
        px = col * cell + (cell - gw) // 2 - bbox[0]
        py = row * cell + (cell - gh) // 2 - bbox[1]
        draw.text((px, py), ch, font=font, fill=(255, 255, 255, 255))

    if args.preview:
        img.save(args.preview)
        print(f"  preview: {args.preview}")

    with open(args.saida, "wb") as f:
        f.write(struct.pack("<II", atlas, atlas))
        f.write(img.tobytes())

    print(f"{args.ttf} -> {args.saida}")
    print(f"  atlas {atlas}x{atlas}, celula {cell}px, fonte {fontsize}px")
    print(f"  {8 + atlas*atlas*4} bytes ({(8+atlas*atlas*4)/1024:.1f} KB)")
    print(f"  no C: #define FONT_CELL {cell}")
    return 0


if __name__ == "__main__":
    sys.exit(main())