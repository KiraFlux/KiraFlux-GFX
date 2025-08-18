#pragma once

#include <rs/primitives.hpp>
#include "Position.hpp"


namespace kf {

/// Представляет моноширинный шрифт с высотой глифов до 8 пикселей
struct Font final {

    /// Код первого символа в шрифте
    static constexpr char start_char = 32;
    /// Код последнего символа в шрифте
    static constexpr char end_char = 127;

    /// Данные шрифта (массив глифов)
    const rs::u8 *data;
    /// Ширина одного глифа в пикселях
    const rs::u8 glyph_width;
    /// Высота глифа в пикселях (1-8)
    const rs::u8 glyph_height;

    /// Получить экземпляр пустого шрифта
    static const Font &blank() {
        static Font instance{
            nullptr,
            3,
            5,
        };
        return instance;
    }

    /// Получить указатель на данные глифа для символа
    /// @returns nullptr если символ вне диапазона
    const rs::u8 *getGlyph(char c) const noexcept {
        if (data == nullptr or c < start_char or c > end_char) { return nullptr; }
        return data + (static_cast<rs::size>(c - start_char) * glyph_width);
    }
};

/// Системные шрифты
namespace fonts {

/// GyverOLED EN Font
extern const Font gyver_5x7_en;

} // namespace fonts

} // namespace kfgfx