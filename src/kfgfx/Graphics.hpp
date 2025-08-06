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
};

}