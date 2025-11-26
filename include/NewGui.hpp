#pragma once
#include <imgui_internal.h>

#include "imgui.h"
#include "Engine.hpp"


class GuiManager
{
private:

    Engine& engine_;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove;

public:
    explicit GuiManager(Engine& engine) : engine_(engine)
    {
    }

    void renderGui(const StateSnapshot& snapshot) const
    {
        // make the dockspace cover the entire viewport automatically
        ImGuiID dockspace_id = ImGui::GetID("MyDashboardDockSpace");
        ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport());

        // If the user hasn't seen this layout yet (or if we reset it), build the 2x2 grid
        static bool first_time = true;
        if (first_time)
        {
            ApplyDefaultLayout(dockspace_id);
            first_time = false;
        }

        //Render the 4 Panels as standard Windows
        RenderJobsWindow(snapshot);
        RenderMachinesWindow(snapshot);
        RenderOperationsWindow(snapshot);
        RenderPartsWindow(snapshot);
        RenderControlGui(snapshot);
    }

    void RenderJobsWindow(const StateSnapshot& snapshot) const
    {
        ImGui::Begin("Jobs_window", nullptr, window_flags);
        {
            if (ImGui::BeginTable("TableJobs", 5,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("Job ID / Parts");
                ImGui::TableSetupColumn("Priority");
                ImGui::TableSetupColumn("Total Parts");
                ImGui::TableSetupColumn("Created");
                ImGui::TableSetupColumn("Started");
                ImGui::TableHeadersRow();

                for (const auto& [id, job] : snapshot.productionState.jobs)
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    bool open = ImGui::TreeNode(reinterpret_cast<void*>(static_cast<intptr_t>(job.jobId)), "%d",
                                                job.jobId);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", toString(job.priority).data());

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%zu types", job.parts.size());

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%s", TimeToString(job.createdTime).c_str());

                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%s", TimeToString(job.startedTime).c_str());

                    if (open)
                    {
                        if (job.parts.empty())
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextDisabled("  (No parts)");
                        }
                        else
                        {
                            for (const auto& [partId, qty] : job.parts)
                            {
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
        }
        ImGui::End();
    }

    void RenderMachinesWindow(const StateSnapshot& snapshot) const
    {
        ImGui::Begin("Machines_window", nullptr, window_flags);
        {
            auto [idle , running, stopped , error] = GetMachineStatusOverviewAmount(snapshot.productionState.machines);
            ImGui::Text("Machines Global Status:");
            ImGui::Text("Total: %lu", snapshot.productionState.machines.size());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Idle: %d", idle);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Running: %d", running);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Stopped: %d", stopped);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %d", error);

            ImGui::Separator();

            MachineGenButton(snapshot);

            ImGui::Separator();
            if (ImGui::BeginTable("TableMachines", 6,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("ID / Tools"); // Updated header
                ImGui::TableSetupColumn("Type");
                ImGui::TableSetupColumn("Status");
                ImGui::TableSetupColumn("Size");
                ImGui::TableSetupColumn("Work Env (mm)");
                ImGui::TableSetupColumn("Queue");
                ImGui::TableHeadersRow();

                for (const auto& [id, machine] : snapshot.productionState.machines)
                {
                    ImGui::TableNextRow();

                    // Col 1: ID (As a Tree Node)
                    ImGui::TableSetColumnIndex(0);
                    // Use ID as the tree identifier. "Machine %d" is the label.
                    bool open = ImGui::TreeNode((void*)(intptr_t)machine.id, "Machine %d", machine.id);

                    // Render other columns normally
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", toString(machine.machineType).data());

                    ImGui::TableSetColumnIndex(2);
                    if (machine.status == MachineState::running) ImGui::TextColored(ImVec4(0, 1, 0, 1), "Running");
                    else if (machine.status == MachineState::error) ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error");
                    else ImGui::Text("%s", toString(machine.status).data());

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%s", toString(machine.sizeClass).data());

                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%.0f x %.0f x %.0f", machine.workEnvelope.X, machine.workEnvelope.Y,
                                machine.workEnvelope.Z);

                    ImGui::TableSetColumnIndex(5);
                    ImGui::Text("%zu ops", machine.operations.size());

                    // If Tree Node is Open -> Render Nested Tools
                    if (open)
                    {
                        if (machine.tools.empty())
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextDisabled("  (No tools assigned)");
                        }
                        else
                        {
                            for (const auto& [slot, toolId] : machine.tools)
                            {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);

                                // Try to find tool name
                                std::string toolName = "Unknown";
                                if (snapshot.productionState.tools.count(toolId))
                                {
                                    toolName = std::string(snapshot.productionState.tools.at(toolId).name);
                                }

                                // Indented list item
                                ImGui::Text("  [Slot %d] Tool %d: %s", slot, toolId, toolName.c_str());
                            }
                        }
                        ImGui::TreePop(); // Close the tree scope
                    }
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void RenderOperationsWindow(const StateSnapshot& snapshot) const
    {
        ImGui::Begin("Operations_window", nullptr, window_flags);
        {
            if (ImGui::BeginTable("TableOps", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn("ID");
                ImGui::TableSetupColumn("Part");
                ImGui::TableSetupColumn("Machine");
                ImGui::TableSetupColumn("Qty");
                ImGui::TableSetupColumn("Time");
                ImGui::TableSetupColumn("Status");
                ImGui::TableHeadersRow();
                for (const auto& [id, op] : snapshot.productionState.operations)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d", op.id);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", op.partId);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%s", toString(op.requiredMachine).data());
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%u", op.quantity);
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%u ms", op.totalTime);
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void RenderPartsWindow(const StateSnapshot& snapshot) const
    {
        ImGui::Begin("Parts_window", nullptr, window_flags);
        {
            ImGui::Text("snapshot %d", snapshot.productionState.count);
        }
        ImGui::End();
    }

    void RenderControlGui(const StateSnapshot& snapshot) const
    {
        ImGui::Begin("Control Panel");
        {
            ToolGenButton(snapshot);
            ImGui::Separator();
            MachineGenButton(snapshot);
            ImGui::Separator();
            JobGenButton(snapshot);
            ImGui::Separator();
        }
        ImGui::End();
    }

    static void ApplyDefaultLayout(ImGuiID dockspace_id)
    {
        // Clear any existing layout for this dockspace
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

        // We want a 2x2 Grid.
        // We will split the main node into Left and Right.
        // Then split the Left into Top/Bottom.
        // Then split the Right into Top/Bottom.

        ImGuiID dock_main_id = dockspace_id; // This is the center
        ImGuiID dock_id_left, dock_id_right;
        ImGuiID dock_id_top_left, dock_id_bottom_left;
        ImGuiID dock_id_top_right, dock_id_bottom_right, dock_id_bottom_right_bottom, dock_id_bottom_right_top;

        // Split Main -> Left (65%) and Right (Remaining)
        dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left,
                                                   0.65f, nullptr, &dock_id_right);

        // Split Left -> Top Left (50%) and Bottom Left
        dock_id_top_left = ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Up,
                                                       0.5f, nullptr, &dock_id_bottom_left);

        // Split Right -> Top Right (50%) and Bottom Right
        dock_id_top_right = ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Up,
                                                        0.4f, nullptr, &dock_id_bottom_right);
        dock_id_bottom_right_bottom = ImGui::DockBuilderSplitNode(dock_id_bottom_right, ImGuiDir_Down,
                                                                  0.5f, nullptr, &dock_id_bottom_right_top);

        // Dock the windows into the IDs we just created
        // The strings here MUST match the names used in ImGui::Begin(...)
        ImGui::DockBuilderDockWindow("Jobs_window", dock_id_top_left);
        ImGui::DockBuilderDockWindow("Machines_window", dock_id_bottom_left);
        ImGui::DockBuilderDockWindow("Operations_window", dock_id_top_right);
        ImGui::DockBuilderDockWindow("Parts_window", dock_id_bottom_right_top);
        ImGui::DockBuilderDockWindow("Control Panel", dock_id_bottom_right_bottom);

        // Commit the layout
        ImGui::DockBuilderFinish(dockspace_id);
    }

    void MachineGenButton(const StateSnapshot& snapshot) const
    {
        //machine generator slider and button that sends command
        static int machine_gen_amount = 0;
        //set width of slider and create slider
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x / 3);
        ImGui::SliderInt("Machine Gen Amount", &machine_gen_amount, 0, 128);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        //create button that sends command with the amount of machines to create
        bool create_machines_disabled = snapshot.productionState.tools.size() == 0;

        if (create_machines_disabled) ImGui::BeginDisabled();
        if (ImGui::Button("Generate Machines"))
        {
            engine_.sendCommand(GenerateRandomMachinesCommand{machine_gen_amount});
            std::cout << "Generated random machines" << std::endl;
        }
        if (create_machines_disabled) ImGui::EndDisabled();
    }

    void ToolGenButton(const StateSnapshot& snapshot) const
    {
        //create button and slider to generate to
        static int tool_gen_amount = 0;
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x / 3);
        ImGui::SliderInt("Tool Gen Amount", &tool_gen_amount, 0, 128);
        ImGui::PopItemWidth();
        ImGui::SameLine();

        if (ImGui::Button("Generate Tools"))
        {
            engine_.sendCommand(GenerateRandomToolsCommand{tool_gen_amount});
        }
    }

    void JobGenButton(const StateSnapshot& snapshot) const
    {
        static int job_gen_amount_min = 0 , job_gen_amount_max = 256;

        float full = ImGui::GetContentRegionAvail().x;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float width = (full - spacing) * 0.5f;


        ImGui::BeginGroup();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Job Gen Min");
        ImGui::SetNextItemWidth(width);
        ImGui::SliderInt("##Job Gen min", &job_gen_amount_min, 0, job_gen_amount_max);
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Job Gen Max");
        ImGui::SetNextItemWidth(width);
        ImGui::SliderInt("##Job Gen max" , &job_gen_amount_max, job_gen_amount_min, 512);
        ImGui::EndGroup();

        bool create_jobs_disabled = snapshot.productionState.machines.size() == 0;
        if (create_jobs_disabled) ImGui::BeginDisabled();

        if (ImGui::Button("Generate Jobs"))
        {
            engine_.sendCommand(GenerateRandomJobsCommand{job_gen_amount_min, job_gen_amount_max});
        }
        if (create_jobs_disabled) ImGui::EndDisabled();
    }

};
