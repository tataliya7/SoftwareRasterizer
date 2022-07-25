#include "Rasterizer.h"
#include "Shader.h"
#include "JobSystem.h"

#include "imgui/imgui.h"

namespace SR
{
    FORCEINLINE float PixelCoverage(const Vector2& a, const Vector2& b, const Vector2& c, bool ccw)
    {
        const float x = (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
        return ccw ? x : -x;
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
    
    struct LaunchVertexShaderExecutionJobData
    {
        VertexShader shader;
        uint32 vertexID;
        ShaderPayload* payload;
        const void* pushConstants;
        Matrix4x4 viewportTransform;
    };

    static void LaunchVertexShaderExecution(LaunchVertexShaderExecutionJobData* data)
    {
        data->shader.Main(data->vertexID, *data->payload, data->pushConstants);

        Vector4& SV_Target = data->payload->SV_Target;

        // Perspective division
        const float invW = 1.0f / SV_Target.w;
        data->payload->invW = invW;
        SV_Target *= invW; // NDC space

        // Perspective correction: the world space properties should be multipy by 1/w before rasterization
        data->payload->clipPosition *= invW;
        data->payload->worldPosition *= invW;
        data->payload->worldNormal *= invW;
        data->payload->worldTangent *= invW;
        data->payload->texCoord *= invW;

        auto& sp = data->payload->screenPosition;

        // Screen-mapping
        {
            // Viewport transform
            sp = data->viewportTransform * SV_Target;
        }
    }
    
    struct ExecuteTriangleRasterizationJobData
    {
        PixelShader shader;
        uint32 primitiveID;
        ShaderPayload* payload0;
        ShaderPayload* payload1;
        ShaderPayload* payload2;
        const void* pushConstants;
        const GraphicsPipelineState* pipelineState;
        Viewport viewport;
        Matrix4x4 viewportTransform;
    };

    static void ExecuteTriangleRasterization(ExecuteTriangleRasterizationJobData* data)
    {
        ShaderPayload* payload0 = data->payload0;
        ShaderPayload* payload1 = data->payload1;
        ShaderPayload* payload2 = data->payload2;

        Vector3& sp0 = payload0->screenPosition;
        Vector3& sp1 = payload1->screenPosition;
        Vector3& sp2 = payload2->screenPosition;

        // Back-face culling in screen space
        if (data->pipelineState->cullMode != CULL_MODE_NONE)
        {
            auto e1 = Vector2(sp2) - Vector2(sp0);
            auto e2 = Vector2(sp1) - Vector2(sp0);
            float orient = e1.x * e2.y - e1.y * e2.x;
            bool frontFacing = data->pipelineState->frontCCW ? orient > 0.0f : orient < 0.0f;
            if ((data->pipelineState->cullMode == CULL_MODE_BACK && !frontFacing) ||
                (data->pipelineState->cullMode == CULL_MODE_FRONT && frontFacing) ||
                (data->pipelineState->cullMode == CULL_MODE_FRONT_AND_BACK))
            {
                return;
            }
        }
        
        float xmin = std::floor(std::min(sp0.x, std::min(sp1.x, sp2.x)));
        float xmax = std::ceil(std::max(sp0.x, std::max(sp1.x, sp2.x)));
        float ymin = std::floor(std::min(sp0.y, std::min(sp1.y, sp2.y)));
        float ymax = std::ceil(std::max(sp0.y, std::max(sp1.y, sp2.y)));

        uint32 minx = std::max<uint32>(0, (int)xmin);
        uint32 maxx = std::min<uint32>((int)data->viewport.width - 1, (int)xmax);
        uint32 miny = std::max<uint32>(0, (int)ymin);
        uint32 maxy = std::min<uint32>((int)data->viewport.height - 1, (int)ymax);

        /*int minx = std::max(0, (int)std::floor(std::min(sp0.x, std::min(sp1.x, sp2.x))));
        int maxx = std::max(0, (int)std::floor(std::min(sp0.y, std::min(sp1.y, sp2.y))));
        int miny = std::min((int)data->viewport.width - 1, (int)std::ceil(std::max(sp0.x, std::max(sp1.x, sp2.x))));
        int maxy = std::min((int)data->viewport.height - 1, (int)std::ceil(std::max(sp0.y, std::max(sp1.y, sp2.y))));*/

        for (uint32 x = minx; x < maxx; x++)
        {
            for (uint32 y = miny; y < maxy; y++)
            {
                ASSERT(x < data->pipelineState->colorBuffer->GetWidth() && y < data->pipelineState->colorBuffer->GetHeight());

                Vector2 pixelCoord = { x + 0.5f, y + 0.5f };

                BarycentricCoordinates barycentric = CalculateBarycentric2D(pixelCoord.x, pixelCoord.y, sp0, sp1, sp2);
                if (!barycentric.IsInside())
                {
                    continue;
                }

                ShaderPayload payload;

                // Perspective-Correct Interpolation
                payload.invW = BarycentricLerp(payload0->invW, payload1->invW, payload2->invW, barycentric, 1.0f);
                const float w = 1.0f / payload.invW;

                const float depth = BarycentricLerp(sp0.z * payload0->invW, sp1.z * payload1->invW, sp2.z * payload2->invW, barycentric, w);
                payload.screenPosition = Vector3(pixelCoord.x, pixelCoord.y, depth);

                // Interpolate vertex attributes
                payload.clipPosition = BarycentricLerp(payload0->clipPosition, payload1->clipPosition, payload2->clipPosition, barycentric, w);
                payload.worldPosition = BarycentricLerp(payload0->worldPosition, payload1->worldPosition, payload2->worldPosition, barycentric, w);
                payload.worldNormal = BarycentricLerp(payload0->worldNormal, payload1->worldNormal, payload2->worldNormal, barycentric, w);
                payload.worldTangent = BarycentricLerp(payload0->worldTangent, payload1->worldTangent, payload2->worldTangent, barycentric, w);
                payload.texCoord = BarycentricLerp(payload0->texCoord, payload1->texCoord, payload2->texCoord, barycentric, w);

                Vector4 color = data->shader.Main(payload, data->pushConstants);

                // Depth testing
                if (data->pipelineState->depthTestEnable)
                {
                    float depthBufferValue = data->pipelineState->depthBuffer->Load(x, y);
                    if ((data->pipelineState->depthCompareOp == COMPARE_OP_LESS_OR_EQUAL) && (depth > depthBufferValue))
                    {
                        continue;
                    }
                    if ((data->pipelineState->depthCompareOp == COMPARE_OP_GREATER) && (depth <= depthBufferValue))
                    {
                        continue;
                    }
                }

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

    void Rasterizer::DrawPrimitives(const GraphicsPipelineState& pipelineState, const void* pushConstants, uint32 numVertices, const std::vector<Primitive>& primitives, uint32 numPrimitives)
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
                viewportTransform,
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
                viewportTransform
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
        viewport.x = x;
        viewport.y = y;
        viewport.width = width;
        viewport.height = height;

        float hwidth = width * 0.5f;
        float hheight = height * 0.5f;
        viewportTransform[0][0] = hwidth; viewportTransform[0][1] = 0.0f;     viewportTransform[0][2] = 0.0f; viewportTransform[0][3] = 0.0f;
        viewportTransform[1][0] = 0.0f;	  viewportTransform[1][1] = -hheight; viewportTransform[1][2] = 0.0f; viewportTransform[1][3] = 0.0f;
        viewportTransform[2][0] = 0.0f;   viewportTransform[2][1] = 0.0f;     viewportTransform[2][2] = 1.0f; viewportTransform[2][3] = 0.0f;
        viewportTransform[3][0] = hwidth; viewportTransform[3][1] = hheight;  viewportTransform[3][2] = 0.0f; viewportTransform[3][3] = 0.0f;
    }
}