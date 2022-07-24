#pragma once

#include "SRCommon.h"
#include "SRMath.h"

#define COORD_MOD(i, n) ((i) & ((n) - 1) + (n)) & ((n) - 1)
#define COORD_MIRROR(i) (i) >= 0 ? (i) : (-1 - (i))

#define SAMPLER_LINEAR_WARP  SamplerState(TEXTURE_FILTER_LINEAR, TEXTURE_ADDRESS_WARP)
#define SAMPLER_LINEAR_CLAMP SamplerState(TEXTURE_FILTER_LINEAR, TEXTURE_ADDRESS_CLAMP)

namespace SR
{
    enum TextureAddressMode 
    {
        TEXTURE_ADDRESS_WARP,
        TEXTURE_ADDRESS_MIRROR,
        TEXTURE_ADDRESS_CLAMP,
    };

    enum FilterMode 
    {
        TEXTURE_FILTER_NEAREST,
        TEXTURE_FILTER_LINEAR,
    };

    struct SamplerState
    {
        SamplerState() = default;
        SamplerState(FilterMode filter, TextureAddressMode address)
            : filter(filter)
            , address(address)
        {

        }
        TextureAddressMode address;
        FilterMode filter;
    };

    class Texture
    {
    public:
        Texture() : width(0), height(0) {}
        void Resize(uint32 w, uint32 h)
        {
            width = w;
            height = h;
            buffer.resize(w * h);
        }
        Vector4 Sample(const SamplerState& smapler, const Vector2& uv)  const;
        Vector4 LoadTexelAddressed(int x, int y, TextureAddressMode address) const;
        Vector4 LoadTexel(uint32 x, uint32 y) const
        {
            if (x >= width || y >= height)
            {
                return Vector4(0.0f);
            }
            uint32 index = y * width + x;
            return buffer[index];
        }
        void StoreTexel(uint32 x, uint32 y, const Vector4& value)
        {
            uint32 index = y * width + x;
            buffer[index] = value;
        }
    private:
        uint32 width;
        uint32 height;
        std::vector<Vector4> buffer;
    };
}