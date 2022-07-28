#include "Texture.h"

namespace SR
{
    Vector4 Texture::LoadTexelAddressed(int x, int y, TextureAddressMode address) const
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
        return LoadTexel(x, y);
    }

    Vector4 Texture::Sample(const SamplerState& smapler, const Vector2& uv) const
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

        Vector4 texel0 = LoadTexelAddressed(x + 0, y + 0, smapler.address);
        Vector4 texel1 = LoadTexelAddressed(x + 1, y + 0, smapler.address);
        Vector4 texel2 = LoadTexelAddressed(x + 0, y + 1, smapler.address);
        Vector4 texel3 = LoadTexelAddressed(x + 1, y + 1, smapler.address);

        xy = glm::fract(xy);
        return glm::mix(glm::mix(texel0, texel1, xy.x), glm::mix(texel2, texel3, xy.x), xy.y);
    }
}