#include "menu.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <magic_enum.hpp>

#include <algorithm>
#include <iterator>
#include <stdexcept>


vkUi::Menu::Menu(vkCommon::Config& config)
: m_config(config)
{}

void vkUi::Menu::draw()
{
    ImGui::Begin("Debug Controls");

    ImGui::Text("Draw controls");
    ImGui::Separator();
    drawDrawControls();

    ImGui::End();
}

void vkUi::Menu::drawDrawControls() {
    // Draw mode selection
    constexpr auto optionsDrawMode = magic_enum::enum_names<vkCommon::DrawType>();
    std::vector<const char*> optionsDrawModePointers;
    std::transform(std::begin(optionsDrawMode), std::end(optionsDrawMode), std::back_inserter(optionsDrawModePointers),
        [](const auto& str) { return str.data(); });
    ImGui::Combo("Draw mode", (int*) &m_config.currentDraw, optionsDrawModePointers.data(), static_cast<int>(optionsDrawModePointers.size()));
    
    // Per draw mode controls
    switch (m_config.currentDraw) {
        case vkCommon::DrawType::CLEAR: {
            ImGui::ColorEdit4("Clear color", glm::value_ptr(m_config.clearColor));
        } break;
        case vkCommon::DrawType::COMPUTE: {
            vkCommon::ComputeEffect& selected = m_config.backgroundEffects[m_config.selectedEffect];
			ImGui::Text("Selected effect: %s", selected.name.c_str());
			ImGui::SliderInt("Effect Index", &m_config.selectedEffect, 0, static_cast<int>(m_config.backgroundEffects.size()) - 1);
			ImGui::ColorEdit4("Parameter 1", glm::value_ptr(selected.data.data1));
			ImGui::ColorEdit4("Parameter 2", glm::value_ptr(selected.data.data2));
			ImGui::ColorEdit4("Parameter 3", glm::value_ptr(selected.data.data3));
			ImGui::ColorEdit4("Parameter 4", glm::value_ptr(selected.data.data4));
        } break;
        default: throw std::runtime_error("Unsupported draw type selected");
    }
}
