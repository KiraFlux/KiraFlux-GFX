#pragma once

#include <cmath>
#include <array>

#include <kf/gfx/Font.hpp>
#include <kf/gfx/FrameView.hpp>
#include <kf/gfx/BitMap.hpp>
#include <kf/gfx/Position.hpp>


namespace kf::gfx {

/// Инструменты для рисования графических примитивов
struct Canvas {

public:

    explicit Canvas() noexcept = default;

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
    FrameView frame{};

private:

    /// Активный шрифт
    /// Всегда указывает на экземпляр шрифта
    /// Гарантированно не nullptr
    const Font *current_font{};

    /// Позиция курсора X
    Position cursor_x{0};

    /// Позиция курсора Y
    Position cursor_y{0};

public:

    /// Автоматический перенос строки
    bool auto_next_line{false};

    explicit Canvas(const FrameView &frame, const Font &font = Font::blank()) noexcept:
        frame{frame}, current_font{&font} {}

    /// Создать дочернюю область графического контекста
    rs::Result<Canvas, FrameView::Error> sub(
        Position width,
        Position height,
        Position offset_x,
        Position offset_y
    ) {
        const auto frame_result = frame.sub(width, height, offset_x, offset_y);

        if (frame_result.fail()) {
            return {frame_result.error};
        }

        return {Canvas{frame_result.value, *current_font}};
    }

    /// Создать дочернюю область графического контекста без проверок
    /// @warning unsafe
    Canvas subUnchecked(
        /// Ширина дочерней области.
        /// sub_width не более parent.width()
        Position width,

        /// Высота дочерней области.
        /// sub_height не более parent.height()
        Position height,

        /// Смещение по X относительно родителя.
        /// 0 .. (parent.width() - sub_width)
        Position offset_x,

        /// Смещение по Y относительно родителя.
        /// 0 .. (parent.height() - sub_height)
        Position offset_y
    ) {
        return Canvas{frame.subUnchecked(width, height, offset_x, offset_y), *current_font};
    }

    /// Установить шрифт
    void setFont(const Font &font) { current_font = &font; }

    // Свойства

    /// Ширина фрейма (Размер X)
    [[nodiscard]] inline Position width() const noexcept { return frame.width; }

    /// Высота фрейма (Размер Y)
    [[nodiscard]] inline Position height() const noexcept { return frame.height; }

    /// Максимальное значение X внутри фрейма
    [[nodiscard]] inline Position maxX() const noexcept { return static_cast<Position>(width() - 1); }

    /// Максимальное значение Y внутри фрейма
    [[nodiscard]] inline Position maxY() const noexcept { return static_cast<Position>(height() - 1); }

    /// Центр фрейма X
    [[nodiscard]] inline Position centerX() const noexcept { return static_cast<Position>(maxX() / 2); }

    /// Центр фрейма X
    [[nodiscard]] inline Position centerY() const noexcept { return static_cast<Position>(maxY() / 2); }

    /// Максимальная позиция X для глифа активного шрифта
    [[nodiscard]] inline Position maxGlyphX() const noexcept { return static_cast<Position>(width() - current_font->glyph_width); }

    /// Максимальная позиция Y для глифа активного шрифта
    [[nodiscard]] inline Position maxGlyphY() const noexcept { return static_cast<Position>(height() - current_font->glyph_height); }

    /// Ширина табуляции (Размер X)
    [[nodiscard]] inline Position tabWidth() const noexcept { return static_cast<Position>(current_font->widthTotal() * 4); }

    // Управление

    /// Создаёт дочерние области с горизонтальным разделением
    template<rs::size N> std::array<Canvas, N> splitHorizontally(std::array<rs::u8, N> weights) {
        auto sizes = calculateSplitSizes<N>(width(), weights);
        std::array<Canvas, N> painters;
        Position x = 0;

        for (rs::size i = 0; i < N; ++i) {
            painters[i] = subUnchecked(sizes[i], height(), x, 0);
            x += sizes[i];
        }

        return painters;
    }

    /// Создаёт дочерние области с вертикальным разделением
    template<rs::size N> std::array<Canvas, N> splitVertically(std::array<rs::u8, N> weights) {
        auto sizes = calculateSplitSizes<N>(height(), weights);
        std::array<Canvas, N> painters;
        Position y = 0;

        for (rs::size i = 0; i < N; ++i) {
            painters[i] = subUnchecked(width(), sizes[i], 0, y);
            y += sizes[i];
        }

        return painters;
    }

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
        const auto dx = static_cast<Position>(abs(x1 - x0));
        const auto dy = static_cast<Position>(-abs(y1 - y0));
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
                x0 = static_cast<Position>(x0 + sx);
            }
            if (double_error <= dx) {
                if (y0 == y1) { break; }
                error += dx;
                y0 = static_cast<Position>(y0 + sy);
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
            frame.subUnchecked(width, height, x0, y0).fill(value);
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
    void circle(Position center_x, Position center_y, Position r, Mode mode) noexcept {
        const bool value = getModeValue(mode);

        Position x = r;
        Position y = 0;
        auto err = 0;

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

                drawLineHorizontal(
                    static_cast<Position>(center_x - start_x),
                    static_cast<Position>(center_y + last_y),
                    static_cast<Position>(center_x + start_x),
                    value
                );
                drawLineHorizontal(
                    static_cast<Position>(center_x - start_x),
                    static_cast<Position>(center_y - last_y),
                    static_cast<Position>(center_x + start_x),
                    value
                );
                drawLineHorizontal(
                    static_cast<Position>(center_x - last_y),
                    static_cast<Position>(center_y + x),
                    static_cast<Position>(center_x + last_y),
                    value
                );
                drawLineHorizontal(
                    static_cast<Position>(center_x - last_y),
                    static_cast<Position>(center_y - x),
                    static_cast<Position>(center_x + last_y),
                    value
                );
            } else {
                drawCirclePoints(center_x, center_y, x, y, value);

                // Дополнительные точки для гладкости
                if (x != y) {
                    drawCirclePoints(center_x, center_y, y, x, value);
                }
            }
        }
    }

    /// Установить позицию курсора
    void setCursor(Position x, Position y) noexcept {
        cursor_x = x;
        cursor_y = y;
    }

    /// Рисует текст с использованием текущего шрифта.
    /// @param text C-style string
    /// @param on Цвет текста
    /// @details <code>'\\n'</code> для перехода на новую строку
    /// @details <code>'\\t'</code> для табуляции
    /// @details <code>'\\x80'</code> для нормального текста
    /// @details <code>'\\x81'</code> для инверсии текста
    /// @details <code>'\\x82'</code> для установки курсора по центру фрейма
    void text(rs::str text, bool on = true) noexcept {
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
                const auto new_x = static_cast<Position>(((cursor_x / tab_width) + 1) * tab_width);
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

            cursor_x = static_cast<Position>(cursor_x + current_font->glyph_width);

            if (cursor_x < width()) {
                drawLineVertical(
                    cursor_x,
                    cursor_y,
                    static_cast<Position>(cursor_y + current_font->glyph_height),
                    not on
                );
            }

            cursor_x = static_cast<Position>(cursor_x + 1);
        }
    }

private:

    /// Рассчитывает размеры областей для разделения
    template<rs::size N> std::array<Position, N> calculateSplitSizes(Position total_size, std::array<rs::u8, N> weights) {
        for (auto &w: weights) {
            if (w <= 0) { w = 1; }
        }

        Position total_weight = 0;
        for (auto w: weights) { total_weight += w; }

        std::array<Position, N> sizes;
        Position remaining = total_size;

        for (rs::size i = 0; i < N; ++i) {
            sizes[i] = (total_size * weights[i]) / total_weight;
            remaining -= sizes[i];
        }

        for (rs::size i = 0; remaining > 0; i = (i + 1) % N) {
            sizes[i] += 1;
            remaining -= 1;
        }

        return sizes;
    }

    /// Очистить сегмент строки от курсора
    void clearLineSegment(Position x, bool on) noexcept {
        rect(
            cursor_x,
            cursor_y,
            x,
            static_cast<Position>(cursor_y + current_font->glyph_height),
            on ? Mode::Clear : Mode::Fill
        );
    }

    /// Перенести курсор на следующую строку
    void nextLine() noexcept {
        cursor_x = 0;
        cursor_y = static_cast<Position>(cursor_y + current_font->heightTotal());
    }

    /// Получить значение режима
    static inline bool getModeValue(Mode mode) noexcept {
        return static_cast<rs::u8>(mode) & 0b10;
    }

    /// Режим является заполняющим
    static inline bool isFillMode(Mode mode) noexcept {
        return static_cast<rs::u8>(mode) & 0b01;
    }

    /// Рисует горизонтальную линию
    void drawLineHorizontal(Position x0, Position y, Position x1, bool on) const noexcept {
        if (x0 > x1) { std::swap(x0, x1); }
        for (Position x = x0; x <= x1; x++) {
            frame.setPixel(x, y, on);
        }
    }

    /// Рисует вертикальную линию
    void drawLineVertical(Position x, Position y0, Position y1, bool on) const noexcept {
        if (y0 > y1) { std::swap(y0, y1); }
        for (Position y = y0; y <= y1; y++) {
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

    /// Рисует глиф
    void drawGlyph(Position x, Position y, const rs::u8 *glyph, bool on) noexcept {
        if (glyph == nullptr) {
            rect(
                x,
                y,
                static_cast<Position>(x + current_font->glyph_width - 1),
                static_cast<Position>(y + current_font->glyph_height - 1),
                on ? Mode::FillBorder : Mode::ClearBorder
            );
            return;
        }

        for (rs::u8 col_index = 0; col_index < current_font->glyph_width; ++col_index) {
            const auto pixel_x = static_cast<Position>(x + col_index);

            for (rs::u8 bit_index = 0; bit_index <= current_font->glyph_height; ++bit_index) {
                const bool lit = glyph[col_index] & (1 << bit_index);
                frame.setPixel(pixel_x, static_cast<Position>(y + bit_index), lit == on);
            }
        }
    }
};
}

