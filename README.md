# KiraFlux Graphics

*Минималистичная графическая библиотека для встроенных систем*

---

#### **FrameView**

Представление прямоугольной области дисплея.

**Создание:**

```cpp
static rs::Result<FrameView, Error> create(  
    rs::u8* buffer,  
    Position stride,  
    Position width,  
    Position height,  
    Position offset_x,  
    Position offset_y  
) noexcept;  
```  

Создает область с проверкой ошибок. Возвращает `Result` с ошибкой при некорректных параметрах.

```cpp
FrameView(  
    rs::u8* buffer,  
    Position stride,  
    Position width,  
    Position height,  
    Position offset_x,  
    Position offset_y  
) noexcept;  
```  

Создание без проверок (только когда параметры гарантированно корректны).

---

**Методы FrameView:**

```cpp
rs::Result<FrameView, Error> sub(  
    Position sub_width,  
    Position sub_height,  
    Position sub_offset_x,  
    Position sub_offset_y  
) const noexcept;  
```  

Создает дочернюю область с проверкой границ. Возможные ошибки:

- `SizeTooSmall`: размер < 1px
- `SizeTooLarge`: дочерняя область > родительской
- `OffsetOutOfBounds`: смещение вне границ

```cpp
FrameView subUnchecked(  
    Position sub_width,  
    Position sub_height,  
    Position sub_offset_x,  
    Position sub_offset_y  
) noexcept;  
```  

Создает дочернюю область без проверок (для критичных к производительности участков).

```cpp
void fill(bool value) noexcept;  
```  

Заливает всю область указанным значением (true - белый, false - черный).

```cpp
void setPixel(Position x, Position y, bool on) noexcept;  
```  

Устанавливает состояние пикселя в координатах (x, y) относительно области.

```cpp
bool getPixel(Position x, Position y) const noexcept;  
```  

Возвращает состояние пикселя в координатах (x, y).

```cpp
template<Position W, Position H>  
void drawBitmap(  
    Position x,  
    Position y,  
    const BitMap<W, H>& bm,  
    bool on = true  
) noexcept;  
```  

Рисует битмап с верхним левым углом в (x, y). Параметр `on` определяет режим рисования (true - обычный, false - инверсный).

---

#### **BitMap**

```cpp
template<Position W, Position H>  
struct BitMap {  
    static constexpr Position width = W;  
    static constexpr Position height = H;  
    const rs::u8 buffer[W * ((H + 7) / 8)];  
};  
```  

Статический битмап с предрасчитанными размерами. Хранит данные в упакованном виде (8 пикселей на байт).

**Пример:**

```cpp
// Шахматный паттерн 16x16  
const kf::BitMap<16, 16> checker = {
0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA
};  
```  

---

#### **Font**

```cpp
struct Font {  
    const rs::u8* data;  
    const rs::u8 glyph_width;  
    const rs::u8 glyph_height;  

    static const Font& blank() noexcept;  
};  
```  

Моноширинный шрифт. Глифы хранятся в виде массивов битов.

**Доступные шрифты:**

```cpp
namespace kf::fonts {  
    extern const Font gyver_5x7_en; // Английский 5x7  
}  
```  

**Метод:**

```cpp
const rs::u8* getGlyph(char c) const noexcept;  
```  

Возвращает указатель на данные глифа для символа или `nullptr` если символ отсутствует.

---

#### **Painter**

Инструмент для рисования с поддержкой текста и примитивов.

**Инициализация:**

```cpp
explicit Painter(  
    const FrameView& frame,  
    const Font& font = Font::blank()  
) noexcept;  
```  

Создает Painter, привязанный к области и шрифту.

---

**Методы Painter:**

```cpp
rs::Result<Painter, FrameView::Error> sub(  
    Position width,  
    Position height,  
    Position offset_x,  
    Position offset_y  
);  
```  

Создает дочерний Painter с проверкой ошибок.

```cpp
Painter subUnchecked(  
    Position width,  
    Position height,  
    Position offset_x,  
    Position offset_y  
) noexcept;  
```  

Создает дочерний Painter без проверок.

```cpp
template<rs::size N>  
std::array<Painter, N> splitHorizontally(  
    std::array<rs::u8, N> weights  
);  
```  

Делит область горизонтально на N частей согласно весам. Пример:

```cpp
// 25% | 75%  
auto[left, right] = painter.splitHorizontally<2>({ 1, 3 });  
```  

```cpp
template<rs::size N>  
std::array<Painter, N> splitVertically(  
    std::array<rs::u8, N> weights  
);  
```  

Делит область вертикально.

```cpp
void setCursor(Position x, Position y) noexcept;  
```  

Устанавливает позицию для текстового вывода (левый верхний угол следующего символа).

```cpp
void setFont(const Font& font) noexcept;  
```  

Смена активного шрифта.

```cpp
void text(rs::str text) noexcept;  
```  

Выводит текст с поддержкой управляющих последовательностей:

- `\n`: Перенос строки
- `\t`: Табуляция (4 символа)
- `\x80`: Нормальный цвет текста
- `\x81`: Инверсный цвет текста
- `\x82`: Устанавливает X-координату курсора в центр области

---

**Графические примитивы:**

```cpp
void dot(Position x, Position y, bool on = true) noexcept;  
```  

Рисует точку в координатах (x, y).

```cpp
void line(Position x0, Position y0, Position x1, Position y1, bool on = true) noexcept;  
```  

Рисует линию между точками (x0,y0) и (x1,y1) алгоритмом Брезенхема.

```cpp
void rect(Position x0, Position y0, Position x1, Position y1, Mode mode) noexcept;  
```  

Рисует прямоугольник с режимом:

```cpp
enum class Mode : rs::u8 {  
    Fill,        // Заливка всей области  
    Clear,       // Очистка всей области  
    FillBorder,  // Только граница (пиксели включены)  
    ClearBorder  // Только граница (пиксели выключены)  
};  
```  

```cpp
void circle(Position cx, Position cy, Position r, Mode mode) noexcept;  
```  

Рисует окружность с центром (cx, cy) и радиусом r. Поддерживает те же режимы, что и `rect()`.

---

### Примеры использования

**1. Центрирование текста:**

```cpp
// Установка начала текста в центр  
painter.setCursor(painter.centerX(), 10);
painter.text("\x82Hello World");

// Ручное центрирование  
auto text = "Centered";
Position text_width = strlen(text) * painter.current_font->glyph_width;
Position start_x = painter.centerX() - text_width/2;
painter.setCursor(start_x, 10);
painter.text(text);
```  

**2. Динамический интерфейс:**

```cpp
void create_ui(kf::Painter& painter) {
    // Разделение на заголовок и контент
    auto [header, content] = painter.splitVertically<2>({1, 5});
    
    // Заголовок
    header.fill(true);
    header.setCursor(header.centerX(), 2);
    header.text("\x82\x80Dashboard"); // Центрированный белый текст
    
    // Контент-область
    content.rect(0, 0, content.width()-1, content.height()-1, 
                 Painter::Mode::FillBorder);
}
```

**3. Анимация с примитивами:**

```cpp
void animate(kf::Painter& painter) {
    static Position y = 0;
    painter.fill(false); // Очистка
    
    // Двигающаяся линия
    painter.line(0, y, painter.width()-1, y);
    
    // Прыгающий круг
    static Position radius = 5;
    Position cx = painter.width() / 2;
    Position cy = y + radius + 2;
    painter.circle(cx, cy, radius, Painter::Mode::FillBorder);
    
    y = (y + 1) % painter.height();
    radius = 3 + (y % 10);
}
```

---

### Особенности работы

1. **Система координат**:
    - Начало (0, 0) в левом верхнем углу
    - X увеличивается вправо, Y - вниз

2. **Производительность**:
    - Примитивы оптимизированы для embedded-систем
    - Все методы `noexcept`
    - Минимальные проверки в `Unchecked` методах

3. **Текстовый вывод**:
    - Автоматический перенос при включенном `auto_next_line`
    - Межсимвольный интервал: 1 пиксель
    - Поддержка только ASCII (32-127)

4. **Ограничения**:
    - Максимальная высота шрифта: 8 пикселей
    - Нет поддержки поворота текста

**Рекомендации**:

- Для статических элементов используйте `BitMap` вместо примитивов
- При работе с под-областями проверяйте ошибки через `Result<>`
- Для динамического контента отключайте `auto_next_line`

```cpp
// Отключение авто-переноса
painter.auto_next_line = false;
```  

Лицензия: MIT ([LICENSE](./LICENSE))