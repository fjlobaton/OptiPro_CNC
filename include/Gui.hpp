#ifndef GUI_HPP
#define GUI_HPP

#include "imgui.h"
#include "types.hpp"
#include <functional>
#include "Commands.hpp"

// Renders the main graphical interface using ImGui.
void renderGui(StateSnapshot snapshot, std::function<void(const CommandVariant&)> sendCommand);

#endif 