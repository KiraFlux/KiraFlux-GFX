#pragma once

#include <rs/primitives.hpp>


namespace kfgfx {

/// БитМап изображение
template<rs::size width, rs::size height> struct BitMap {
    static constexpr auto pages_count = (height / 8) + (height % 8 != 0);
    static constexpr auto buffer_size = width * pages_count;

    const rs::u8 buffer[buffer_size];

    BitMap() = delete;
};

}