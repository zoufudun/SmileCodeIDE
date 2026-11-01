"""High-quality logo generator for PhudonTools.

- Small sizes (16-32px): hand-tuned pixel-perfect simplified icons
- Medium sizes (48-64px): clean gradient circle with bold 'Phudon'
- Large sizes (128-512px): full artistic design
- Supersample: 16x for small, 8x for medium, 4x for large
"""
import math, os, sys
from PIL import Image, ImageDraw, ImageFont

OUT_DIR = os.path.dirname(os.path.abspath(__file__))

# ── Color Palette ──────────────────────────────────────────────
BLUE_DARKEST = (8,   55,  130)   # #083782 - deep navy (small icons)
BLUE_DARK    = (13,  71,  161)   # #0D47A1
BLUE_MID     = (25,  118, 210)   # #1976D2
BLUE_LIGHT   = (66,  165, 245)   # #42A5F5
WHITE        = (255, 255, 255)
WHITE_SOFT   = (240, 245, 255)
CLEAR        = (0, 0, 0, 0)


def lerp(c1, c2, t):
    return tuple(int(c1[i] + (c2[i] - c1[i]) * t) for i in range(3))


def find_font(size_px, bold=False):
    """Find the best available font at given pixel size."""
    names = [
        ("C:/Windows/Fonts/segoeui.ttf", False),
        ("C:/Windows/Fonts/seguisb.ttf", True),   # Segoe UI Semibold
        ("C:/Windows/Fonts/segoeuib.ttf", True),  # Segoe UI Bold
        ("C:/Windows/Fonts/segoeuil.ttf", False), # Segoe UI Light
        ("C:/Windows/Fonts/arial.ttf", False),
        ("C:/Windows/Fonts/arialbd.ttf", True),
    ]
    for path, is_bold in names:
        try:
            return ImageFont.truetype(path, size_px)
        except (OSError, IOError):
            continue
    return ImageFont.load_default()


def draw_circle_gradient(img, cx, cy, r, c_top, c_bot):
    """Slice-composited vertical gradient inside a circle."""
    w, h = img.size
    strips = max(60, int(2 * r * 0.6))
    for i in range(strips):
        t = i / max(strips - 1, 1)
        color = lerp(c_top, c_bot, t)
        y = (cy - r) + (2 * r) * i / strips
        strip_h = int((2 * r) / strips) + 1

        mask = Image.new('L', (w, strip_h), 0)
        mdraw = ImageDraw.Draw(mask)
        mdraw.ellipse([cx - r, -y + (cy - r), cx + r, -y + (cy + r)], fill=255)

        strip = Image.new('RGBA', (w, strip_h), (*color, 255))
        strip.putalpha(mask)
        img.paste(strip, (0, int(y)), strip)


# ══════════════════════════════════════════════════════════════
#  SMALL ICONS  (16, 24, 32px) — pixel-perfect hand-tuned
# ══════════════════════════════════════════════════════════════

def render_tiny(size):
    """16-32px: solid circle + single bold letter, no gradients."""
    ss = 16  # extreme supersample
    big = size * ss
    img = Image.new('RGBA', (big, big), CLEAR)
    draw = ImageDraw.Draw(img)
    cx = cy = big / 2

    # Circle radius — leave 1px margin at target size
    margin = int(1.5 * ss)
    r = big / 2 - margin

    # Solid blue circle
    draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(*BLUE_DARK, 255))

    # Thin white border
    border_w = max(2, int(1.2 * ss))
    draw.ellipse([cx - r, cy - r, cx + r, cy + r],
                 outline=(255, 255, 255, 80), width=border_w)

    # Letter "P" — largest possible font
    font_size = int(size * 0.65 * ss)
    font = find_font(font_size, bold=True)

    text = "P"
    bbox = draw.textbbox((0, 0), text, font=font)
    tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
    tx = cx - tw / 2 - bbox[0]
    ty = cy - th / 2 - bbox[1] - int(ss * 0.5)  # slight optical adjustment

    # White text with slight shadow for pop
    shadow_offset = max(1, int(ss * 0.4))
    draw.text((tx + shadow_offset, ty + shadow_offset), text, fill=(0, 0, 0, 60), font=font)
    draw.text((tx, ty), text, fill=WHITE, font=font)

    # Downscale with LANCZOS
    img = img.resize((size, size), Image.LANCZOS)
    return img


# ══════════════════════════════════════════════════════════════
#  MEDIUM ICONS  (48, 64px) — gradient circle + "Phudon"
# ══════════════════════════════════════════════════════════════

def render_medium(size):
    """48-64px: gradient circle, bold 'Phudon' text."""
    ss = 8
    big = size * ss
    img = Image.new('RGBA', (big, big), CLEAR)
    draw = ImageDraw.Draw(img)
    cx = cy = big / 2

    margin = int(3 * ss)
    r = big / 2 - margin

    # Gradient circle
    draw_circle_gradient(img, cx, cy, r, BLUE_DARK, BLUE_LIGHT)

    # Border
    draw.ellipse([cx - r, cy - r, cx + r, cy + r],
                 outline=(255, 255, 255, 60), width=max(2, int(1.5 * ss)))

    # Text "Phudon"
    font_size = int(size * 0.38 * ss)
    font = find_font(font_size, bold=True)

    text = "Phudon"
    bbox = draw.textbbox((0, 0), text, font=font)
    tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
    tx = cx - tw / 2 - bbox[0]
    ty = cy - th / 2 - bbox[1] - int(ss * 0.3)

    draw.text((tx + 1, ty + 1), text, fill=(0, 0, 0, 40), font=font)
    draw.text((tx, ty), text, fill=WHITE, font=font)

    img = img.resize((size, size), Image.LANCZOS)
    return img


# ══════════════════════════════════════════════════════════════
#  LARGE ICONS  (128, 256, 512px) — full artistic design
# ══════════════════════════════════════════════════════════════

def render_large(size):
    """128-512px: full art, gradient + rings + arcs + 'Phudon' + 'TOOLS'."""
    ss = 4
    big = size * ss
    img = Image.new('RGBA', (big, big), CLEAR)
    s = big / 512.0  # scale from 512 reference
    cx = cy = big / 2
    r = int(220 * s)

    # ── Gradient background ──
    draw_circle_gradient(img, cx, cy, r, BLUE_DARK, BLUE_LIGHT)

    draw = ImageDraw.Draw(img)

    # ── Rings ──
    ring_alpha = [60, 30, 15]
    ring_width = [max(2, int(3 * s)), max(2, int(2 * s)), max(1, int(1.5 * s))]
    for i in range(3):
        rr = r - int(6 * s) - i * int(12 * s)
        draw.ellipse([cx - rr, cy - rr, cx + rr, cy + rr],
                     outline=(255, 255, 255, ring_alpha[i]), width=ring_width[i])

    # ── Top decorative arc ──
    arc_r = int(164 * s)
    arc_bbox = [cx - arc_r, cy - int(186 * s), cx + arc_r, cy + int(186 * s)]
    draw.arc(arc_bbox, start=200, end=340,
             fill=(255, 255, 255, 40), width=max(2, int(3 * s)))
    draw.arc([cx - arc_r + int(8 * s), cy - int(178 * s),
              cx + arc_r - int(8 * s), cy + int(178 * s)],
             start=200, end=340, fill=(255, 255, 255, 20), width=max(1, int(1.5 * s)))

    # ── Bottom decorative arc ──
    draw.arc(arc_bbox, start=20, end=160,
             fill=(255, 255, 255, 30), width=max(2, int(2.5 * s)))

    # ── "Phudon" main text ──
    font_main = find_font(int(92 * s), bold=True)
    text = "Phudon"
    bbox = draw.textbbox((0, 0), text, font=font_main)
    tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
    tx = cx - tw / 2 - bbox[0]
    ty = cy - int(18 * s) - th / 2 - bbox[1]

    # Shadow + main
    shadow_off = max(2, int(2.5 * s))
    draw.text((tx + shadow_off, ty + shadow_off), text, fill=(0, 0, 0, 50), font=font_main)
    draw.text((tx, ty), text, fill=WHITE, font=font_main)

    # ── "TOOLS" subtitle ──
    font_sub = find_font(int(28 * s), bold=False)
    sub_text = "TOOLS"
    spacing = int(24 * s)
    # Measure
    char_widths = []
    total_w = 0
    for ch in sub_text:
        cb = draw.textbbox((0, 0), ch, font=font_sub)
        cw = cb[2] - cb[0]
        char_widths.append(cw)
        total_w += cw
    total_w += spacing * (len(sub_text) - 1)

    sx = cx - total_w / 2
    sy = cy + int(74 * s)
    for i, ch in enumerate(sub_text):
        draw.text((sx, sy), ch, fill=(255, 255, 255, 160), font=font_sub)
        sx += char_widths[i] + spacing

    # ── Decorative dots ──
    dot_top_y = cy - int(166 * s)
    dot_bot_y = cy + int(168 * s)
    r_dot_big = max(2, int(4 * s))
    r_dot_sml = max(1, int(2.5 * s))
    draw.ellipse([cx - r_dot_big, dot_top_y - r_dot_big,
                  cx + r_dot_big, dot_top_y + r_dot_big],
                 fill=(255, 255, 255, 100))
    draw.ellipse([cx - r_dot_sml, dot_top_y - int(12 * s) - r_dot_sml,
                  cx + r_dot_sml, dot_top_y - int(12 * s) + r_dot_sml],
                 fill=(255, 255, 255, 60))
    draw.ellipse([cx - r_dot_sml, dot_bot_y - r_dot_sml,
                  cx + r_dot_sml, dot_bot_y + r_dot_sml],
                 fill=(255, 255, 255, 60))

    # ── Highlight glow overlay ──
    overlay = Image.new('RGBA', (big, big), CLEAR)
    odraw = ImageDraw.Draw(overlay)
    # Top-left soft highlight
    for i in range(30):
        alpha = int(8 * (1 - i / 30))
        rr = r - i * (r / 40)
        odraw.ellipse([cx - rr * 0.6, cy - rr * 0.6, cx + rr * 0.3, cy + rr * 0.1],
                      fill=(255, 255, 255, alpha))
    img = Image.alpha_composite(img, overlay)

    img = img.resize((size, size), Image.LANCZOS)
    return img


# ══════════════════════════════════════════════════════════════
#  DISPATCH
# ══════════════════════════════════════════════════════════════

def render(size):
    if size <= 32:
        return render_tiny(size)
    elif size <= 64:
        return render_medium(size)
    else:
        return render_large(size)


# ══════════════════════════════════════════════════════════════
#  MAIN
# ══════════════════════════════════════════════════════════════

if __name__ == '__main__':
    # ── PNG 512px ──
    logo_512 = render_large(512)
    png_path = os.path.join(OUT_DIR, 'logo.png')
    logo_512.save(png_path, 'PNG')
    size_kb = os.path.getsize(png_path) / 1024
    print(f'[OK] logo.png  512x512  {size_kb:.0f} KB')

    # ── ICO with all sizes, each individually rendered ──
    ico_defs = [
        (256, 'large'), (128, 'large'), (64, 'medium'),
        (48, 'medium'), (32, 'tiny'),   (24, 'tiny'), (16, 'tiny')
    ]
    frames = []
    for w, mode in ico_defs:
        f = render(w)
        if f.mode != 'RGBA':
            f = f.convert('RGBA')
        frames.append(f)
        print(f'  |-- {w}x{w} ({mode})')

    ico_sizes = [(w, w) for w, _ in ico_defs]
    for name in ('app.ico', 'logo.ico'):
        path = os.path.join(OUT_DIR, name)
        frames[0].save(path, format='ICO', sizes=ico_sizes,
                       append_images=frames[1:])
        info = Image.open(path).info.get('sizes', '?')
        size_kb = os.path.getsize(path) / 1024
        print(f'[OK] {name}  {size_kb:.0f} KB  sizes={info}')

    print('\nDone — all logo files generated.')
