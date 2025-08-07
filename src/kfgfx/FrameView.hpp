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

    template<rs::size bm_width, rs::size bm_height>
    void drawBitmap(Position x, Position y, const BitMap<bm_width, bm_height>& bitmap, bool on = true) {
        constexpr auto bm_pages = BitMap<bm_width, bm_height>::pages_count;

        // Абсолютные координаты в буфере дисплея
        Position abs_x = offset_x + x;
        Position abs_y = offset_y + y;

        for (Position page_idx = 0; page_idx < bm_pages; ++page_idx) {
            const rs::u8* src = bitmap.buffer + page_idx * bm_width;

            // Вычисляем целевую страницу в буфере
            Position target_page = (abs_y + page_idx * 8) / 8;
            Position bit_offset = (abs_y + page_idx * 8) % 8;

            // Проверка границ
            if (target_page < 0 || target_page >= (height + 7) / 8) continue;

            for (Position col = 0; col < bm_width; ++col) {
                Position target_x = abs_x + col;
                if (target_x < offset_x || target_x >= offset_x + width) continue;

                rs::u8 value = src[col];
                rs::u8* target = buffer + target_page * stride + target_x;

                // Обработка с учетом смещения
                if (bit_offset == 0) {
                    if (on) *target |= value;
                    else *target &= ~value;
                } else {
                    // Младшие биты (текущая страница)
                    if (target_page >= 0 && target_page < (height + 7) / 8) {
                        rs::u8 lower = value << bit_offset;
                        if (on) *target |= lower;
                        else *target &= ~lower;
                    }

                    // Старшие биты (следующая страница)
                    if (target_page + 1 < (height + 7) / 8) {
                        rs::u8 upper = value >> (8 - bit_offset);
                        target = buffer + (target_page + 1) * stride + target_x;
                        if (on) *target |= upper;
                        else *target &= ~upper;
                    }
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
    int getPage(Position y) const {
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