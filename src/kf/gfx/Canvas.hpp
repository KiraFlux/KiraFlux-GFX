#pragma once

#include <array>
#include <cmath>
#include <kf/Result.hpp>

#include <kf/gfx/BitMap.hpp>
#include <kf/gfx/Font.hpp>
#include <kf/gfx/FrameView.hpp>

namespace kf::gfx {

/// @brief Инструменты для рисования графических примитивов
struct Canvas {

    /// @brief Режимы отрисовки фигур
    enum class Mode : u8 {

        /// @brief Заполнить
        Fill = 0b11,

        /// @brief Очистить
        Clear = 0b01,

        /// @brief Заполнить только границу
        FillBorder = 0b10,

        /// @brief Очистить только границу
        ClearBorder = 0b00,
    };

    /// @brief Целевой кадр для рисования
    FrameView frame;

private:
    /// @brief Активный шрифт
    /// @brief Всегда указывает на экземпляр шрифта
    /// @brief Гарантированно не nullptr
    const Font *current_font;

    /// @brief Позиция курсора X
    Pixel cursor_x{0};

    /// @brief Позиция курсора Y
    Pixel cursor_y{0};

public:
    /// @brief Автоматический перенос строки
    bool auto_next_line{false};

    explicit Canvas(const FrameView &frame, const Font &font = Font::blank()) noexcept :
        frame{frame}, current_font{&font} {}

    explicit Canvas() :
        frame{}, current_font{&Font::blank()} {}

    /// @brief Создать дочернюю область графического контекста
    kf::Result<Canvas, FrameView::Error> sub(
        Pixel width,
        Pixel height,
        Pixel offset_x,
        Pixel offset_y) {
        const auto frame_result = frame.sub(width, height, offset_x, offset_y);

        if (frame_result.isOk()) {
            return {Canvas{frame_result.ok().value(), *current_font}};
        } else {
            return {frame_result.error().value()};
        }
    }

    /// @brief Создать дочернюю область графического контекста без проверок
    /// @brief @warning unsafe
    Canvas subUnchecked(
        /// @brief Ширина дочерней области.
        /// @details sub_width не более parent.width()
        Pixel width,

        /// @brief Высота дочерней области.
        /// @details sub_height не более parent.height()
        Pixel height,

        /// @brief Смещение по X относительно родителя.
        /// @details 0 .. (parent.width() - sub_width)
        Pixel offset_x,

        /// @brief Смещение по Y относительно родителя.
        /// @details 0 .. (parent.height() - sub_height)
        Pixel offset_y) {
        return Canvas{frame.subUnchecked(width, height, offset_x, offset_y), *current_font};
    }

    /// @brief Установить шрифт
    void setFont(const Font &font) { current_font = &font; }

    // Свойства

    /// @brief Ширина фрейма (Размер X)
    [[nodiscard]] inline Pixel width() const noexcept { return frame.width; }

    /// @brief Высота фрейма (Размер Y)
    [[nodiscard]] inline Pixel height() const noexcept { return frame.height; }

    /// @brief Максимальное значение X внутри фрейма
    [[nodiscard]] inline Pixel maxX() const noexcept { return static_cast<Pixel>(width() - 1); }

    /// @brief Максимальное значение Y внутри фрейма
    [[nodiscard]] inline Pixel maxY() const noexcept { return static_cast<Pixel>(height() - 1); }

    /// @brief Центр фрейма X
    [[nodiscard]] inline Pixel centerX() const noexcept { return static_cast<Pixel>(maxX() / 2); }

    /// @brief Центр фрейма X
    [[nodiscard]] inline Pixel centerY() const noexcept { return static_cast<Pixel>(maxY() / 2); }

    /// @brief Максимальная позиция X для глифа активного шрифта
    [[nodiscard]] inline Pixel maxGlyphX() const noexcept { return static_cast<Pixel>(width() - current_font->glyph_width); }

    /// @brief Максимальная позиция Y для глифа активного шрифта
    [[nodiscard]] inline Pixel maxGlyphY() const noexcept { return static_cast<Pixel>(height() - current_font->glyph_height); }

    /// @brief Ширина табуляции (Размер X)
    [[nodiscard]] inline Pixel tabWidth() const noexcept { return static_cast<Pixel>(current_font->widthTotal() * 4); }

    // Управление

    /// @brief Создаёт дочерние области с горизонтальным разделением
    template<usize N> std::array<Canvas, N> splitHorizontally(std::array<u8, N> weights) {
        auto sizes = calculateSplitSizes<N>(width(), weights);
        std::array<Canvas, N> painters;
        Pixel x = 0;

        for (usize i = 0; i < N; ++i) {
            painters[i] = subUnchecked(sizes[i], height(), x, 0);
            x += sizes[i];
        }

        return painters;
    }

    /// @brief Создаёт дочерние области с вертикальным разделением
    template<usize N> std::array<Canvas, N> splitVertically(std::array<u8, N> weights) {
        auto sizes = calculateSplitSizes<N>(height(), weights);
        std::array<Canvas, N> painters;
        Pixel y = 0;

        for (usize i = 0; i < N; ++i) {
            painters[i] = subUnchecked(width(), sizes[i], 0, y);
            y += sizes[i];
        }

        return painters;
    }

    // Графика

    /// @brief Заполняет весь фрейм
    inline void fill(bool value) const noexcept {
        frame.fill(value);
    }

    /// @brief Рисует точку в указанных координатах
    inline void dot(Pixel x, Pixel y, bool on = true) const noexcept {
        frame.setPixel(x, y, on);
    }

    /// @brief Рисует битмап в указанных координатах
    template<Pixel W, Pixel H> inline void bitmap(Pixel x, Pixel y, const BitMap<W, H> &bm, bool on = true) noexcept {
        frame.drawBitmap(x, y, bm, on);
    }

    /// @brief Рисует линию
    void line(Pixel x0, Pixel y0, Pixel x1, Pixel y1, bool on = true) const noexcept {
        if (x0 == x1) {
            if (y0 == y1) {
                dot(x0, y0, on);
            } else {
                drawLineVertical(x0, y0, y1, on);
            }
            return;
        }

        if (y0 == y1) {
            drawLineHorizontal(x0, y0, x1, on);
            return;
        }

        // алгоритм Брезенхема
        const auto dx = static_cast<Pixel>(abs(x1 - x0));
        const auto dy = static_cast<Pixel>(-abs(y1 - y0));
        const auto sx = (x0 < x1) ? 1 : -1;
        const auto sy = (y0 < y1) ? 1 : -1;

        auto error = dx + dy;

        while (true) {
            dot(x0, y0, on);
            if (x0 == x1 and y0 == y1) { break; }

            const auto double_error = 2 * error;
            if (double_error >= dy) {
                if (x0 == x1) { break; }
                error += dy;
                x0 = static_cast<Pixel>(x0 + sx);
            }
            if (double_error <= dx) {
                if (y0 == y1) { break; }
                error += dx;
                y0 = static_cast<Pixel>(y0 + sy);
            }
        }
    }

    /// @brief Рисует прямоугольник с указанным режимом
    void rect(Pixel x0, Pixel y0, Pixel x1, Pixel y1, Mode mode) noexcept {
        // Нормализация координат
        if (x0 > x1) { std::swap(x0, x1); }
        if (y0 > y1) { std::swap(y0, y1); }

        const bool value = getModeValue(mode);

        if (isFillMode(mode)) {
            // Оптимизированная заливка через subview
            const auto width = static_cast<Pixel>(x1 - x0 + 1);
            const auto height = static_cast<Pixel>(y1 - y0 + 1);
            frame.subUnchecked(width, height, x0, y0).fill(value);
        } else {
            // Рисование границ без дублирования углов
            drawLineHorizontal(x0, y0, x1, value);// Верхняя сторона
            drawLineHorizontal(x0, y1, x1, value);// Нижняя сторона

            // Боковые стороны (исключая углы)
            for (auto y = static_cast<Pixel>(y0 + 1); y < y1; y++) {
                frame.setPixel(x0, y, value);
                frame.setPixel(x1, y, value);
            }
        }
    }

    /// @brief Рисует окружность с указанным режимом
    void circle(Pixel center_x, Pixel center_y, Pixel r, Mode mode) noexcept {
        const bool value = getModeValue(mode);

        Pixel x = r;
        Pixel y = 0;
        auto err = 0;

        while (x >= y) {
            const Pixel last_y = y;
            y++;
            err += 2 * y + 1;

            if (2 * (err - x) + 1 > 0) {
                x--;
                err -= 2 * x + 1;
            }

            if (isFillMode(mode)) {
                const Pixel start_x = (x == last_y) ? last_y : x;

                drawLineHorizontal(
                    static_cast<Pixel>(center_x - start_x),
                    static_cast<Pixel>(center_y + last_y),
                    static_cast<Pixel>(center_x + start_x),
                    value);
                drawLineHorizontal(
                    static_cast<Pixel>(center_x - start_x),
                    static_cast<Pixel>(center_y - last_y),
                    static_cast<Pixel>(center_x + start_x),
                    value);
                drawLineHorizontal(
                    static_cast<Pixel>(center_x - last_y),
                    static_cast<Pixel>(center_y + x),
                    static_cast<Pixel>(center_x + last_y),
                    value);
                drawLineHorizontal(
                    static_cast<Pixel>(center_x - last_y),
                    static_cast<Pixel>(center_y - x),
                    static_cast<Pixel>(center_x + last_y),
                    value);
            } else {
                drawCirclePoints(center_x, center_y, x, y, value);

                // Дополнительные точки для гладкости
                if (x != y) {
                    drawCirclePoints(center_x, center_y, y, x, value);
                }
            }
        }
    }

    /// @brief Установить позицию курсора
    void setCursor(Pixel x, Pixel y) noexcept {
        cursor_x = x;
        cursor_y = y;
    }

    /// @brief Рисует текст с использованием текущего шрифта.
    /// @param text C-style string
    /// @param on Цвет текста
    /// @details <code>'\\n'</code> для перехода на новую строку
    /// @details <code>'\\t'</code> для табуляции
    /// @details <code>'\\x80'</code> для нормального текста
    /// @details <code>'\\x81'</code> для инверсии текста
    /// @details <code>'\\x82'</code> для установки курсора по центру фрейма
    void text(const char *text, bool on = true) noexcept {
        for (; *text != '\0'; text += 1) {
            if (*text == '\x80') {
                on = true;
                continue;
            }
            if (*text == '\x81') {
                on = false;
                continue;
            }
            if (*text == '\x82') {
                const auto new_x = centerX();
                clearLineSegment(new_x, on);
                cursor_x = new_x;
                continue;
            }
            if (*text == '\n') {
                clearLineSegment(maxX(), on);
                nextLine();
                continue;
            }
            if (*text == '\t') {
                const auto tab_width = tabWidth();
                const auto new_x = static_cast<Pixel>(((cursor_x / tab_width) + 1) * tab_width);
                clearLineSegment(new_x, on);
                cursor_x = new_x;
                continue;
            }

            if (cursor_x > maxGlyphX()) {
                clearLineSegment(maxX(), on);

                if (auto_next_line) {
                    nextLine();
                } else {
                    return;
                }
            }

            if (cursor_y > maxGlyphY()) { return; }

            drawGlyph(cursor_x, cursor_y, current_font->getGlyph(*text), on);

            cursor_x = static_cast<Pixel>(cursor_x + current_font->glyph_width);

            if (cursor_x < width()) {
                drawLineVertical(
                    cursor_x,
                    cursor_y,
                    static_cast<Pixel>(cursor_y + current_font->glyph_height),
                    not on);
            }

            cursor_x = static_cast<Pixel>(cursor_x + 1);
        }
    }

private:
    /// @brief Рассчитывает размеры областей для разделения
    template<usize N> std::array<Pixel, N> calculateSplitSizes(Pixel total_size, std::array<u8, N> weights) {
        for (auto &w: weights) {
            if (w <= 0) { w = 1; }
        }

        Pixel total_weight = 0;
        for (auto w: weights) { total_weight += w; }

        std::array<Pixel, N> sizes;
        Pixel remaining = total_size;

        for (usize i = 0; i < N; ++i) {
            sizes[i] = (total_size * weights[i]) / total_weight;
            remaining -= sizes[i];
        }

        for (usize i = 0; remaining > 0; i = (i + 1) % N) {
            sizes[i] += 1;
            remaining -= 1;
        }

        return sizes;
    }

    /// @brief Очистить сегмент строки от курсора
    void clearLineSegment(Pixel x, bool on) noexcept {
        rect(
            cursor_x,
            cursor_y,
            x,
            static_cast<Pixel>(cursor_y + current_font->glyph_height),
            on ? Mode::Clear : Mode::Fill);
    }

    /// @brief Перенести курсор на следующую строку
    void nextLine() noexcept {
        cursor_x = 0;
        cursor_y = static_cast<Pixel>(cursor_y + current_font->heightTotal());
    }

    /// @brief Получить значение режима
    static inline bool getModeValue(Mode mode) noexcept {
        return static_cast<u8>(mode) & 0b10;
    }

    /// @brief Режим является заполняющим
    static inline bool isFillMode(Mode mode) noexcept {
        return static_cast<u8>(mode) & 0b01;
    }

    /// @brief Рисует горизонтальную линию
    void drawLineHorizontal(Pixel x0, Pixel y, Pixel x1, bool on) const noexcept {
        if (x0 > x1) { std::swap(x0, x1); }
        for (Pixel x = x0; x <= x1; x++) {
            frame.setPixel(x, y, on);
        }
    }

    /// @brief Рисует вертикальную линию
    void drawLineVertical(Pixel x, Pixel y0, Pixel y1, bool on) const noexcept {
        if (y0 > y1) { std::swap(y0, y1); }
        for (Pixel y = y0; y <= y1; y++) {
            frame.setPixel(x, y, on);
        }
    }

    /// @brief Рисует 8 симметричных точек окружности
    void drawCirclePoints(Pixel cx, Pixel cy, Pixel dx, Pixel dy, bool value) const noexcept {
        frame.setPixel(static_cast<Pixel>(cx + dx), static_cast<Pixel>(cy + dy), value);
        frame.setPixel(static_cast<Pixel>(cx + dy), static_cast<Pixel>(cy + dx), value);
        frame.setPixel(static_cast<Pixel>(cx - dy), static_cast<Pixel>(cy + dx), value);
        frame.setPixel(static_cast<Pixel>(cx - dx), static_cast<Pixel>(cy + dy), value);
        frame.setPixel(static_cast<Pixel>(cx - dx), static_cast<Pixel>(cy - dy), value);
        frame.setPixel(static_cast<Pixel>(cx - dy), static_cast<Pixel>(cy - dx), value);
        frame.setPixel(static_cast<Pixel>(cx + dy), static_cast<Pixel>(cy - dx), value);
        frame.setPixel(static_cast<Pixel>(cx + dx), static_cast<Pixel>(cy - dy), value);
    }

    /// @brief Рисует глиф
    void drawGlyph(Pixel x, Pixel y, const u8 *glyph, bool on) noexcept {
        if (glyph == nullptr) {
            rect(
                x,
                y,
                static_cast<Pixel>(x + current_font->glyph_width - 1),
                static_cast<Pixel>(y + current_font->glyph_height - 1),
                on ? Mode::FillBorder : Mode::ClearBorder);
            return;
        }

        for (u8 col_index = 0; col_index < current_font->glyph_width; ++col_index) {
            const auto pixel_x = static_cast<Pixel>(x + col_index);

            for (u8 bit_index = 0; bit_index <= current_font->glyph_height; ++bit_index) {
                const bool lit = glyph[col_index] & (1 << bit_index);
                frame.setPixel(pixel_x, static_cast<Pixel>(y + bit_index), lit == on);
            }
        }
    }
};
}// namespace kf::gfx
