#pragma once

#include <rs/primitives.hpp>
#include "Position.hpp"

namespace kfgfx {

/// Представляет моноширинный шрифт с высотой глифов до 8 пикселей
struct Font final {

    /// Указатель на данные шрифта (массив глифов)
    const rs::u8* data;
    /// Ширина одного глифа в пикселях
    rs::u8 glyph_width;
    /// Высота глифа в пикселях (1-8)
    rs::u8 glyph_height;
    /// Код первого символа в шрифте
    char start_char;
    /// Код последнего символа в шрифте
    char end_char;

    /// Конструктор шрифта
    constexpr Font(
        const rs::u8* data,
        rs::u8 glyph_width,
        rs::u8 glyph_height,
        char start_char,
        char end_char
    ) noexcept:
        data(data),
        glyph_width(glyph_width),
        glyph_height(glyph_height),
        start_char(start_char),
        end_char(end_char) {}

    /// Возвращает указатель на данные глифа для символа
    /// @returns nullptr если символ вне диапазона
    const rs::u8* get_glyph(char ch) const noexcept {
        if (ch < start_char or ch > end_char) return nullptr;
        return data + (static_cast<rs::size>(ch - start_char) * glyph_width);
    }
};

} // namespace kfgfx