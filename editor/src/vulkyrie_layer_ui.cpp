#include "vulkyrie_layer_ui.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>

namespace Vulkyrie::Editor {

    void VulkyrieLayerUI::OnAttached() {
        Application &app = Application::GetSingleton();
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // IF using Docking Branch

        // Setup Platform/Renderer backends
        // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
        ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow *>(app.GetWindowHandle()), true);
        ImGui_ImplOpenGL3_Init("#version 460");
    }

    void VulkyrieLayerUI::OnUpdate(Timestep deltaTime) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::ShowDemoWindow(); // Show demo window! :)
        }

        // Rendering
        // (Your code clears your framebuffer, renders your other stuff etc.)
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void VulkyrieLayerUI::OnEvent(Event &event) {
        EventDispatcher dispatcher(event);

        dispatcher.Dispatch<KeyPressedEvent>([](KeyPressedEvent &event) {
            if (event.KeyCode == KeyCode::Escape) {
                Application::GetSingleton().Stop();
                return true;
            }

            return false;
        });
    }

    void VulkyrieLayerUI::OnDetached() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

} // namespace Vulkyrie::Editor
