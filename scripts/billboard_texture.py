from PIL import Image
im = Image.open("../raw_img/billboard/tree.png").convert("RGBA")
a = [p[3] for p in im.getdata()]
print("opacos %d  transparentes %d  PARCIAIS %d"
      % (a.count(255), a.count(0), len(a)-a.count(255)-a.count(0)))