#include "Rasterizer.h"
#include "Shader.h"
#include "JobSystem.h"

#include "imgui/imgui.h"

namespace SR
{
    static bool ClipSpaceCulling(const Vector4& a, const Vector4& b, const Vector4& c)
    {
        if(a.x >  a.w && b.x >  b.w && c.x >  c.w)
        {
            return true;
        }
        if(a.x < -a.w && b.x < -b.w && c.x < -c.w)
        {
            return true;
        }
        if(a.y >  a.w && b.y >  b.w && c.y >  c.w)
        {
            return true;
        }
        if(a.y < -a.w && b.y < -b.w && c.y < -c.w)
        {
            return true;
        }
        if(a.z >  a.w && b.z >  b.w && c.z >  c.w)
        {
            return true;
        }
        if(a.z < -a.w && b.z < -b.w && c.z < -c.w)
        {
            return true;
        }
        return false;
    }

    static std::tuple<float, float, float, float> CalculateTriangleBounds2D(const Vector2& a, const Vector2& b, const Vector2& c)
    {
        return
        {
            std::floor(std::min(std::min(a.x, b.x), c.x)),
            std::ceil( std::max(std::max(a.x, b.x), c.x)),
            std::floor(std::min(std::min(a.y, b.y), c.y)),
            std::ceil( std::max(std::max(a.y, b.y), c.y))
        };
    }

    struct BarycentricCoordinates
    {
        float alpha;
        float beta;
        float gamma;
        bool IsInside() const
        {
            return alpha >= 0.0f && beta >= 0.0f && gamma >= 0.0f;
        }
    };
    
    static BarycentricCoordinates CalculateBarycentric2D(float x, float y, const Vector4& v0, const Vector4& v1, const Vector4& v2)
    {
        float alpha = (x * (v1.y - v2.y) + (v2.x - v1.x) * y + v1.x * v2.y - v2.x * v1.y) / (v0.x * (v1.y - v2.y) + (v2.x - v1.x) * v0.y + v1.x * v2.y - v2.x * v1.y);
        float beta  = (x * (v2.y - v0.y) + (v0.x - v2.x) * y + v2.x * v0.y - v0.x * v2.y) / (v1.x * (v2.y - v0.y) + (v0.x - v2.x) * v1.y + v2.x * v0.y - v0.x * v2.y);
        float gamma = (x * (v0.y - v1.y) + (v1.x - v0.x) * y + v0.x * v1.y - v1.x * v0.y) / (v2.x * (v0.y - v1.y) + (v1.x - v0.x) * v2.y + v0.x * v1.y - v1.x * v0.y);
        return { alpha, beta, gamma };
    }

    static float BarycentricLerp(float a, float b, float c, const BarycentricCoordinates& bary, float weight)
    {
        return (bary.alpha * a + bary.beta * b + bary.gamma * c) * weight;
    }

    static Vector2 BarycentricLerp(const Vector2& a, const Vector2& b, const Vector2& c, const BarycentricCoordinates& bary, float weight)
    {
        return (bary.alpha * a + bary.beta * b + bary.gamma * c) * weight;
    }

    static Vector3 BarycentricLerp(const Vector3& a, const Vector3& b, const Vector3& c, const BarycentricCoordinates& bary, float weight)
    {
        return (bary.alpha * a + bary.beta * b + bary.gamma * c) * weight;
    }

    static Vector4 BarycentricLerp(const Vector4& a, const Vector4& b, const Vector4& c, const BarycentricCoordinates& bary, float weight)
    {
        return (bary.alpha * a + bary.beta * b + bary.gamma * c) * weight;
    }
    
    struct LaunchVertexShaderExecutionJobData
    {
        VertexShader shader;
        uint32 vertexID;
        ShaderPayload* payload;
        const void* pushConstants;
        Viewport viewport;
        float zNear;
        float zFar;
    };

    static void LaunchVertexShaderExecution(LaunchVertexShaderExecutionJobData* data)
    {
        data->shader.Main(data->vertexID, *data->payload, data->pushConstants);
        Vector4& clipPosition = data->payload->clipPosition;

        const float invW = 1.0f / clipPosition.w;
        data->payload->invW = invW;

        // Perspective correction: the world space properties should be multipy by 1/w before rasterization
        data->payload->worldPosition *= invW;
        data->payload->worldNormal *= invW;
        data->payload->worldTangent *= invW;
        data->payload->texCoord *= invW;

        // Perspective division
        Vector4& screenPosition = data->payload->screenPosition;
        screenPosition = clipPosition;
        screenPosition.x *= invW;
        screenPosition.y *= invW;
        screenPosition.z *= invW;

        // Viewport transform
        screenPosition.x = data->viewport.x + 0.5f * (screenPosition.x + 1.0f) * data->viewport.width;
        screenPosition.y = data->viewport.y + 0.5f * (screenPosition.y + 1.0f) * data->viewport.height;
        screenPosition.z = 0.5f * ((data->zFar + data->zNear) - (data->zFar - data->zNear) * screenPosition.z);
        screenPosition.z = -screenPosition.z;
    }
    
    struct ExecuteTriangleRasterizationJobData
    {
        PixelShader shader;
        uint32 primitiveID;
        const ShaderPayload* payload0;
        const ShaderPayload* payload1;
        const ShaderPayload* payload2;
        const void* pushConstants;
        const GraphicsPipelineState* pipelineState;
        Viewport viewport;
    };

    static void ExecuteTriangleRasterization(ExecuteTriangleRasterizationJobData* data)
    {
        const ShaderPayload* payload0 = data->payload0;
        const ShaderPayload* payload1 = data->payload1;
        const ShaderPayload* payload2 = data->payload2;

        const Vector4& cp0 = payload0->clipPosition;
        const Vector4& cp1 = payload1->clipPosition;
        const Vector4& cp2 = payload2->clipPosition;

        if (ClipSpaceCulling(cp0, cp1, cp2))
        {
            return;
        }

        const Vector4& sp0 = payload0->screenPosition;
        const Vector4& sp1 = payload1->screenPosition;
        const Vector4& sp2 = payload2->screenPosition;

        // Back-face culling
        bool frontFacing = data->pipelineState->frontCCW ?
            Math::Cross(Vector2(sp2) - Vector2(sp0), Vector2(Vector2(sp1) - Vector2(sp0))) >= 0:
            Math::Cross(Vector2(sp2) - Vector2(sp0), Vector2(Vector2(sp1) - Vector2(sp0))) < 0;
        if ((data->pipelineState->cullMode == CULL_MODE_BACK && !frontFacing) ||
            (data->pipelineState->cullMode == CULL_MODE_FRONT && frontFacing) ||
            (data->pipelineState->cullMode == CULL_MODE_FRONT_AND_BACK))
        {
            return;
        }
        
        auto [minx, maxx, miny, maxy] = CalculateTriangleBounds2D(sp0, sp1, sp2);

        for (int x = std::max(0, (int)minx); x < std::min((int)data->pipelineState->colorBuffer->GetWidth(), (int)maxx); x++)
        {
            for (int y = std::max(0, (int)miny); y < std::min((int)data->pipelineState->colorBuffer->GetHeight(), (int)maxy); y++)
            {
                ASSERT(x < data->pipelineState->colorBuffer->GetWidth() && y < data->pipelineState->colorBuffer->GetHeight());

                BarycentricCoordinates barycentric = CalculateBarycentric2D(x + 0.5f, y + 0.5f, sp0, sp1, sp2);
                if (!barycentric.IsInside())
                {
                    continue;
                }

                ShaderPayload payload;

                // Perspective-Correct Interpolation
                payload.invW = BarycentricLerp(payload0->invW, payload1->invW, payload2->invW, barycentric, 1.0f);
                const float w = 1.0f / payload.invW;
                // const float depth = w * (barycentric.alpha * cp0.z / cp0.w + barycentric.beta * cp1.z / cp1.w + barycentric.gamma * cp2.z / cp2.w);
                const float depth = BarycentricLerp(payload0->clipPosition.z, payload1->clipPosition.z, payload2->clipPosition.z, barycentric, w);
                
                // Depth testing
                if (data->pipelineState->depthTestEnable)
                {
                    float depthBufferValue = data->pipelineState->depthBuffer->Load(x, y);
                    if((data->pipelineState->depthCompareOp == COMPARE_OP_LESS_OR_EQUAL) && (depth <= depthBufferValue))
                    {
                        continue;
                    }
                    if((data->pipelineState->depthCompareOp == COMPARE_OP_GREATER) && (depth <= depthBufferValue))
                    {
                        continue;
                    }
                    data->pipelineState->depthBuffer->Store(x, y, depth);
                }

                // Perspective correction: the world space properties should be multipy by w after rasterization

                // Interpolate vertex attributes
                payload.worldPosition = BarycentricLerp(payload0->worldPosition, payload1->worldPosition, payload2->worldPosition, barycentric, w);
                payload.worldNormal = BarycentricLerp(payload0->worldNormal, payload1->worldNormal, payload2->worldNormal, barycentric, w);
                payload.worldTangent = BarycentricLerp(payload0->worldTangent, payload1->worldTangent, payload2->worldTangent, barycentric, w);
                payload.texCoord = BarycentricLerp(payload0->texCoord, payload1->texCoord, payload2->texCoord, barycentric, w);

                Vector4 color = data->shader.Main(payload, data->pushConstants);

                color = glm::clamp(color, 0.0f, 1.0f);
                glm::u8vec4 pixel = { (uint8)(color.x * 255.0f), (uint8)(color.y * 255.0f), (uint8)(color.z * 255.0f), (uint8)(color.w * 255.0f) };
                data->pipelineState->colorBuffer->Store(x, y, pixel);

                // Depth writing
                if (data->pipelineState->depthWriteEnable)
                {
                    data->pipelineState->depthBuffer->Store(x, y, depth);
                }
            }
        }
    }

    void Rasterizer::DrawPrimitives(const GraphicsPipelineState& pipelineState, const void* pushConstants, uint32 numVertices, const std::vector<Primitive>& primitives, uint32 numPrimitives, float zNear, float zFar)
    {
        payloads.resize(numVertices);

        std::vector<JobDecl> jobDecls;

        // Launch vertex shaders
        std::vector<LaunchVertexShaderExecutionJobData> vertexShaderExecuteJobData(numVertices);
        jobDecls.resize(numVertices);
        for (uint32 vertexID = 0; vertexID < numVertices; vertexID++) 
        {
            vertexShaderExecuteJobData[vertexID] = {
                pipelineState.vertexShader,
                vertexID, 
                &payloads[vertexID], 
                pushConstants,
                viewport,
                zNear,
                zFar,
            };
            jobDecls[vertexID] = { 
                JOB_SYSTEM_JOB_ENTRY_POINT(LaunchVertexShaderExecution), 
                &vertexShaderExecuteJobData[vertexID]
            };
        }
        JobSystemAtomicCounterHandle vertexShaderExecuteJobCounter = JobSystem::RunJobs(jobDecls.data(), numVertices);
        JobSystem::WaitForCounterAndFreeWithoutFiber(vertexShaderExecuteJobCounter);

        // Rasterization stage
        std::vector<ExecuteTriangleRasterizationJobData> triangleRasterizeJobData(numPrimitives);
        jobDecls.resize(numPrimitives);
        for (uint32 primitiveID = 0; primitiveID < numPrimitives; primitiveID++)
        {
            triangleRasterizeJobData[primitiveID] = { 
                pipelineState.pixelShader,
                primitiveID, 
                &payloads[primitives[primitiveID].indices[0]], 
                &payloads[primitives[primitiveID].indices[1]],
                &payloads[primitives[primitiveID].indices[2]],
                pushConstants,
                &pipelineState,
                viewport,
            };
            jobDecls[primitiveID] = {
                JOB_SYSTEM_JOB_ENTRY_POINT(ExecuteTriangleRasterization),
                &triangleRasterizeJobData[primitiveID]
            };
            ExecuteTriangleRasterization(&triangleRasterizeJobData[primitiveID]);
        }
        //JobSystemAtomicCounterHandle triangleRasterizeJobCounter = JobSystem::RunJobs(jobDecls.data(), numPrimitives);
        //JobSystem::WaitForCounterAndFreeWithoutFiber(triangleRasterizeJobCounter);
    }

    void Rasterizer::SetViewport(float x, float y, float width, float height)
    {
        viewport = { x, y, width, height };
    }
}