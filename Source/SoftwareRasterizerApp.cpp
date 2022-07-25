#include "SoftwareRasterizerApp.h"
#include "WindowSystem.h"
#include "Logging.h"
#include "Input.h"
#include "GUI.h"

namespace SR
{
    SoftwareRasterizerApp* SoftwareRasterizerApp::Instance = nullptr;

    SoftwareRasterizerApp::SoftwareRasterizerApp()
        : window(nullptr)
        , isExitRequested(false)
        , frameCounter(0)
    {
        ASSERT(!Instance);
        Instance = this;
    }

    SoftwareRasterizerApp::~SoftwareRasterizerApp()
    {
        ASSERT(Instance);
        Instance = nullptr;
    }

    bool SoftwareRasterizerApp::Init()
    {
        if (!SR::WindowSystemInit())
        {
            SR_LOG_ERROR("Failed to init window system.");
            return false;
        }

        // Create main window
        SRWindowCreateInfo windowInfo = {};
        windowInfo.width = INITIAL_WINDOW_WIDTH;
        windowInfo.height = INITIAL_WINDOW_HEIGHT;
        windowInfo.title = APPLICATION_NAME;

        window = new SRWindow(&windowInfo);

        Input::SetCurrentContext(window->GetGLFWwindow());

        if (!ImGuiInit(window->GetGLFWwindow()))
        {
            SR_LOG_ERROR("Failed to init imgui.");
            return false;
        }
        
        char dir[MAX_PATH];
        _getcwd(dir, MAX_PATH);
        std::string currentDir(dir);
        ImportGLTF2((currentDir + "/../../Assets/DamagedHelmet/glTF/DamagedHelmet.gltf").c_str(), &model);
        ImportGLTF2((currentDir + "/../../Assets/floor/floor.gltf").c_str(), &floor);
        
        camera.euler = Vector3(0.0f, 0.0f, 0.0f);
        camera.position = Vector3(0.0f, 0.0f, 5.0f);
        camera.fieldOfView = 60.0f;
        camera.zNear = 0.03f;
        camera.zFar = 1000.0f;

        light.color = Vector3(1.0f, 1.0f, 1.0f);
        light.direction = Vector3(1.0f, -1.0f, 1.0f);
        light.intensity = 1.0f;

        modelTransform.position = Vector3(0.0f, 0.0f, 0.0f);
        modelTransform.rotation = Vector3(78.0f, 20.0f, 0.0f);
        modelTransform.scale = Vector3(1.0f, 1.0f, 1.0f);

        floorTransform.position = Vector3(0.0f, 0.0f, 0.0f);
        floorTransform.rotation = Vector3(0.0f, 0.0f, 0.0f);
        floorTransform.scale = Vector3(1.0f, 1.0f, 1.0f);

        rasterizer = new Rasterizer();

        sceneColor = new RenderTarget<glm::u8vec4>(1, 1);
        depthBuffer = new RenderTarget<float>(1, 1);

        VertexShader pbrVertexShader;
        pbrVertexShader.Main = PBRMainVS;
        PixelShader pbrPixelShader;
        pbrPixelShader.Main = PBRMainPS;

        pipelineState0.vertexShader = pbrVertexShader;
        pipelineState0.pixelShader = pbrPixelShader;
        pipelineState0.fillMode = FILL_MODE_SOLID;
        pipelineState0.cullMode = CULL_MODE_BACK;
        pipelineState0.frontCCW = true;
        pipelineState0.depthTestEnable = true;
        pipelineState0.depthWriteEnable = true;
        pipelineState0.depthCompareOp = COMPARE_OP_LESS_OR_EQUAL;
        pipelineState0.colorBuffer = sceneColor;
        pipelineState0.depthBuffer = depthBuffer;

        pipelineState1.vertexShader = pbrVertexShader;
        pipelineState1.pixelShader = pbrPixelShader;
        pipelineState1.fillMode = FILL_MODE_SOLID;
        pipelineState1.cullMode = CULL_MODE_NONE;
        pipelineState1.frontCCW = true;
        pipelineState1.depthTestEnable = true;
        pipelineState1.depthWriteEnable = true;
        pipelineState1.depthCompareOp = COMPARE_OP_LESS_OR_EQUAL;
        pipelineState1.colorBuffer = sceneColor;
        pipelineState1.depthBuffer = depthBuffer;

        pipelineState2.vertexShader = { ShaderMapShaderMainVS };
        pipelineState2.pixelShader = { ShaderMapShaderMainPS };
        pipelineState2.fillMode = FILL_MODE_SOLID;
        pipelineState2.cullMode = CULL_MODE_NONE;
        pipelineState2.frontCCW = true;
        pipelineState2.depthTestEnable = true;
        pipelineState2.depthWriteEnable = true;
        pipelineState2.depthCompareOp = COMPARE_OP_LESS_OR_EQUAL;
        pipelineState2.colorBuffer = nullptr;
        pipelineState2.depthBuffer = shadowMap;

        perFrameData.gamma = 2.2f;
        perFrameData.exposure = 1.4f;
        perFrameData.debugView = DEBUG_VIEW_NONE;

        shadowMapSize = 512;
        shadowMap = new RenderTarget<float>(shadowMapSize, shadowMapSize);
        shadowMap->Resize(shadowMapSize, shadowMapSize);

        return true;
    }

    void SoftwareRasterizerApp::Exit()
    {
        delete rasterizer;
        delete sceneColor;
        delete depthBuffer;
        delete shadowMap;

        ImGuiExit();
        if (window)
        {
            delete window;
        }
	    SR::WindowSystemExit();
    }

    void SoftwareRasterizerApp::Update(float deltaTime)
    {
        modelTransform.world = Math::Compose(modelTransform.position, Quaternion(Math::DegreesToRadians(modelTransform.rotation)), modelTransform.scale);
        floorTransform.world = Math::Compose(floorTransform.position, Quaternion(Math::DegreesToRadians(floorTransform.rotation)), floorTransform.scale);
        
		cameraController.Update(deltaTime, camera.position, camera.euler);

        camera.aspectRatio = ((float)window->GetWidth()) / window->GetHeight();
        perFrameData.invViewMatrix = Math::Compose(camera.position, Quaternion(Math::DegreesToRadians(camera.euler)), Vector3(1.0f, 1.0f, 1.0f));
        perFrameData.viewMatrix = Math::Inverse(perFrameData.invViewMatrix);
        perFrameData.projectionMatrix = glm::perspective(Math::DegreesToRadians(camera.fieldOfView), camera.aspectRatio, camera.zNear, camera.zFar);
        perFrameData.viewProjectionMatrix = perFrameData.projectionMatrix * perFrameData.viewMatrix;
        perFrameData.invViewProjectionMatrix = perFrameData.invViewMatrix * perFrameData.invProjectionMatrix;
        perFrameData.cameraPosition = camera.position;
        perFrameData.mainLightIntensity = light.intensity;
        perFrameData.mainLightColor = light.color;
        perFrameData.mainLightDirection = light.direction;

        light.Update();

        ImGuiBeginFrame();
        OnImGui();
    }

    void SoftwareRasterizerApp::ShadowPass()
    {
        shadowMap->Clear(1.0f);

        SMShaderPushConstants pc;
        rasterizer->SetViewport(0.0f, 0.0f, (float)shadowMapSize, (float)shadowMapSize);
        
        pc.vertices = model.positions.data();
        pc.mvp = light.vp * modelTransform.world;
        rasterizer->DrawPrimitives(pipelineState2, &pc, model.numVertices, model.primitives, model.numPrimitives);

        pc.vertices = floor.positions.data();
        pc.mvp = light.vp * floorTransform.world;
        rasterizer->DrawPrimitives(pipelineState2, &pc, floor.numVertices, floor.primitives, floor.numPrimitives);
    }

    void SoftwareRasterizerApp::Render()
    {
        // Resize framebuffer if needed
        uint32 displayWidth = window->GetWidth();
        uint32 displayHeight = window->GetHeight();
        sceneColor->Resize(displayWidth, displayHeight);
        depthBuffer->Resize(displayWidth, displayHeight);
        
        PBRShaderPushConstants pushConstantBlock0;
        pushConstantBlock0.positions = model.positions.data();
        pushConstantBlock0.normals = model.normals.data();
        pushConstantBlock0.tangents = model.tangents.data();
        pushConstantBlock0.texCoords = model.texCoords.data();
        pushConstantBlock0.worldMatrix = &modelTransform.world;
        pushConstantBlock0.perFrameData = &perFrameData;
        pushConstantBlock0.material = &model.material;
        pushConstantBlock0.lightMatrix = &light.vp;
        pushConstantBlock0.shadowMap = nullptr;
        //pushConstantBlock0.shadowMap = shadowMap;

        PBRShaderPushConstants pushConstantBlock1;
        pushConstantBlock1.positions = floor.positions.data();
        pushConstantBlock1.normals = floor.normals.data();
        pushConstantBlock1.tangents = floor.tangents.data();
        pushConstantBlock1.texCoords = floor.texCoords.data();
        pushConstantBlock1.worldMatrix = &floorTransform.world;
        pushConstantBlock1.perFrameData = &perFrameData;
        pushConstantBlock1.material = &floor.material;
        pushConstantBlock1.lightMatrix = &light.vp;
        pushConstantBlock1.shadowMap = nullptr;
        //pushConstantBlock1.shadowMap = shadowMap;

        // Clear render target
        sceneColor->Clear(glm::u8vec4(1, 1, 1, 1));
        depthBuffer->Clear(1.0f);

        if (renderShadow)
        {
            // ShadowPass();
        }

        rasterizer->SetViewport(0.0f, 0.0f, (float)displayWidth, (float)displayHeight);
        rasterizer->DrawPrimitives(pipelineState0, &pushConstantBlock0, model.numVertices, model.primitives, model.numPrimitives);
        rasterizer->DrawPrimitives(pipelineState1, &pushConstantBlock1, floor.numVertices, floor.primitives, floor.numPrimitives);

        UpdateSceneColorTexture(displayWidth, displayHeight, sceneColor->GetDataPtr());
        
        ImGuiEndFrame();
        ImGuiRender();
    }

    int SoftwareRasterizerApp::Run()
    {
        while (!IsExitRequest())
        {
            window->ProcessEvents();

            if (window->ShouldClose())
            {
                isExitRequested = true;
            }

            SRWindowState state = window->GetState();

            if (state == SRWindowState::Minimized)
            {
                continue;
            }

            static std::chrono::steady_clock::time_point previousTimePoint { std::chrono::steady_clock::now() };
            std::chrono::steady_clock::time_point timePoint = std::chrono::steady_clock::now();
            std::chrono::duration<float> timeDuration = std::chrono::duration_cast<std::chrono::duration<float>>(timePoint - previousTimePoint);
            float deltaTime = timeDuration.count();
            previousTimePoint = timePoint;

            Update(deltaTime);
            
            Render();

            frameCounter++;
        }

        return 0;
    }
}

int SoftwareRasterizerMain()
{
    int exitCode = EXIT_SUCCESS;
    SR::SoftwareRasterizerApp* app = new SR::SoftwareRasterizerApp();
    bool result = app->Init();
    if (result)
    {
        exitCode = app->Run();
    }
    else
    {
        exitCode = EXIT_FAILURE;
    }
    app->Exit();
    delete app;
    return exitCode;
}
