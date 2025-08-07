#pragma once

#include "Position.hpp"
#include "rs/Result.hpp"
#include "rs/primitives.hpp"
#include <algorithm>

namespace kfgfx {

struct FrameView {
public:
    enum class Error {
        SizeTooSmall,
        SizeTooLarge,
        OffsetOutOfBounds,
    };

private:
    rs::u8* buffer;
    const Position stride;

public:
    const Position width;
    const Position height;
    const Position offset_x;
    const Position offset_y;

    static rs::Result<FrameView, Error> create(
        rs::u8* buffer,
        Position stride,
        Position width,
        Position height,
        Position offset_x,
        Position offset_y
    ) {
        if (width < 1 or height < 1) {
            return rs::Result<FrameView, Error>{Error::SizeTooSmall};
        }

        return rs::Result<FrameView, Error>{
            FrameView(buffer, stride, width, height, offset_x, offset_y)
        };
    }

    rs::Result<FrameView, Error> sub(
        Position sub_width,
        Position sub_height,
        Position sub_offset_x,
        Position sub_offset_y
    ) const {
        if (sub_offset_x >= width or sub_offset_y >= height) {
            return rs::Result<FrameView, Error>{Error::OffsetOutOfBounds};
        }

        if (sub_width > width - sub_offset_x or sub_height > height - sub_offset_y) {
            return rs::Result<FrameView, Error>{Error::SizeTooLarge};
        }

        return FrameView::create(
            buffer,
            stride,
            sub_width,
            sub_height,
            static_cast<Position>(offset_x + sub_offset_x),
            static_cast<Position>(offset_y + sub_offset_y)
        );
    }

    void setPixel(Position x, Position y, bool on) {
        if (isOutOfBoundsX(x) or isOutOfBoundsY(y)) return;

        const Position abs_x = absoluteX(x);
        const Position page = getPage(y);
        const rs::u8 mask = getByteBitMask(y);
        write(abs_x, page, mask, on);
    }

    bool getPixel(Position x, Position y) const {
        if (isOutOfBoundsX(x) or isOutOfBoundsY(y)) return false;

        return buffer[getByteIndex(x, y)] & getByteBitMask(y);
    }

    void fill(bool value) {
        const Position top = offset_y;
        const Position bottom = offset_y + height - 1;

        const Position start_page = top >> 3;
        const Position end_page = bottom >> 3;

        for (auto page = start_page; page <= end_page; ++page) {
            const Position page_top = page << 3;
            const Position page_bottom = page_top + 7;

            const Position y_start = std::max(top, page_top);
            const Position y_end = std::min(bottom, page_bottom);

            if (y_start > y_end) continue;

            const rs::u8 mask = createPageMask(
                static_cast<rs::u8>(y_start - page_top),
                static_cast<rs::u8>(y_end - page_top)
            );

            for (Position x = 0; x < width; ++x) {
                const Position abs_x = absoluteX(x);
                if (abs_x < 0 or abs_x >= stride) continue;
                write(abs_x, page, mask, value);
            }
        }
    }

    template<Position W, Position H>
    void drawBitmap(Position x, Position y, const BitMap<W, H>& bitmap, bool on = true) {
        const Position abs_x = offset_x + x;
        const Position abs_y = offset_y + y;

        const Position frame_top = offset_y;
        const Position frame_bottom = offset_y + height;
        const Position frame_left = offset_x;
        const Position frame_right = offset_x + width;

        for (Position page_idx = 0; page_idx < BitMap<W, H>::pages_count; ++page_idx) {
            const Position page_y = abs_y + (page_idx << 3);

            if (page_y + 7 < frame_top or page_y >= frame_bottom) continue;

            rs::u8 clip_top = 0;
            if (page_y < frame_top) {
                clip_top = static_cast<rs::u8>(frame_top - page_y);
            }

            rs::u8 clip_bottom = 7;
            if (page_y + 7 >= frame_bottom) {
                clip_bottom = static_cast<rs::u8>(frame_bottom - page_y - 1);
            }

            const rs::u8 mask = createPageMask(clip_top, clip_bottom);

            for (Position bx = 0; bx < W; ++bx) {
                const Position target_x = abs_x + bx;
                if (target_x < frame_left or target_x >= frame_right) continue;

                const rs::u8 data = bitmap.buffer[page_idx * W + bx] & mask;
                if (data == 0) continue;

                const Position target_page = page_y >> 3;
                const rs::u8 bit_offset = static_cast<rs::u8>(page_y & 0x07);

                if (bit_offset == 0) {
                    write(target_x, target_page, data, on);
                } else {
                    if (target_page < getPageCount()) {
                        write(target_x, target_page, data << bit_offset, on);
                    }

                    const Position next_page = target_page + 1;
                    if (next_page < getPageCount()) {
                        write(target_x, next_page, data >> (8 - bit_offset), on);
                    }
                }
            }
        }
    }

private:
    void write(Position abs_x, Position page, rs::u8 data, bool on) const {
        const rs::size index = page * stride + abs_x;
        if (on) {
            buffer[index] |= data;
        } else {
            buffer[index] &= ~data;
        }
    }

    static rs::u8 createPageMask(rs::u8 start_bit, rs::u8 end_bit) {
        if (start_bit > end_bit) return 0;
        return ((1 << (end_bit + 1)) - 1) ^ ((1 << start_bit) - 1);
    }

    inline Position absoluteX(Position x) const {
        return offset_x + x;
    }

    inline Position absoluteY(Position y) const {
        return offset_y + y;
    }

    inline Position getPage(Position y) const {
        return absoluteY(y) >> 3;
    }

    inline rs::u8 getByteBitMask(Position y) const {
        return static_cast<rs::u8>(1) << (absoluteY(y) & 0x07);
    }

    inline rs::size getByteIndex(Position x, Position y) const {
        return getPage(y) * stride + absoluteX(x);
    }

    inline bool isOutOfBoundsX(Position x) const {
        return x < 0 or x >= width;
    }

    inline bool isOutOfBoundsY(Position y) const {
        return y < 0 or y >= height;
    }

    inline Position getPageCount() const {
        return (absoluteY(height) + 7) >> 3;
    }

    explicit FrameView(
        rs::u8* buffer,
        Position stride,
        Position width,
        Position height,
        Position offset_x,
        Position offset_y
    ) :
        buffer{buffer},
        stride{stride},
        width{width},
        height{height},
        offset_x{offset_x},
        offset_y{offset_y} {}
};

} // namespace kfgfx