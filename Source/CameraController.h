#pragma once

#include "SRCommon.h"
#include "SRMath.h"

namespace SR
{
    class SimpleFirstPersonCameraController
    {
    public:

        SimpleFirstPersonCameraController() = default;

        ~SimpleFirstPersonCameraController() = default;

        void Update(float deltaTime, Vector3& outCameraPosition, Vector3& outCameraEuler);

        float cameraSpeed = 1.0f;

        float maxTranslationVelocity = FLOAT_MAX;

        float maxRotationVelocity = FLOAT_MAX;

        float translationMultiplier = 0.5f;

        float rotationMultiplier = 3.0f;
    };
}