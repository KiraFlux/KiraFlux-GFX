# KiraFlux Graphics

*Минималистичная графическая библиотека для встроенных систем*

---

## Содержание
- [FrameView](#frameview)
- [BitMap](#bitmap)
- [Font](#font)
- [Canvas](#canvas)
- [Примеры использования](#примеры-использования)
- [Особенности работы](#особенности-работы)

---

## FrameView

Представление прямоугольной области дисплея.

### Создание

```cpp
static kf::Result<FrameView, Error> create(
    kf::u8 * buffer,
    kf::Pixel stride,
    kf::Pixel width,
    kf::Pixel height,
    kf::Pixel offset_x,
    kf::Pixel offset_y
) noexcept;
```

Создает область с проверкой ошибок. Возвращает `Result` с ошибкой при некорректных параметрах.

```cpp
FrameView(
    kf::u8 * buffer,
    kf::Pixel stride,
    kf::Pixel width,
    kf::Pixel height,
    kf::Pixel offset_x,
    kf::Pixel offset_y
) noexcept;
```

Создание без проверок (только когда параметры гарантированно корректны).

### Методы

```cpp
kf::Result<FrameView, Error> sub(
    kf::Pixel sub_width,
    kf::Pixel sub_height,
    kf::Pixel sub_offset_x,
    kf::Pixel sub_offset_y
) const noexcept;
```

Создает дочернюю область с проверкой границ. Возможные ошибки:
- `BufferNotInit`: буфер не инициализирован
- `SizeTooSmall`: размер < 1px
- `SizeTooLarge`: дочерняя область > родительской
- `OffsetOutOfBounds`: смещение вне границ

```cpp
FrameView subUnchecked(
    kf::Pixel sub_width,
    kf::Pixel sub_height,
    kf::Pixel sub_offset_x,
    kf::Pixel sub_offset_y
) noexcept;
```

Создает дочернюю область без проверок.

```cpp
void fill(bool value) noexcept;
```

Заливает всю область указанным значением (true - включено, false - выключено).

```cpp
void setPixel(kf::Pixel x, kf::Pixel y, bool on) noexcept;
```

Устанавливает состояние пикселя в координатах (x, y) относительно области.

```cpp
bool getPixel(kf::Pixel x, kf::Pixel y) const noexcept;
```

Возвращает состояние пикселя в координатах (x, y).

```cpp
template<kf::Pixel W, kf::Pixel H>
void drawBitmap(
    kf::Pixel x,
    kf::Pixel y,
    const BitMap<W, H> & bitmap,
    bool on = true
) noexcept;
```

Рисует битмап с верхним левым углом в (x, y).

---

## BitMap

Статический битмап с предрасчитанными размерами.

```cpp
template<kf::Pixel W, kf::Pixel H>
struct BitMap final {
    kf::Pixel width() const;          // Ширина битмапа
    kf::Pixel height() const;         // Высота битмапа
    static constexpr auto pages;      // Количество страниц
    const kf::u8 buffer[W * pages];   // Буфер данных
};
```

Хранит данные в упакованном виде (8 пикселей на байт).

**Пример:**
```cpp
// Шахматный паттерн 16x16
const kf::gfx::BitMap<16, 16> checker = {
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
    0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA
};
```

---

## Font

Моноширинный шрифт с высотой глифов до 8 пикселей.

```cpp
struct Font final {
    const kf::u8 * data;          // Данные шрифта
    const kf::u8 glyph_width;     // Ширина глифа
    const kf::u8 glyph_height;    // Высота глифа (1-8)

    static const Font & blank();  // Пустой шрифт
    
    kf::u8 widthTotal() const;    // Полная ширина глифа
    kf::u8 heightTotal() const;   // Полная высота глифа
    const kf::u8 * getGlyph(char c) const; // Получить глиф символа
};
```

### Доступные шрифты

```cpp
namespace kf::gfx::fonts {
    extern const Font gyver_5x7_en; // Английский шрифт 5x7
}
```

---

## Canvas

Инструмент для рисования с поддержкой текста и примитивов.

### Инициализация

```cpp
explicit Canvas(
    const FrameView & frame,
    const Font & font = Font::blank()
) noexcept;
```

### Свойства области

```cpp
kf::Pixel width() const;      // Ширина фрейма
kf::Pixel height() const;     // Высота фрейма
kf::Pixel maxX() const;       // Максимальная координата X
kf::Pixel maxY() const;       // Максимальная координата Y
kf::Pixel centerX() const;    // Центр по X
kf::Pixel centerY() const;    // Центр по Y
kf::Pixel maxGlyphX() const;  // Макс. X для глифа
kf::Pixel maxGlyphY() const;  // Макс. Y для глифа
kf::Pixel tabWidth() const;   // Ширина табуляции
kf::u8 widthInGlyph() const;  // Ширина в глифах
kf::u8 heightInGlyph() const; // Высота в глифах
```

### Управление областями

```cpp
kf::Result<Canvas, FrameView::Error> sub(
    kf::Pixel width,
    kf::Pixel height,
    kf::Pixel offset_x,
    kf::Pixel offset_y
);

Canvas subUnchecked(
    kf::Pixel width,
    kf::Pixel height,
    kf::Pixel offset_x,
    kf::Pixel offset_y
) noexcept;

template<kf::usize N>
std::array<Canvas, N> splitHorizontally(std::array<kf::u8, N> weights);

template<kf::usize N>
std::array<Canvas, N> splitVertically(std::array<kf::u8, N> weights);
```

### Графические примитивы

```cpp
void fill(bool value) const noexcept;
void dot(kf::Pixel x, kf::Pixel y, bool on = true) const noexcept;
void line(kf::Pixel x0, kf::Pixel y0, kf::Pixel x1, kf::Pixel y1, bool on = true) const noexcept;

void rect(kf::Pixel x0, kf::Pixel y0, kf::Pixel x1, kf::Pixel y1, Mode mode) noexcept;
void circle(kf::Pixel cx, kf::Pixel cy, kf::Pixel r, Mode mode) noexcept;

template<kf::Pixel W, kf::Pixel H>
void bitmap(kf::Pixel x, kf::Pixel y, const BitMap<W, H> & bm, bool on = true) noexcept;
```

**Режимы отрисовки:**
```cpp
enum class Mode : kf::u8 {
    Fill,           // Заливка всей области
    Clear,          // Очистка всей области  
    FillBorder,     // Только граница (включено)
    ClearBorder     // Только граница (выключено)
};
```

### Работа с текстом

```cpp
void setCursor(kf::Pixel x, kf::Pixel y) noexcept;
void setFont(const Font & font) noexcept;
void text(const char * text, bool on = true) noexcept;
```

**Управляющие последовательности:**
- `\n` - перенос строки
- `\t` - табуляция (4 символа)
- `\x80` - нормальный цвет текста
- `\x81` - инверсный цвет текста
- `\x82` - установка курсора по центру X

---

## Примеры использования

### 1. Простой интерфейс с разделением

```cpp
void create_dashboard(kf::gfx::Canvas & canvas) {
    // Вертикальное разделение: заголовок 20%, контент 80%
    auto [header, content] = canvas.splitVertically<2>({1, 4});
    
    // Заголовок
    header.fill(true); // Белый фон
    header.setCursor(header.centerX(), 2);
    header.text("\x82Status", false); // Центрированный черный текст
    
    // Контент с рамкой
    content.rect(0, 0, content.maxX(), content.maxY(), 
                 kf::gfx::Canvas::Mode::FillBorder);
    
    // Разделение контента на две колонки
    auto [left_panel, right_panel] = content.splitHorizontally<2>({1, 1});
    
    left_panel.setCursor(2, 2);
    left_panel.text("Temp: 25C");
    
    right_panel.setCursor(2, 2); 
    right_panel.text("Hum: 60%");
}
```

### 2. Анимация с графическими примитивами

```cpp
void animate_meter(kf::gfx::Canvas & canvas) {
    static kf::Pixel angle = 0;
    
    // Очистка и рамка
    canvas.fill(false);
    canvas.rect(0, 0, canvas.maxX(), canvas.maxY(), 
                kf::gfx::Canvas::Mode::FillBorder);
    
    // Центр и радиус
    kf::Pixel center_x = canvas.centerX();
    kf::Pixel center_y = canvas.centerY();
    kf::Pixel radius = canvas.height() / 3;
    
    // Окружность
    canvas.circle(center_x, center_y, radius, 
                  kf::gfx::Canvas::Mode::FillBorder);
    
    // Стрелка (линия из центра)
    kf::Pixel end_x = center_x + (radius * cos(angle)) / 256;
    kf::Pixel end_y = center_y + (radius * sin(angle)) / 256;
    canvas.line(center_x, center_y, end_x, end_y);
    
    angle += 10; // Следующий кадр
}
```

### 3. Работа с битмапами и текстом

```cpp
// Битмап иконки 8x8
const kf::gfx::BitMap<8, 8> icon = {
    0x3C, 0x42, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3C
};

void draw_notification(kf::gfx::Canvas & canvas, const char * message) {
    canvas.fill(false); // Очистка
    
    // Иконка слева
    canvas.bitmap(2, 2, icon);
    
    // Текст справа от иконки
    canvas.setCursor(12, 2);
    canvas.text(message);
    
    // Разделительная линия
    canvas.line(0, canvas.maxY(), canvas.maxX(), canvas.maxY());
}
```

### 4. Сложное разделение интерфейса

```cpp
void create_complex_ui(kf::gfx::Canvas & canvas) {
    // Главное разделение: сайдбар + основной контент
    auto [sidebar, main] = canvas.splitHorizontally<2>({1, 3});
    
    // Сайдбар: заголовок + меню
    auto [sidebar_header, menu] = sidebar.splitVertically<2>({1, 4});
    
    sidebar_header.fill(true);
    sidebar_header.setCursor(sidebar_header.centerX(), 2);
    sidebar_header.text("\x82Menu", false);
    
    // Основной контент: заголовок + данные + статус
    auto [main_header, content, status] = main.splitVertically<3>({1, 3, 1});
    
    main_header.rect(0, 0, main_header.maxX(), main_header.maxY(),
                     kf::gfx::Canvas::Mode::FillBorder);
    main_header.setCursor(main_header.centerX(), 2);
    main_header.text("\x82Data View");
    
    status.fill(true);
    status.setCursor(2, 2);
    status.text("Ready", false);
}
```

---

## Особенности работы

### Система координат
- Начало (0, 0) в левом верхнем углу
- X увеличивается вправо, Y - вниз
- Все координаты относительные внутри области

### Производительность
- Все методы `noexcept`
- Алгоритм Брезенхема для линий
- Оптимизированные методы заливки
- Минимальные проверки в `Unchecked` методах

### Текстовый вывод
- Моноширинные шрифты до 8px высотой
- Межсимвольный интервал: 1 пиксель
- Поддержка ASCII (32-127)
- Автоперенос при `auto_next_line = true`

### Рекомендации
- Используйте `BitMap` для статических элементов
- Проверяйте ошибки через `Result<>` при создании под-областей
- Для максимальной производительности используйте `Unchecked` методы
- Отключайте `auto_next_line` для ручного управления текстом

**Лицензия: MIT** ([LICENSE](./LICENSE))