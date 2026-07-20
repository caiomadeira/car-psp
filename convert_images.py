from PIL import Image

def next_pow2(n):
    p = 1
    while p < n:
        p *= 2
    return min(p, 512)

def convert(path_in, path_out):
    # Lê a imagem
    img = Image.open(path_in).convert("RGBA")
    
    # Inverte a imagem de ponta-cabeça para casar com o OpenGL
    img = img.transpose(Image.FLIP_TOP_BOTTOM)

    w, h = img.size
    nw, nh = next_pow2(w), next_pow2(h)

    if (nw, nh) != (w, h):
        # CORREÇÃO: Usando LANCZOS para tudo! 
        # Ele suaviza a imagem ao redimensionar, tirando o aspecto "nojento/serrilhado" da areia e asfalto.
        img = img.resize((nw, nh), Image.LANCZOS)

    data = img.tobytes("raw", "RGBA")

    with open(path_out, "wb") as f:
        f.write(nw.to_bytes(4, "little"))
        f.write(nh.to_bytes(4, "little"))
        f.write(data)

    print(f"{path_in} -> {path_out} ({nw}x{nh}, original {w}x{h})")

if __name__ == "__main__":
    model_textures = ["colormap", "variation-a", "variation-b"]
    for name in model_textures:
        convert(f"assets/tri/{name}.png", f"assets/tri/{name}.raw")

    map_textures = [
        "CROSS.jpg", "DL.png", "DLR.png", "DR.png", "LR.png", "None.png",
        "UD.png", "UDL.png", "UDR.png", "UL.png", "ULR.png", "UR.png",
        "bricks.png", "Piso.jpg", "sand.png", "sidewalk_beach.png", "sidewalk.png",
        "asphalt.png", "house_textures.png"
    ]
    for name in map_textures:
        base = name.rsplit(".", 1)[0]
        convert(f"raw_img/tex/{name}", f"assets/tex/{base}.raw")