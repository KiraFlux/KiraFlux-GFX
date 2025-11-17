#pragma once

#include <algorithm>
#include <kf/Result.hpp>
#include <kf/units.hpp>

#include <kf/gfx/BitMap.hpp>

namespace kf::gfx {

/// @brief Представление прямоугольной области дисплея
struct FrameView final {

public:
    /// @brief Возможные ошибки при создании FrameView
    enum class Error : u8 {

        /// @brief Буфер не инициализирован
        BufferNotInit,

        /// @brief Размер области < 1
        SizeTooSmall,

        /// @brief Дочерняя область > родительской
        SizeTooLarge,

        /// @brief Смещение выходит за границы
        OffsetOutOfBounds,
    };

private:
    /// @brief Указатель на буфер дисплея
    u8 *buffer;

    /// @brief Шаг строки (ширина всего дисплея)
    Pixel stride;

public:
    /// @brief Абсолютное смещение по X
    Pixel offset_x;

    /// @brief Абсолютное смещение по Y
    Pixel offset_y;

    /// @brief Ширина области
    Pixel width;

    /// @brief Высота области
    Pixel height;

    /// @brief Создает FrameView с проверкой ошибок
    [[nodiscard]] static Result<FrameView, Error> create(
        /// @brief Буфер дисплея
        u8 *buffer,

        /// @brief Шаг строки (ширина дисплея)
        Pixel stride,

        /// @brief Ширина области
        Pixel width,

        /// @brief Высота области
        Pixel height,

        /// @brief Смещение по X
        Pixel offset_x,

        /// @brief Смещение по Y
        Pixel offset_y) noexcept {
        if (nullptr == buffer) {
            return Error::BufferNotInit;
        }

        if (width < 1 or height < 1) {
            return Error::SizeTooSmall;
        }

        return FrameView(buffer, stride, width, height, offset_x, offset_y);
    }

    FrameView() :
        buffer{nullptr}, stride{0}, offset_x{0}, offset_y{0}, width{0}, height{0} {};

    /// @brief Создать FrameView без проверок
    /// @warning unsafe
    explicit FrameView(
        /// @brief Буфер дисплея
        u8 *buffer,

        /// @brief Шаг строки (ширина дисплея)
        /// @details > 1
        Pixel stride,

        /// @brief Ширина области
        /// @details > 1
        Pixel width,

        /// @brief Высота области
        /// @details > 1
        Pixel height,

        /// @brief Смещение X
        Pixel offset_x,

        /// @brief Смещение Y
        Pixel offset_y) noexcept :
        buffer{buffer},
        stride{stride},
        offset_x{offset_x},
        offset_y{offset_y},
        width{width},
        height{height} {}

    /// @brief Создает дочернюю область
    [[nodiscard]] Result<FrameView, Error> sub(
        /// @brief Ширина дочерней области
        Pixel sub_width,

        /// @brief Высота дочерней области
        Pixel sub_height,

        /// @brief Смещение по X относительно родителя
        Pixel sub_offset_x,

        /// @brief Смещение по Y относительно родителя
        Pixel sub_offset_y) const noexcept {
        // Проверка выхода за границы родителя
        if (sub_offset_x >= width or sub_offset_y >= height) {
            return Error::OffsetOutOfBounds;
        }

        if (sub_width > width - sub_offset_x or sub_height > height - sub_offset_y) {
            return Error::SizeTooLarge;
        }

        const auto new_x = static_cast<Pixel>(offset_x + sub_offset_x);
        const auto new_y = static_cast<Pixel>(offset_y + sub_offset_y);

        return create(buffer, stride, sub_width, sub_height, new_x, new_y);
    }

    /// @brief Создает дочернюю область без проверок
    /// @warning unsafe
    FrameView subUnchecked(
        /// @brief Ширина дочерней области.
        /// @brief sub_width не более parent.width
        Pixel sub_width,

        /// @brief Высота дочерней области.
        /// @brief sub_height не более parent.height
        Pixel sub_height,

        /// @brief Смещение по X относительно родителя.
        /// @brief 0 .. (parent.width - sub_width)
        Pixel sub_offset_x,

        /// @brief Смещение по Y относительно родителя.
        /// @brief 0 .. (parent.height - sub_height)
        Pixel sub_offset_y) {
        return FrameView{
            buffer,
            stride,
            sub_width,
            sub_height,
            static_cast<Pixel>(offset_x + sub_offset_x),
            static_cast<Pixel>(offset_y + sub_offset_y)};
    }

    [[nodiscard]] bool isValid() const {
        return nullptr != buffer or stride > 0 or width > 0 or height > 0;
    }

    [[nodiscard]] bool inside(Pixel x, Pixel y) const {
        return x >= 0 and x < width and y >= 0 and y < height;
    }

    /// @brief Устанавливает состояние пикселя
    inline void setPixel(Pixel x, Pixel y, bool on) const noexcept {
        if (isValid() and inside(x, y)) {
            writePixel(x, y, on);
        }
    }

    /// @brief Возвращает состояние пикселя
    [[nodiscard]] inline bool getPixel(Pixel x, Pixel y) const noexcept {
        if (isValid() and inside(x, y)) {
            return buffer[getByteIndex(x, y)] & getBitMask(y);
        }
        return false;
    }

    /// @brief Заливает область указанным значением
    void fill(bool value) const noexcept {
        const auto start_page = static_cast<Pixel>((offset_y) >> 3);
        const auto end_page = static_cast<Pixel>((offset_y + height + 6) >> 3);

        for (Pixel page = start_page; page <= end_page; ++page) {
            const u8 mask = calculatePageMask(page);
            if (mask == 0) { continue; }

            for (Pixel x = 0; x < width; ++x) {
                const Pixel abs_x = toAbsoluteX(x);
                if (abs_x < 0 or abs_x >= stride) { continue; }
                writeData(abs_x, page, mask, value);
            }
        }
    }

    /// @brief Рисует битмап в указанной позиции
    template<Pixel W, Pixel H> void drawBitmap(Pixel x, Pixel y, const BitMap<W, H> &bitmap, bool on = true) noexcept {
        for (Pixel page_idx = 0; page_idx < BitMap<W, H>::pages; ++page_idx) {
            const auto page_y = static_cast<Pixel>(toAbsoluteY(y) + (page_idx << 3));

            // Пропуск невидимых страниц
            if (page_y + 7 < offset_y or page_y >= offset_y + height) { continue; }

            const u8 mask = calculateBitmapMask(page_y);
            if (mask == 0) { continue; }

            drawBitmapRow(bitmap, page_idx, x, page_y, mask, on);
        }
    }

    /// @brief Преобразует X в абсолютную координату
    [[nodiscard]] inline Pixel toAbsoluteX(Pixel x) const noexcept {
        return static_cast<Pixel>(offset_x + x);
    }

    /// @brief Преобразует Y в абсолютную координату
    [[nodiscard]] inline Pixel toAbsoluteY(Pixel y) const noexcept {
        return static_cast<Pixel>(offset_y + y);
    }

    /// @brief Возвращает номер страницы для Y
    [[nodiscard]] inline Pixel getPage(Pixel y) const noexcept {
        return static_cast<Pixel>(toAbsoluteY(y) >> 3);
    }

    /// @brief Возвращает битовую маску для Y
    [[nodiscard]] inline u8 getBitMask(Pixel y) const noexcept {
        return static_cast<u8>(1) << (toAbsoluteY(y) & 0x07);
    }

    /// @brief Возвращает индекс байта в буфере
    [[nodiscard]] inline usize getByteIndex(Pixel x, Pixel y) const noexcept {
        return getPage(y) * stride + toAbsoluteX(x);
    }

    /// @brief Вычисляет маску видимости для страницы
    [[nodiscard]] inline u8 calculatePageMask(Pixel page) const noexcept {
        const auto page_top = static_cast<Pixel>(page << 3);
        const auto page_bottom = static_cast<Pixel>(page_top + 7);

        const Pixel visible_top = std::max(offset_y, page_top);
        const auto visible_bottom = static_cast<Pixel>(std::min(offset_y + height, page_bottom + 1));

        if (visible_top >= visible_bottom) { return 0; }

        return createPageMask(
            static_cast<u8>(visible_top - page_top),
            static_cast<u8>(visible_bottom - page_top - 1));
    }

    /// @brief Вычисляет маску видимости для битмапа
    [[nodiscard]] inline u8 calculateBitmapMask(Pixel page_y) const noexcept {
        u8 clip_top = 0;
        if (page_y < offset_y) {
            clip_top = static_cast<u8>(offset_y - page_y);
        }

        u8 clip_bottom = 7;
        if (page_y + 7 >= offset_y + height) {
            clip_bottom = static_cast<u8>(offset_y + height - page_y - 1);
        }

        return createPageMask(clip_top, clip_bottom);
    }

    /// @brief Рисует строку битмапа
    template<Pixel W, Pixel H> inline void drawBitmapRow(
        const BitMap<W, H> &bitmap,
        Pixel page_idx,
        Pixel x,
        Pixel page_y,
        u8 mask,
        bool on) noexcept {
        const u8 *source = bitmap.buffer + page_idx * W;
        const auto abs_x = static_cast<Pixel>(offset_x + x);

        for (Pixel bx = 0; bx < W; ++bx) {
            const auto target_x = static_cast<Pixel>(abs_x + bx);
            if (target_x < offset_x or target_x >= offset_x + width) { continue; }

            const u8 data = source[bx] & mask;
            if (data == 0) { continue; }

            writeBitmapData(target_x, page_y, data, on);
        }
    }

    /// @brief Записывает данные битмапа с учетом смещения
    void writeBitmapData(Pixel abs_x, Pixel page_y, u8 data, bool on) const noexcept {
        const auto page = static_cast<Pixel>(page_y >> 3);
        const auto offset = static_cast<u8>(page_y & 0x07);

        if (offset == 0) {
            writeData(abs_x, page, data, on);
        } else {
            // Верхняя часть (текущая страница)
            writeData(abs_x, page, data << offset, on);

            // Нижняя часть (следующая страница)
            const auto next_page = static_cast<Pixel>(page + 1);
            writeData(abs_x, next_page, data >> (8 - offset), on);
        }
    }

    /// @brief Записывает пиксель
    inline void writePixel(Pixel x, Pixel y, bool on) const noexcept {
        const Pixel abs_x = toAbsoluteX(x);
        const Pixel page = getPage(y);
        const u8 mask = getBitMask(y);
        writeData(abs_x, page, mask, on);
    }

    /// @brief Записывает данные в буфер
    inline void writeData(Pixel abs_x, Pixel page, u8 data, bool on) const noexcept {
        const usize index = page * stride + abs_x;
        if (on) {
            buffer[index] |= data;
        } else {
            buffer[index] &= ~data;
        }
    }

    /// @brief Создает битовую маску для диапазона битов
    static inline u8 createPageMask(u8 start_bit, u8 end_bit) noexcept {
        if (start_bit > end_bit) { return 0; }
        const u8 mask = ((1 << (end_bit + 1)) - 1) ^ ((1 << start_bit) - 1);
        return mask;
    }
};
}// namespace kf::gfx
