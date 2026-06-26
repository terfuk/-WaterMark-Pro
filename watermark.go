// watermark.go
package main

import (
	"flag"
	"fmt"
	"image"
	"image/color"
	"image/draw"
	"image/jpeg"
	"image/png"
	"io"
	"os"
	"path/filepath"
	"strings"

	"github.com/fogleman/gg"
)

const (
	reset  = "\033[0m"
	green  = "\033[92m"
	red    = "\033[91m"
	yellow = "\033[93m"
)

func colorize(text, color string) string {
	return color + text + reset
}

var positions = map[string][2]float64{
	"tl": {0.05, 0.05},
	"tc": {0.5, 0.05},
	"tr": {0.95, 0.05},
	"ml": {0.05, 0.5},
	"c":  {0.5, 0.5},
	"mr": {0.95, 0.5},
	"bl": {0.05, 0.95},
	"bc": {0.5, 0.95},
	"br": {0.95, 0.95},
}

func getPosition(img image.Image, posKey string) (int, int) {
	bounds := img.Bounds()
	w, h := bounds.Dx(), bounds.Dy()
	pos := positions[posKey]
	if posKey == "c" {
		return w / 2, h / 2
	}
	return int(float64(w) * pos[0]), int(float64(h) * pos[1])
}

func addTextWatermark(inputPath, outputPath, text, position string, opacity float64,
	fontSize float64, colorStr string, rotate float64) error {
	img, err := gg.LoadImage(inputPath)
	if err != nil {
		return err
	}
	w, h := img.Bounds().Dx(), img.Bounds().Dy()

	dc := gg.NewContext(w, h)
	dc.DrawImage(img, 0, 0)

	// Настройка шрифта
	if err := dc.LoadFontFace("/usr/share/fonts/truetype/dejavu/DejaVu-Sans.ttf", fontSize); err != nil {
		dc.SetRGB(1, 1, 1) // fallback
	}

	// Цвет
	var col color.Color
	if strings.HasPrefix(colorStr, "#") {
		r, g, b := parseHexColor(colorStr)
		col = color.RGBA{uint8(r), uint8(g), uint8(b), uint8(255 * opacity)}
	} else {
		col = color.RGBA{255, 255, 255, uint8(255 * opacity)}
	}
	dc.SetColor(col)

	// Позиция
	x, y := getPosition(img, position)
	if position == "c" {
		dc.DrawStringAnchored(text, float64(x), float64(y), 0.5, 0.5)
	} else {
		dc.DrawStringAnchored(text, float64(x), float64(y), 1, 1)
	}

	// Поворот
	if rotate != 0 {
		dc.RotateAbout(gg.Radians(rotate), float64(x), float64(y))
	}

	dc.DrawString(text, float64(x), float64(y))

	return saveImage(dc.Image(), outputPath)
}

func addImageWatermark(inputPath, outputPath, watermarkPath, position string,
	opacity float64, rotate float64) error {
	img, err := gg.LoadImage(inputPath)
	if err != nil {
		return err
	}
	wm, err := gg.LoadImage(watermarkPath)
	if err != nil {
		return err
	}

	w, h := img.Bounds().Dx(), img.Bounds().Dy()
	dc := gg.NewContext(w, h)
	dc.DrawImage(img, 0, 0)

	// Масштабируем водяной знак
	wmW, wmH := wm.Bounds().Dx(), wm.Bounds().Dy()
	if wmW > w/2 || wmH > h/2 {
		scale := min(float64(w)/float64(wmW), float64(h)/float64(wmH)) * 0.4
		newW := int(float64(wmW) * scale)
		newH := int(float64(wmH) * scale)
		wm = resizeImage(wm, newW, newH)
	}

	x, y := getPosition(img, position)
	if position == "c" {
		x = (w - wm.Bounds().Dx()) / 2
		y = (h - wm.Bounds().Dy()) / 2
	} else {
		if strings.Contains(position, "r") {
			x -= wm.Bounds().Dx()
		}
		if strings.Contains(position, "b") {
			y -= wm.Bounds().Dy()
		}
	}

	// Применяем прозрачность
	if opacity < 1.0 {
		wm = setOpacity(wm, opacity)
	}

	// Поворот
	if rotate != 0 {
		wm = rotateImage(wm, rotate)
	}

	draw.Draw(dc.Image(), image.Rect(x, y, x+wm.Bounds().Dx(), y+wm.Bounds().Dy()), wm, image.Point{0, 0}, draw.Over)

	return saveImage(dc.Image(), outputPath)
}

func saveImage(img image.Image, path string) error {
	f, err := os.Create(path)
	if err != nil {
		return err
	}
	defer f.Close()

	ext := strings.ToLower(filepath.Ext(path))
	if ext == ".png" {
		return png.Encode(f, img)
	}
	return jpeg.Encode(f, img, &jpeg.Options{Quality: 95})
}

func batchProcess(inputDir, outputDir, mode, text, watermark, position string,
	opacity float64, fontSize int, color string, rotate int) error {
	if err := os.MkdirAll(outputDir, 0755); err != nil {
		return err
	}

	files, err := filepath.Glob(filepath.Join(inputDir, "*.*"))
	if err != nil {
		return err
	}

	extensions := map[string]bool{".jpg": true, ".jpeg": true, ".png": true, ".bmp": true, ".tiff": true, ".webp": true}
	for i, f := range files {
		ext := strings.ToLower(filepath.Ext(f))
		if !extensions[ext] {
			continue
		}
		outPath := filepath.Join(outputDir, filepath.Base(f))
		fmt.Println(colorize(fmt.Sprintf("[%d/%d] Processing %s...", i+1, len(files), filepath.Base(f)), yellow))

		var err error
		if mode == "text" {
			err = addTextWatermark(f, outPath, text, position, opacity, float64(fontSize), color, float64(rotate))
		} else {
			err = addImageWatermark(f, outPath, watermark, position, opacity, float64(rotate))
		}
		if err != nil {
			fmt.Println(colorize("  Error: "+err.Error(), red))
		}
	}
	return nil
}

func main() {
	mode := flag.String("mode", "", "text|image|batch")
	input := flag.String("input", "", "Input file or directory")
	output := flag.String("output", "", "Output file or directory")
	text := flag.String("text", "", "Watermark text")
	image := flag.String("image", "", "Watermark image")
	position := flag.String("position", "br", "Position: tl,tc,tr,ml,c,mr,bl,bc,br")
	opacity := flag.Float64("opacity", 0.5, "Opacity 0.0-1.0")
	size := flag.Int("size", 48, "Font size")
	color := flag.String("color", "#FFFFFF", "Text color #RRGGBB")
	rotate := flag.Int("rotate", 0, "Rotation angle in degrees")
	verbose := flag.Bool("verbose", false, "Verbose output")
	flag.Parse()

	if *mode == "" {
		fmt.Println(colorize("Usage: watermark -mode=text|image|batch -input=<file/dir> -output=<file/dir> [options]", yellow))
		flag.PrintDefaults()
		os.Exit(1)
	}

	if *mode == "batch" && *output == "" {
		fmt.Println(colorize("Error: output directory required for batch mode", red))
		os.Exit(1)
	}
	if *mode == "text" && *text == "" {
		fmt.Println(colorize("Error: text required for text mode", red))
		os.Exit(1)
	}
	if *mode == "image" && *image == "" {
		fmt.Println(colorize("Error: watermark image required for image mode", red))
		os.Exit(1)
	}

	out := *output
	if out == "" {
		out = "watermarked_" + filepath.Base(*input)
	}

	var err error
	if *mode == "batch" {
		err = batchProcess(*input, *output, *mode, *text, *image, *position, *opacity, *size, *color, *rotate)
	} else if *mode == "text" {
		err = addTextWatermark(*input, out, *text, *position, *opacity, float64(*size), *color, float64(*rotate))
	} else {
		err = addImageWatermark(*input, out, *image, *position, *opacity, float64(*rotate))
	}

	if err != nil {
		fmt.Println(colorize("Error: "+err.Error(), red))
		os.Exit(1)
	}
	if *mode != "batch" {
		fmt.Println(colorize("Done! Result saved to "+out, green))
	} else {
		fmt.Println(colorize("Batch processing completed!", green))
	}
}
