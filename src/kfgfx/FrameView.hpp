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
    const Position stride;

public:

    /// Размер окна
    const Position width, height;
    /// Смещение системы координат
    const Position offset_x, offset_y;

    /// Создать представление кадра
    static rs::Result<FrameView, Error> create(
        rs::u8 *buffer,
        Position stride,
        Position width,
        Position height,
        Position offset_x,
        Position offset_y
    ) {
        if (width < 1 or height < 1) {
            return {Error::SizeTooSmall};
        }

        return {FrameView(buffer, stride, width, height, offset_x, offset_y)};
    }

    /// Создать дочерние представление кадра
    rs::Result<FrameView, Error> sub(
        Position sub_width,
        Position sub_height,
        Position sub_offset_x,
        Position sub_offset_y
    ) const {
        if (sub_offset_x >= width or sub_offset_y >= height) {
            return {Error::OffsetOutOfBounds};
        }

        if (sub_width > width - sub_offset_x or sub_height > height - sub_offset_y) {
            return {Error::SizeTooLarge};
        }

        return FrameView::create(
            buffer,
            stride,
            sub_width,
            sub_height,
            static_cast<Position>(offset_x + sub_offset_x),
            static_cast<Position>(offset_y + sub_offset_y)
        );
    }

    /// Установить состояние пикселя
    /// @returns false on fail
    bool setPixel(Position x, Position y, bool on) {
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

    bool getPixel(Position x, Position y) const {
        if (isOutOfBounds(x, y)) {
            return false;
        }

        return buffer[getByteIndex(x, y)] & getByteBitMask(y);
    }

    /// Эффективно заполнить область представления указанным значением
    void fill(bool value) {
        const int top = offset_y;
        const auto bottom = offset_y + height - 1;

        const auto start_page = top >> 3;
        const auto end_page = bottom >> 3;

        for (auto page = start_page; page <= end_page; page++) {
            const auto page_top = page << 3;
            const auto page_bottom = page_top + 7;

            const auto y_start = std::max(top, page_top);
            const auto y_end = std::min(bottom, page_bottom);

            if (y_start > y_end) { continue; }

            const rs::u8 mask = createPageMask(y_start - page_top, y_end - page_top);

            rs::u8 *page_buffer = buffer + page * stride + offset_x;

            for (auto i = 0; i < width; i++) {
                if (value) {
                    page_buffer[i] |= mask;
                } else {
                    page_buffer[i] &= ~mask;
                }
            }
        }
    }

    template<Position W, Position H> void drawBitmap(Position x, Position y, const BitMap<W, H> &bitmap, bool on = true) {
        for (Position bitmap_page = 0; bitmap_page < BitMap<W, H>::pages_count; ++bitmap_page) {
            const rs::u8 *source = bitmap.buffer + bitmap_page * W;

            auto target_page = Position((absoluteY(y) + (bitmap_page << 3)) >> 3);
            auto bit_offset = (absoluteY(y) + (bitmap_page << 3)) & 0b0111;

            if (target_page < 0 or target_page >= (height + 7) / 8) { continue; }

            for (Position bitmap_x = 0; bitmap_x < W; ++bitmap_x) {
                Position target_x = absoluteX(x) + bitmap_x;

                if (target_x < offset_x or target_x >= offset_x + width) { continue; }

                rs::u8 value = source[bitmap_x];

                if (bit_offset == 0) {
                    write(target_x, target_page, value, on);
                } else {
                    if (target_page < (height + 7) / 8) {
                        write(target_x, target_page, value << bit_offset, on);
                    }

                    if (target_page < ((height + 7) / 8) - 1) {
                        write(target_x, target_page + 1, value >> (8 - bit_offset), on);
                    }
                }
            }
        }
    }

private:

    /// Записать пиксели на странице
    void write(Position x, Position page, rs::u8 data, bool on) const {
        const auto index = page * stride + x;
        if (on) { buffer[index] |= data; }
        else { buffer[index] &= ~data; }
    }

    /// Создать битовую маску для диапазона [start_bit, end_bit]
    static rs::u8 createPageMask(rs::u8 start_bit, rs::u8 end_bit) {
        return ((1 << (end_bit + 1)) - 1) ^ ((1 << start_bit) - 1);
    }

    /// Абсолютная позиция по X
    inline Position absoluteX(Position x) const {
        return static_cast<Position>(offset_x + x);
    }

    /// Абсолютная позиция по Y
    inline Position absoluteY(Position y) const {
        return static_cast<Position>(offset_y + y);
    }

    /// Маска бита
    rs::u8 getByteBitMask(Position y) const {
        return 1 << (absoluteY(y) & 0b0111);
    }

    /// Номер страницы
    inline int getPage(Position y) const {
        return absoluteY(y) >> 3;
    }

    /// Индекс байта в буфере
    rs::size getByteIndex(Position x, Position y) const {
        return getPage(y) * stride + absoluteX(x);
    }

    /// Пиксель за границей
    bool isOutOfBounds(Position x, Position y) const {
        return x < 0 or y < 0 or x >= width or y >= height;
    }

    explicit FrameView(
        rs::u8 *buffer,
        Position stride,
        Position width,
        Position height,
        Position offset_x,
        Position offset_y
    ) :
        buffer{buffer},
        stride{stride},
        width{width},
        height{height},
        offset_x{offset_x},
        offset_y{offset_y} {}
};

} // namespace kfgfx