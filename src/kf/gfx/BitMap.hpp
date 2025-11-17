#pragma once

#include <kf/units.hpp>

namespace kf::gfx {

/// @brief БитМап изображение
template<Pixel W, Pixel H> struct BitMap final {

    /// @brief Ширина в пикселях
    static constexpr auto width = W;

    /// @brief Высота в пикселях
    static constexpr auto height = H;

    /// @brief Количество страниц
    static constexpr auto pages = (height + 7) / 8;

    /// @brief Буфер
    const u8 buffer[width * pages];

    BitMap() = delete;
};

}// namespace kf::gfx
