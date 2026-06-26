#!/usr/bin/env ruby
# watermark.rb
# encoding: UTF-8

require 'rmagick'
require 'optparse'
require 'fileutils'

COLORS = {
  green: "\e[92m",
  red: "\e[91m",
  yellow: "\e[93m",
  reset: "\e[0m"
}

def colorize(text, color)
  "#{COLORS[color]}#{text}#{COLORS[:reset]}"
end

POSITIONS = {
  'tl' => [0.05, 0.05],
  'tc' => [0.5, 0.05],
  'tr' => [0.95, 0.05],
  'ml' => [0.05, 0.5],
  'c'  => [0.5, 0.5],
  'mr' => [0.95, 0.5],
  'bl' => [0.05, 0.95],
  'bc' => [0.5, 0.95],
  'br' => [0.95, 0.95]
}

def get_position(img, pos_key)
  w, h = img.columns, img.rows
  x_ratio, y_ratio = POSITIONS[pos_key] || POSITIONS['br']
  [(w * x_ratio).to_i, (h * y_ratio).to_i]
end

def add_text_watermark(input_path, output_path, text, position: 'br',
                       opacity: 0.5, font_size: 48, color: '#FFFFFF',
                       rotate: 0)
  img = Magick::Image.read(input_path).first
  w, h = img.columns, img.rows

  # Создаём слой для текста
  overlay = Magick::Image.new(w, h) { self.background_color = 'none' }
  draw = Magick::Draw.new

  # Настройка шрифта
  draw.font_family = 'Arial'
  draw.font_size = font_size
  draw.fill = color
  draw.gravity = Magick::CenterGravity

  # Позиция
  x, y = get_position(img, position)
  if position == 'c'
    x = w / 2
    y = h / 2
    draw.gravity = Magick::CenterGravity
  else
    draw.gravity = Magick::NorthWestGravity
    if position.include?('r')
      draw.gravity = Magick::NorthEastGravity
      x = w - x
    end
    if position.include?('b')
      draw.gravity = Magick::SouthWestGravity
      y = h - y
    end
    if position.include?('r') && position.include?('b')
      draw.gravity = Magick::SouthEastGravity
    end
  end

  draw.annotate(overlay, 0, 0, x, y, text)

  # Применяем прозрачность
  overlay.alpha(Magick::ActivateAlphaChannel)
  if opacity < 1.0
    overlay.each_pixel do |pixel, col, row|
      pixel.alpha = (pixel.alpha * opacity).to_i
    end
  end

  # Поворот
  if rotate != 0
    overlay = overlay.rotate(rotate)
  end

  # Накладываем
  result = img.composite(overlay, Magick::CenterGravity, Magick::OverCompositeOp)
  result.write(output_path)
end

def add_image_watermark(input_path, output_path, watermark_path, position: 'br',
                        opacity: 0.5, rotate: 0)
  img = Magick::Image.read(input_path).first
  wm = Magick::Image.read(watermark_path).first

  # Масштабируем
  if wm.columns > img.columns / 2 || wm.rows > img.rows / 2
    scale = [img.columns.to_f / wm.columns, img.rows.to_f / wm.rows].min * 0.4
    wm.resize!( (wm.columns * scale).to_i, (wm.rows * scale).to_i )
  end

  # Применяем прозрачность
  if opacity < 1.0
    wm.alpha(Magick::ActivateAlphaChannel)
    wm.each_pixel do |pixel, col, row|
      pixel.alpha = (pixel.alpha * opacity).to_i
    end
  end

  # Позиция
  x, y = get_position(img, position)
  if position == 'c'
    x = (img.columns - wm.columns) / 2
    y = (img.rows - wm.rows) / 2
  else
    if position.include?('r')
      x = img.columns - wm.columns - x
    end
    if position.include?('b')
      y = img.rows - wm.rows - y
    end
  end

  # Накладываем
  result = img.composite(wm, x, y, Magick::OverCompositeOp)
  result.write(output_path)
end

def batch_process(input_dir, output_dir, mode, **kwargs)
  FileUtils.mkdir_p(output_dir)

  extensions = %w[.jpg .jpeg .png .bmp .tiff .webp]
  files = Dir.entries(input_dir)
             .select { |f| extensions.include?(File.extname(f).downcase) }

  files.each_with_index do |fname, i|
    in_path = File.join(input_dir, fname)
    out_path = File.join(output_dir, fname)
    puts colorize("[#{i+1}/#{files.length}] Processing #{fname}...", :yellow)

    begin
      if mode == 'text'
        add_text_watermark(in_path, out_path, kwargs[:text],
                           position: kwargs[:position],
                           opacity: kwargs[:opacity],
                           font_size: kwargs[:font_size],
                           color: kwargs[:color],
                           rotate: kwargs[:rotate])
      else
        add_image_watermark(in_path, out_path, kwargs[:watermark],
                            position: kwargs[:position],
                            opacity: kwargs[:opacity],
                            rotate: kwargs[:rotate])
      end
    rescue => e
      puts colorize("  Error: #{e.message}", :red)
    end
  end
end

options = { position: 'br', opacity: 0.5, font_size: 48, color: '#FFFFFF', rotate: 0 }
parser = OptionParser.new do |opts|
  opts.banner = "Usage: ruby watermark.rb text|image|batch <input> [output] [options]"
  opts.on("-t", "--text TEXT", "Watermark text") { |v| options[:text] = v }
  opts.on("-i", "--image FILE", "Watermark image") { |v| options[:watermark] = v }
  opts.on("-p", "--position POS", "Position: tl,tc,tr,ml,c,mr,bl,bc,br (default: br)") { |v| options[:position] = v }
  opts.on("-o", "--opacity N", Float, "Opacity 0.0-1.0 (default: 0.5)") { |v| options[:opacity] = v }
  opts.on("-s", "--size N", Integer, "Font size (default: 48)") { |v| options[:font_size] = v }
  opts.on("-c", "--color COLOR", "Text color #RRGGBB (default: #FFFFFF)") { |v| options[:color] = v }
  opts.on("-r", "--rotate DEG", Integer, "Rotation angle (default: 0)") { |v| options[:rotate] = v }
end
parser.parse!

args = ARGV
if args.length < 2
  puts colorize(parser.banner, :yellow)
  exit 1
end

mode = args[0]
input = args[1]
output = args[2] || "watermarked_#{File.basename(input)}"

begin
  if mode == 'batch'
    if output.nil? || File.directory?(input) == false
      raise "Input must be a directory for batch mode"
    end
    batch_process(input, output, mode, **options)
    puts colorize("Batch processing completed!", :green)
  elsif mode == 'text'
    if options[:text].nil? || options[:text].empty?
      raise "Text required for text mode"
    end
    add_text_watermark(input, output, options[:text],
                       position: options[:position],
                       opacity: options[:opacity],
                       font_size: options[:font_size],
                       color: options[:color],
                       rotate: options[:rotate])
    puts colorize("Done! Result saved to #{output}", :green)
  elsif mode == 'image'
    if options[:watermark].nil? || options[:watermark].empty?
      raise "Watermark image required for image mode"
    end
    add_image_watermark(input, output, options[:watermark],
                        position: options[:position],
                        opacity: options[:opacity],
                        rotate: options[:rotate])
    puts colorize("Done! Result saved to #{output}", :green)
  else
    raise "Invalid mode. Use text, image, or batch"
  end
rescue => e
  puts colorize("Error: #{e.message}", :red)
  exit 1
end
