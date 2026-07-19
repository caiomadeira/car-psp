from PIL import Image

def next_pow2(n):
    p = 1
    while p < n:
        p *= 2
    return min(p, 512)

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
        if w > 512 or h > 512:
            img = img.resize((nw, nh), Image.LANCZOS)
        else:
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
        "CROSS.jpg", "DL.png", "DLR.png", "DR.png", "LR.png", "None.png", 
        "UD.png", "UDL.png", "UDR.png", "UL.png", "ULR.png", "UR.png",
        "bricks.jpg", "Piso.jpg", "sand.png", "sidewalk_beach.png", "sidewalk.png"
    ]
    out = ""
    for name in map_textures:
        if ".png" in name:
            out = f"assets/tex/{name.replace(".png", "")}.raw"
        if ".jpg" in name:
            out = f"assets/tex/{name.replace(".jpg", "")}.raw"
        convert(f"raw_img/tex/{name}", out)