#pragma once

#include "rs/Result.hpp"
#include "Position.hpp"

/// KiraFlux GFX
namespace kfgfx {

/// Представление кадра
struct FrameView {

public:

    /// Ошибка операции
    enum class Error {
        /// Размер окна слишком мал
        SizeTooSmall,

        /// Размер окна слишком велик
        SizeTooLarge,

        /// Смещение выходит за пределы
        OffsetOutOfBounds,
    };

private:

    /// Буфер
    void *buffer;

public:

    /// Размер окна
    const Position size;

    /// Смещение системы координат
    const Position offset;

    /// Создать представление кадра
    static rs::Result<FrameView, Error> create(void *buffer, Position size, Position offset) {
        if (size.x < 1 or size.y < 1) {
            return {Error::SizeTooSmall};
        }

        return {FrameView(buffer, size, offset)};
    }

    /// Создать дочерние представление кадра
    rs::Result<FrameView, Error> sub(Position sub_size, Position sub_offset) const {
        if (sub_size.x > size.x or sub_size.y > size.y) {
            return {Error::SizeTooLarge};
        }

        if (sub_offset.x < 0 or sub_offset.y < 0 ||
            sub_offset.x + sub_size.x > size.x ||
            sub_offset.y + sub_size.y > size.y) {
            return {Error::OffsetOutOfBounds};
        }

        return create(buffer, sub_size, {
            static_cast<Position::Value>(offset.x + sub_offset.x),
            static_cast<Position::Value>(offset.y + sub_offset.y)
        });
    }

private:

    explicit FrameView(void *_buffer, Position _size, Position _offset) :
        size{_size}, offset{_offset}, buffer{_buffer} {}
};

} // namespace kfgfx
