WaterMark Pro – Утилита для добавления водяных знаков на 7 языках
Профессиональный инструмент для нанесения текстовых и графических водяных знаков на изображения.
Поддерживает пакетную обработку, настройку прозрачности, позиционирование, поворот и многое другое.
Реализован на семи языках программирования с единым интерфейсом командной строки.

🚀 Возможности
Текстовые водяные знаки – любой текст, шрифт, размер, цвет.

Графические водяные знаки – наложение логотипа поверх изображения.

Гибкое позиционирование – 9 предустановленных позиций (углы, центр, края).

Настройка прозрачности – регулировка непрозрачности водяного знака.

Поворот – наложение под углом (например, 45° для защиты от удаления).

Пакетная обработка – обработка всех изображений в папке.

Поддержка форматов – JPEG, PNG, BMP, TIFF, WEBP и другие.

Единый интерфейс – одинаковые команды для всех языков.

📖 Использование
Синтаксис (единый для всех версий):

bash
<команда> <режим> <входной_файл/папка> [выходной_файл/папка] [опции]
Режимы
text – добавить текстовый водяной знак.

image – добавить графический водяной знак (логотип).

batch – пакетная обработка всех изображений в папке.

Основные опции
Опция	Описание
-t, --text	Текст водяного знака (для режима text)
-i, --image	Путь к изображению-водяному знаку (для режима image)
-p, --position	Позиция: tl (верх-лево), tr (верх-право), bl (низ-лево), br (низ-право), c (центр) – по умолчанию br
-o, --opacity	Прозрачность (0.0 – полностью прозрачный, 1.0 – непрозрачный) – по умолчанию 0.5
-s, --size	Размер шрифта (для текстового водяного знака) – по умолчанию 48
-r, --rotate	Угол поворота в градусах – по умолчанию 0
-c, --color	Цвет текста в формате #RRGGBB – по умолчанию #FFFFFF
-v, --verbose	Показать подробную информацию о процессе
Примеры
bash
# Добавить текстовый водяной знак в правый нижний угол
python watermark.py text photo.jpg output.jpg -t "© My Brand" -p br -o 0.6

# Добавить логотип с прозрачностью 70% в центр
python watermark.py image photo.jpg output.jpg -i logo.png -p c -o 0.3

# Пакетная обработка всех JPEG в папке
python watermark.py batch ./images ./output -t "© 2024" -p br -o 0.5

# Водяной знак с поворотом на 45°
python watermark.py text photo.jpg output.jpg -t "CONFIDENTIAL" -r 45 -o 0.3
🛠 Установка и запуск
Python
bash
pip install pillow
python watermark.py text|image|batch <input> [output] [options]
Go
bash
go get github.com/fogleman/gg
go build watermark.go
./watermark text|image|batch <input> [output] [options]
JavaScript (Node.js)
bash
npm install sharp
node watermark.js text|image|batch <input> [output] [options]
C++
bash
sudo apt-get install libopencv-dev
g++ -std=c++11 watermark.cpp -o watermark `pkg-config --cflags --libs opencv4`
./watermark text|image|batch <input> [output] [options]
C#
bash
dotnet add package SixLabors.ImageSharp
dotnet run watermark.cs text|image|batch <input> [output] [options]
Java
bash
javac watermark.java
java watermark text|image|batch <input> [output] [options]
Ruby
bash
gem install rmagick
ruby watermark.rb text|image|batch <input> [output] [options]
🧠 Архитектура
Все реализации построены по единому шаблону:

Парсинг аргументов – режим, входной/выходной файлы, опции.

Загрузка изображения – чтение исходного файла.

Создание водяного знака – текст или изображение с учётом прозрачности.

Наложение – размещение водяного знака в указанной позиции.

Сохранение – запись результата в выходной файл.

✨ Дополнительные фичи
Автоматическое создание папок – если выходная папка не существует, она создаётся.

Поддержка прозрачности PNG – корректная обработка альфа-канала.

Масштабирование водяного знака – автоматическая подгонка размера под изображение.

Цветной вывод – информативные сообщения об успехе/ошибке.

📂 Состав репозитория
Язык	Файл	Статус
Python	watermark.py	✅
Go	watermark.go	✅
JavaScript	watermark.js	✅
C++	watermark.cpp	✅
C#	watermark.cs	✅
Java	watermark.java	✅
Ruby	watermark.rb	✅
🤝 Вклад в проект
Приветствуются улучшения:

Добавление поддержки видео.

Реализация невидимых (цифровых) водяных знаков.

Интеграция с облачными сервисами.

Создавайте Issues и Pull Requests.

📜 Лицензия
MIT License – свободное использование, модификация и распространение.
