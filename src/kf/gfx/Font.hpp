#pragma once

#include <kf/units.hpp>

namespace kf::gfx {

/// @brief Представляет моноширинный шрифт с высотой глифов до 8 пикселей
struct Font final {

    /// @brief Код первого символа в шрифте
    static constexpr char start_char = 32;

    /// @brief Код последнего символа в шрифте
    static constexpr char end_char = 127;

    /// @brief Данные шрифта (массив глифов)
    const u8 *data;

    /// @brief Ширина одного глифа в пикселях
    const u8 glyph_width;

    /// @brief Высота глифа в пикселях (1-8)
    const u8 glyph_height;

    /// @brief Получить экземпляр пустого шрифта
    static const Font &blank() {
        static Font instance{
            nullptr,
            3,
            5,
        };
        return instance;
    }

    /// @brief Полная ширина глифа
    [[nodiscard]] inline u8 widthTotal() const noexcept { return glyph_width + 1; }

    /// @brief Полная высота глифа
    [[nodiscard]] inline u8 heightTotal() const noexcept { return glyph_height + 1; }

    /// @brief Получить указатель на данные глифа для символа
    /// @returns nullptr если символ вне диапазона
    [[nodiscard]] const u8 *getGlyph(char c) const noexcept {
        if (nullptr == data or c < start_char or c > end_char) {
            return nullptr;
        }

        return data + (static_cast<usize>(c - start_char) * glyph_width);
    }
};

/// @brief Системные шрифты
namespace fonts {

/// @brief GyverOLED EN Font
extern const Font gyver_5x7_en;

}// namespace fonts
}// namespace kf::gfx

// namespace fonts

// namespace kf::gfx