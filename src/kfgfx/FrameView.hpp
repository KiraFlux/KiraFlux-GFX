#pragma once

#include "rs/Result.hpp"
#include "Position.hpp"
#include "rs/primitives.hpp"

/// KiraFlux GFX
namespace kfgfx {

/// Представление кадра
struct FrameView {

public:

    /// Ошибка операции
    enum class Error {

        /// Размер окна слишком мал
        SizeTooSmall,

        /// Размер окна слишком велик
        SizeTooLarge,

        /// Смещение выходит за пределы
        OffsetOutOfBounds,

    };

private:

    /// Буфер дисплея (организован по страницам)
    rs::u8 *buffer;
    /// Шаг строки в байтах (ширина всего дисплея)
    const Position::Value stride;

public:

    /// Размер окна
    const Position size;
    /// Смещение системы координат
    const Position offset;

    /// Создать представление кадра
    static rs::Result<FrameView, Error> create(
        rs::u8 *buffer,
        Position::Value stride,
        Position size,
        Position offset
    ) {
        if (size.x < 1 || size.y < 1) {
            return {Error::SizeTooSmall};
        }

        return {FrameView(buffer, stride, size, offset)};
    }

    /// Создать дочерние представление кадра
    rs::Result<FrameView, Error> sub(Position sub_size, Position sub_offset) const {
        if (sub_size.x > size.x || sub_size.y > size.y) {
            return {Error::SizeTooLarge};
        }

        if (sub_offset.x > size.x || sub_offset.y > size.y ||
            sub_offset.x + sub_size.x > size.x ||
            sub_offset.y + sub_size.y > size.y) {
            return {Error::OffsetOutOfBounds};
        }

        return FrameView::create(
            buffer,
            stride,
            sub_size,
            {
                static_cast<Position::Value>(offset.x + sub_offset.x),
                static_cast<Position::Value>(offset.y + sub_offset.y)
            }
        );
    }

    /// Установить состояние пикселя
    /// @returns false on fail
    bool setPixel(Position::Value x, Position::Value y, bool on) {
        if (x < 0 or y < 0 or x >= size.x or y >= size.y) {
            return false;
        }

        // Абсолютные координаты в буфере
        const auto abs_x = offset.x + x;
        const auto abs_y = offset.y + y;

        // Расчет позиции в буфере (OLED организация)
        const auto page = abs_y >> 3;              // Страница (группа из 8 строк)
        const auto page_offset = abs_y & 0b0111;   // Смещение внутри страницы
        const auto bit_mask = 1 << page_offset;    // Маска бита

        // Индекс байта в буфере
        const auto byte_index = page * stride + abs_x;

        if (on) {
            buffer[byte_index] |= bit_mask;
        } else {
            buffer[byte_index] &= ~bit_mask;
        }

        return true;
    }

    bool setPixel(Position &&pos, bool on) {
        return setPixel(pos.x, pos.y, on);
    }

private:

    explicit FrameView(
        rs::u8 *buffer,
        Position::Value stride,
        Position size,
        Position offset
    ) :
        buffer{buffer},
        stride{stride},
        size{size},
        offset{offset} {}
};

} // namespace kfgfx