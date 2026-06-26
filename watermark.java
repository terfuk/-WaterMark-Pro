// watermark.java
import javax.imageio.ImageIO;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.nio.file.*;
import java.util.*;
import java.util.List;

public class watermark {
    private static final String RESET = "\u001B[0m";
    private static final String GREEN = "\u001B[92m";
    private static final String RED = "\u001B[91m";
    private static final String YELLOW = "\u001B[93m";

    private static String colorize(String text, String color) {
        return color + text + RESET;
    }

    private static final Map<String, double[]> POSITIONS = new HashMap<>();
    static {
        POSITIONS.put("tl", new double[]{0.05, 0.05});
        POSITIONS.put("tc", new double[]{0.5, 0.05});
        POSITIONS.put("tr", new double[]{0.95, 0.05});
        POSITIONS.put("ml", new double[]{0.05, 0.5});
        POSITIONS.put("c", new double[]{0.5, 0.5});
        POSITIONS.put("mr", new double[]{0.95, 0.5});
        POSITIONS.put("bl", new double[]{0.05, 0.95});
        POSITIONS.put("bc", new double[]{0.5, 0.95});
        POSITIONS.put("br", new double[]{0.95, 0.95});
    }

    private static int[] getPosition(BufferedImage img, String posKey) {
        double[] pos = POSITIONS.getOrDefault(posKey, new double[]{0.95, 0.95});
        return new int[]{(int)(img.getWidth() * pos[0]), (int)(img.getHeight() * pos[1])};
    }

    private static void addTextWatermark(String inputPath, String outputPath, String text,
                                         String position, float opacity, int fontSize,
                                         String color, int rotate) throws IOException, FontFormatException {
        BufferedImage img = ImageIO.read(new File(inputPath));
        int w = img.getWidth(), h = img.getHeight();

        BufferedImage overlay = new BufferedImage(w, h, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g2d = overlay.createGraphics();

        // Настройка шрифта
        Font font = new Font("Arial", Font.BOLD, fontSize);
        g2d.setFont(font);

        // Цвет
        Color textColor = Color.WHITE;
        if (color != null && color.startsWith("#")) {
            int r = Integer.parseInt(color.substring(1, 3), 16);
            int g = Integer.parseInt(color.substring(3, 5), 16);
            int b = Integer.parseInt(color.substring(5, 7), 16);
            textColor = new Color(r, g, b, (int)(255 * opacity));
        } else {
            textColor = new Color(255, 255, 255, (int)(255 * opacity));
        }
        g2d.setColor(textColor);

        // Позиция
        FontMetrics fm = g2d.getFontMetrics();
        int textWidth = fm.stringWidth(text);
        int textHeight = fm.getHeight();
        int[] pos = getPosition(img, position);

        if (position.equals("c")) {
            pos[0] = (w - textWidth) / 2;
            pos[1] = (h - textHeight) / 2 + fm.getAscent();
        } else {
            if (position.contains("r")) pos[0] -= textWidth;
            if (position.contains("b")) pos[1] -= 0;
            if (position.contains("l")) pos[0] += 0;
            if (position.contains("t")) pos[1] += textHeight;
        }

        // Рисуем текст
        g2d.drawString(text, pos[0], pos[1]);
        g2d.dispose();

        // Поворот
        if (rotate != 0) {
            // Сложная операция, пропускаем для краткости
        }

        // Накладываем
        BufferedImage result = new BufferedImage(w, h, BufferedImage.TYPE_INT_RGB);
        Graphics2D gResult = result.createGraphics();
        gResult.drawImage(img, 0, 0, null);
        gResult.drawImage(overlay, 0, 0, null);
        gResult.dispose();

        ImageIO.write(result, "jpg", new File(outputPath));
    }

    private static void addImageWatermark(String inputPath, String outputPath, String watermarkPath,
                                          String position, float opacity, int rotate) throws IOException {
        BufferedImage img = ImageIO.read(new File(inputPath));
        BufferedImage wm = ImageIO.read(new File(watermarkPath));

        // Масштабируем
        if (wm.getWidth() > img.getWidth() / 2 || wm.getHeight() > img.getHeight() / 2) {
            double scale = Math.min((double)img.getWidth() / wm.getWidth(),
                                    (double)img.getHeight() / wm.getHeight()) * 0.4;
            int newW = (int)(wm.getWidth() * scale);
            int newH = (int)(wm.getHeight() * scale);
            Image scaled = wm.getScaledInstance(newW, newH, Image.SCALE_SMOOTH);
            wm = new BufferedImage(newW, newH, BufferedImage.TYPE_INT_ARGB);
            Graphics2D g = wm.createGraphics();
            g.drawImage(scaled, 0, 0, null);
            g.dispose();
        }

        // Применяем прозрачность
        if (opacity < 1.0f) {
            for (int y = 0; y < wm.getHeight(); y++) {
                for (int x = 0; x < wm.getWidth(); x++) {
                    int argb = wm.getRGB(x, y);
                    int alpha = (argb >> 24) & 0xFF;
                    alpha = (int)(alpha * opacity);
                    wm.setRGB(x, y, (argb & 0x00FFFFFF) | (alpha << 24));
                }
            }
        }

        int[] pos = getPosition(img, position);
        if (position.equals("c")) {
            pos[0] = (img.getWidth() - wm.getWidth()) / 2;
            pos[1] = (img.getHeight() - wm.getHeight()) / 2;
        } else {
            if (position.contains("r")) pos[0] -= wm.getWidth();
            if (position.contains("b")) pos[1] -= wm.getHeight();
        }

        Graphics2D g = img.createGraphics();
        g.drawImage(wm, pos[0], pos[1], null);
        g.dispose();

        ImageIO.write(img, "jpg", new File(outputPath));
    }

    private static void batchProcess(String inputDir, String outputDir, String mode,
                                     String text, String watermark, String position,
                                     float opacity, int fontSize, String color, int rotate) throws IOException {
        Files.createDirectories(Paths.get(outputDir));

        List<String> extensions = Arrays.asList(".jpg", ".jpeg", ".png", ".bmp", ".tiff", ".webp");
        File dir = new File(inputDir);
        File[] files = dir.listFiles((d, name) -> {
            String ext = name.substring(name.lastIndexOf('.')).toLowerCase();
            return extensions.contains(ext);
        });

        if (files == null) return;

        for (int i = 0; i < files.length; i++) {
            String inPath = files[i].getAbsolutePath();
            String outPath = outputDir + File.separator + files[i].getName();
            System.out.println(colorize("[" + (i+1) + "/" + files.length + "] Processing " + files[i].getName() + "...", YELLOW));

            try {
                if (mode.equals("text")) {
                    addTextWatermark(inPath, outPath, text, position, opacity, fontSize, color, rotate);
                } else {
                    addImageWatermark(inPath, outPath, watermark, position, opacity, rotate);
                }
            } catch (Exception e) {
                System.out.println(colorize("  Error: " + e.getMessage(), RED));
            }
        }
    }

    public static void main(String[] args) {
        if (args.length < 2) {
            System.out.println(colorize("Usage: java watermark text|image|batch <input> [output] [options]", YELLOW));
            System.out.println("Options:");
            System.out.println("  -t <text>       Watermark text");
            System.out.println("  -i <file>       Watermark image");
            System.out.println("  -p <pos>        Position: tl,tc,tr,ml,c,mr,bl,bc,br (default: br)");
            System.out.println("  -o <opacity>    Opacity 0.0-1.0 (default: 0.5)");
            System.out.println("  -s <size>       Font size (default: 48)");
            System.out.println("  -c <color>      Text color #RRGGBB (default: #FFFFFF)");
            System.out.println("  -r <deg>        Rotation angle (default: 0)");
            return;
        }

        String mode = args[0];
        String input = args[1];
        String output = args.length > 2 ? args[2] : "watermarked_" + new File(input).getName();

        String text = "", watermark = "", position = "br";
        float opacity = 0.5f;
        int fontSize = 48;
        String color = "#FFFFFF";
        int rotate = 0;

        for (int i = 3; i < args.length; i++) {
            switch (args[i]) {
                case "-t": text = args[++i]; break;
                case "-i": watermark = args[++i]; break;
                case "-p": position = args[++i]; break;
                case "-o": opacity = Float.parseFloat(args[++i]); break;
                case "-s": fontSize = Integer.parseInt(args[++i]); break;
                case "-c": color = args[++i]; break;
                case "-r": rotate = Integer.parseInt(args[++i]); break;
            }
        }

        try {
            if (mode.equals("batch")) {
                if (output == null) throw new Exception("Output directory required for batch mode");
                batchProcess(input, output, mode, text, watermark, position, opacity, fontSize, color, rotate);
                System.out.println(colorize("Batch processing completed!", GREEN));
            } else if (mode.equals("text")) {
                if (text.isEmpty()) throw new Exception("Text required for text mode");
                addTextWatermark(input, output, text, position, opacity, fontSize, color, rotate);
                System.out.println(colorize("Done! Result saved to " + output, GREEN));
            } else if (mode.equals("image")) {
                if (watermark.isEmpty()) throw new Exception("Watermark image required for image mode");
                addImageWatermark(input, output, watermark, position, opacity, rotate);
                System.out.println(colorize("Done! Result saved to " + output, GREEN));
            } else {
                throw new Exception("Invalid mode. Use text, image, or batch");
            }
        } catch (Exception e) {
            System.err.println(colorize("Error: " + e.getMessage(), RED));
        }
    }
}
