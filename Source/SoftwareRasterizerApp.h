#pragma once

#include "SRCommon.h"
#include "SRMath.h"
#include "CameraController.h"
#include "Rasterizer.h"
#include "Scene.h"
#include "Shaders/ShaderCommon.h"
#include "Shaders/PBRShader.h"
#include "Shaders/ShadowMapShader.h"

#define APPLICATION_NAME "Software Rasterizer"

namespace SR
{

    class SRWindow;
    class Rasterizer;
    class SoftwareRasterizerApp
    {
    public:

    	static SoftwareRasterizerApp* Instance;

        static SoftwareRasterizerApp* GetInstance()
        {
            return Instance;
        }

        SoftwareRasterizerApp();
        ~SoftwareRasterizerApp();

        SoftwareRasterizerApp(const SoftwareRasterizerApp&) = delete;
	    SoftwareRasterizerApp& operator=(const SoftwareRasterizerApp&) = delete;

        bool Init();
        void Exit();
        int Run();

        void Update(float deltaTime);
        void Render();

        void ShadowPass();

        bool IsExitRequest() const
        {
            return isExitRequested;
        }

        void OnImGui();
        
    private:

        SRWindow* window;

        bool isExitRequested;
        std::string currentDir;

        uint64 frameCounter;

        Camera camera;
        SimpleFirstPersonCameraController cameraController;
        
        int modelID = 0;
        Mesh model;
        Transform modelTransform;
        Mesh floor;
        Transform floorTransform;
        DirectionalLight light;

        GraphicsPipelineState pipelineState0;
        GraphicsPipelineState pipelineState1;
        GraphicsPipelineState pipelineState2;
        RenderTarget<glm::u8vec4>* sceneColor;
        RenderTarget<float>* depthBuffer;
        RenderTarget<float>* shadowMap;
        uint32 shadowMapSize;

        Rasterizer* rasterizer;
        PerFrameData perFrameData;

        DebugView debugView;

        bool renderShadow = false;
    };
}

extern int SoftwareRasterizerMain();
