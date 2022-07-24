#pragma once

#include "SRConfig.h"
#include "SRDefine.h"
#include "STLHeaders.h"

namespace SR
{
    using int8 = signed char;
    using int16 = short;
    using int32 = int;
    using int64 = long long;

    using uint8 = unsigned char;
    using uint16 = unsigned short;
    using uint32 = unsigned int;
    using uint64 = unsigned long long;

    struct Primitive
    {
        uint32 indices[3];
    };
}