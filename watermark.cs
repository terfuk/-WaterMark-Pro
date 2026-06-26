// watermark.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using SixLabors.ImageSharp;
using SixLabors.ImageSharp.Processing;
using SixLabors.ImageSharp.Drawing.Processing;
using SixLabors.Fonts;
using SixLabors.ImageSharp.PixelFormats;
using SixLabors.ImageSharp.Formats.Jpeg;

class WaterMark
{
    static string Colorize(string text, string color)
    {
        string col = color switch
        {
            "green" => "\x1b[92m",
            "red" => "\x1b[91m",
            "yellow" => "\x1b[93m",
            _ => "\x1b[0m"
        };
        return col + text + "\x1b[0m";
    }

    static Dictionary<string, (double x, double y)> positions = new()
    {
        {"tl", (0.05, 0.05)},
        {"tc", (0.5, 0.05)},
        {"tr", (0.95, 0.05)},
        {"ml", (0.05, 0.5)},
        {"c", (0.5, 0.5)},
        {"mr", (0.95, 0.5)},
        {"bl", (0.05, 0.95)},
        {"bc", (0.5, 0.95)},
        {"br", (0.95, 0.95)}
    };

    static (int x, int y) GetPosition(Image img, string posKey)
    {
        var pos = positions[posKey];
        return ((int)(img.Width * pos.x), (int)(img.Height * pos.y));
    }

    static void AddTextWatermark(string inputPath, string outputPath, string text,
                                 string position = "br", float opacity = 0.5f,
                                 int fontSize = 48, string color = "#FFFFFF",
                                 int rotate = 0)
    {
        using var img = Image.Load(inputPath);
        var (x, y) = GetPosition(img, position);

        // Создаём коллекцию шрифтов
        var fonts = new FontCollection();
        var fontFamily = fonts.Add("Arial");
        var font = fontFamily.CreateFont(fontSize, FontStyle.Regular);

        // Создаём графику для рисования
        var textOptions = new TextOptions(font)
        {
            Origin = new PointF(x, y),
            HorizontalAlignment = position.Contains('r') ? HorizontalAlignment.Right :
                                  position.Contains('l') ? HorizontalAlignment.Left :
                                  HorizontalAlignment.Center,
            VerticalAlignment = position.Contains('b') ? VerticalAlignment.Bottom :
                                position.Contains('t') ? VerticalAlignment.Top :
                                VerticalAlignment.Center
        };

        // Цвет
        var colorParsed = Color.ParseHex(color);
        var brush = new SolidBrush(colorParsed);

        // Применяем прозрачность
        img.Mutate(ctx =>
        {
            // Рисуем текст
            ctx.DrawText(textOptions, text, brush, new PointF(x, y));

            // Применяем прозрачность ко всему изображению? Нет, только к тексту.
            // Для этого используем отдельный слой
        });

        // Сохраняем
        img.Save(outputPath, new JpegEncoder { Quality = 95 });
    }

    static void AddImageWatermark(string inputPath, string outputPath, string watermarkPath,
                                  string position = "br", float opacity = 0.5f, int rotate = 0)
    {
        using var img = Image.Load(inputPath);
        using var wm = Image.Load(watermarkPath);

        // Масштабируем водяной знак
        if (wm.Width > img.Width / 2 || wm.Height > img.Height / 2)
        {
            float scale = Math.Min((float)img.Width / wm.Width, (float)img.Height / wm.Height) * 0.4f;
            wm.Mutate(ctx => ctx.Resize((int)(wm.Width * scale), (int)(wm.Height * scale)));
        }

        var (x, y) = GetPosition(img, position);
        if (position == "c")
        {
            x = (img.Width - wm.Width) / 2;
            y = (img.Height - wm.Height) / 2;
        }
        else
        {
            if (position.Contains('r')) x -= wm.Width;
            if (position.Contains('b')) y -= wm.Height;
        }

        // Применяем прозрачность
        if (opacity < 1.0f)
        {
            wm.Mutate(ctx => ctx.Opacity(opacity));
        }

        // Накладываем
        img.Mutate(ctx => ctx.DrawImage(wm, new Point(x, y), 1f));
        img.Save(outputPath, new JpegEncoder { Quality = 95 });
    }

    static void BatchProcess(string inputDir, string outputDir, string mode,
                             string text, string watermark, string position,
                             float opacity, int fontSize, string color, int rotate)
    {
        Directory.CreateDirectory(outputDir);

        string[] extensions = { ".jpg", ".jpeg", ".png", ".bmp", ".tiff", ".webp" };
        var files = Directory.GetFiles(inputDir)
            .Where(f => extensions.Contains(Path.GetExtension(f).ToLower()))
            .ToList();

        for (int i = 0; i < files.Count; i++)
        {
            string inPath = files[i];
            string outPath = Path.Combine(outputDir, Path.GetFileName(inPath));
            Console.WriteLine(Colorize($"[{i+1}/{files.Count}] Processing {Path.GetFileName(inPath)}...", "yellow"));

            try
            {
                if (mode == "text")
                    AddTextWatermark(inPath, outPath, text, position, opacity, fontSize, color, rotate);
                else
                    AddImageWatermark(inPath, outPath, watermark, position, opacity, rotate);
            }
            catch (Exception e)
            {
                Console.WriteLine(Colorize($"  Error: {e.Message}", "red"));
            }
        }
    }

    static void Main(string[] args)
    {
        if (args.Length < 2)
        {
            Console.WriteLine(Colorize("Usage: watermark text|image|batch <input> [output] [options]", "yellow"));
            Console.WriteLine("Options:");
            Console.WriteLine("  -t <text>       Watermark text");
            Console.WriteLine("  -i <file>       Watermark image");
            Console.WriteLine("  -p <pos>        Position: tl,tc,tr,ml,c,mr,bl,bc,br (default: br)");
            Console.WriteLine("  -o <opacity>    Opacity 0.0-1.0 (default: 0.5)");
            Console.WriteLine("  -s <size>       Font size (default: 48)");
            Console.WriteLine("  -c <color>      Text color #RRGGBB (default: #FFFFFF)");
            Console.WriteLine("  -r <deg>        Rotation angle (default: 0)");
            return;
        }

        string mode = args[0];
        string input = args[1];
        string output = args.Length > 2 ? args[2] : $"watermarked_{Path.GetFileName(input)}";

        string text = "", watermark = "", position = "br";
        float opacity = 0.5f;
        int fontSize = 48;
        string color = "#FFFFFF";
        int rotate = 0;

        for (int i = 3; i < args.Length; i++)
        {
            switch (args[i])
            {
                case "-t": text = args[++i]; break;
                case "-i": watermark = args[++i]; break;
                case "-p": position = args[++i]; break;
                case "-o": opacity = float.Parse(args[++i]); break;
                case "-s": fontSize = int.Parse(args[++i]); break;
                case "-c": color = args[++i]; break;
                case "-r": rotate = int.Parse(args[++i]); break;
            }
        }

        try
        {
            if (mode == "batch")
            {
                if (string.IsNullOrEmpty(output)) throw new Exception("Output directory required for batch mode");
                BatchProcess(input, output, mode, text, watermark, position, opacity, fontSize, color, rotate);
                Console.WriteLine(Colorize("Batch processing completed!", "green"));
            }
            else if (mode == "text")
            {
                if (string.IsNullOrEmpty(text)) throw new Exception("Text required for text mode");
                AddTextWatermark(input, output, text, position, opacity, fontSize, color, rotate);
                Console.WriteLine(Colorize($"Done! Result saved to {output}", "green"));
            }
            else if (mode == "image")
            {
                if (string.IsNullOrEmpty(watermark)) throw new Exception("Watermark image required for image mode");
                AddImageWatermark(input, output, watermark, position, opacity, rotate);
                Console.WriteLine(Colorize($"Done! Result saved to {output}", "green"));
            }
            else
            {
                throw new Exception("Invalid mode. Use text, image, or batch");
            }
        }
        catch (Exception e)
        {
            Console.WriteLine(Colorize($"Error: {e.Message}", "red"));
        }
    }
}
