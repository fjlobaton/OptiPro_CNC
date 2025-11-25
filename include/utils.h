//
// Created by fj on 11/19/25.
//
#pragma once
#include <imgui.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

template <typename Key, typename Value>
void printMapCustom(const std::map<Key, Value>& m,
                    const std::string& label = "Map Content",
                    const std::string& kvSeparator = ": ") {

    std::cout << "--- " << label << " ---\n";

    if (m.empty()) {
        std::cout << "(empty)\n";
        return;
    }

    for (const auto& pair : m) {
        // We use pair.first and pair.second here to be compatible
        // with older C++ standards if needed, though structured binding works too.
        std::cout << pair.first << kvSeparator << pair.second << "\n";
    }
    std::cout << "--------------------\n";
}
// Helper to format time to string for UI
inline std::string TimeToString(const std::chrono::system_clock::time_point& tp) {
    if (tp.time_since_epoch().count() == 0) return "-";
    auto time_t_val = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_val), "%H:%M:%S"); // Short format for tables
    return ss.str();
}
// --- Main Render Function ---

inline void RenderProductionStateUI(const ProductionState& state) {
    if (ImGui::BeginTabBar("ProductionStateTabs")) {

        // -------------------------
        // TAB 1: MACHINES (With Tool Dropdown)
        // -------------------------
        // if (ImGui::BeginTabItem("Machines")) {
        //     if (ImGui::BeginTable("TableMachines", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        //         ImGui::TableSetupColumn("ID / Tools"); // Updated header
        //         ImGui::TableSetupColumn("Type");
        //         ImGui::TableSetupColumn("Status");
        //         ImGui::TableSetupColumn("Size");
        //         ImGui::TableSetupColumn("Work Env (mm)");
        //         ImGui::TableSetupColumn("Queue");
        //         ImGui::TableHeadersRow();

        //         for (const auto& [id, machine] : state.machines) {
        //             ImGui::TableNextRow();

        //             // Col 1: ID (As a Tree Node)
        //             ImGui::TableSetColumnIndex(0);
        //             // Use ID as the tree identifier. "Machine %d" is the label.
        //             bool open = ImGui::TreeNode((void*)(intptr_t)machine.id, "Machine %d", machine.id);

        //             // Render other columns normally
        //             ImGui::TableSetColumnIndex(1);
        //             ImGui::Text("%s", toString(machine.machineType).data());

        //             ImGui::TableSetColumnIndex(2);
        //             if (machine.status == MachineState::running) ImGui::TextColored(ImVec4(0,1,0,1), "Running");
        //             else if (machine.status == MachineState::error) ImGui::TextColored(ImVec4(1,0,0,1), "Error");
        //             else ImGui::Text("%s", toString(machine.status).data());

        //             ImGui::TableSetColumnIndex(3);
        //             ImGui::Text("%s", toString(machine.sizeClass).data());

        //             ImGui::TableSetColumnIndex(4);
        //             ImGui::Text("%.0f x %.0f x %.0f", machine.workEnvelope.X, machine.workEnvelope.Y, machine.workEnvelope.Z);

        //             ImGui::TableSetColumnIndex(5);
        //             ImGui::Text("%zu ops", machine.operations.size());

        //             // If Tree Node is Open -> Render Nested Tools
        //             if (open) {
        //                 if (machine.tools.empty()) {
        //                     ImGui::TableNextRow();
        //                     ImGui::TableSetColumnIndex(0);
        //                     ImGui::TextDisabled("  (No tools assigned)");
        //                 } else {
        //                     for (const auto& [slot, toolId] : machine.tools) {
        //                         ImGui::TableNextRow();
        //                         ImGui::TableSetColumnIndex(0);

        //                         // Try to find tool name
        //                         std::string toolName = "Unknown";
        //                         if (state.tools.count(toolId)) {
        //                             toolName = std::string(state.tools.at(toolId).name);
        //                         }

        //                         // Indented list item
        //                         ImGui::Text("  [Slot %d] Tool %d: %s", slot, toolId, toolName.c_str());
        //                     }
        //                 }
        //                 ImGui::TreePop(); // Close the tree scope
        //             }
        //         }
        //         ImGui::EndTable();
        //     }
        //     ImGui::EndTabItem();
        // }

        // -------------------------
        // TAB 2: JOBS (With Parts Dropdown)
        // -------------------------
        if (ImGui::BeginTabItem("Jobs")) {
            if (ImGui::BeginTable("TableJobs", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                ImGui::TableSetupColumn("Job ID / Parts");
                ImGui::TableSetupColumn("Priority");
                ImGui::TableSetupColumn("Total Parts");
                ImGui::TableSetupColumn("Created");
                ImGui::TableSetupColumn("Started");
                ImGui::TableHeadersRow();

                for (const auto& [id, job] : state.jobs) {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    bool open = ImGui::TreeNode(reinterpret_cast<void *>(static_cast<intptr_t>(job.jobId)), "%d", job.jobId);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", toString(job.priority).data());

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%zu types", job.parts.size());

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%s", TimeToString(job.createdTime).c_str());

                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%s", TimeToString(job.startedTime).c_str());

                    if (open) {
                        if (job.parts.empty()) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextDisabled("  (No parts)");
                        } else {
                            for (const auto& [partId, qty] : job.parts) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                // Look up part info if needed, here we just show ID and Qty
                                ImGui::Text("  Part %d (x%u)", partId, qty);
                            }
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        // -------------------------
        // TAB 3: PARTS (With Operations Dropdown)
        // -------------------------
        if (ImGui::BeginTabItem("Parts")) {
            if (ImGui::BeginTable("TableParts", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                ImGui::TableSetupColumn("Part ID / Operations");
                ImGui::TableSetupColumn("Size (mm)");
                ImGui::TableSetupColumn("Ops Count");
                ImGui::TableSetupColumn("Base Time");
                ImGui::TableHeadersRow();

                for (const auto& [id, part] : state.parts) {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    bool open = ImGui::TreeNode((void*)(intptr_t)part.id, "Part %d", part.id);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.1f x %.1f x %.1f", part.partSize.X, part.partSize.Y, part.partSize.Z);

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%zu", part.operations.size());

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%u ms", part.baseMachineTime);

                    if (open) {
                        if (part.operations.empty()) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextDisabled("  (No operations)");
                        } else {
                            for (int opId : part.operations) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);

                                // Lookup op details
                                std::string opDesc = "Unknown Op";
                                if (state.operations.count(opId)) {
                                    const auto& op = state.operations.at(opId);
                                    opDesc = std::string(toString(op.requiredMachine));
                                }

                                ImGui::Text("  Op %d: %s", opId, opDesc.c_str());
                            }
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        // -------------------------
        // TAB 4 & 5: TOOLS / OPS (Standard Tables)
        // -------------------------
        // (Keep the previous implementation for Tools and Operations tabs here)
        if (ImGui::BeginTabItem("Tools")) {
             if (ImGui::BeginTable("TableTools", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("ID"); ImGui::TableSetupColumn("Name"); ImGui::TableSetupColumn("Life"); ImGui::TableSetupColumn("Max"); ImGui::TableSetupColumn("Bar");
                ImGui::TableHeadersRow();
                for (const auto& [id, tool] : state.tools) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%d", tool.toolId);
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%s", tool.name.data());
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%d", tool.currentToolLife);
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%d", tool.maxToolLife);
                    ImGui::TableSetColumnIndex(4); ImGui::ProgressBar(static_cast<float>(tool.currentToolLife) / tool.maxToolLife, ImVec2(-1, 0));
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Operations")) {
            if (ImGui::BeginTable("TableOps", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("ID"); ImGui::TableSetupColumn("Part"); ImGui::TableSetupColumn("Machine"); ImGui::TableSetupColumn("Qty"); ImGui::TableSetupColumn("Time");
                ImGui::TableHeadersRow();
                for (const auto& [id, op] : state.operations) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%d", op.id);
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%d", op.partId);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%s", toString(op.requiredMachine).data());
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%u", op.quantity);
                    ImGui::TableSetColumnIndex(4); ImGui::Text("%u ms", op.totalTime);
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}