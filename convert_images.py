from PIL import Image

def next_pow2(n):
    p = 1
    while p < n:
        p *= 2
    return p

def convert(path_in, path_out):
    # Lê a imagem
    img = Image.open(path_in).convert("RGBA")
    
    # MAGIA 1: Inverte a imagem de ponta-cabeça! 
    # Isso faz os UVs do OpenGL casarem perfeitamente com a leitura do PSP.
    img = img.transpose(Image.FLIP_TOP_BOTTOM)

    w, h = img.size
    nw, nh = next_pow2(w), next_pow2(h)

    if (nw, nh) != (w, h):
        # MAGIA 2: Usa NEAREST para não borrar as cores do seu Atlas!
        img = img.resize((nw, nh), Image.NEAREST)

    data = img.tobytes("raw", "RGBA")

    with open(path_out, "wb") as f:
        f.write(nw.to_bytes(4, "little"))
        f.write(nh.to_bytes(4, "little"))
        f.write(data)

    print(f"{path_in} -> {path_out} ({nw}x{nh}, original {w}x{h})")

if __name__ == "__main__":
# 1. Texturas dos modelos 3D (estão na pasta assets/tri/)
    model_textures = ["colormap", "variation-a", "variation-b"]
    for name in model_textures:
        convert(f"assets/tri/{name}.png", f"assets/tri/{name}.raw")

    # 2. Texturas das ruas (estão soltas na pasta assets/)
    map_textures = [
        "CROSS", "DL", "DLR", "DR", "LR", "None", 
        "UD", "UDL", "UDR", "UL", "ULR", "UR"
    ]
    for name in map_textures:
        # Lê de assets/ e salva o .raw na própria pasta assets/
        convert(f"assets/{name}.png", f"assets/{name}.raw")