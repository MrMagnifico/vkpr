#include "menu.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include <algorithm>
#include <iterator>
#include <stdexcept>


vkUi::Menu::Menu(vkCommon::Config& config, const nfdwindowhandle_t& nfdSdlWindowHandle)
: m_config(config)
, m_nfdSdlWindowHandle(nfdSdlWindowHandle)
{}

void vkUi::Menu::draw() {
    ImGui::Begin("Controls");

    ImGui::Text("Model");
    ImGui::Separator();
    drawFileControls();
    ImGui::Spacing();
    ImGui::Text("Object rotation");
    ImGui::Separator();
    drawObjectControls();

    ImGui::End();
}

void vkUi::Menu::drawFileControls() {
    // Show name of currently loaded file
    std::string loadedFileString = std::format("Current file: {}", m_config.currentPointFile.filename().string());
    ImGui::Text(loadedFileString.c_str());
    
    ImGui::Spacing();

    // Button to load new file
    if (ImGui::Button("Load New File")) {
        // Attempt to get handle to OBJ file
        nfdu8char_t *outPath;
        nfdu8filteritem_t filters[2] = { { "Wavefront OBJ", "obj" } };
        nfdopendialogu8args_t args = {0};
        args.defaultPath    = vkCommon::constants::RESOURCES_DIR_PATH.string().c_str();
        args.filterList     = filters;
        args.filterCount    = 1;
        args.parentWindow   = m_nfdSdlWindowHandle;
        nfdresult_t result  = NFD_OpenDialogU8_With(&outPath, &args);
        
        // Parse path and indicate change if a file was selected
        if (result == NFD_OKAY) {
            m_config.currentPointFile   = std::filesystem::path(outPath);
            m_config.loadNewFile        = true;
            NFD_FreePathU8(outPath);
        }
    }
}

void vkUi::Menu::drawObjectControls() {
    ImGui::SliderFloat("Rotation sensitivity", &m_config.rotationSensitivity, 0.0001f, 0.01f, "%.4f");
    ImGui::SliderFloat("Zoom sensitivity", &m_config.zoomSensitivity, 0.001f, 0.1f, "%.3f");
}
