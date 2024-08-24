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

void vkUi::Menu::draw() {
    ImGui::Begin("Debug Controls");

    // TODO: Add UI controls

    ImGui::End();
}
