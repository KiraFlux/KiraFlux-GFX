# KiraFlux Graphics
Минималистичная графическая библиотека для встроенных систем

## Основные компоненты

### FrameView
Абстракция прямоугольной области в буфере дисплея:
```cpp
// Создание области
static Result<FrameView, Error> create(
    uint8_t *buffer,    // Буфер дисплея
    int16_t stride,     // Ширина дисплея (пиксели)
    int16_t width,      // Ширина области
    int16_t height,     // Высота области
    int16_t offset_x,   // Смещение X
    int16_t offset_y    // Смещение Y
);

// Создание дочерней области
Result<FrameView, Error> sub(
    int16_t sub_width,    // Ширина под-области
    int16_t sub_height,   // Высота под-области
    int16_t sub_offset_x, // Смещение X
    int16_t sub_offset_y  // Смещение Y
) const;

// Основные операции
void fill(bool value);                     // Заливка области
void setPixel(int16_t x, int16_t y, bool on); // Управление пикселями
void drawBitmap(int16_t x, int16_t y, const BitMap& bm); // Рисование битмапа
```

**Типы ошибок:**
```cpp
enum class Error {
    SizeTooSmall,       // Размер < 1px
    SizeTooLarge,       // Дочерняя область > родительской
    OffsetOutOfBounds   // Смещение вне границ
};
```

### BitMap
Шаблонный статический битмап:
```cpp
template<int16_t W, int16_t H>
struct BitMap {
    static constexpr int16_t width = W;
    static constexpr int16_t height = H;
    const uint8_t buffer[(W * H + 7)/8]; // Пакованные данные
};
```

### Graphics
**Инструменты рисования примитивов:**
```cpp
explicit Graphics(FrameView &frame);  // Конструктор

// Режимы рисования
enum class Mode : uint8_t {
    Fill,        // Заливка области
    Clear,       // Очистка области
    FillBorder,  // Контур (заполненный)
    ClearBorder  // Контур (очищенный)
};

// Примитивы
void dot(int16_t x, int16_t y, bool on = true); // Точка
void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool on = true); // Линия
void rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Mode mode); // Прямоугольник
void circle(int16_t cx, int16_t cy, int16_t r, Mode mode); // Окружность
```

**Особенности Graphics:**
- Алгоритм Брезенхема для линий
- Midpoint-алгоритм для окружностей
- Режимы заливки и контуров
- Автоматическая обрезка по границам FrameView
- Оптимизация для микроконтроллеров

## Ключевые преимущества
- **Иерархические области** - Создавайте вложенные FrameView для сложных интерфейсов
- **Нулевые накладные расходы** - Прямая работа с буфером без копирования
- **Автоматическая обрезка** - Графика не выходит за границы области
- **Эффективные алгоритмы** - Оптимизировано для ресурсо-ограниченных систем
- **Статические битмапы** - Минимальное потребление памяти для ресурсов

## Пример использования
```cpp
#include <kfgfx/FrameView.hpp>
#include <kfgfx/Graphics.hpp>
#include <kfgfx/BitMap.hpp>

// Битмап 16x16 (шахматный паттерн)
const auto checker = kfgfx::BitMap<16, 16>{
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55
};

// Буфер дисплея 128x64
uint8_t screen_buffer[1024] = {0};
kfgfx::FrameView display(screen_buffer, 128, 128, 64, 0, 0);
kfgfx::Graphics gfx(display);

void setup() {
    // Создаем дочернюю область с отступом
    auto panel = display.sub(100, 50, 14, 7).value;
    gfx.rect(0, 0, 99, 49, kfgfx::Graphics::Mode::FillBorder);
    
    // Рисуем битмап в центре
    panel.drawBitmap(42, 17, checker);
}

void loop() {
    // Анимация: перемещающаяся линия
    static int y_pos = 0;
    display.fill(false);
    gfx.line(0, y_pos, 127, y_pos);
    y_pos = (y_pos + 1) % 64;
    delay(50);
}
```

## Рабочий процесс
1. Объявить буфер дисплея
2. Создать основной FrameView
3. Инициализировать Graphics для рисования
4. Создавать дочерние области для компонентов UI
5. Использовать:
    - `fill()` для заливки областей
    - `rect()`/`circle()` для фигур
    - `drawBitmap()` для статичных изображений
    - `line()`/`dot()` для динамических элементов

---

Лицензия: MIT ([LICENSE](./LICENSE))