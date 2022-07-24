#pragma once

#include "Shader.h"

namespace SR
{
    struct PerFrameData
    {
        float gamma;
        float exposure;
        Vector3 cameraPosition;
        Matrix4x4 viewMatrix;
        Matrix4x4 invViewMatrix;
        Matrix4x4 projectionMatrix;
        Matrix4x4 invProjectionMatrix;
        Matrix4x4 viewProjectionMatrix;
        Matrix4x4 invViewProjectionMatrix;
        Vector3 mainLightColor;
        float mainLightIntensity;
        Vector3 mainLightDirection;
        DebugView debugView;
    };

    struct PBRShaderPushConstants
    {
        BufferAddres positions;
        BufferAddres normals;
        BufferAddres tangents;
        BufferAddres texCoords;
        BufferAddres perFrameData;
        BufferAddres material;
        Matrix4x4 worldMatrix;
    };

    extern void PBRMainVS(uint32 SV_VertexID, ShaderPayload& output, const void* pushConstants);
    extern Vector4 PBRMainPS(const ShaderPayload& input, const void* pushConstants);
}