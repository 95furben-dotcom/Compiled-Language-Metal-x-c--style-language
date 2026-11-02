#pragma once

#include <sys/types.h>

namespace Specifiers{
    static constexpr uint STATIC    = 0b1 << 0;
    static constexpr uint CONST     = 0b1 << 1;
    static constexpr uint INLINE    = 0b1 << 2;
    static constexpr uint GPUSTORED = 0b1 << 3;
    static constexpr uint KERNEL    = 0b1 << 4;
    static constexpr uint POINTER   = 0b1 << 5;

    static bool IsStatic(uint value){
        return (STATIC & value) == 1;
    }
    static bool IsConst(uint value){
        return (CONST & value) == 1;
    }
    static bool IsGpuStored(uint value){
        return (GPUSTORED & value) == 1;
    }
    
}