#pragma once

#include "FrameView.hpp"


namespace kfgfx {

/// Графика
struct Graphics {

public:

    /// Режим отрисовки
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

private:

    static inline bool isFillMode(Mode mode) {
        constexpr rs::u8 fill_bit = 0b01;
        return static_cast<rs::u8>(mode) & fill_bit;
    }

    static inline bool modeValue(Mode mode) {
        constexpr rs::u8 value_bit = 0b10;
        return static_cast<rs::u8>(mode) & value_bit;
    }

    /// Кадр для записи
    FrameView &frame;

public:

    explicit Graphics(FrameView &frame) :
        frame(frame) {}

    /// Нарисовать точку
    /// @returns true - success
    inline bool dot(Position x, Position y, bool on = true) {
        return frame.setPixel(x, y, on);
    }

    /// Нарисовать линию (алгоритм Брезенхема)
    void line(
        Position x0, Position y0,
        Position x1, Position y1,
        bool on = true
    ) {
        const bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);

        if (steep) {
            std::swap(x0, y0);
            std::swap(x1, y1);
        }

        if (x0 > x1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }

        const auto dx = x1 - x0;
        const auto dy = std::abs(y1 - y0);
        auto error = dx / 2;
        const auto y_step = Position((y0 < y1) ? 1 : -1);
        Position y = y0;

        for (Position x = x0; x <= x1; x++) {
            if (steep) {
                dot(y, x, on);
            } else {
                dot(x, y, on);
            }

            error -= dy;
            if (error < 0) {
                y += y_step;
                error += dx;
            }
        }
    }

    /// Нарисовать прямоугольник
    void rect(
        Position x0, Position y0,
        Position x1, Position y1,
        Mode mode
    ) {
        if (x0 > x1) { std::swap(x0, x1); }
        if (y0 > y1) { std::swap(y0, y1); }

        const bool fill = isFillMode(mode);
        const bool value = modeValue(mode);

        if (fill) {
            const Position width = x1 - x0 + 1;
            const Position height = y1 - y0 + 1;

            auto rect_view = frame.sub(width, height, x0, y0);

            if (rect_view.ok()) {
                rect_view.value.fill(value);
            }
        } else {
            line(x0, y0, x1, y0, value);
            line(x0, y1, x1, y1, value);
            line(x0, y0, x0, y1, value);
            line(x1, y0, x1, y1, value);
        }
    }

    /// Нарисовать окружность
    void circle(
        Position center_x, Position center_y,
        Position radius,
        Mode mode
    ) {
        const bool fill = isFillMode(mode);
        const bool value = modeValue(mode);

        // Алгоритм средней точки для окружности
        Position x = radius;
        Position y = 0;
        Position err = 0;

        while (x >= y) {
            if (fill) {
                line(center_x - x, center_y + y, center_x + x, center_y + y, value);
                line(center_x - x, center_y - y, center_x + x, center_y - y, value);
                line(center_x - y, center_y + x, center_x + y, center_y + x, value);
                line(center_x - y, center_y - x, center_x + y, center_y - x, value);
            } else {
                dot(center_x + x, center_y + y, value);
                dot(center_x + y, center_y + x, value);
                dot(center_x - y, center_y + x, value);
                dot(center_x - x, center_y + y, value);
                dot(center_x - x, center_y - y, value);
                dot(center_x - y, center_y - x, value);
                dot(center_x + y, center_y - x, value);
                dot(center_x + x, center_y - y, value);
            }

            y++;
            err += 2 * y + 1;

            if (2 * (err - x) + 1 > 0) {
                x--;
                err -= 2 * x + 1;
            }
        }
    }
};

}