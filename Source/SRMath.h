#pragma once

#define GLM_FORCE_ALIGNED
#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#define ONE_PI 				  (3.1415926535897932f)	
#define INV_PI			      (0.31830988618f)
#define HALF_PI			      (1.57079632679f)
#define TWO_PI			      (6.28318530717f)
#define PI_SQUARED		      (9.86960440108f)
#define SMALL_NUMBER		  (1.e-8f)
#define KINDA_SMALL_NUMBER    (1.e-4f)
#define BIG_NUMBER			  (3.4e+38f)
#define DELTA			      (0.00001f)
#define FLOAT_MAX			  (3.402823466e+38f)

namespace SR
{
    using Vector2 = glm::vec2;
    using Vector3 = glm::vec3;
    using Vector4 = glm::vec4;

    using Vector2i = glm::ivec2;
    using Vector3i = glm::ivec3;
    using Vector4i = glm::ivec4;

    using Vector2u = glm::uvec2;
    using Vector3u = glm::uvec3;
    using Vector4u = glm::uvec4;

    using Vector2f = glm::fvec2;
    using Vector3f = glm::fvec3;
    using Vector4f = glm::fvec4;

    using Vector2d = glm::dvec2;
    using Vector3d = glm::dvec3;
    using Vector4d = glm::dvec4;
    
    using Matrix3x3 = glm::mat3;
    using Matrix4x4 = glm::mat4;

    using Quaternion = glm::quat;
    
namespace Math
{
    FORCEINLINE float Cos(float radians)
    {
        return cos(radians);
    }

    template <typename T>
    FORCEINLINE T Max(const T& a, const T& b)
    {
        return (a >= b) ? a : b;
    }

    template <typename T>
    FORCEINLINE T Min(const T& a, const T& b)
    {
        return (a <= b) ? a : b;
    }

    FORCEINLINE float Lerp(float x, float y, float t)
    {
        return x + (y - x) * t;
    }

    FORCEINLINE Vector3 Lerp(const Vector3& x, const Vector3& y, float t)
    {
        return x + (y - x) * t;
    }

    FORCEINLINE float Dot(const Vector3& v1, const Vector3& v2)
    {
        return glm::dot(v1, v2);
    }
    
    FORCEINLINE float Cross(const Vector2& v1, const Vector2& v2)
    {
         return v1.x * v2.y - v1.y * v2.x;
    }

    FORCEINLINE Vector3 Cross(const Vector3& v1, const Vector3& v2)
    {
         return glm::cross(v1, v2);
    }

    FORCEINLINE float Abs(float x)
    {
        return abs(x);
    }

    FORCEINLINE float Fmod(float x, float y)
    {
        return fmod(x, y);
    }

    FORCEINLINE float Square(float x)
    {
        return x * x;
    }

    FORCEINLINE Vector3 Normalize(const Vector3& v)
    {
        return glm::normalize(v);
    }

    FORCEINLINE float Length(const Vector3& v)
    {
        return glm::length(v);
    }

    FORCEINLINE float LengthSquared(const Vector3& v)
    {
        return v.x * v.x + v.y * v.y + v.z * v.z;
    }

    FORCEINLINE bool IsPowerOfTwo(uint32_t value)
    {
        return (value > 0) && ((value & (value - 1)) == 0);
    }

    FORCEINLINE Matrix4x4 Transpose(const Matrix4x4& matrix)
    {
        return glm::transpose(matrix);
    }

    FORCEINLINE Matrix4x4 Compose(const Vector3& translation, const Quaternion& rotation, const Vector3& scale)
    {
        return glm::translate(glm::mat4(1), translation) * glm::mat4_cast(glm::normalize(rotation)) * glm::scale(glm::mat4(1.0f), scale);
    }

    FORCEINLINE void Decompose(const Matrix4x4& matrix, Vector3& outTranslation, Quaternion& outRotation, Vector3& outScale)
    {
        Vector3 skew;
        Vector4 perspective;
        glm::decompose(matrix, outScale, outRotation, outTranslation, skew, perspective);
    }

    FORCEINLINE uint32_t MaxMipLevelCount(uint32_t size)
    {
        return 1 + uint32_t(std::floor(std::log2(size)));
    }

    FORCEINLINE uint32_t MaxMipLevelCount(uint32_t width, uint32_t height)
    {
        return 1 + uint32_t(std::floor(std::log2(glm::min(width, height))));
    }

    FORCEINLINE float Clamp(float x, float min = 0, float max = 1)
    {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    FORCEINLINE uint32 Clamp(uint32 x, uint32 min = 0, uint32 max = 1)
    {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    FORCEINLINE Vector3 Clamp(const Vector3& vec, float min = 0, float max = 1)
    {
        return Vector3(Clamp(vec[0]), Clamp(vec[1]), Clamp(vec[2]));
    }

    FORCEINLINE Vector3 Bezier3(float t, Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3)
    {
        t = Math::Clamp(t, 0.0f, 1.0f);
        float d = 1.0f - t;
        return d * d * d * p0 + 3.0f * d * d * t * p1 + 3.0f * d * t * t * p2 + t * t * t * p3;
    }

    FORCEINLINE float DegreesToRadians(float x)
    {
        return glm::radians(x);
    }

    FORCEINLINE Vector3 DegreesToRadians(const Vector3& v)
    {
        return glm::radians(v);
    }

    FORCEINLINE float RadiansToDegrees(float x)
    {
        return glm::radians(x);
    }

    FORCEINLINE Quaternion AngleAxis(float angle, const Vector3& axis)
    {
        return glm::angleAxis(angle, axis);
    }

    FORCEINLINE Matrix4x4 Inverse(const Matrix4x4& matrix)
    {
        return glm::inverse(matrix);
    }
}

}