#pragma once

#include "SRCommon.h"
#include "SRMath.h"
#include "Shader.h"
#include "Texture.h"

namespace SR
{
    enum FillMode
    {
        FILL_MODE_SOLID          = 0,
        FILL_MODE_WIREFRAME      = 1,
    };

    enum CullMode
    {
        CULL_MODE_NONE           = 0,
        CULL_MODE_BACK           = 1,
        CULL_MODE_FRONT          = 2,
        CULL_MODE_FRONT_AND_BACK = 3,
    };

    enum CompareOp
    {
        COMPARE_OP_LESS_OR_EQUAL = 0,
        COMPARE_OP_GREATER       = 1,
    };

    struct GraphicsPipelineState
    {
        // Shaders
        VertexShader vertexShader;
        PixelShader pixelShader;
        // Rasterization state
        FillMode fillMode;
        CullMode cullMode;
        bool frontCCW;
        // Depth state
        bool depthTestEnable;
        bool depthWriteEnable;
        CompareOp depthCompareOp;
        RenderTarget<float>* depthBuffer;
        RenderTarget<glm::u8vec4>* colorBuffer;
        RenderTarget<float>* shadowMap = nullptr;
    };

    //struct RasterizerStatatics
    //{
    //    uint32 numVertices;
    //};

    struct Viewport
    {
        float x;
        float y;
        float width;
        float height;
        float minDepth;
        float maxDepth;
    };

    class Rasterizer
    {
    public:
        std::vector<ShaderPayload> payloads;
        void SetViewport(float x, float y, float width, float height); 
        void DrawPrimitives(const GraphicsPipelineState& pipelineState, const void* pushConstants, uint32 numVertices, const std::vector<Primitive>& primitives, uint32 numPrimitives, float zNear, float zFar);
    private:
        Viewport viewport;
    };
}