#pragma once

#include "SRCommon.h"
#include "SRMath.h"

namespace SR
{
    using BufferAddres = void*;

    struct ShaderPayload
    {
        Vector4 clipPosition;
        Vector3 ndcPosition;
        Vector3 worldPosition;
        Vector3 worldNormal;
        Vector3 worldTangent;
        Vector2 texCoord;   
        float invW;
    };

    struct VertexShader
    {
        void (*Main)(uint32 SV_VertexID, ShaderPayload& output, const void* pushConstants);
    };

    struct PixelShader
    {
        Vector4 (*Main)(const ShaderPayload& input, const void* pushConstants);
    };
}