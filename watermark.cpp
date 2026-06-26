// watermark.cpp
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

const string RESET = "\033[0m";
const string GREEN = "\033[92m";
const string RED = "\033[91m";
const string YELLOW = "\033[93m";

string colorize(const string& text, const string& color) {
    return color + text + RESET;
}

struct Position {
    double x, y;
};

map<string, Position> positions = {
    {"tl", {0.05, 0.05}},
    {"tc", {0.5, 0.05}},
    {"tr", {0.95, 0.05}},
    {"ml", {0.05, 0.5}},
    {"c", {0.5, 0.5}},
    {"mr", {0.95, 0.5}},
    {"bl", {0.05, 0.95}},
    {"bc", {0.5, 0.95}},
    {"br", {0.95, 0.95}}
};

Point getPosition(const Mat& img, const string& posKey, int offset = 10) {
    int w = img.cols, h = img.rows;
    auto pos = positions[posKey];
    int x = (int)(w * pos.x) - offset;
    int y = (int)(h * pos.y) - offset;
    return Point(x, y);
}

void addTextWatermark(const string& inputPath, const string& outputPath,
                      const string& text, const string& position = "br",
                      double opacity = 0.5, int fontSize = 48,
                      const Scalar& color = Scalar(255, 255, 255),
                      int rotate = 0) {
    Mat img = imread(inputPath, IMREAD_UNCHANGED);
    if (img.empty()) throw runtime_error("Cannot read image");

    int w = img.cols, h = img.rows;
    Mat overlay = img.clone();

    // Создаём прозрачный слой для текста
    Mat textLayer(h, w, CV_8UC4, Scalar(0, 0, 0, 0));
    Point pos = getPosition(img, position);

    // Настройка шрифта
    int fontFace = FONT_HERSHEY_SIMPLEX;
    double fontScale = fontSize / 30.0;
    int thickness = 2;

    // Вычисляем размер текста
    int baseline;
    Size textSize = getTextSize(text, fontFace, fontScale, thickness, &baseline);

    // Корректировка позиции
    if (position == "c") {
        pos.x = (w - textSize.width) / 2;
        pos.y = (h + textSize.height) / 2;
    } else {
        if (position.find('r') != string::npos) pos.x -= textSize.width;
        if (position.find('b') != string::npos) pos.y -= 0;
        if (position.find('l') != string::npos) pos.x += 0;
        if (position.find('t') != string::npos) pos.y += textSize.height;
    }

    // Рисуем текст на прозрачном слое
    putText(textLayer, text, pos, fontFace, fontScale, Scalar(color[0], color[1], color[2], 255),
            thickness, LINE_AA);

    // Применяем прозрачность
    if (opacity < 1.0) {
        for (int y = 0; y < textLayer.rows; y++) {
            for (int x = 0; x < textLayer.cols; x++) {
                Vec4b& pixel = textLayer.at<Vec4b>(y, x);
                pixel[3] = (uchar)(pixel[3] * opacity);
            }
        }
    }

    // Поворот
    if (rotate != 0) {
        Point2f center(w/2.0, h/2.0);
        Mat rot = getRotationMatrix2D(center, rotate, 1.0);
        warpAffine(textLayer, textLayer, rot, Size(w, h));
    }

    // Накладываем
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Vec4b& overlay_pixel = overlay.at<Vec4b>(y, x);
            Vec4b& text_pixel = textLayer.at<Vec4b>(y, x);
            if (text_pixel[3] > 0) {
                double alpha = text_pixel[3] / 255.0;
                overlay_pixel[0] = (uchar)(overlay_pixel[0] * (1 - alpha) + text_pixel[0] * alpha);
                overlay_pixel[1] = (uchar)(overlay_pixel[1] * (1 - alpha) + text_pixel[1] * alpha);
                overlay_pixel[2] = (uchar)(overlay_pixel[2] * (1 - alpha) + text_pixel[2] * alpha);
            }
        }
    }

    imwrite(outputPath, overlay);
}

void addImageWatermark(const string& inputPath, const string& outputPath,
                       const string& watermarkPath, const string& position = "br",
                       double opacity = 0.5, int rotate = 0) {
    Mat img = imread(inputPath, IMREAD_UNCHANGED);
    Mat wm = imread(watermarkPath, IMREAD_UNCHANGED);
    if (img.empty() || wm.empty()) throw runtime_error("Cannot read image");

    // Масштабируем водяной знак
    if (wm.cols > img.cols / 2 || wm.rows > img.rows / 2) {
        double scale = min((double)img.cols / wm.cols, (double)img.rows / wm.rows) * 0.4;
        resize(wm, wm, Size(), scale, scale);
    }

    // Если watermark не имеет альфа-канала, добавляем
    if (wm.channels() == 3) {
        Mat wmAlpha;
        cvtColor(wm, wmAlpha, COLOR_BGR2BGRA);
        wm = wmAlpha;
    }

    // Применяем прозрачность
    if (opacity < 1.0) {
        for (int y = 0; y < wm.rows; y++) {
            for (int x = 0; x < wm.cols; x++) {
                Vec4b& pixel = wm.at<Vec4b>(y, x);
                pixel[3] = (uchar)(pixel[3] * opacity);
            }
        }
    }

    Point pos = getPosition(img, position);
    if (position == "c") {
        pos.x = (img.cols - wm.cols) / 2;
        pos.y = (img.rows - wm.rows) / 2;
    } else {
        if (position.find('r') != string::npos) pos.x -= wm.cols;
        if (position.find('b') != string::npos) pos.y -= wm.rows;
    }

    // Накладываем
    for (int y = 0; y < wm.rows; y++) {
        for (int x = 0; x < wm.cols; x++) {
            int imgX = pos.x + x;
            int imgY = pos.y + y;
            if (imgX >= 0 && imgX < img.cols && imgY >= 0 && imgY < img.rows) {
                Vec4b& imgPixel = img.at<Vec4b>(imgY, imgX);
                Vec4b& wmPixel = wm.at<Vec4b>(y, x);
                double alpha = wmPixel[3] / 255.0;
                if (alpha > 0) {
                    imgPixel[0] = (uchar)(imgPixel[0] * (1 - alpha) + wmPixel[0] * alpha);
                    imgPixel[1] = (uchar)(imgPixel[1] * (1 - alpha) + wmPixel[1] * alpha);
                    imgPixel[2] = (uchar)(imgPixel[2] * (1 - alpha) + wmPixel[2] * alpha);
                }
            }
        }
    }

    imwrite(outputPath, img);
}

void batchProcess(const string& inputDir, const string& outputDir,
                  const string& mode, const string& text, const string& watermark,
                  const string& position, double opacity, int fontSize,
                  const string& color, int rotate) {
    fs::create_directories(outputDir);

    vector<string> extensions = {".jpg", ".jpeg", ".png", ".bmp", ".tiff", ".webp"};
    vector<string> files;
    for (const auto& entry : fs::directory_iterator(inputDir)) {
        if (entry.is_regular_file()) {
            string ext = entry.path().extension().string();
            transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                files.push_back(entry.path().filename().string());
            }
        }
    }

    Scalar textColor;
    if (!color.empty() && color[0] == '#') {
        int r = stoi(color.substr(1,2), nullptr, 16);
        int g = stoi(color.substr(3,2), nullptr, 16);
        int b = stoi(color.substr(5,2), nullptr, 16);
        textColor = Scalar(b, g, r);
    } else {
        textColor = Scalar(255, 255, 255);
    }

    for (size_t i = 0; i < files.size(); i++) {
        string inPath = inputDir + "/" + files[i];
        string outPath = outputDir + "/" + files[i];
        cout << colorize("[" + to_string(i+1) + "/" + to_string(files.size()) + "] Processing " + files[i] + "...", YELLOW) << endl;

        try {
            if (mode == "text") {
                addTextWatermark(inPath, outPath, text, position, opacity, fontSize, textColor, rotate);
            } else {
                addImageWatermark(inPath, outPath, watermark, position, opacity, rotate);
            }
        } catch (const exception& e) {
            cout << colorize("  Error: " + string(e.what()), RED) << endl;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << colorize("Usage: watermark text|image|batch <input> [output] [options]", YELLOW) << endl;
        cout << "Options:" << endl;
        cout << "  -t <text>       Watermark text" << endl;
        cout << "  -i <file>       Watermark image" << endl;
        cout << "  -p <pos>        Position: tl,tc,tr,ml,c,mr,bl,bc,br (default: br)" << endl;
        cout << "  -o <opacity>    Opacity 0.0-1.0 (default: 0.5)" << endl;
        cout << "  -s <size>       Font size (default: 48)" << endl;
        cout << "  -c <color>      Text color #RRGGBB (default: #FFFFFF)" << endl;
        cout << "  -r <deg>        Rotation angle (default: 0)" << endl;
        return 1;
    }

    string mode = argv[1];
    string input = argv[2];
    string output = (argc > 3) ? argv[3] : "watermarked_" + fs::path(input).filename().string();
    string text = "", watermark = "", position = "br";
    double opacity = 0.5;
    int fontSize = 48;
    string color = "#FFFFFF";
    int rotate = 0;

    for (int i = 4; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-t" && i+1 < argc) text = argv[++i];
        else if (arg == "-i" && i+1 < argc) watermark = argv[++i];
        else if (arg == "-p" && i+1 < argc) position = argv[++i];
        else if (arg == "-o" && i+1 < argc) opacity = stod(argv[++i]);
        else if (arg == "-s" && i+1 < argc) fontSize = stoi(argv[++i]);
        else if (arg == "-c" && i+1 < argc) color = argv[++i];
        else if (arg == "-r" && i+1 < argc) rotate = stoi(argv[++i]);
    }

    try {
        if (mode == "batch") {
            batchProcess(input, output, mode, text, watermark, position, opacity, fontSize, color, rotate);
            cout << colorize("Batch processing completed!", GREEN) << endl;
        } else if (mode == "text") {
            if (text.empty()) throw runtime_error("Text required for text mode");
            Scalar textColor;
            if (!color.empty() && color[0] == '#') {
                int r = stoi(color.substr(1,2), nullptr, 16);
                int g = stoi(color.substr(3,2), nullptr, 16);
                int b = stoi(color.substr(5,2), nullptr, 16);
                textColor = Scalar(b, g, r);
            } else {
                textColor = Scalar(255, 255, 255);
            }
            addTextWatermark(input, output, text, position, opacity, fontSize, textColor, rotate);
            cout << colorize("Done! Result saved to " + output, GREEN) << endl;
        } else if (mode == "image") {
            if (watermark.empty()) throw runtime_error("Watermark image required for image mode");
            addImageWatermark(input, output, watermark, position, opacity, rotate);
            cout << colorize("Done! Result saved to " + output, GREEN) << endl;
        } else {
            throw runtime_error("Invalid mode. Use text, image, or batch");
        }
    } catch (const exception& e) {
        cout << colorize("Error: " + string(e.what()), RED) << endl;
        return 1;
    }
    return 0;
}
