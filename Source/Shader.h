#pragma once

#include "SRCommon.h"
#include "SRMath.h"

namespace SR
{
    using BufferAddres = void*;

    struct ShaderPayload
    {
        Vector4 SV_Target;
        Vector3 screenPosition;

        Vector3 worldPosition;
        Vector3 worldNormal;
        Vector3 worldTangent;
        Vector2 texCoord;   
        Vector4 clipPosition;
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