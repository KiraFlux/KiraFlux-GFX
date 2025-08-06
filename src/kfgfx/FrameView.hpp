#pragma once

#include "Position.hpp"

#include <cstring>

#include "rs/Result.hpp"
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
    static rs::Result<FrameView, Error> create(rs::u8 *buffer, Position::Value stride, Position size, Position offset) {
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
        if (isOutOfBounds(x, y)) {
            return false;
        }

        if (on) {
            buffer[getByteIndex(x, y)] |= getByteBitMask(y);
        } else {
            buffer[getByteIndex(x, y)] &= ~getByteBitMask(y);
        }

        return true;
    }

    bool setPixel(Position &&pos, bool on) {
        return setPixel(pos.x, pos.y, on);
    }

    bool getPixel(Position::Value x, Position::Value y) const {
        if (isOutOfBounds(x, y)) {
            return false;
        }

        return buffer[getByteIndex(x, y)] & getByteBitMask(y);
    }

    void fill(bool value) {
        for (short x = 0; x < size.x; x++) {
            for (short y = 0; y < size.y; y++) {
                setPixel(x, y, value);
            }
        }
    }

private:

    /// Абсолютная позиция по X
    inline Position::Value absoluteX(Position::Value x) const {
        return static_cast<Position::Value>(offset.x + x);
    }

    /// Абсолютная позиция по Y
    inline Position::Value absoluteY(Position::Value y) const {
        return static_cast<Position::Value>(offset.y + y);
    }

    /// Маска бита
    rs::u8 getByteBitMask(Position::Value y) const {
        return 1 << (absoluteY(y) & 0b0111);
    }

    /// Номер страницы
    int getPage(Position::Value y) const {
        return absoluteY(y) >> 3;
    }

    /// Индекс байта в буфере
    rs::size getByteIndex(Position::Value x, Position::Value y) const {
        return getPage(y) * stride + absoluteX(x);
    }

    /// Пиксель за границей
    bool isOutOfBounds(Position::Value x, Position::Value y) const {
        return x < 0 or y < 0 or x >= size.x or y >= size.y;
    }

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