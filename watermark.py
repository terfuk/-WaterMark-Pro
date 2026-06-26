# watermark.py
#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import argparse
from PIL import Image, ImageDraw, ImageFont, ImageEnhance

# ANSI colors
COLORS = {
    'green': '\033[92m',
    'red': '\033[91m',
    'yellow': '\033[93m',
    'reset': '\033[0m'
}

def colorize(text, color):
    return f"{COLORS.get(color, '')}{text}{COLORS['reset']}"

POSITIONS = {
    'tl': (0.05, 0.05),
    'tc': (0.5, 0.05),
    'tr': (0.95, 0.05),
    'ml': (0.05, 0.5),
    'c': (0.5, 0.5),
    'mr': (0.95, 0.5),
    'bl': (0.05, 0.95),
    'bc': (0.5, 0.95),
    'br': (0.95, 0.95)
}

def get_position(img, pos_key, offset=10):
    w, h = img.size
    x_ratio, y_ratio = POSITIONS.get(pos_key, POSITIONS['br'])
    x = int(w * x_ratio) - offset
    y = int(h * y_ratio) - offset
    return x, y

def add_text_watermark(input_path, output_path, text, position='br', opacity=0.5,
                       font_size=48, color='#FFFFFF', rotate=0):
    img = Image.open(input_path).convert('RGBA')
    txt = Image.new('RGBA', img.size, (255, 255, 255, 0))
    draw = ImageDraw.Draw(txt)

    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVu-Sans.ttf", font_size)
    except:
        font = ImageFont.load_default()

    # Вычисляем размер текста
    bbox = draw.textbbox((0, 0), text, font=font)
    text_w = bbox[2] - bbox[0]
    text_h = bbox[3] - bbox[1]

    x, y = get_position(img, position)
    if position == 'c':
        x = (img.width - text_w) // 2
        y = (img.height - text_h) // 2
    else:
        # Корректировка для выравнивания
        if 'r' in position:
            x -= text_w
        if 'l' in position:
            x += 0
        if 'b' in position:
            y -= text_h
        if 't' in position:
            y += 0

    # Поворот
    if rotate != 0:
        txt_rot = Image.new('RGBA', txt.size, (255, 255, 255, 0))
        draw_rot = ImageDraw.Draw(txt_rot)
        draw_rot.text((x, y), text, font=font, fill=color)
        txt_rot = txt_rot.rotate(rotate, expand=1)
        # Смещаем повёрнутое изображение обратно
        txt = txt_rot
    else:
        draw.text((x, y), text, font=font, fill=color)

    # Применяем прозрачность
    if opacity < 1.0:
        alpha = txt.split()[3]
        alpha = alpha.point(lambda p: int(p * opacity))
        txt.putalpha(alpha)

    # Накладываем
    result = Image.alpha_composite(img, txt)
    result = result.convert('RGB')
    result.save(output_path, quality=95)

def add_image_watermark(input_path, output_path, watermark_path, position='br',
                        opacity=0.5, rotate=0):
    img = Image.open(input_path).convert('RGBA')
    wm = Image.open(watermark_path).convert('RGBA')

    # Масштабируем водяной знак, если он больше изображения
    if wm.width > img.width or wm.height > img.height:
        ratio = min(img.width / wm.width, img.height / wm.height) * 0.5
        new_size = (int(wm.width * ratio), int(wm.height * ratio))
        wm = wm.resize(new_size, Image.Resampling.LANCZOS)

    x, y = get_position(img, position)
    if position == 'c':
        x = (img.width - wm.width) // 2
        y = (img.height - wm.height) // 2
    else:
        if 'r' in position:
            x -= wm.width
        if 'b' in position:
            y -= wm.height

    # Применяем прозрачность
    if opacity < 1.0:
        alpha = wm.split()[3]
        alpha = alpha.point(lambda p: int(p * opacity))
        wm.putalpha(alpha)

    # Поворот
    if rotate != 0:
        wm = wm.rotate(rotate, expand=1)

    img.paste(wm, (x, y), wm)
    img = img.convert('RGB')
    img.save(output_path, quality=95)

def batch_process(input_dir, output_dir, mode, **kwargs):
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    extensions = ('.jpg', '.jpeg', '.png', '.bmp', '.tiff', '.webp')
    files = [f for f in os.listdir(input_dir)
             if f.lower().endswith(extensions)]

    for i, fname in enumerate(files):
        in_path = os.path.join(input_dir, fname)
        out_path = os.path.join(output_dir, fname)
        print(colorize(f"[{i+1}/{len(files)}] Processing {fname}...", 'yellow'))

        try:
            if mode == 'text':
                add_text_watermark(in_path, out_path, kwargs.get('text', ''),
                                   kwargs.get('position', 'br'),
                                   kwargs.get('opacity', 0.5),
                                   kwargs.get('font_size', 48),
                                   kwargs.get('color', '#FFFFFF'),
                                   kwargs.get('rotate', 0))
            else:
                add_image_watermark(in_path, out_path, kwargs.get('watermark', ''),
                                    kwargs.get('position', 'br'),
                                    kwargs.get('opacity', 0.5),
                                    kwargs.get('rotate', 0))
        except Exception as e:
            print(colorize(f"  Error: {e}", 'red'))

def main():
    parser = argparse.ArgumentParser(description="WaterMark Pro – добавление водяных знаков")
    parser.add_argument('mode', choices=['text', 'image', 'batch'],
                        help='Режим: text – текстовый, image – графический, batch – пакетный')
    parser.add_argument('input', help='Входной файл или папка')
    parser.add_argument('output', nargs='?', help='Выходной файл или папка')
    parser.add_argument('-t', '--text', help='Текст водяного знака')
    parser.add_argument('-i', '--image', help='Изображение-водяной знак')
    parser.add_argument('-p', '--position', default='br',
                        choices=POSITIONS.keys(), help='Позиция (default: br)')
    parser.add_argument('-o', '--opacity', type=float, default=0.5,
                        help='Прозрачность 0.0-1.0 (default: 0.5)')
    parser.add_argument('-s', '--size', type=int, default=48,
                        help='Размер шрифта (default: 48)')
    parser.add_argument('-c', '--color', default='#FFFFFF',
                        help='Цвет текста в формате #RRGGBB (default: #FFFFFF)')
    parser.add_argument('-r', '--rotate', type=int, default=0,
                        help='Угол поворота в градусах (default: 0)')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Подробный вывод')

    args = parser.parse_args()

    if args.mode == 'batch':
        if not args.output:
            sys.exit(colorize("Ошибка: для пакетного режима укажите выходную папку", 'red'))
        batch_process(args.input, args.output, args.mode,
                      text=args.text, watermark=args.image,
                      position=args.position, opacity=args.opacity,
                      font_size=args.size, color=args.color,
                      rotate=args.rotate)
        print(colorize("Пакетная обработка завершена!", 'green'))
        return

    if args.mode == 'text' and not args.text:
        sys.exit(colorize("Ошибка: укажите текст водяного знака (-t)", 'red'))
    if args.mode == 'image' and not args.image:
        sys.exit(colorize("Ошибка: укажите изображение-водяной знак (-i)", 'red'))

    output = args.output or f"watermarked_{os.path.basename(args.input)}"

    try:
        if args.mode == 'text':
            add_text_watermark(args.input, output, args.text,
                               args.position, args.opacity,
                               args.size, args.color, args.rotate)
        else:
            add_image_watermark(args.input, output, args.image,
                                args.position, args.opacity, args.rotate)
        print(colorize(f"Готово! Результат сохранён в {output}", 'green'))
    except Exception as e:
        sys.exit(colorize(f"Ошибка: {e}", 'red'))

if __name__ == '__main__':
    main()
