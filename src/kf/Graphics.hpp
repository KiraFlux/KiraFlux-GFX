#pragma once

#include "Font.hpp"
#include "FrameView.hpp"

#include <algorithm>
#include <cmath>


namespace kf {

/// Инструменты для рисования графических примитивов
struct Graphics {

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
    FrameView &frame;

private:

    /// Активный шрифт
    const Font *font{nullptr};


public:
    /// Создает графический контекст для FrameView
    explicit Graphics(FrameView &frame) noexcept:
        frame(frame) {}

    /// Установить шрифт
    void setFont(const Font &new_font) {
        font = &new_font;
    }

    /// Заполняет весь фрейм
    inline void fill(bool value) noexcept {
        frame.fill(value);
    }

    /// Рисует точку в указанных координатах
    void dot(Position x, Position y, bool on = true) noexcept {
        frame.setPixel(x, y, on);
    }

    /// Рисует линию по алгоритму Брезенхема
    void line(Position x0, Position y0, Position x1, Position y1, bool on = true) noexcept {
        const Position dx = std::abs(x1 - x0);
        const Position dy = -std::abs(y1 - y0);
        const Position sx = (x0 < x1) ? 1 : -1;
        const Position sy = (y0 < y1) ? 1 : -1;

        Position err = dx + dy;

        while (true) {
            dot(x0, y0, on);
            if (x0 == x1 && y0 == y1) { break; }

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
            const Position width = x1 - x0 + 1;
            const Position height = y1 - y0 + 1;
            auto view = frame.sub(width, height, x0, y0);

            if (view.ok()) {
                view.value.fill(value);
            }
        } else {
            // Рисование границ без дублирования углов
            drawLineHorizontal(x0, y0, x1, value);  // Верхняя сторона
            drawLineHorizontal(x0, y1, x1, value);  // Нижняя сторона

            // Боковые стороны (исключая углы)
            for (Position y = y0 + 1; y < y1; y++) {
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

    /// Рендерит текст с использованием текущего шрифта
    void text(Position x, Position y, const char *str, bool on = true) noexcept {
        if (font == nullptr) {
            render_missing_glyphs(x, y, str, on);
            return;
        }

        Position cursor_x = x;

        // Маска для обрезки неиспользуемых битов
        const rs::u8 height_mask = (1 << font->glyph_height) - 1;

        // Обрабатываем каждый символ строки
        for (const char *ptr = str; *ptr != '\0'; ++ptr) {
            const rs::u8 *glyph = font->getGlyph(*ptr);

            if (glyph != nullptr) {
                renderGlyph(cursor_x, y, glyph, height_mask, on);
            } else {
                drawMissingGlyphBox(cursor_x, y, font->glyph_width, font->glyph_height, on);
            }

            cursor_x += (font->glyph_width + 1);
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
    void drawLineHorizontal(Position x0, Position y, Position x1, bool on) noexcept {
        if (x0 > x1) { std::swap(x0, x1); }
        for (Position x = x0; x <= x1; x++) {
            frame.setPixel(x, y, on);
        }
    }

    /// Рисует 8 симметричных точек окружности
    void drawCirclePoints(Position cx, Position cy, Position dx, Position dy, bool value) noexcept {
        frame.setPixel(cx + dx, cy + dy, value);
        frame.setPixel(cx + dy, cy + dx, value);
        frame.setPixel(cx - dy, cy + dx, value);
        frame.setPixel(cx - dx, cy + dy, value);
        frame.setPixel(cx - dx, cy - dy, value);
        frame.setPixel(cx - dy, cy - dx, value);
        frame.setPixel(cx + dy, cy - dx, value);
        frame.setPixel(cx + dx, cy - dy, value);
    }

    /// Рисует прямоугольник-заглушку
    void drawMissingGlyphBox(Position x, Position y, Position width, Position height, bool on) noexcept {
        rect(
            x,
            y,
            x + width - 1,
            y + height - 1,
            on ? Mode::FillBorder : Mode::ClearBorder
        );
    }

    /// Рендерит глиф
    void renderGlyph(
        Position x,
        Position y,
        const rs::u8 *glyph,
        rs::u8 height_mask,
        bool on
    ) noexcept {
        for (rs::u8 col_index = 0; col_index < font->glyph_width; ++col_index) {
            const Position pixel_x = x + col_index;
            const rs::u8 glyph_data = glyph[col_index] & height_mask;

            // Пропускаем пустые столбцы
            if (glyph_data == 0) { continue; }

            // Обрабатываем каждый бит в столбце
            for (rs::u8 bit_index = 0; bit_index < font->glyph_height; ++bit_index) {
                if (glyph_data & (1 << bit_index)) {
                    frame.setPixel(pixel_x, y + bit_index, on);
                }
            }
        }
    }

    /// Рендерит заглушки для всей строки
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

};

} // namespace kfgfx