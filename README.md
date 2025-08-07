# KiraFlux Graphics

Минималистичная графическая библиотека.

---

## FrameView
Представляет прямоугольную область в буфере дисплея.

```cpp
// Создание FrameView
static rs::Result<FrameView, FrameView::Error> create(
    uint8_t *buffer,        // Указатель на буфер дисплея
    int16_t stride,         // Шаг строки (ширина всего дисплея в пикселях)
    int16_t width,          // Ширина области
    int16_t height,         // Высота области
    int16_t offset_x,       // Смещение по X
    int16_t offset_y        // Смещение по Y
);

// Создание дочерней области
rs::Result<FrameView, FrameView::Error> sub(
    int16_t sub_width,     // Ширина под-области
    int16_t sub_height,    // Высота под-области
    int16_t sub_offset_x,  // Смещение по X относительно родителя
    int16_t sub_offset_y   // Смещение по Y относительно родителя
) const;

// Основные методы
bool setPixel(int16_t x, int16_t y, bool on); // Установка пикселя
void fill(bool value);                       // Заливка области
```

**Ошибка операции**

```cpp
enum class Error {
    SizeTooSmall,       // Размер окна слишком мал
    SizeTooLarge,       // Размер окна слишком велик
    OffsetOutOfBounds,  // Смещение выходит за пределы
};
```

## Graphics
Инструменты для рисования в FrameView.

```cpp
// Конструктор
explicit Graphics(FrameView &frame);

// Режимы рисования
enum class Mode : uint8_t {
    Fill,          // Заливка области (true)
    Clear,         // Очистка области (false)
    FillBorder,   // Контур (true)
    ClearBorder   // Контур (false)
};

// Методы рисования

// Линия
void line(                                      
    int16_t x0, int16_t y0,
    int16_t x1, int16_t y1,
    bool on = true
);

// Прямоугольник
void rect(                                      
    int16_t x0, int16_t y0,
    int16_t x1, int16_t y1,
    Mode mode
);

// Окружность
void circle(                                    
    int16_t center_x, 
    int16_t center_y,
    int16_t radius,
    Mode mode
);
```

## Arduino пример

```cpp
#include <kfgfx/FrameView.hpp>
#include <kfgfx/Graphics.hpp>

// Буфер дисплея 128x64
static uint8_t screen_buffer[128 * 64 / 8] = {0};

kfgfx::FrameView main_frame(screen_buffer, 128, 128, 64, 0, 0);
kfgfx::Graphics gfx(main_frame);

void setup() {
    // Инициализация дисплея
    // ...
}

void loop() {
    // Очистка экрана
    main_frame.fill(false);
    
    // Рисование примитивов
    gfx.rect(10, 10, 50, 30, kfgfx::Graphics::Mode::Fill);
    gfx.circle(64, 32, 20, kfgfx::Graphics::Mode::FillBorder);
    
    // Создание и использование под-области
    auto widget = main_frame.sub(40, 20, 20, 10);
    if (widget.ok()) {
        kfgfx::Graphics widget_gfx(widget.value);
        // ...
    }
    
    delay(20);
}
```

## Ключевые особенности
1. **Иерархические области**: Создавайте вложенные FrameView для виджетов
2. **Нулевые накладные расходы**: Работа напрямую с буфером
3. **Эффективные алгоритмы**: Оптимизированные для микроконтроллеров
4. **Автоматические проверки**: Граничные проверки при операциях
5. **Простой API**: Минимум абстракций

**Типовой workflow:**
1. Создать буфер дисплея
2. Инициализировать основной FrameView один раз
3. В loop() очищать экран и рисовать примитивы
4. Создавать дочерние FrameView для компонентов UI

---

Лицензия: MIT (см. [LICENSE](./LICENSE))