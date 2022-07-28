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
        uint32 GetWidth() const
        {
            return width;
        }
        uint32 GetHeight() const
        {
            return height;
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

    template <typename T>
    class RenderTarget
    {
    public:
        RenderTarget(uint32 w, uint32 h)
            : width(w)
            , height(h)
        {

        }
        uint32 GetWidth() const
        {
            return width;
        }
        uint32 GetHeight() const
        {
            return height;
        }
        void Resize(uint32 w, uint32 h)
        {
            width = w;
            height = h;
            buffer.resize(width * height);
        }
        void Clear(const T& clearValue)
        {
            for (uint32 i = 0; i < width * height; i++)
            {
                buffer[i] = clearValue;
            }
        }
        const T& Load(uint32 x, uint32 y) const
        {
            uint32 index = y * width + x;
            return buffer[index];
        }
        void Store(uint32 x, uint32 y, const T& value)
        {
            uint32 index = y * width + x;
            buffer[index] = value;
        }
        void* GetDataPtr()
        {
            return buffer.data();
        } 
        T LoadTexelAddressed(int x, int y, TextureAddressMode address) const;
        T Sample(const SamplerState& smapler, const Vector2& uv) const;
    private:
        uint32 width;
        uint32 height;
        std::vector<T> buffer;
    };

    template <typename T>
    T RenderTarget<T>::LoadTexelAddressed(int x, int y, TextureAddressMode address) const
    {
        int w = (int)width;
        int h = (int)height;
        switch (address)
        {
        case TEXTURE_ADDRESS_WARP:
        {
            x = COORD_MOD(x, w);
            y = COORD_MOD(y, h);
        } break;
        case TEXTURE_ADDRESS_MIRROR:
        {
            x = COORD_MOD(x, 2 * w);
            y = COORD_MOD(y, 2 * h);

            x -= w;
            y -= h;

            x = COORD_MIRROR(x);
            y = COORD_MIRROR(y);

            x = w - 1 - x;
            y = h - 1 - y;
        } break;
        case TEXTURE_ADDRESS_CLAMP:
        {
            if (x < 0) x = 0;
            if (y < 0) y = 0;
            if (x >= w) x = w - 1;
            if (y >= h) y = h - 1;
        } break;
        default: break;
        }
        return Load(x, y);
    }

    template <typename T>
    T RenderTarget<T>::Sample(const SamplerState& smapler, const Vector2& uv) const
    {
        if (smapler.filter == TEXTURE_FILTER_NEAREST)
        {
            Vector2 xy = uv * Vector2(width, height);
            int x = (int)xy.x;
            int y = static_cast<int>(xy.y);

            return LoadTexelAddressed(x, y, smapler.address);
        }
        Vector2 xy = uv * Vector2(width, height) - Vector2(0.5f);
        auto x = (int)xy.x;
        auto y = (int)xy.y;

        T texel0 = LoadTexelAddressed(x + 0, y + 0, smapler.address);
        T texel1 = LoadTexelAddressed(x + 1, y + 0, smapler.address);
        T texel2 = LoadTexelAddressed(x + 0, y + 1, smapler.address);
        T texel3 = LoadTexelAddressed(x + 1, y + 1, smapler.address);

        xy = glm::fract(xy);
        return glm::mix(glm::mix(texel0, texel1, xy.x), glm::mix(texel2, texel3, xy.x), xy.y);
    }
}