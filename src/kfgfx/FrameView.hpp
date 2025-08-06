#pragma once

#include "Position.hpp"

#include <algorithm>

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
        if (size.x < 1 or size.y < 1) {
            return {Error::SizeTooSmall};
        }

        return {FrameView(buffer, stride, size, offset)};
    }

    /// Создать дочерние представление кадра
    rs::Result<FrameView, Error> sub(Position sub_size, Position sub_offset) const {
        // Проверяем что смещение не выходит за границы
        if (sub_offset.x >= size.x || sub_offset.y >= size.y) {
            return {Error::OffsetOutOfBounds};
        }

        // Проверяем что под-область помещается в родительскую
        if (sub_size.x > size.x - sub_offset.x || sub_size.y > size.y - sub_offset.y) {
            return {Error::SizeTooLarge};
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

    bool getPixel(Position::Value x, Position::Value y) const {
        if (isOutOfBounds(x, y)) {
            return false;
        }

        return buffer[getByteIndex(x, y)] & getByteBitMask(y);
    }

    /// Эффективно заполнить область представления указанным значением
    void fill(bool value) {
        const int top = offset.y;
        const auto bottom = offset.y + size.y - 1;

        const auto start_page = top >> 3;
        const auto end_page = bottom >> 3;

        for (auto page = start_page; page <= end_page; page++) {
            const auto page_top = page << 3;
            const auto page_bottom = page_top + 7;

            const auto y_start = std::max(top, page_top);
            const auto y_end = std::min(bottom, page_bottom);

            if (y_start > y_end) { continue; }

            const rs::u8 mask = createPageMask(y_start - page_top, y_end - page_top);

            rs::u8 *page_buffer = buffer + page * stride + offset.x;

            for (auto i = 0; i < size.x; i++) {
                if (value) {
                    page_buffer[i] |= mask;
                } else {
                    page_buffer[i] &= ~mask;
                }
            }
        }
    }

private:

    /// Создать битовую маску для диапазона [start_bit, end_bit]
    static rs::u8 createPageMask(rs::u8 start_bit, rs::u8 end_bit) {
        return ((1 << (end_bit + 1)) - 1) ^ ((1 << start_bit) - 1);
    }

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