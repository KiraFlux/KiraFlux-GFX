#pragma once

#include <rs/Result.hpp>
#include <rs/aliases.hpp>

#include <algorithm>

#include <kf/gfx/Position.hpp>
#include <kf/gfx/BitMap.hpp>
#include "Position.hpp"


namespace kf::gfx {

/// Представление прямоугольной области дисплея
struct FrameView final {

public:

    /// Возможные ошибки при создании FrameView
    enum class Error : rs::u8 {
        /// Размер области < 1
        SizeTooSmall,
        /// Дочерняя область > родительской
        SizeTooLarge,
        /// Смещение выходит за границы
        OffsetOutOfBounds
    };

private:

    /// Указатель на буфер дисплея
    rs::u8 *buffer; // todo Проверка на null

    /// Шаг строки (ширина всего дисплея)
    Position stride;

public:

    /// Абсолютное смещение по X
    Position offset_x;
    /// Абсолютное смещение по Y
    Position offset_y;
    /// Ширина области
    Position width;
    /// Высота области
    Position height;

    /// Создает FrameView с проверкой ошибок
    [[nodiscard]] static rs::Result<FrameView, Error> create(
        /// Буфер дисплея
        rs::u8 *buffer,
        /// Шаг строки (ширина дисплея)
        Position stride,
        /// Ширина области
        Position width,
        /// Высота области
        Position height,
        /// Смещение по X
        Position offset_x,
        /// Смещение по Y
        Position offset_y
    ) noexcept {
        if (width < 1 or height < 1) {
            return Error::SizeTooSmall;
        }
        return FrameView(buffer, stride, width, height, offset_x, offset_y);
    }

    FrameView() noexcept = default;

    /// Создать FrameView без проверок
    /// @warning unsafe
    explicit FrameView(
        /// Буфер дисплея
        /// Должен быть валидным и иметь достаточный размер
        rs::u8 *buffer,

        /// Шаг строки (ширина дисплея)
        /// > 1
        Position stride,

        /// Ширина области
        /// > 1
        Position width,

        /// Высота области
        /// > 1
        Position height,

        /// Смещение X
        Position offset_x,

        /// Смещение Y
        Position offset_y
    ) noexcept:
        buffer{buffer},
        stride{stride},
        offset_x{offset_x},
        offset_y{offset_y},
        width{width},
        height{height} {}

    /// Создает дочернюю область
    [[nodiscard]] rs::Result<FrameView, Error> sub(
        /// Ширина дочерней области
        Position sub_width,

        /// Высота дочерней области
        Position sub_height,

        /// Смещение по X относительно родителя
        Position sub_offset_x,

        /// Смещение по Y относительно родителя
        Position sub_offset_y
    ) const noexcept {
        // Проверка выхода за границы родителя
        if (sub_offset_x >= width or sub_offset_y >= height) {
            return Error::OffsetOutOfBounds;
        }

        if (sub_width > width - sub_offset_x or sub_height > height - sub_offset_y) {
            return Error::SizeTooLarge;
        }

        const auto new_x = static_cast<Position>(offset_x + sub_offset_x);
        const auto new_y = static_cast<Position>(offset_y + sub_offset_y);

        return create(buffer, stride, sub_width, sub_height, new_x, new_y);
    }

    /// Создает дочернюю область без проверок
    /// @warning unsafe
    FrameView subUnchecked(
        /// Ширина дочерней области.
        /// sub_width не более parent.width
        Position sub_width,

        /// Высота дочерней области.
        /// sub_height не более parent.height
        Position sub_height,

        /// Смещение по X относительно родителя.
        /// 0 .. (parent.width - sub_width)
        Position sub_offset_x,

        /// Смещение по Y относительно родителя.
        /// 0 .. (parent.height - sub_height)
        Position sub_offset_y
    ) {
        return FrameView{
            buffer,
            stride,
            sub_width,
            sub_height,
            static_cast<Position>(offset_x + sub_offset_x),
            static_cast<Position>(offset_y + sub_offset_y)
        };
    }

    /// Устанавливает состояние пикселя
    inline void setPixel(Position x, Position y, bool on) const noexcept {
        if (x < 0 or x >= width or y < 0 or y >= height) { return; }
        writePixel(x, y, on);
    }

    /// Возвращает состояние пикселя
    [[nodiscard]] inline bool getPixel(Position x, Position y) const noexcept {
        if (x < 0 or x >= width or y < 0 or y >= height) { return false; }
        return buffer[getByteIndex(x, y)] & getBitMask(y);
    }

    /// Заливает область указанным значением
    void fill(bool value) const noexcept {
        const auto start_page = static_cast<Position>((offset_y) >> 3);
        const auto end_page = static_cast<Position>((offset_y + height + 6) >> 3);

        for (Position page = start_page; page <= end_page; ++page) {
            const rs::u8 mask = calculatePageMask(page);
            if (mask == 0) { continue; }

            for (Position x = 0; x < width; ++x) {
                const Position abs_x = toAbsoluteX(x);
                if (abs_x < 0 or abs_x >= stride) { continue; }
                writeData(abs_x, page, mask, value);
            }
        }
    }

    /// Рисует битмап в указанной позиции
    template<Position W, Position H> void drawBitmap(Position x, Position y, const BitMap<W, H> &bitmap, bool on = true) noexcept {
        for (Position page_idx = 0; page_idx < BitMap<W, H>::pages_count; ++page_idx) {
            const auto page_y = static_cast<Position>(toAbsoluteY(y) + (page_idx << 3));

            // Пропуск невидимых страниц
            if (page_y + 7 < offset_y or page_y >= offset_y + height) { continue; }

            const rs::u8 mask = calculateBitmapMask(page_y);
            if (mask == 0) { continue; }

            drawBitmapRow(bitmap, page_idx, x, page_y, mask, on);
        }
    }


    /// Преобразует X в абсолютную координату
    [[nodiscard]] inline Position toAbsoluteX(Position x) const noexcept {
        return static_cast<Position>(offset_x + x);
    }

    /// Преобразует Y в абсолютную координату
    [[nodiscard]] inline Position toAbsoluteY(Position y) const noexcept {
        return static_cast<Position>(offset_y + y);
    }

    /// Возвращает номер страницы для Y
    [[nodiscard]] inline Position getPage(Position y) const noexcept {
        return static_cast<Position>(toAbsoluteY(y) >> 3);
    }

    /// Возвращает битовую маску для Y
    [[nodiscard]] inline rs::u8 getBitMask(Position y) const noexcept {
        return static_cast<rs::u8>(1) << (toAbsoluteY(y) & 0x07);
    }

    /// Возвращает индекс байта в буфере
    [[nodiscard]] inline rs::size getByteIndex(Position x, Position y) const noexcept {
        return getPage(y) * stride + toAbsoluteX(x);
    }

    /// Вычисляет маску видимости для страницы
    [[nodiscard]] inline rs::u8 calculatePageMask(Position page) const noexcept {
        const auto page_top = static_cast<Position>(page << 3);
        const auto page_bottom = static_cast<Position>(page_top + 7);

        const Position visible_top = std::max(offset_y, page_top);
        const auto visible_bottom = static_cast<Position>(std::min(offset_y + height, page_bottom + 1));

        if (visible_top >= visible_bottom) { return 0; }

        return createPageMask(
            static_cast<rs::u8>(visible_top - page_top),
            static_cast<rs::u8>(visible_bottom - page_top - 1)
        );
    }

    /// Вычисляет маску видимости для битмапа
    [[nodiscard]] inline rs::u8 calculateBitmapMask(Position page_y) const noexcept {
        rs::u8 clip_top = 0;
        if (page_y < offset_y) {
            clip_top = static_cast<rs::u8>(offset_y - page_y);
        }

        rs::u8 clip_bottom = 7;
        if (page_y + 7 >= offset_y + height) {
            clip_bottom = static_cast<rs::u8>(offset_y + height - page_y - 1);
        }

        return createPageMask(clip_top, clip_bottom);
    }

    /// Рисует строку битмапа
    template<Position W, Position H> inline void drawBitmapRow(
        const BitMap<W, H> &bitmap,
        Position page_idx,
        Position x,
        Position page_y,
        rs::u8 mask,
        bool on
    ) noexcept {
        const rs::u8 *source = bitmap.buffer + page_idx * W;
        const auto abs_x = static_cast<Position>(offset_x + x);

        for (Position bx = 0; bx < W; ++bx) {
            const auto target_x = static_cast<Position>(abs_x + bx);
            if (target_x < offset_x or target_x >= offset_x + width) { continue; }

            const rs::u8 data = source[bx] & mask;
            if (data == 0) { continue; }

            writeBitmapData(target_x, page_y, data, on);
        }
    }

    /// Записывает данные битмапа с учетом смещения
    void writeBitmapData(Position abs_x, Position page_y, rs::u8 data, bool on) const noexcept {
        const auto page = static_cast<Position>(page_y >> 3);
        const auto offset = static_cast<rs::u8>(page_y & 0x07);

        if (offset == 0) {
            writeData(abs_x, page, data, on);
        } else {
            // Верхняя часть (текущая страница)
            writeData(abs_x, page, data << offset, on);

            // Нижняя часть (следующая страница)
            const auto next_page = static_cast<Position>(page + 1);
            writeData(abs_x, next_page, data >> (8 - offset), on);
        }
    }

    /// Записывает пиксель
    inline void writePixel(Position x, Position y, bool on) const noexcept {
        const Position abs_x = toAbsoluteX(x);
        const Position page = getPage(y);
        const rs::u8 mask = getBitMask(y);
        writeData(abs_x, page, mask, on);
    }

    /// Записывает данные в буфер
    inline void writeData(Position abs_x, Position page, rs::u8 data, bool on) const noexcept {
        const rs::size index = page * stride + abs_x;
        if (on) {
            buffer[index] |= data;
        } else {
            buffer[index] &= ~data;
        }
    }

    /// Создает битовую маску для диапазона битов
    static inline rs::u8 createPageMask(rs::u8 start_bit, rs::u8 end_bit) noexcept {
        if (start_bit > end_bit) { return 0; }
        const rs::u8 mask = ((1 << (end_bit + 1)) - 1) ^ ((1 << start_bit) - 1);
        return mask;
    }
};
}
