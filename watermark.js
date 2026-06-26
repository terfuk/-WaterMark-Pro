// watermark.js
#!/usr/bin/env node
'use strict';

const sharp = require('sharp');
const fs = require('fs');
const path = require('path');

const COLORS = {
    green: '\x1b[92m',
    red: '\x1b[91m',
    yellow: '\x1b[93m',
    reset: '\x1b[0m'
};

function colorize(text, color) {
    return COLORS[color] + text + COLORS.reset;
}

const POSITIONS = {
    tl: { x: 0.05, y: 0.05 },
    tc: { x: 0.5, y: 0.05 },
    tr: { x: 0.95, y: 0.05 },
    ml: { x: 0.05, y: 0.5 },
    c:  { x: 0.5, y: 0.5 },
    mr: { x: 0.95, y: 0.5 },
    bl: { x: 0.05, y: 0.95 },
    bc: { x: 0.5, y: 0.95 },
    br: { x: 0.95, y: 0.95 }
};

async function addTextWatermark(inputPath, outputPath, text, position = 'br',
                                 opacity = 0.5, fontSize = 48, color = '#FFFFFF',
                                 rotate = 0) {
    const img = sharp(inputPath);
    const metadata = await img.metadata();

    // Создаём SVG с текстом
    const svg = `
        <svg width="${metadata.width}" height="${metadata.height}">
            <defs>
                <style>
                    .text { font-size: ${fontSize}px; font-family: sans-serif; fill: ${color};
                            opacity: ${opacity}; }
                </style>
            </defs>
            <text class="text" x="${POSITIONS[position].x * metadata.width}"
                  y="${POSITIONS[position].y * metadata.height}"
                  text-anchor="${position.includes('r') ? 'end' : position.includes('l') ? 'start' : 'middle'}"
                  dominant-baseline="${position.includes('b') ? 'baseline' : position.includes('t') ? 'hanging' : 'central'}">
                ${text}
            </text>
        </svg>
    `;

    const overlay = Buffer.from(svg);

    await img
        .composite([{ input: overlay, blend: 'over' }])
        .toFile(outputPath);
}

async function addImageWatermark(inputPath, outputPath, watermarkPath, position = 'br',
                                  opacity = 0.5, rotate = 0) {
    const img = sharp(inputPath);
    const metadata = await img.metadata();
    const wm = sharp(watermarkPath);
    const wmMeta = await wm.metadata();

    // Масштабируем водяной знак
    let wmW = wmMeta.width, wmH = wmMeta.height;
    const maxW = metadata.width * 0.4, maxH = metadata.height * 0.4;
    if (wmW > maxW || wmH > maxH) {
        const ratio = Math.min(maxW / wmW, maxH / wmH);
        wmW = Math.floor(wmW * ratio);
        wmH = Math.floor(wmH * ratio);
    }

    let x = POSITIONS[position].x * metadata.width;
    let y = POSITIONS[position].y * metadata.height;
    if (position === 'c') {
        x = (metadata.width - wmW) / 2;
        y = (metadata.height - wmH) / 2;
    } else {
        if (position.includes('r')) x -= wmW;
        if (position.includes('b')) y -= wmH;
        if (position.includes('l')) x += 0;
        if (position.includes('t')) y += 0;
    }

    const wmBuffer = await wm
        .resize(wmW, wmH)
        .ensureAlpha()
        .raw()
        .toBuffer({ resolveWithObject: true });

    // Применяем прозрачность
    const data = wmBuffer.data;
    for (let i = 3; i < data.length; i += 4) {
        data[i] = Math.round(data[i] * opacity);
    }

    const wmWithAlpha = sharp(data, {
        raw: { width: wmW, height: wmH, channels: 4 }
    });

    const wmPng = await wmWithAlpha.png().toBuffer();

    await img
        .composite([{ input: wmPng, left: Math.round(x), top: Math.round(y) }])
        .toFile(outputPath);
}

async function batchProcess(inputDir, outputDir, mode, options) {
    if (!fs.existsSync(outputDir)) fs.mkdirSync(outputDir, { recursive: true });

    const extensions = ['.jpg', '.jpeg', '.png', '.bmp', '.tiff', '.webp'];
    const files = fs.readdirSync(inputDir)
        .filter(f => extensions.includes(path.extname(f).toLowerCase()));

    for (let i = 0; i < files.length; i++) {
        const inPath = path.join(inputDir, files[i]);
        const outPath = path.join(outputDir, files[i]);
        console.log(colorize(`[${i+1}/${files.length}] Processing ${files[i]}...`, 'yellow'));

        try {
            if (mode === 'text') {
                await addTextWatermark(inPath, outPath, options.text, options.position,
                                       options.opacity, options.size, options.color, options.rotate);
            } else {
                await addImageWatermark(inPath, outPath, options.image, options.position,
                                        options.opacity, options.rotate);
            }
        } catch (err) {
            console.log(colorize(`  Error: ${err.message}`, 'red'));
        }
    }
}

async function main() {
    const args = process.argv.slice(2);
    if (args.length < 2) {
        console.log(colorize('Usage: node watermark.js text|image|batch <input> [output] [options]', 'yellow'));
        console.log('Options:');
        console.log('  -t, --text <text>       Watermark text');
        console.log('  -i, --image <file>      Watermark image');
        console.log('  -p, --position <pos>    Position: tl,tc,tr,ml,c,mr,bl,bc,br (default: br)');
        console.log('  -o, --opacity <n>       Opacity 0.0-1.0 (default: 0.5)');
        console.log('  -s, --size <n>          Font size (default: 48)');
        console.log('  -c, --color <color>     Text color #RRGGBB (default: #FFFFFF)');
        console.log('  -r, --rotate <deg>      Rotation angle (default: 0)');
        process.exit(1);
    }

    const mode = args[0];
    const input = args[1];
    let output = args[2] || (mode === 'batch' ? null : `watermarked_${path.basename(input)}`);

    const options = { position: 'br', opacity: 0.5, size: 48, color: '#FFFFFF', rotate: 0 };
    for (let i = 0; i < args.length; i++) {
        switch (args[i]) {
            case '-t': case '--text': options.text = args[++i]; break;
            case '-i': case '--image': options.image = args[++i]; break;
            case '-p': case '--position': options.position = args[++i]; break;
            case '-o': case '--opacity': options.opacity = parseFloat(args[++i]); break;
            case '-s': case '--size': options.size = parseInt(args[++i]); break;
            case '-c': case '--color': options.color = args[++i]; break;
            case '-r': case '--rotate': options.rotate = parseInt(args[++i]); break;
        }
    }

    try {
        if (mode === 'batch') {
            if (!output) throw new Error('Output directory required for batch mode');
            await batchProcess(input, output, mode, options);
            console.log(colorize('Batch processing completed!', 'green'));
        } else if (mode === 'text') {
            if (!options.text) throw new Error('Text required for text mode');
            await addTextWatermark(input, output, options.text, options.position,
                                   options.opacity, options.size, options.color, options.rotate);
            console.log(colorize(`Done! Result saved to ${output}`, 'green'));
        } else if (mode === 'image') {
            if (!options.image) throw new Error('Watermark image required for image mode');
            await addImageWatermark(input, output, options.image, options.position,
                                    options.opacity, options.rotate);
            console.log(colorize(`Done! Result saved to ${output}`, 'green'));
        } else {
            throw new Error('Invalid mode. Use text, image, or batch');
        }
    } catch (err) {
        console.log(colorize(`Error: ${err.message}`, 'red'));
        process.exit(1);
    }
}

main();
