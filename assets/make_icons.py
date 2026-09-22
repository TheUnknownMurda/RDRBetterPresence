"""Generates the small-image assets for the Discord Rich Presence.

    python assets/make_icons.py            -> assets/small/<key>.png (512x512) + assets/preview_small.png

Every icon is a bold cream pictogram on a dark disc with a red ring, drawn as SVG in a
100x100 box and rasterised with resvg. Discord shows the small image as a ~30 px circle,
so shapes stay chunky and centred inside a radius of ~36 units.

Requires: pip install resvg-py pillow
"""
import io
import math
import os

import resvg_py
from PIL import Image, ImageDraw, ImageFont

BG = "#1b1918"
RING = "#b2261c"
INK = "#f4ead6"
SIZE = 512

OUT_DIR = os.path.join(os.path.dirname(__file__), "small")


def star(cx, cy, r_out, r_in, points=5):
    pts = []
    for i in range(points * 2):
        r = r_out if i % 2 == 0 else r_in
        a = -math.pi / 2 + i * math.pi / points
        pts.append(f"{cx + r * math.cos(a):.1f},{cy + r * math.sin(a):.1f}")
    return " ".join(pts)


def gun_lines(rotation):
    # A pistol drawn with two thick strokes (barrel + grip), used twice for the duel icon.
    return (f'<g transform="translate(50 52) rotate({rotation})">'
            f'<path d="M-22 -6 H18 M-6 -6 L-12 14" stroke="{INK}" stroke-width="9" stroke-linecap="round" fill="none"/>'
            f'</g>')


# Each glyph is SVG markup in a 100x100 box. Fills/strokes use INK unless they cut into a shape (BG).
GLYPHS = {
    "paused": f'<rect x="33" y="30" width="11" height="40" rx="3" fill="{INK}"/>'
              f'<rect x="56" y="30" width="11" height="40" rx="3" fill="{INK}"/>',

    "dead": f'<circle cx="50" cy="44" r="21" fill="{INK}"/>'
            f'<rect x="37" y="56" width="26" height="16" rx="4" fill="{INK}"/>'
            f'<circle cx="42" cy="44" r="5.5" fill="{BG}"/><circle cx="58" cy="44" r="5.5" fill="{BG}"/>'
            f'<path d="M47 54 L53 54 L50 49 Z" fill="{BG}"/>'
            f'<path d="M45 64 V72 M50 64 V72 M55 64 V72" stroke="{BG}" stroke-width="2.5"/>',

    "cutscene": f'<rect x="26" y="44" width="48" height="30" rx="3" fill="{INK}"/>'
                f'<g transform="rotate(-15 26 42)"><rect x="26" y="28" width="48" height="14" rx="2" fill="{INK}"/>'
                f'<path d="M38 28 L32 42 M52 28 L46 42 M66 28 L60 42" stroke="{BG}" stroke-width="4"/></g>'
                f'<circle cx="50" cy="60" r="4" fill="{BG}"/>',

    "minigame": f'<rect x="33" y="24" width="34" height="52" rx="4" fill="{INK}"/>'
                f'<path d="M50 36 C44 42 38 46 38 52 C38 56 42 58 46 56 C47 55 48 54 50 53 C52 54 53 55 54 56 '
                f'C58 58 62 56 62 52 C62 46 56 42 50 36 Z" fill="{BG}"/>'
                f'<path d="M46 63 L54 63 L50 55 Z" fill="{BG}"/>',

    "mission": f'<polygon points="{star(50, 52, 27, 11.5)}" fill="{INK}"/>',

    "duel": gun_lines(-40) + gun_lines(40),

    "lasso": f'<circle cx="46" cy="42" r="17" stroke="{INK}" stroke-width="8" fill="none"/>'
             f'<path d="M60 54 C72 62 56 70 66 82" stroke="{INK}" stroke-width="8" stroke-linecap="round" fill="none"/>'
             f'<circle cx="60" cy="54" r="5.5" fill="{INK}"/>',

    "deadeye": f'<circle cx="50" cy="50" r="21" stroke="{INK}" stroke-width="8" fill="none"/>'
               f'<path d="M50 18 V34 M50 66 V82 M18 50 H34 M66 50 H82" stroke="{INK}" stroke-width="8" stroke-linecap="round"/>'
               f'<circle cx="50" cy="50" r="5" fill="{INK}"/>',

    "train": f'<rect x="28" y="22" width="9" height="14" fill="{INK}"/>'
             f'<rect x="22" y="34" width="34" height="22" rx="9" fill="{INK}"/>'
             f'<rect x="54" y="26" width="22" height="30" rx="2" fill="{INK}"/>'
             f'<rect x="60" y="32" width="10" height="9" fill="{BG}"/>'
             f'<rect x="18" y="56" width="64" height="8" rx="2" fill="{INK}"/>'
             f'<circle cx="32" cy="70" r="8" fill="{INK}"/><circle cx="50" cy="70" r="8" fill="{INK}"/><circle cx="68" cy="70" r="8" fill="{INK}"/>',

    "stagecoach": f'<rect x="22" y="28" width="56" height="6" rx="2" fill="{INK}"/>'
                  f'<rect x="26" y="32" width="48" height="26" rx="4" fill="{INK}"/>'
                  f'<rect x="34" y="38" width="12" height="10" fill="{BG}"/><rect x="54" y="38" width="12" height="10" fill="{BG}"/>'
                  f'<circle cx="34" cy="68" r="10" stroke="{INK}" stroke-width="7" fill="none"/>'
                  f'<circle cx="66" cy="68" r="10" stroke="{INK}" stroke-width="7" fill="none"/>'
                  f'<circle cx="34" cy="68" r="3" fill="{INK}"/><circle cx="66" cy="68" r="3" fill="{INK}"/>',

    # Cowboy boot: shaft, foot with a pointed toe, heel and a spur.
    "onfoot": f'<path d="M36 18 H60 V50 C60 54 64 56 70 58 L82 62 C86 64 86 70 82 72 H40 L36 66 Z" fill="{INK}"/>'
              f'<rect x="40" y="66" width="12" height="10" fill="{INK}"/>'
              f'<path d="M36 26 H60" stroke="{BG}" stroke-width="3"/>'
              f'<polygon points="{star(30, 63, 7, 3, 6)}" fill="{INK}"/>',

    "horse": f'<path d="M24 60 L28 48 C32 40 40 34 50 30 L54 20 L58 30 L64 22 L66 32 C74 36 78 46 78 58 L78 78 '
             f'L56 78 C56 66 50 60 42 62 L38 70 C30 72 26 68 24 60 Z" fill="{INK}"/>'
             f'<circle cx="57" cy="42" r="3.2" fill="{BG}"/><circle cx="31" cy="55" r="2.5" fill="{BG}"/>',

    "weapon_pistol": f'<rect x="22" y="34" width="54" height="15" rx="2" fill="{INK}"/>'
                     f'<polygon points="44,49 62,49 57,76 41,76" fill="{INK}"/>'
                     f'<path d="M36 49 C30 56 34 62 40 60" stroke="{INK}" stroke-width="5" fill="none" stroke-linecap="round"/>',

    "weapon_revolver": f'<rect x="16" y="38" width="38" height="9" rx="2" fill="{INK}"/>'
                       f'<circle cx="56" cy="45" r="11" fill="{INK}"/><circle cx="56" cy="45" r="3.5" fill="{BG}"/>'
                       f'<rect x="60" y="38" width="16" height="14" rx="2" fill="{INK}"/>'
                       f'<path d="M64 52 L78 52 L84 72 L70 78 L62 62 Z" fill="{INK}"/>'
                       f'<path d="M50 52 C46 60 52 66 58 62" stroke="{INK}" stroke-width="5" fill="none" stroke-linecap="round"/>'
                       f'<path d="M72 34 L78 28" stroke="{INK}" stroke-width="5" stroke-linecap="round"/>',

    "weapon_repeater": f'<path d="M16 44 H60" stroke="{INK}" stroke-width="6" stroke-linecap="round"/>'
                       f'<rect x="50" y="39" width="18" height="11" rx="2" fill="{INK}"/>'
                       f'<polygon points="66,39 86,44 86,62 74,62 66,50" fill="{INK}"/>'
                       f'<ellipse cx="58" cy="57" rx="8" ry="6" stroke="{INK}" stroke-width="5" fill="none"/>',

    "weapon_rifle": f'<path d="M14 42 H62" stroke="{INK}" stroke-width="6" stroke-linecap="round"/>'
                    f'<rect x="28" y="42" width="26" height="6" rx="2" fill="{INK}"/>'
                    f'<rect x="52" y="37" width="16" height="11" rx="2" fill="{INK}"/>'
                    f'<path d="M60 38 L66 28" stroke="{INK}" stroke-width="5" stroke-linecap="round"/><circle cx="67" cy="27" r="3.5" fill="{INK}"/>'
                    f'<polygon points="68,37 86,42 86,60 74,60 68,48" fill="{INK}"/>',

    "weapon_shotgun": f'<path d="M14 40 H58 M14 47.5 H58" stroke="{INK}" stroke-width="5.5" stroke-linecap="round"/>'
                      f'<rect x="54" y="35" width="14" height="16" rx="2" fill="{INK}"/>'
                      f'<polygon points="68,35 86,40 86,60 74,60 68,48" fill="{INK}"/>',

    "weapon_sniper": f'<path d="M14 46 H62" stroke="{INK}" stroke-width="6" stroke-linecap="round"/>'
                     f'<rect x="52" y="41" width="16" height="11" rx="2" fill="{INK}"/>'
                     f'<polygon points="68,41 86,46 86,64 74,64 68,52" fill="{INK}"/>'
                     f'<rect x="38" y="28" width="26" height="8" rx="4" fill="{INK}"/>'
                     f'<path d="M44 36 V41 M58 36 V41" stroke="{INK}" stroke-width="4"/>',

    "weapon_lasso": f'<circle cx="48" cy="46" r="20" stroke="{INK}" stroke-width="8" fill="none"/>'
                    f'<circle cx="48" cy="46" r="9" stroke="{INK}" stroke-width="6" fill="none"/>'
                    f'<path d="M64 60 C74 66 60 74 70 84" stroke="{INK}" stroke-width="8" stroke-linecap="round" fill="none"/>',

    "weapon_melee": f'<polygon points="16,50 54,40 58,50 54,58" fill="{INK}"/>'
                    f'<rect x="52" y="38" width="6" height="24" rx="1" fill="{INK}"/>'
                    f'<rect x="57" y="44" width="26" height="12" rx="4" fill="{INK}"/>',

    "weapon_explosive": f'<g transform="rotate(-30 50 52)"><rect x="24" y="44" width="46" height="16" rx="4" fill="{INK}"/>'
                        f'<path d="M36 44 V60 M48 44 V60" stroke="{BG}" stroke-width="3"/></g>'
                        f'<path d="M66 36 C72 32 70 24 76 22" stroke="{INK}" stroke-width="5" stroke-linecap="round" fill="none"/>'
                        f'<polygon points="{star(78, 20, 7, 3, 4)}" fill="{INK}"/>',

    "weapon_thrown": f'<g transform="rotate(-45 50 50)"><polygon points="22,50 50,43 54,50 50,57" fill="{INK}"/>'
                     f'<rect x="52" y="45" width="22" height="10" rx="4" fill="{INK}"/></g>'
                     f'<path d="M24 26 L34 32 M18 38 L28 42" stroke="{INK}" stroke-width="4" stroke-linecap="round"/>',

    "weapon_turret": f'<path d="M16 38 H60 M16 46 H60 M16 54 H60" stroke="{INK}" stroke-width="5" stroke-linecap="round"/>'
                     f'<circle cx="64" cy="46" r="12" fill="{INK}"/>'
                     f'<path d="M64 58 L50 82 M64 58 L78 82" stroke="{INK}" stroke-width="6" stroke-linecap="round"/>'
                     f'<path d="M74 40 L82 34" stroke="{INK}" stroke-width="5" stroke-linecap="round"/><circle cx="83" cy="33" r="3.5" fill="{INK}"/>',

    "weapon_cannon": f'<g transform="rotate(-22 50 50)"><polygon points="18,42 70,36 70,54 18,58" fill="{INK}"/>'
                     f'<rect x="20" y="38" width="6" height="24" rx="2" fill="{INK}"/></g>'
                     f'<circle cx="62" cy="64" r="12" stroke="{INK}" stroke-width="7" fill="none"/>'
                     f'<path d="M62 52 V76 M50 64 H74" stroke="{INK}" stroke-width="4"/>',

    "weapon_bow": f'<path d="M34 18 Q74 50 34 82" stroke="{INK}" stroke-width="8" stroke-linecap="round" fill="none"/>'
                  f'<path d="M34 18 V82" stroke="{INK}" stroke-width="3.5"/>'
                  f'<path d="M22 50 H70" stroke="{INK}" stroke-width="5" stroke-linecap="round"/>'
                  f'<polygon points="82,50 68,43 68,57" fill="{INK}"/>'
                  f'<path d="M26 44 L32 50 L26 56" stroke="{INK}" stroke-width="4" fill="none" stroke-linecap="round"/>',
}


def svg_for(glyph):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" width="{SIZE}" height="{SIZE}">'
            f'<circle cx="50" cy="50" r="49" fill="{BG}"/>'
            f'<circle cx="50" cy="50" r="45" stroke="{RING}" stroke-width="4" fill="none"/>'
            f'{glyph}</svg>')


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    images = []
    for key, glyph in GLYPHS.items():
        png = resvg_py.svg_to_bytes(svg_string=svg_for(glyph), width=SIZE, height=SIZE)
        path = os.path.join(OUT_DIR, f"{key}.png")
        with open(path, "wb") as f:
            f.write(png)
        images.append((key, Image.open(io.BytesIO(png)).convert("RGBA")))
        print("wrote", path)

    # Contact sheet: each icon at 128 px and, next to it, at the ~32 px Discord shows it.
    cols = 6
    cell, pad = 150, 16
    rows = math.ceil(len(images) / cols)
    sheet = Image.new("RGBA", (cols * cell + pad, rows * (cell + 30) + pad), "#2b2d31")
    draw = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.truetype("segoeui.ttf", 15)
    except OSError:
        font = ImageFont.load_default()
    for i, (key, img) in enumerate(images):
        x = pad + (i % cols) * cell
        y = pad + (i // cols) * (cell + 30)
        sheet.paste(img.resize((100, 100), Image.LANCZOS), (x, y), img.resize((100, 100), Image.LANCZOS))
        small = img.resize((32, 32), Image.LANCZOS)
        sheet.paste(small, (x + 104, y + 68), small)
        draw.text((x, y + 108), key, fill="#dddddd", font=font)
    preview = os.path.join(os.path.dirname(__file__), "preview_small.png")
    sheet.save(preview)
    print("wrote", preview)


if __name__ == "__main__":
    main()
