#pragma once

#include "SRCommon.h"
#include "SRMath.h"

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
}