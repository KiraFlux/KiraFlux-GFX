#pragma once

#include <rs/primitives.hpp>
#include "Position.hpp"


namespace kf {

/// БитМап изображение
template<Position W, Position H> struct BitMap final {
    /// Ширина в пикселях
    static constexpr auto width = W;
    /// Высота в пикселях
    static constexpr auto height = H;
    /// Количество страниц
    static constexpr auto pages_count = (height + 7) / 8;

    /// Буфер
    const rs::u8 buffer[width * pages_count];

    BitMap() = delete;
};

}