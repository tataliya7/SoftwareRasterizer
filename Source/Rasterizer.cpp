#include "Rasterizer.h"
#include "Shader.h"
#include "JobSystem.h"

#include "imgui/imgui.h"

namespace SR
{
    bool ClipSpacaeCulling(const Vector4& a, const Vector4& b, const Vector4& c, float zNear, float zFar)
    {
        if (a.w < zNear && b.w < zNear && c.w < zNear)
        {
            return false;
        }
        if (a.w > zFar && b.w > zFar && c.w > zFar)
        {
            return false;
        }
        if (a.x > a.w && b.x > b.w && c.x > c.w)
        {
            return false;
        }
        if (a.x < -a.w && b.x < -b.w && c.x < -c.w)
        {
            return false;
        }
        if (a.y > a.w && b.y > b.w && c.y > c.w)
        {
            return false;
        }
        if (a.y < -a.w && b.y < -b.w && c.y < -c.w)
        {
            return false;
        }
        if (a.z > a.w && b.z > b.w && c.z > c.w)
        {
            return false;
        }
        if (a.z < -a.w && b.z < -b.w && c.z < -c.w)
        {
            return false;
        }
        return true;
    }

    struct BarycentricCoordinates
    {
        float alpha;
        float beta;
        float gamma;
        bool IsInsideTriangle() const
        {
            return alpha >= 0.0f && beta >= 0.0f && gamma >= 0.0f;
        }
    };
    
    static BarycentricCoordinates CalculateBarycentric2D(float x, float y, const Vector2& v0, const Vector2& v1, const Vector2& v2)
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

    void PerspectiveDivision(ShaderPayload* payload)
    {
        const float invW = 1.0f / payload->clipPosition.w;
        payload->invW = invW;
        payload->ndcPosition = Vector3(payload->clipPosition) * invW;
        // Perspective correction: the world space properties should be multipy by 1/w before rasterization
        payload->worldPosition *= invW;
        payload->worldNormal *= invW;
        payload->worldTangent *= invW;
        payload->texCoord *= invW;
    }

    struct VertexShaderJobData
    {
        VertexShader shader;
        uint32 vertexID;
        ShaderPayload* payload;
        const void* pushConstants;
    };

    static void LaunchVertexShaderExecution(VertexShaderJobData* data)
    {
        data->shader.Main(data->vertexID, *data->payload, data->pushConstants);
        // Perspective division
        PerspectiveDivision(data->payload);

    }

    struct PixelShaderJobData
    {
        BarycentricCoordinates barycentric;
        const ShaderPayload* payload[3];
        Vector3 screenPos[3];
        int x, y;
        float zNear, zFar;
        const GraphicsPipelineState* pipelineState;
        const void* pushConstants;
    };

    static void LauchPixelShaderExecution(PixelShaderJobData* data)
    {
        BarycentricCoordinates& barycentric = data->barycentric;
        ShaderPayload payload;

        Vector3 screenPos[3] = { data->screenPos[0], data->screenPos[1], data->screenPos[2] };

        // Perspective-Correct Interpolation
        payload.invW = BarycentricLerp(data->payload[0]->invW, data->payload[1]->invW, data->payload[2]->invW, barycentric, 1.0f);
        const float w = 1.0f / payload.invW;

        Vector3 bc_clip = Vector3(barycentric.alpha * data->payload[0]->invW, barycentric.beta * data->payload[1]->invW, barycentric.gamma * data->payload[2]->invW);
        bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);
        const float depth = glm::dot(Vector3(screenPos[0].z, screenPos[1].z, screenPos[2].z), bc_clip);
        //const float depth = glm::dot(Vector3(data->payload[0]->clipPosition.z, data->payload[1]->clipPosition.z, data->payload[2]->clipPosition.z), bc_clip);
        //const float depth = BarycentricLerp(data->payload[0]->clipPosition.z * data->payload[0]->invW, data->payload[1]->clipPosition.z * data->payload[1]->invW, data->payload[2]->clipPosition.z * data->payload[2]->invW, barycentric, 1.0f);
        
        // Interpolate vertex attributes
        payload.clipPosition = BarycentricLerp(data->payload[0]->clipPosition, data->payload[1]->clipPosition, data->payload[2]->clipPosition, barycentric, 1.0f);
        payload.worldPosition = BarycentricLerp(data->payload[0]->worldPosition, data->payload[1]->worldPosition, data->payload[2]->worldPosition, barycentric, w);
        payload.worldNormal = BarycentricLerp(data->payload[0]->worldNormal, data->payload[1]->worldNormal, data->payload[2]->worldNormal, barycentric, w);
        payload.worldTangent = BarycentricLerp(data->payload[0]->worldTangent, data->payload[1]->worldTangent, data->payload[2]->worldTangent, barycentric, w);
        payload.texCoord = BarycentricLerp(data->payload[0]->texCoord, data->payload[1]->texCoord, data->payload[2]->texCoord, barycentric, w);

        Vector4 color = data->pipelineState->pixelShader.Main(payload, data->pushConstants);

        // Depth testing
        if (data->pipelineState->depthTestEnable)
        {
            float depthBufferValue = data->pipelineState->depthBuffer->Load(data->x, data->y);
            if ((data->pipelineState->depthCompareOp == COMPARE_OP_LESS_OR_EQUAL) && (depth > depthBufferValue))
            {
                return;
            }
            if ((data->pipelineState->depthCompareOp == COMPARE_OP_GREATER) && (depth <= depthBufferValue))
            {
                return;
            }
        }

        if (data->pipelineState->shadowMap)
        {
            float d = BarycentricLerp(data->payload[0]->clipPosition.z, data->payload[1]->clipPosition.z, data->payload[2]->clipPosition.z, barycentric, 1.0f);
            //float d = depth;
            d = data->zNear * data->zFar / (data->zFar + d * (data->zNear - data->zFar));
            data->pipelineState->shadowMap->Store(data->x, data->y, d);
            return;
        }

        if (data->pipelineState->colorBuffer)
        {
            color = glm::clamp(color, 0.0f, 1.0f);
            glm::u8vec4 pixel = { (uint8)(color.x * 255.0f), (uint8)(color.y * 255.0f), (uint8)(color.z * 255.0f), (uint8)(color.w * 255.0f) };
            data->pipelineState->colorBuffer->Store(data->x, data->y, pixel);
        }
        // Depth writing
        if (data->pipelineState->depthWriteEnable)
        {
            data->pipelineState->depthBuffer->Store(data->x, data->y, depth);
        }
    }

    struct TriangleRasterizationJobData
    {
        PixelShader shader;
        uint32 primitiveID;
        ShaderPayload* payload0;
        ShaderPayload* payload1;
        ShaderPayload* payload2;
        const void* pushConstants;
        const GraphicsPipelineState* pipelineState;
        Viewport viewport;
        float zNear;
        float zFar;
    };

    static void ExecuteTriangleRasterization(TriangleRasterizationJobData* data)
    {
        ShaderPayload* payload0 = data->payload0;
        ShaderPayload* payload1 = data->payload1;
        ShaderPayload* payload2 = data->payload2;

        Vector4 clipPosition[3] = { data->payload0->clipPosition, data->payload1->clipPosition, data->payload2->clipPosition };
        Vector3 ndcPos[3] = { data->payload0->ndcPosition, data->payload1->ndcPosition, data->payload2->ndcPosition };

        if (!ClipSpacaeCulling(clipPosition[0], clipPosition[1], clipPosition[2], data->zNear, data->zFar))
        {
            return;
        }

        // Viewport transform
        Vector3 screenPos[3];
        for (uint32 i = 0; i < 3; i++)
        {
            screenPos[i].x = (data->viewport.width * 0.5f) * (1.0f + ndcPos[i].x) + data->viewport.x;
            screenPos[i].y = (data->viewport.height * 0.5f) * (1.0f + ndcPos[i].y) + data->viewport.y;
            screenPos[i].z = (data->viewport.maxDepth - data->viewport.minDepth) * ndcPos[i].z + data->viewport.minDepth;
        }

        // Back-face culling in screen space
        if (data->pipelineState->cullMode != CULL_MODE_NONE)
        {
            auto e1 = Vector2(screenPos[2]) - Vector2(screenPos[0]);
            auto e2 = Vector2(screenPos[1]) - Vector2(screenPos[0]);
            float orient = e1.x * e2.y - e1.y * e2.x;
            bool frontFacing = data->pipelineState->frontCCW ? orient > 0.0f : orient < 0.0f;
            if ((data->pipelineState->cullMode == CULL_MODE_BACK && !frontFacing) ||
                (data->pipelineState->cullMode == CULL_MODE_FRONT && frontFacing) ||
                (data->pipelineState->cullMode == CULL_MODE_FRONT_AND_BACK))
            {
                return;
            }
        }
        
        int minx = std::clamp(std::min((int)screenPos[0].x, std::min((int)screenPos[1].x, (int)screenPos[2].x)), (int)data->viewport.x, (int)data->viewport.width  - 1);
        int maxx = std::clamp(std::max((int)screenPos[0].x, std::max((int)screenPos[1].x, (int)screenPos[2].x)), (int)data->viewport.x, (int)data->viewport.width  - 1);
        int miny = std::clamp(std::min((int)screenPos[0].y, std::min((int)screenPos[1].y, (int)screenPos[2].y)), (int)data->viewport.y, (int)data->viewport.height - 1);
        int maxy = std::clamp(std::max((int)screenPos[0].y, std::max((int)screenPos[1].y, (int)screenPos[2].y)), (int)data->viewport.y, (int)data->viewport.height - 1);

#pragma omp parallel for
        for (int x = minx; x <= maxx; x++)
        {
            for (int y = miny; y <= maxy; y++)
            {
                BarycentricCoordinates barycentric = CalculateBarycentric2D((float)x, (float)y, screenPos[0], screenPos[1], screenPos[2]);
                if (!barycentric.IsInsideTriangle())
                {
                    continue;
                }

                PixelShaderJobData pixelShaderJobData = {
                    barycentric,
                    { payload0, payload1, payload2 },
                    { screenPos[0], screenPos[1], screenPos[2] },
                    x, y,
                    data->zNear, data->zFar,
                    data->pipelineState,
                    data->pushConstants
                };
                LauchPixelShaderExecution(&pixelShaderJobData);
            }
        }
    }

    void Rasterizer::DrawPrimitives(const GraphicsPipelineState& pipelineState, const void* pushConstants, uint32 numVertices, const std::vector<Primitive>& primitives, uint32 numPrimitives, float zNear, float zFar)
    {
        payloads.resize(numVertices);

        std::vector<JobDecl> jobDecls;

        // Launch vertex shaders
        std::vector<VertexShaderJobData> vertexShaderExecuteJobData(numVertices);
        jobDecls.resize(numVertices);
        for (uint32 vertexID = 0; vertexID < numVertices; vertexID++) 
        {
            vertexShaderExecuteJobData[vertexID] = {
                pipelineState.vertexShader,
                vertexID, 
                &payloads[vertexID], 
                pushConstants
            };
            jobDecls[vertexID] = { 
                JOB_SYSTEM_JOB_ENTRY_POINT(LaunchVertexShaderExecution), 
                &vertexShaderExecuteJobData[vertexID]
            };
        }
        JobSystemAtomicCounterHandle vertexShaderExecuteJobCounter = JobSystem::RunJobs(jobDecls.data(), numVertices);
        JobSystem::WaitForCounterAndFreeWithoutFiber(vertexShaderExecuteJobCounter);

        // Rasterization stage
        std::vector<TriangleRasterizationJobData> triangleRasterizeJobData(numPrimitives);
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
                zNear,
                zFar
            };
            jobDecls[primitiveID] = {
                JOB_SYSTEM_JOB_ENTRY_POINT(ExecuteTriangleRasterization),
                &triangleRasterizeJobData[primitiveID]
            };
            ExecuteTriangleRasterization(&triangleRasterizeJobData[primitiveID]);
        }
        JobSystemAtomicCounterHandle triangleRasterizeJobCounter = JobSystem::RunJobs(jobDecls.data(), numPrimitives);
        JobSystem::WaitForCounterAndFreeWithoutFiber(triangleRasterizeJobCounter);
    }

    void Rasterizer::SetViewport(float x, float y, float width, float height)
    {
        viewport.x = x;
        viewport.y = y;
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
    }
}