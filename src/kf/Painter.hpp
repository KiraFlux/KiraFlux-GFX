#pragma once

#include "Font.hpp"
#include "FrameView.hpp"

#include <algorithm>
#include <cmath>


namespace kf {

/// Инструменты для рисования графических примитивов
struct Painter {

public:

    /// Режимы отрисовки фигур
    enum class Mode : rs::u8 {
        /// Заполнить
        Fill = 0b11,
        /// Очистить
        Clear = 0b01,
        /// Заполнить только границу
        FillBorder = 0b10,
        /// Очистить только границу
        ClearBorder = 0b00,
    };

    /// Целевой кадр для рисования
    FrameView frame;

private:

    /// Активный шрифт
    const Font *current_font{nullptr};

public:

    /// Создать дочернюю область графического контекста
    rs::Result<Painter, FrameView::Error> sub(
        Position sub_width,
        Position sub_height,
        Position sub_offset_x,
        Position sub_offset_y
    ) {
        const auto frame_result = frame.sub(sub_width, sub_height, sub_offset_x, sub_offset_y);

        if (frame_result.fail()) {
            return {frame_result.error};
        }

        return {Painter{frame_result.value, current_font}};
    }

    /// Создает графический контекст для FrameView
    explicit Painter(const FrameView &frame) noexcept:
        frame{frame} {}

    explicit Painter(const FrameView &frame, const Font *font) noexcept:
        frame{frame}, current_font{font} {}

    /// Установить шрифт
    void setFont(const Font &new_font) { current_font = &new_font; }

    // Свойства

    /// Ширина фрейма
    inline Position width() const noexcept { return frame.width; }

    /// Высота фрейма
    inline Position height() const noexcept { return frame.height; }

    /// Максимальное значение X внутри фрейма
    inline Position maxX() const noexcept { return static_cast<Position>(width() - 1); }

    /// Максимальное значение Y внутри фрейма
    inline Position maxY() const noexcept { return static_cast<Position>(height() - 1); }

    /// Центр фрейма X
    inline Position centerX() const noexcept { return static_cast<Position>(maxX() / 2); }

    /// Центр фрейма X
    inline Position centerY() const noexcept { return static_cast<Position>(maxY() / 2); }

    // Графика

    /// Заполняет весь фрейм
    inline void fill(bool value) const noexcept {
        frame.fill(value);
    }

    /// Рисует точку в указанных координатах
    inline void dot(Position x, Position y, bool on = true) const noexcept {
        frame.setPixel(x, y, on);
    }

    /// Рисует битмап в указанных координатах
    template<Position W, Position H> inline void bitmap(Position x, Position y, const BitMap<W, H> &bm, bool on = true) noexcept {
        frame.drawBitmap(x, y, bm, on);
    }

    /// Рисует линию
    void line(Position x0, Position y0, Position x1, Position y1, bool on = true) const noexcept {
        // алгоритм Брезенхема
        const auto dx = static_cast<Position>(std::abs(x1 - x0));
        const auto dy = static_cast<Position>(-std::abs(y1 - y0));
        const Position sx = (x0 < x1) ? 1 : -1;
        const Position sy = (y0 < y1) ? 1 : -1;

        auto err = dx + dy;

        while (true) {
            dot(x0, y0, on);
            if (x0 == x1 and y0 == y1) { break; }

            const Position e2 = 2 * err;
            if (e2 >= dy) {
                if (x0 == x1) { break; }
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                if (y0 == y1) { break; }
                err += dx;
                y0 += sy;
            }
        }
    }

    /// Рисует прямоугольник с указанным режимом
    void rect(Position x0, Position y0, Position x1, Position y1, Mode mode) noexcept {
        // Нормализация координат
        if (x0 > x1) { std::swap(x0, x1); }
        if (y0 > y1) { std::swap(y0, y1); }

        const bool value = getModeValue(mode);

        if (isFillMode(mode)) {
            // Оптимизированная заливка через subview
            const auto width = static_cast<Position>(x1 - x0 + 1);
            const auto height = static_cast<Position>(y1 - y0 + 1);
            auto view = frame.sub(width, height, x0, y0);

            if (view.ok()) {
                view.value.fill(value);
            }
        } else {
            // Рисование границ без дублирования углов
            drawLineHorizontal(x0, y0, x1, value);  // Верхняя сторона
            drawLineHorizontal(x0, y1, x1, value);  // Нижняя сторона

            // Боковые стороны (исключая углы)
            for (auto y = static_cast<Position>(y0 + 1); y < y1; y++) {
                frame.setPixel(x0, y, value);
                frame.setPixel(x1, y, value);
            }
        }
    }

    /// Рисует окружность с указанным режимом
    void circle(Position cx, Position cy, Position r, Mode mode) noexcept {
        const bool value = getModeValue(mode);

        Position x = r;
        Position y = 0;
        Position err = 0;

        while (x >= y) {
            const Position last_y = y;
            y++;
            err += 2 * y + 1;

            if (2 * (err - x) + 1 > 0) {
                x--;
                err -= 2 * x + 1;
            }

            if (isFillMode(mode)) {
                const Position start_x = (x == last_y) ? last_y : x;

                drawLineHorizontal(cx - start_x, cy + last_y, cx + start_x, value);
                drawLineHorizontal(cx - start_x, cy - last_y, cx + start_x, value);
                drawLineHorizontal(cx - last_y, cy + x, cx + last_y, value);
                drawLineHorizontal(cx - last_y, cy - x, cx + last_y, value);
            } else {
                drawCirclePoints(cx, cy, x, y, value);

                // Дополнительные точки для гладкости
                if (x != y) {
                    drawCirclePoints(cx, cy, y, x, value);
                }
            }
        }
    }

    /// Рисует текст с использованием текущего шрифта
    void text(Position x, Position y, const char *str, bool on = true) noexcept {
        if (current_font == nullptr) {
            render_missing_glyphs(x, y, str, on);
            return;
        }

        Position cursor_x = x;

        // Маска для обрезки неиспользуемых битов
        const rs::u8 height_mask = (1 << current_font->glyph_height) - 1;

        // Обрабатываем каждый символ строки
        for (const char *ptr = str; *ptr != '\0'; ++ptr) {
            const rs::u8 *glyph = current_font->getGlyph(*ptr);

            if (glyph != nullptr) {
                renderGlyph(cursor_x, y, glyph, height_mask, on);
            } else {
                drawMissingGlyphBox(cursor_x, y, current_font->glyph_width, current_font->glyph_height, on);
            }

            cursor_x += (current_font->glyph_width + 1);
        }
    }

private:

    /// Получить значение режима
    static inline bool getModeValue(Mode mode) noexcept {
        return static_cast<rs::u8>(mode) & 0b10;
    }

    /// Режим является заполняющим
    static inline bool isFillMode(Mode mode) noexcept {
        return static_cast<rs::u8>(mode) & 0b01;
    }

    /// Рисует горизонтальную линию (оптимизированная версия)
    void drawLineHorizontal(Position x0, Position y, Position x1, bool on) const noexcept {
        if (x0 > x1) { std::swap(x0, x1); }
        for (Position x = x0; x <= x1; x++) {
            frame.setPixel(x, y, on);
        }
    }

    /// Рисует 8 симметричных точек окружности
    void drawCirclePoints(Position cx, Position cy, Position dx, Position dy, bool value) const noexcept {
        frame.setPixel(static_cast<Position>(cx + dx), static_cast<Position>(cy + dy), value);
        frame.setPixel(static_cast<Position>(cx + dy), static_cast<Position>(cy + dx), value);
        frame.setPixel(static_cast<Position>(cx - dy), static_cast<Position>(cy + dx), value);
        frame.setPixel(static_cast<Position>(cx - dx), static_cast<Position>(cy + dy), value);
        frame.setPixel(static_cast<Position>(cx - dx), static_cast<Position>(cy - dy), value);
        frame.setPixel(static_cast<Position>(cx - dy), static_cast<Position>(cy - dx), value);
        frame.setPixel(static_cast<Position>(cx + dy), static_cast<Position>(cy - dx), value);
        frame.setPixel(static_cast<Position>(cx + dx), static_cast<Position>(cy - dy), value);
    }

    /// Рисует прямоугольник-заглушку
    void drawMissingGlyphBox(Position x, Position y, Position width, Position height, bool on) noexcept {
        rect(
            x,
            y,
            static_cast<Position>(x + width - 1),
            static_cast<Position>(y + height - 1),
            on ? Mode::FillBorder : Mode::ClearBorder
        );
    }

    /// Рисует глиф
    void renderGlyph(
        Position x,
        Position y,
        const rs::u8 *glyph,
        rs::u8 height_mask,
        bool on
    ) noexcept {
        for (rs::u8 col_index = 0; col_index < current_font->glyph_width; ++col_index) {
            const Position pixel_x = x + col_index;
            const rs::u8 glyph_data = glyph[col_index] & height_mask;

            // Пропускаем пустые столбцы
            if (glyph_data == 0) { continue; }

            // Обрабатываем каждый бит в столбце
            for (rs::u8 bit_index = 0; bit_index < current_font->glyph_height; ++bit_index) {
                if (glyph_data & (1 << bit_index)) {
                    frame.setPixel(pixel_x, static_cast<Position>(y + bit_index), on);
                }
            }
        }
    }

    /// Рисует заглушки для всей строки
    void render_missing_glyphs(
        Position x,
        Position y,
        const char *str,
        bool on
    ) noexcept {
        constexpr Position default_glyph_width = 3;
        constexpr Position default_glyph_height = 5;

        Position cursor_x = x;

        // Обрабатываем каждый символ строки
        for (const char *ptr = str; *ptr != '\0'; ++ptr) {
            drawMissingGlyphBox(cursor_x, y, default_glyph_width, default_glyph_height, on);
            cursor_x += (default_glyph_width + 1);
        }
    }

public:

    Painter() = delete;

};

} // namespace kfgfx