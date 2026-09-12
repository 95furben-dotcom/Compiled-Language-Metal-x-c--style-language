#pragma once

#include <sys/types.h>

namespace Specifiers{
    static constexpr uint STATIC = 0b1<<0;
    static constexpr uint CONST = 0b1<<1;
    static constexpr uint INLINE = 0b1<<2;
    static constexpr uint GPUSTORED = 0b1<<3;
    static constexpr uint KERNEL = 0b1<<4;
}