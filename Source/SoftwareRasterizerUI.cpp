#include "SoftwareRasterizerApp.h"
#include "GUI.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>

int gizmoOperationType = ImGuizmo::OPERATION::TRANSLATE;

void BeginDockSpace()
{
	static bool dockSpaceOpen = true;

	// Imgui dock node flags.
	static ImGuiDockNodeFlags dockNodeflags = ImGuiDockNodeFlags_PassthruCentralNode;

	// Imgui window flags.
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

	static bool isFullscreenPersistant = true;
	bool isFullscreen = isFullscreenPersistant;
	if (isFullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	windowFlags |= ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	// When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
	ImGui::Begin("Dockspace Demo", &dockSpaceOpen, windowFlags);
	ImGui::PopStyleVar();

	if (isFullscreen)
	{
		ImGui::PopStyleVar(2);
	}

	// Set min width
	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();
	float minWinSizeX = style.WindowMinSize.x;
	style.WindowMinSize.x = 300.0f;
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockSpaceID = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockSpaceID, ImVec2(0.0f, 0.0f), dockNodeflags);
	}
	style.WindowMinSize.x = minWinSizeX;
}

void EndDockSpace()
{
	ImGui::End();
}

float GetSnapValue()
{
	switch (gizmoOperationType)
	{
	case ImGuizmo::OPERATION::TRANSLATE: return 5.0f;
	case ImGuizmo::OPERATION::ROTATE: return 10.0f;
	case ImGuizmo::OPERATION::SCALE: return 0.1f;
	}
	return 0.0f;
}

void DrawOverlay()
{
	static int corner = 0;
	ImGuiIO& io = ImGui::GetIO();
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	if (corner != -1)
	{
		const float PAD = 10.0f;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
		ImVec2 work_size = viewport->WorkSize;
		ImVec2 window_pos, window_pos_pivot;
		window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
		window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
		window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
		window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
		ImGui::SetNextWindowViewport(viewport->ID);
		window_flags |= ImGuiWindowFlags_NoMove;
	}
	ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
	static bool p_open = true;
	if (ImGui::Begin("Overlay", &p_open, window_flags))
	{
		ImGui::Text("Software rasterizer");
		ImGui::Separator();
		ImGui::Text("FPS: %.1f (%.2f ms/frame)", ImGui::GetIO().Framerate, (1000.0f / ImGui::GetIO().Framerate));
	}
	ImGui::End();
}

namespace SR
{
	static bool showTransformManipulater = true;
	static uint32 currentObject = 0;
	static bool showGrid = false;

	void SoftwareRasterizerApp::OnImGui()
    {
		BeginDockSpace();

		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			auto flags = ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
			ImGui::SetNextWindowBgAlpha(0.0f);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
			bool viewportActive = true;
			ImGui::Begin("Viewport", &viewportActive, flags);

			ImGuiIO& io = ImGui::GetIO();
			ImGui::Image(GetSceneColorTextureID(), viewport->WorkSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));

			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(viewport->WorkPos.x, viewport->WorkPos.y, viewport->WorkSize.x, viewport->WorkSize.y);
			
			if (showTransformManipulater)
			{
				Transform& transform = currentObject == 0 ? modelTransform : floorTransform;
				Matrix4x4 world = transform.world;
				ImGuizmo::Manipulate(glm::value_ptr(perFrameData.viewMatrix),
					glm::value_ptr(perFrameData.projectionMatrix),
					(ImGuizmo::OPERATION)gizmoOperationType,
					ImGuizmo::LOCAL,
					glm::value_ptr(world),
					nullptr,
					nullptr);

				ImGuizmo::DecomposeMatrixToComponents(
					glm::value_ptr(world), 
					glm::value_ptr(transform.position),
					glm::value_ptr(transform.rotation),
					glm::value_ptr(transform.scale));
			}

			if (showGrid)
			{
				ImGuizmo::DrawGrid(glm::value_ptr(perFrameData.viewMatrix),
					glm::value_ptr(perFrameData.projectionMatrix),
					glm::value_ptr(Matrix4x4(1.0f)),
					10.0f);
			}

			ImGui::End();
			ImGui::PopStyleVar();
		}
		{
			ImGui::Begin("Settings");
			{
				/*const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				float lineHeight = ImGui::GetCurrentContext()->Font->FontSize + ImGui::GetCurrentContext()->Style.FramePadding.y * 2.0f;
				bool open = ImGui::TreeNodeEx((void*)typeid(UIFunction).hash_code(), treeNodeFlags, name);
				ImGui::PopStyleVar();
				ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
				if (open)
				{
					uiFunction();
					ImGui::TreePop();
				}*/

				if(ImGui::CollapsingHeader("User Settings"))
				{
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
					ImGui::Columns(1);
					ImGui::Separator();
					
					ImGui::Checkbox("Show Transform Manipulater", &showTransformManipulater);
					ImGui::Checkbox("Show Grid", &showGrid);
					
					static const char* debugViewNames[] = {
						"None",
						"Wolrd Position",
						"Wolrd Normal",
						"Base Color",
						"Metallic",
						"Roughness",
						"Depth"
					};
					int idx = 0;
					if (ImGui::BeginCombo("Debug View", debugViewNames[perFrameData.debugView]))
					{
						for (int n = 0; n < IM_ARRAYSIZE(debugViewNames); n++)
						{
							if (ImGui::Selectable(debugViewNames[n], idx == n))
							{
								idx = n;
								perFrameData.debugView = (DebugView)idx;
							}
						}
						ImGui::EndCombo();
					}
					ImGui::Separator();
					ImGui::PopStyleVar();
				}

				if (ImGui::CollapsingHeader("Camera"))
				{
					ImGui::Separator();

					ImGui::Text("Tip: Press left-shitf to boost camera speed");

					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
					ImGui::Columns(2);
					ImGui::Separator();

					ImGui::AlignTextToFramePadding();
					ImGui::TextUnformatted("Position");
					ImGui::NextColumn();
					ImGui::PushItemWidth(-1);
					ImGui::DragFloat3("##Position", &camera.position.x, 0.1f);
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::TextUnformatted("Rotation");
					ImGui::NextColumn();
					ImGui::PushItemWidth(-1);
					ImGui::DragFloat3("##Rotation", &camera.euler.x, 0.1f);
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::TextUnformatted("Exposure");
					ImGui::NextColumn();
					ImGui::PushItemWidth(-1);
					ImGui::DragFloat("##Exposure", &perFrameData.exposure, 0.1f, 0.1f);
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					ImGui::Columns(1);
					ImGui::Separator();
					ImGui::PopStyleVar();
				}

				if (ImGui::CollapsingHeader("Directional Light"))
				{
					ImGui::Separator();
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
					ImGui::Columns(2);
					ImGui::Separator();

					ImGui::AlignTextToFramePadding();
					ImGui::TextUnformatted("Direction");
					ImGui::NextColumn();
					ImGui::PushItemWidth(-1);
					ImGui::DragFloat3("##Direction", &light.direction.x, 0.1f);
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::TextUnformatted("Color");
					ImGui::NextColumn();
					ImGui::PushItemWidth(-1);
					ImGui::DragFloat3("##Color", &light.color.x, 0.1f);
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::TextUnformatted("Intensity");
					ImGui::NextColumn();
					ImGui::PushItemWidth(-1);
					ImGui::DragFloat("##Intensity", &light.intensity, 0.1f, 0.0f);
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					ImGui::Columns(1);
					ImGui::Separator();
					ImGui::PopStyleVar();
				}

				if (ImGui::CollapsingHeader("Model"))
				{
					ImGui::Separator();
					if (ImGui::TreeNode("Transform"))
					{
						ImGui::Text("Transform Manipulate");
						ImGui::SameLine();
						if (ImGui::Button("Translate"))
						{
							showTransformManipulater = true;
							currentObject = 0;
							gizmoOperationType = ImGuizmo::OPERATION::TRANSLATE;
						}
						ImGui::SameLine();
						if (ImGui::Button("Rotate"))
						{
							showTransformManipulater = true;
							currentObject = 0;
							gizmoOperationType = ImGuizmo::OPERATION::ROTATE;
						}
						ImGui::SameLine();
						if (ImGui::Button("Scale"))
						{
							showTransformManipulater = true;
							currentObject = 0;
							gizmoOperationType = ImGuizmo::OPERATION::SCALE;
						}

						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
						ImGui::Columns(2);
						ImGui::Separator();

						ImGui::AlignTextToFramePadding();
						ImGui::TextUnformatted("Position");
						ImGui::NextColumn();
						ImGui::PushItemWidth(-1);
						ImGui::DragFloat3("##Position", &floorTransform.position.x, 0.1f);
						ImGui::PopItemWidth();
						ImGui::NextColumn();

						ImGui::AlignTextToFramePadding();
						ImGui::TextUnformatted("Rotation");
						ImGui::NextColumn();
						ImGui::PushItemWidth(-1);
						ImGui::DragFloat3("##Rotation", &floorTransform.rotation.x, 0.1f);
						ImGui::PopItemWidth();
						ImGui::NextColumn();

						ImGui::AlignTextToFramePadding();
						ImGui::TextUnformatted("Scale");
						ImGui::NextColumn();
						ImGui::PushItemWidth(-1);
						ImGui::DragFloat3("##Scale", &modelTransform.scale.x, 0.1f);
						ImGui::PopItemWidth();
						ImGui::NextColumn();

						ImGui::Columns(1);
						ImGui::Separator();
						ImGui::PopStyleVar();

						ImGui::TreePop();
					}

					if (ImGui::TreeNode("Material"))
					{
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
						ImGui::Columns(2);
						ImGui::Separator();

						ImGui::AlignTextToFramePadding();
						ImGui::TextUnformatted("Base Color");
						ImGui::NextColumn();
						ImGui::PushItemWidth(-1);
						ImGui::DragFloat4("##Base Color", &model.material.baseColor.x, 0.1f);
						ImGui::PopItemWidth();
						ImGui::NextColumn();

						ImGui::AlignTextToFramePadding();
						ImGui::TextUnformatted("Metallic");
						ImGui::NextColumn();
						ImGui::PushItemWidth(-1);
						ImGui::DragFloat("##Metallic", &model.material.metallic, 0.01f, 0.0f);
						ImGui::PopItemWidth();
						ImGui::NextColumn();

						ImGui::AlignTextToFramePadding();
						ImGui::TextUnformatted("Roughness");
						ImGui::NextColumn();
						ImGui::PushItemWidth(-1);
						ImGui::DragFloat("##Roughness", &model.material.roughness, 0.01f, 0.0f);
						ImGui::PopItemWidth();
						ImGui::NextColumn();

						ImGui::Separator();
						ImGui::PopStyleVar();
						
						ImGui::TreePop();
					}
				}

				ImGui::End();
			}
		}

		DrawOverlay();

		EndDockSpace();
    }
}