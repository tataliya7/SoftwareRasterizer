#pragma once

#include "Shaders/ShaderCommon.h"
#include "Shader.h"

namespace SR
{
    class Texture;
    struct PBRShaderPushConstants
    {
        BufferAddres positions;
        BufferAddres normals;
        BufferAddres tangents;
        BufferAddres texCoords;
        BufferAddres perFrameData;
        BufferAddres material;
        Matrix4x4* worldMatrix;
        Matrix4x4* lightMatrix;
        Texture* shadowMap;
    };

    extern void PBRMainVS(uint32 SV_VertexID, ShaderPayload& output, const void* pushConstants);
    extern Vector4 PBRMainPS(const ShaderPayload& input, const void* pushConstants);
}