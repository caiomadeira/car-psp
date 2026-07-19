from PIL import Image

def convert_bg_psp(path_in, path_out):
    img = Image.open(path_in).convert("RGBA")
    
    # O SEGREDO: Forçar uma resolução "Potência de 2" (512x512)
    # O PSP vai espremer isso perfeitamente de volta para 480x272 na tela
    img = img.resize((512, 512), Image.LANCZOS)

    data = img.tobytes("raw", "RGBA")

    with open(path_out, "wb") as f:
        # Salvando as dimensões reais da textura
        f.write((512).to_bytes(4, "little"))
        f.write((512).to_bytes(4, "little"))
        f.write(data)

    print(f"Sucesso: {path_in} -> {path_out} (Convertido para 512x512 para o PSP)")

if __name__ == "__main__":
    # Roda o script de novo para gerar o seu background corretamente
    convert_bg_psp("assets/img/test_bg.jpg", "assets/img/test_bg.raw")