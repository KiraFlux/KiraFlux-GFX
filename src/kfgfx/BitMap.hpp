#pragma once

#include <rs/primitives.hpp>
#include "Position.hpp"


namespace kfgfx {

/// БитМап изображение
template<Position width, Position height> struct BitMap {
    static constexpr auto pages_count = (height / 8) + (height % 8 != 0);
    static constexpr auto buffer_size = width * pages_count;

    const rs::u8 buffer[buffer_size];

    BitMap() = delete;
};

}