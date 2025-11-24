#pragma once

#include <kf/units.hpp>


namespace kf::gfx {

/// @brief БитМап изображение
template<Pixel W, Pixel H> struct BitMap final {

    /// @brief Ширина
    [[nodiscard]] inline constexpr Pixel width() const { return W; }

    /// @brief Высота
    [[nodiscard]] inline constexpr Pixel height() const { return H; }

    /// @brief Количество страниц
    static constexpr auto pages = (H + 7) / 8;

    /// @brief Буфер
    const u8 buffer[W * pages];

    BitMap() = delete;
};

}// namespace kf::gfx
