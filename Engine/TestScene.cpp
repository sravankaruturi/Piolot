﻿#include "TestScene.h"
#include "AssetManager.h"
#include "Window.h"
#include <glm/gtc/matrix_transform.inl>
#include "external_files/ImGUI/imgui.h"

namespace piolot {

	static void test_scene_resize(GLFWwindow * _window, int )
	{
		
	}

	std::string Vec3ToString(glm::vec3 _in)
	{
		return "(" + std::to_string(_in.x) + ", " + std::to_string(_in.y) + ", " + std::to_string(_in.z) + ")";
	}

	TestScene::TestScene(std::shared_ptr<Window> _window)
		: Scene(_window)
	{

		auto w2 = window->GetWidth() / 2;
		auto h2 = window->GetHeight() / 2;


		PE_GL(glViewportIndexedf(0, 0, 0, w2, h2));
		PE_GL(glViewportIndexedf(1, w2, 0, w2, h2));
		PE_GL(glViewportIndexedf(2, 0, h2, w2, h2));
		PE_GL(glViewportIndexedf(3, w2, h2, w2, h2));

		PE_GL(glEnable(GL_SCISSOR_TEST));

		ASMGR.ClearAllData();

		ASMGR.LoadShaders();
		ASMGR.LoadTextures();

		cameras.push_back(std::make_shared<Camera>("First", glm::vec3(0, 0, 10), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)));
		cameras.push_back(std::make_shared<Camera>("Second", glm::vec3(10, 0, 10), glm::vec3(-1, 0, -1), glm::vec3(0, 1, 0)));

		entities.push_back(std::make_shared<Entity>("lowpolytree/lowpolytree.obj", "good_test"));

		ActiveCamera(cameras[0]);

		// We need to wait for the Shaders to be loaded to call this function.
		testGrid.Init();
		
		//terrain_test = std::make_shared<Terrain>(10, 10, 0.5, 0.5, "Assets/Textures/heightmap.jpg");
		testTerrain = std::make_shared<Terrain>(10, 10, 0.5, 0.5, "Assets/Textures/heightmap.jpg");

	}

	void TestScene::OnUpdate(float _deltaTime, float _totalTime)
	{

		const auto projection_matrix = glm::perspective(45.0f, float(window->GetWidth()) / window->GetHeight(), 0.1f, 100.0f);

		for (auto& it : cameras) {
			it->UpdateVectors();
		}

		for (const auto& it : entities) {
			it->Update(_deltaTime);
		}

		glm::vec3 mouse_pointer_ray = activeCamera->GetMouseRayDirection(window->mouseX, window->mouseY, window->GetWidth(), window->GetHeight(), projection_matrix);

		/* Ray Picking */
		{
			float int_distance = 0;
			float min_int_distance = 10000.0f;
			// Do Ray Picking Here.
			// For each Bounding Box, we check for the collision, and do what we want, as part of the Scene Update.

			// We reset this every frame.
			std::shared_ptr<Entity> selected_entity;
			// Loop through all Entities that can be selected.
			for (auto it : entities)
			{
				if (it->CheckIfMouseOvered(this->activeCamera->GetPosition(), mouse_pointer_ray, min_int_distance))
				{
					if (int_distance < min_int_distance)
					{
						min_int_distance = int_distance;
						selected_entity = it;
					}
				}
				it->SetSelectedInScene(false);
			}

			if (nullptr != selected_entity)
			{
				selected_entity->SetSelectedInScene(true);
			}

		}

		/* Find Random Paths */
		{
			const float interval = 5;

			testTerrain->ClearColours();

			glm::vec3 startPosition = glm::vec3(startxz.x, 0, startxz.y);
			glm::vec3 endPosition = glm::vec3(endxz.x, 0, endxz.y);

			// Get the node pos.
			const glm::vec2 test_get_node = testTerrain->GetNodeIndicesFromPos(startPosition.x, startPosition.z);

			//LOGGER.AddToLog(
			//	"Random Point Selected: " + std::to_string(startPosition.x) + " , " + std::to_string(startPosition.z) + " "
			//	"The node returned is: " + std::to_string(test_get_node.x) + ", " + std::to_string(test_get_node.y)
			//);

			testTerrain->HighlightNode(test_get_node.x, test_get_node.y);

			startPosition.y = testTerrain->GetHeightAtPos(startPosition.x, startPosition.z);
			endPosition.y = testTerrain->GetHeightAtPos(endPosition.x, endPosition.z);

			glm::vec2 test_end_node = testTerrain->GetNodeIndicesFromPos(endPosition.x, endPosition.z);

			testTerrain->HighlightNode(test_end_node.x, test_end_node.y);

			path = testTerrain->GetPathFromPositions(startPosition, endPosition);

			std::string log_temp = "The path b/w the tiles, ";

			log_temp += Vec3ToString(startPosition) + " and " + Vec3ToString(endPosition) + " has " + std::to_string(path.size()) + " nodes";

			for ( auto it : path)
			{
				testTerrain->HighlightNode(it->tileIndexX, it->tileIndexZ);
			}

		}

		testTerrain->Update(_deltaTime, _totalTime);

		testGrid.Update(activeCamera->GetViewMatrix(), projection_matrix);
	}

	void TestScene::OnRender()
	{
		const auto persp_projection_matrix = glm::perspective(45.0f, float(window->GetWidth()) / window->GetHeight(), 0.1f, 100.0f);
		//const auto ortho_projection_matrix = glm::ortho(-1 * window->GetWidth()/2, window->GetWidth()/2, - 1 * window->GetHeight()/2, window->GetHeight()/2);
		const auto ortho_projection_matrix = glm::ortho(-16.0f, 16.0f, -9.0f, 9.0f, 0.1f, 100.f);

		glm::mat4 projection_matrices[4] = {persp_projection_matrix, ortho_projection_matrix, ortho_projection_matrix, ortho_projection_matrix};
		glm::mat4 view_matrices[4] = {this->activeCamera->GetViewMatrix()};

		view_matrices[1] = glm::lookAt(glm::vec3(8, 0, 0), glm::vec3(), glm::vec3(0, 1, 0));
		view_matrices[2] = glm::lookAt(glm::vec3(0, 8, 0), glm::vec3(), glm::vec3(0, 0, 1));;
		view_matrices[3] = glm::lookAt(glm::vec3(0, 0, 8), glm::vec3(), glm::vec3(0, 1, 0));;

		// We set the View and Projection Matrices for all the Shaders that has them ( They all should have them ideally ).
		for (auto it : ASMGR.shaders)
		{
			it.second->use();

			if ( it.first != "terrain" && it.first != "axes")
			{
				it.second->setMat4("u_ViewMatrix", view_matrices[0]);
				it.second->setMat4("u_ProjectionMatrix", projection_matrices[0]);
			}

		}

		{
			// Set the terrain Shader View and projection matrices for multiple viewports.
			auto terrain_shader = ASMGR.shaders.at("terrain");
			terrain_shader->use();
			//PE_GL(glUniformMatrix4fv(it.second->GetUniformLocation("u_ViewMatrix"), GL_FALSE, 4, &view_matrices[0][0][0]));
			auto loc = terrain_shader->GetUniformLocation("u_ViewMatrix");
			PE_GL(glUniformMatrix4fv(loc, 4, GL_FALSE, &view_matrices[0][0][0]));

			loc = terrain_shader->GetUniformLocation("u_ProjectionMatrix");
			PE_GL(glUniformMatrix4fv(loc, 4, GL_FALSE, glm::value_ptr(projection_matrices[0])));


			auto axes_shader = ASMGR.shaders.at("axes");
			axes_shader->use();
			//PE_GL(glUniformMatrix4fv(it.second->GetUniformLocation("u_ViewMatrix"), GL_FALSE, 4, &view_matrices[0][0][0]));
			loc = axes_shader->GetUniformLocation("u_ViewMatrix");
			PE_GL(glUniformMatrix4fv(loc, 4, GL_FALSE, &view_matrices[0][0][0]));

			loc = axes_shader->GetUniformLocation("u_ProjectionMatrix");
			PE_GL(glUniformMatrix4fv(loc, 4, GL_FALSE, glm::value_ptr(projection_matrices[0])));
		}
		

		auto w2 = window->GetWidth() / 2;
		auto h2 = window->GetHeight() / 2;

		PE_GL(glViewportIndexedf(0, 0, 0, w2, h2));
		PE_GL(glViewportIndexedf(1, w2, 0, w2, h2));
		PE_GL(glViewportIndexedf(2, 0, h2, w2, h2));
		PE_GL(glViewportIndexedf(3, w2, h2, w2, h2));

		for (const auto& it : entities) {
			it->Render();
		}

		testTerrain->Render();

		testGrid.Render();

	}

	void TestScene::OnImguiRender()
	{

		ImGui::NewFrame();

		if (ImGui::BeginMainMenuBar())
		{
			if ( ImGui::BeginMenu("Windows"))
			{
				if ( ImGui::MenuItem("Pathing Debug Window"))
				{
					pathingDebugWindow = true;
				}
				if( ImGui::MenuItem("Asset Manager Debug Window"))
				{
					displayAssetManagerWindow = true;
				}
				if ( ImGui::MenuItem("Log Window"))
				{
					displayLogWindow = true;
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View")) {

				if (ImGui::MenuItem("Cameras")) {
					displayCameraControls = true;
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		if ( pathingDebugWindow )
		{
			ImGui::Begin("Terrain Pathing", &pathingDebugWindow);

			ImGui::SliderFloat2("Start Position", glm::value_ptr(startxz), 0.0f, 10.0f);
			ImGui::SliderFloat2("End Position", glm::value_ptr(endxz), 0.0f, 10.0f);

			std::string temp_log = "The path b/w these points has size + " + std::to_string(path.size());

			ImGui::Text(temp_log.c_str());

			if (path.empty())
			{
				// Print the tilesets.
				std::string temp_log = "The node sets for the nodes are " + std::to_string(testTerrain->GetNodeSetFromPos(startxz.x, startxz.y)) + ", " + std::to_string(testTerrain->GetNodeSetFromPos(endxz.x, endxz.y));
				ImGui::Text(temp_log.c_str());
			}

			ImGui::End();
		}

		if (displayCameraControls)
		{
			ImGui::Begin("Available Cameras", &displayCameraControls);

			static std::shared_ptr<Camera> selected_camera;
			if (nullptr == selected_camera) {
				selected_camera = cameras[0];
			}

			ImGui::BeginChild("Cameras##List", ImVec2(150, 0), true);
			// Show all the cameras here.
			for (auto& it : cameras) {

				ImGui::PushID(&it);
				if (ImGui::Selectable(it->GetCameraName().c_str(), selected_camera == it)) {

					selected_camera = it;

				}
				ImGui::PopID();
			}
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginGroup();

				ImGui::BeginChild("Details", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

				ImGui::Text(selected_camera->GetCameraName().c_str());
				ImGui::Separator();

				// Display Camera Details here.
				selected_camera->DisplayCameraDetailsImgui();

				ImGui::EndChild();

				if (ImGui::Button("Activate Camera")) { activeCamera = selected_camera; }

			ImGui::EndGroup();

			ImGui::End();
		}

		if ( displayAssetManagerWindow )
		{
			ASMGR.GuiRender(&displayAssetManagerWindow);
		}

		if ( displayLogWindow)
		{
			LOGGER.Render(&displayLogWindow);
		}

	}

}
