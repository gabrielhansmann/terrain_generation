#include "options_menu.h"

#include <imgui.h>

void renderOptionsMenu(Camera& camera) {
    ImGui::Begin("Options Menu");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    bool cameraControlsEnabled = camera.controlsEnabled();
    if (ImGui::Checkbox("Camera Movement", &cameraControlsEnabled)) {
        camera.setControlsEnabled(cameraControlsEnabled);
    }
    ImGui::Text("Press C to toggle camera controls");

    float cameraSpeed = camera.movementSpeed();
    if (ImGui::SliderFloat("Camera Speed", &cameraSpeed, 0.1f, 20.0f, "%.2f")) {
        camera.setMovementSpeed(cameraSpeed);
    }

    float cameraSensitivity = camera.mouseSensitivity();
    if (ImGui::SliderFloat("Mouse Sensitivity", &cameraSensitivity, 0.01f, 1.0f, "%.3f")) {
        camera.setMouseSensitivity(cameraSensitivity);
    }

    float cameraFov = camera.fieldOfView();
    if (ImGui::SliderFloat("Field of View", &cameraFov, 20.0f, 100.0f, "%.1f")) {
        camera.setFieldOfView(cameraFov);
    }

    ImGui::End();
}