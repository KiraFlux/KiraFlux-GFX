#pragma once

#include "FrameView.hpp"


namespace kfgfx {

/// Графика
struct Graphics {

public:

    /// Режим отрисовки
    enum class Mode {

        /// Заполнить
        Fill,

        /// Очистить
        Clear,

        /// Заполнить контур
        FillContour,

        /// Очистить контур
        ClearContour,
    };

private:

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

        switch (mode) {
            case Mode::Fill:
            case Mode::Clear: {
                const Position width = x1 - x0 + 1;
                const Position height = y1 - y0 + 1;

                auto rect_view = frame.sub(width, height, x0, y0);

                if (rect_view.ok()) {
                    rect_view.value.fill(mode == Mode::Fill);
                }

                return;
            }
            case Mode::FillContour:
            case Mode::ClearContour: {
                const bool value = mode == Mode::ClearContour;

                line(x0, y0, x1, y0, value);
                line(x0, y1, x1, y1, value);
                line(x0, y0, x0, y1, value);
                line(x1, y0, x1, y1, value);

                return;
            }

        }
    }

    /// Нарисовать окружность
    void circle(
        Position center_x, Position center_y,
        Position radius,
        Mode mode
    ) {
        const bool fill = (mode == Mode::Fill || mode == Mode::Clear);
        const bool value = (mode == Mode::Fill || mode == Mode::FillContour);

        // Алгоритм средней точки для окружности
        Position x = radius;
        Position y = 0;
        Position err = 0;

        const auto draw_points = [&](Position px, Position py) {
            dot(center_x + px, center_y + py, value);
            dot(center_x + py, center_y + px, value);
            dot(center_x - py, center_y + px, value);
            dot(center_x - px, center_y + py, value);
            dot(center_x - px, center_y - py, value);
            dot(center_x - py, center_y - px, value);
            dot(center_x + py, center_y - px, value);
            dot(center_x + px, center_y - py, value);
        };

        const auto draw_lines = [&](Position px, Position py) {
            // Верхняя и нижняя линии
            line(center_x - px, center_y + py, center_x + px, center_y + py, value);
            line(center_x - px, center_y - py, center_x + px, center_y - py, value);

            // Левая и правая линии (только для fill)
            if (fill) {
                line(center_x - py, center_y + px, center_x + py, center_y + px, value);
                line(center_x - py, center_y - px, center_x + py, center_y - px, value);
            }
        };

        while (x >= y) {
            if (fill) {
                draw_lines(x, y);
            } else {
                draw_points(x, y);
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