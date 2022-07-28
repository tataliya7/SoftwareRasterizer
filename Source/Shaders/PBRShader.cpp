#include "PBRShader.h"
#include "SRMath.h"
#include "Scene.h"
#include "Texture.h"

#define EPSILON 0.00001f

namespace SR
{
    // Constant normal incidence Fresnel factor for all dielectrics.
    const Vector3 Fdielectric = Vector3(0.04f);

    Vector3 UVToCubemapCoord(Vector2 uv, uint32 faceIndex)
    {
        uv = 2.0f * Vector2(uv.x, 1.0f - uv.y) - Vector2(1.0f);
        switch (faceIndex)
        {
        case 0: return glm::normalize(Vector3(1.0f, uv.y, -uv.x));
        case 1: return glm::normalize(Vector3(-1.0f, uv.y, uv.x));
        case 2: return glm::normalize(Vector3(uv.x, 1.0f, -uv.y));
        case 3: return glm::normalize(Vector3(uv.x, -1.0f, uv.y));
        case 4: return glm::normalize(Vector3(uv.x, uv.y, 1.0f));
        case 5: return glm::normalize(Vector3(-uv.x, uv.y, -1.0f));
        default: return Vector3(0.0f);
        }
    }

    void GetTangentBasis(Vector3& T, Vector3& B, const Vector3& N)
    {
        Vector3 up = glm::abs(N.z) < 0.999f ? Vector3(0.0f, 0.0f, 1.0f) : Vector3(1.0f, 0.0f, 0.0f);
        T = glm::normalize(glm::cross(up, N));
        B = glm::normalize(glm::cross(N, T));
    }

    Vector3 TangentToWorld(const Vector3 vec, const Vector3 T, const Vector3 B, const Vector3 N)
    {
        return normalize(T * vec.x + B * vec.y + N * vec.z);
    }

    Vector2 Hammersley2D(uint32 index, uint32 numSamples)
    {
        uint32 bits = (index << 16u) | (index >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        const float radicalInverse_VdC = float(bits) * 2.3283064365386963e-10f;
        return Vector2(float(index) / float(numSamples), radicalInverse_VdC);
    }

    Vector3 HemisphereSampleUniform(Vector2 Xi)
    {
        float phi = Xi.y * 2.0f * ONE_PI;
        float cosTheta = 1.0f - Xi.x;
        float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
        return Vector3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    }

    Vector3 ImportanceSampleGGX(Vector2 Xi, float roughness)
    {
        float alpha = roughness * roughness;
        float phi = 2.0f * ONE_PI * Xi.x;
        float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (alpha * alpha - 1.0f) * Xi.y));
        float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
        Vector3 H = Vector3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
        return H;
    }

    float GeometrySchlicksmithGGX(float NdotL, float NdotV, float roughness)
    {
        float k = (roughness * roughness) / 2.0f;
        float GL = NdotL / (NdotL * (1.0f - k) + k);
        float GV = NdotV / (NdotV * (1.0f - k) + k);
        return GL * GV;
    }

    float D_GGX(float NdotH, float roughness)
    {
        float alpha = roughness * roughness;
        float alphaSquare = alpha * alpha;
        float denom = (NdotH * NdotH) * (alphaSquare - 1.0f) + 1.0f;
        return alphaSquare / (ONE_PI * denom * denom);
    }

    Vector3 FresnelSchlick(Vector3 F0, float cosTheta)
    {
        return F0 + (1.0f - F0) * glm::pow(1.0f - cosTheta, 5.0f);
    }

    Vector3 FresnelSchlickRoughness(Vector3 F0, float cosTheta, float roughness)
    {
        return F0 + (glm::max(Vector3(1.0f - roughness), F0) - F0) * glm::pow(1.0f - cosTheta, 5.0f);
    }

    float Gsub(float cosTheta, float k)
    {
        return cosTheta / (cosTheta * (1.0f - k) + k);
    }

    float SchlickGGX(float NdotL, float NdotV, float roughness)
    {
        float r = roughness + 1.0f;
        float k = (r * r) / 8.0f;
        return Gsub(NdotL, k) * Gsub(NdotV, k);
    }

    Vector3 ACESFilm(Vector3 x)
    {
        float a = 2.51f;
        float b = 0.03f;
        float c = 2.43f;
        float d = 0.59f;
        float e = 0.14f;
        return glm::clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
    }

    Vector3 Tonemap(Vector3 color, float exposure)
    {
        color *= exposure;
        return ACESFilm(color);
    }

    Vector3 GammaCorrection(Vector3 color, float gamma)
    {
        return glm::pow(glm::abs(color), Vector3(1.0f / gamma));
    }

    Vector4 SRGBToLinear(Vector4 sRGB, float gamma)
    {
        Vector3 linear = glm::pow(Vector3(sRGB), Vector3(gamma));
        return Vector4(linear, sRGB.w);
    }

    float PCF(RenderTarget<float>* shadowMap, Vector3 shadowMapCoord)
    {
        if ((shadowMapCoord.y < 0.0f) || (shadowMapCoord.y > 1.0f)) return 1.0f;
        if ((shadowMapCoord.x < 0.0f) || (shadowMapCoord.x > 1.0f)) return 1.0f;
        if (shadowMapCoord.z < 0.0f) return 1.0f;
        if (shadowMapCoord.z > 1.0f) return 1.0f;
        shadowMapCoord.z -= 0.1f;

        uint32 shadowMapSize = shadowMap->GetWidth();
        float dx = 1.0f / float(shadowMapSize);
        float dy = 1.0f / float(shadowMapSize);

        float result = 0.0f;
        for (int i = -2; i <= 2; i++)
        {
            for (int j = -2; j <= 2; j++)
            {
                float depth = shadowMap->Sample(SAMPLER_LINEAR_WARP, shadowMapCoord + Vector3(dx * i, dy * j, 0));
                //printf("%f %f %f\n", shadowMapCoord.x, shadowMapCoord.y, depth);
                if (shadowMapCoord.z < depth)
                {
                    result++;
                }
            }
        }
        result /= (5 * 5);
        return result;
    }

    void PBRMainVS(uint32 SV_VertexID, ShaderPayload& output, const void* pushConstants)
    {
        const PBRShaderPushConstants& pc = *(PBRShaderPushConstants*)pushConstants;

        Matrix4x4& worldMatrix = *pc.worldMatrix;
        Vector4 localPosition = Vector4(((Vector3*)pc.positions)[SV_VertexID], 1.0f);
        Vector4 worldPosition = worldMatrix * localPosition;
        Vector3 worldNormal = ((Vector3*)pc.normals)[SV_VertexID];
        worldNormal = Math::Normalize(Matrix3x3(worldMatrix) * worldNormal);
        Vector3 worldTangent = ((Vector3*)pc.tangents)[SV_VertexID];
        worldTangent = Math::Normalize(Matrix3x3(worldMatrix) * worldTangent);
        Vector2 texCoord = ((Vector2*)pc.texCoords)[SV_VertexID];

        output.clipPosition = ((PerFrameData*)pc.perFrameData)->viewProjectionMatrix * worldPosition;
        //output.clipPosition = *pc.lightMatrix * worldPosition;
        output.clipPosition.y = -output.clipPosition.y;
        output.worldPosition = Vector3(worldPosition);
        output.worldNormal = worldNormal;
        output.worldTangent = worldTangent;
        output.texCoord = texCoord;
    }

    Vector4 PBRMainPS(const ShaderPayload& input, const void* pushConstants)
    {
        const PBRShaderPushConstants& pc = *(PBRShaderPushConstants*)pushConstants;
        const PerFrameData& perFrameData = *(PerFrameData*)pc.perFrameData;
        const PBRMaterial& material = *(PBRMaterial*)pc.material;

        Vector3 geometricWorldNormal = glm::normalize(input.worldNormal);
        Vector3 worldTangent = glm::normalize(input.worldTangent);
        Vector3 position = input.worldPosition;
        Vector2 texCoord0 = input.texCoord;
        float gamma = perFrameData.gamma;
        DebugView debugView = perFrameData.debugView;

        Vector4 baseColor = material.baseColorMap ? material.baseColor * SRGBToLinear(material.baseColorMap->Sample(SAMPLER_LINEAR_WARP, texCoord0), gamma) : material.baseColor;
        
        Vector3 N = geometricWorldNormal;
        if (material.normalMap)
        {
            Vector3 tangentNormal = glm::normalize(Vector3(material.normalMap->Sample(SAMPLER_LINEAR_WARP, texCoord0)) * 2.0f - Vector3(1.0f));
            Vector3 T = worldTangent;
            Vector3 B = glm::cross(N, T);
            Matrix3x3 TBN = Matrix3x3(T, B, N);
            N = glm::normalize(TBN * tangentNormal);
        }

        Vector4 metallicRoughness = material.metallicRoughnessMap ? material.metallicRoughnessMap->Sample(SAMPLER_LINEAR_WARP, texCoord0) : Vector4(0.0f);
        float metallic = material.metallic * metallicRoughness.b;
        float roughness = material.roughness * metallicRoughness.g;

        float depth01 = input.clipPosition.z / input.clipPosition.w;

        Vector3 V = glm::normalize(perFrameData.cameraPosition - input.worldPosition);
        Vector3 R = glm::reflect(-V, N);
        float NdotV = glm::clamp(dot(N, V), 0.0f, 1.0f);

        Vector3 F0 = glm::mix(Fdielectric, Vector3(baseColor), metallic);
        Vector3 finalColor = Vector3(0.0f);

        float visibility = 1.0f;
        if (pc.shadowMap)
        {
            Vector4 shadowMapCoord = *pc.lightMatrix * Vector4(position, 1.0f);
            shadowMapCoord /= shadowMapCoord.w;
            shadowMapCoord.x = (1.0f + shadowMapCoord.x) * 0.5f;
            shadowMapCoord.y = (1.0f + shadowMapCoord.y) * 0.5f;
            visibility = PCF(pc.shadowMap, Vector3(shadowMapCoord));
        }

        // Direct Lighting
        float intensity = perFrameData.mainLightIntensity;
        Vector3 lightColor = perFrameData.mainLightColor;
        Vector3 Li = -perFrameData.mainLightDirection;
        float cosLi = glm::clamp(glm::dot(N, Li), 0.0f, 1.0f);
        if (cosLi > 0.0f)
        {
            Vector3 Lh = glm::normalize(Li + V);
            float cosLh = glm::clamp(glm::dot(N, Lh), 0.0f, 1.0f);

            Vector3 F = FresnelSchlick(F0, NdotV);
            float D = D_GGX(cosLh, roughness);
            float G = SchlickGGX(cosLi, NdotV, roughness);
            Vector3 Kd = (Vector3(1.0f) - F) * (1.0f - metallic);

            Vector3 diffuse = Kd * Vector3(baseColor) / ONE_PI;
            Vector3 specular = (F * D * G) / glm::max(0.001f, 4.0f * cosLi * NdotV);
            Vector3 radiance = lightColor * intensity;

            finalColor += (diffuse + specular) * radiance * cosLi * visibility;
        }

        finalColor = Tonemap(finalColor, perFrameData.exposure);
        finalColor = GammaCorrection(finalColor, perFrameData.gamma);

        Vector4 color = Vector4(finalColor, 1.0f);

        if (debugView == DEBUG_VIEW_POSITION)
        {
            color = Vector4(position.r, position.g, position.b, 1.0f);
        }
        else if (debugView == DEBUG_VIEW_NORMAL)
        {
            color = Vector4(N.r, N.g, N.b, 1.0f);
        }
        else if (debugView == DEBUG_VIEW_BASE_COLOR)
        {
            color = Vector4(baseColor.r, baseColor.g, baseColor.b, 1.0f);
        }
        else if (debugView == DEBUG_VIEW_METALLIC)
        {
            color = Vector4(metallic, metallic, metallic, 1.0f);
        }
        else if (debugView == DEBUG_VIEW_ROUGHNESS)
        {
            color = Vector4(roughness, roughness, roughness, 1.0f);
        }
        else if (debugView == DEBUG_VIEW_DEPTH)
        {
            color = Vector4(depth01, depth01, depth01, 1.0f);
        }
        return color;
    }
}