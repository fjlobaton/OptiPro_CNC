#include "Gui.hpp"

#include <string>
#include <algorithm>
#include <cstring>
#include "NewGui.hpp"

void renderGui(StateSnapshot snapshot)
{
    static int counter = 0;

    if (ImGuiViewport* vp = ImGui::GetMainViewport())
        ImGui::DockSpaceOverViewport(vp->ID);


    ImGui::Begin("Machine Dashboard");
    // calculates from a snapshot
    const auto &machines_map = snapshot.productionState.machines;
    int total = (int)machines_map.size();
    int online = 0, offline = 0, processing = 0;
    for (const auto &kv : machines_map) {
        const auto &m = kv.second;
        if (m.status == MachineState::error) ++offline; else ++online;
        auto it_rt = snapshot.runtime.find(kv.first);
        bool has_running = (it_rt != snapshot.runtime.end() && it_rt->second.current_op.has_value());
        if (has_running || !m.operations.empty()) ++processing;
    }

    ImGui::Columns(4);
    ImGui::Text("Total: %d", total);
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(0,1,0,1), "Online: %d", online);
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(1,1,0,1), "Processing: %d", processing);
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(1,0,0,1), "Offline: %d", offline);
    ImGui::Columns(1);

    ImGui::Separator();

    static char search[128] = "";
    ImGui::InputText("Search by name", search, IM_ARRAYSIZE(search));

    if (ImGui::BeginTable("Machines", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Work Env (mm)");
        ImGui::TableSetupColumn("Queue");
        ImGui::TableHeadersRow();

        auto icontains = [](const char* hay, const char* needle) -> bool {
            if (!needle || !*needle) return true;
            std::string h(hay), n(needle);
            std::transform(h.begin(), h.end(), h.begin(), ::tolower);
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            return h.find(n) != std::string::npos;
        };

        for (const auto & [mid, m] : machines_map) {
            std::string name = std::string("Machine") + std::to_string(mid);
            if (!icontains(name.c_str(), search)) continue;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", mid);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", name.c_str());
            ImGui::TableSetColumnIndex(2); ImGui::Text("%s", toString(m.machineType).data());

            ImGui::TableSetColumnIndex(3);
            // show status machine data 
            ImVec4 statusColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            if (m.status == MachineState::error) statusColor = ImVec4(1,0,0,1);
            else if (m.status == MachineState::idle) statusColor = ImVec4(1,1,0,1);
            else statusColor = ImVec4(0,1,0,1);
            ImGui::TextColored(statusColor, "%s", toString(m.status).data());

            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.0f x %.0f x %.0f", m.workEnvelope.X, m.workEnvelope.Y, m.workEnvelope.Z);

            ImGui::TableSetColumnIndex(5);
            // snapshot.runtime to know if an operation is running and its remaining time
            auto it_rt = snapshot.runtime.find(mid);
            int qsize = (int)m.operations.size();
            bool running_now = false;
            double remaining = 0.0;
            if (it_rt != snapshot.runtime.end() && it_rt->second.current_op.has_value()) {
                running_now = true;
                remaining = it_rt->second.remaining_time;
                qsize += 1;
            }
            if (qsize > 0) {
                ImGui::TextColored(ImVec4(1,0.5f,0,1), "%d ops", qsize);
                if (running_now) ImGui::SameLine(); ImGui::TextDisabled("(%.0fs)", remaining);
            } else ImGui::Text("Empty");
            
        }
        ImGui::EndTable();
    }
    ImGui::End();
    
    
    ImGui::Begin("Part Queue");
    
    if (ImGui::BeginTabBar("PartQueueTabs")) 
    {
        //Visualizer Jobs
         if (ImGui::BeginTabItem("JOBS")) 
        {
            if (ImGui::BeginTable("PendingJobs", 2, ImGuiTableFlags_Borders)) 
            {
                ImGui::TableSetupColumn("Job ID");
                ImGui::TableSetupColumn("Priority");
                ImGui::TableHeadersRow();

                struct PartData { int id; const char* Priority; };
                for(auto i = snapshot.productionState.jobs.begin(); i != snapshot.productionState.jobs.end(); ++snapshot.productionState.jobs.begin())
                {
                    const auto& job = i->second;
                    
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%d", job.jobId);
                    ImGui::TableSetColumnIndex(1); 
                    switch(job.priority)
                    {
                        case Priority::low:
                            ImGui::Text("Low");
                            break;
                        case Priority::normal:
                            ImGui::Text("Medium");
                            break;
                        case Priority::urgent:
                            ImGui::Text("High");
                            break;
                        default:
                            ImGui::Text("Unknown");
                            break;
                    }
                    ++i;
                    
                        
                    
                }
                
                

                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Pending")) 
        {
            if (ImGui::BeginTable("PendingParts", 4, ImGuiTableFlags_Borders)) 
            {
                ImGui::TableSetupColumn("Part ID");
                ImGui::TableSetupColumn("Description");
                ImGui::TableSetupColumn("Machine");
                ImGui::TableSetupColumn("Est. Time");
                ImGui::TableHeadersRow();

                struct PartData { int id; const char* desc; MachineType machine; float time; };
                static PartData parts[] = {
                    {101, "Bracket A", MachineType::VMC_3AXIS, 45.5},
                    {102, "Shaft Housing", MachineType::VMC_4AXIS, 120.0},
                    {103, "Gear Blank", MachineType::LATHE, 30.0},
                    {104, "Cover Plate", MachineType::LASER_CUTTER, 15.5},
                    {105, "Complex Bracket", MachineType::VMC_5AXIS, 200.0},
                };
                
                for (const auto& part : parts) 
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%d", part.id);
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%s", part.desc);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%s", toString(part.machine).data());
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%.1f min", part.time);
                }

                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("In Progress")) 
        {
            ImGui::Text("Parts currently being processed:");
            ImGui::BulletText("Part 201 - VMC-01 (45 min remaining)");
            ImGui::BulletText("Part 203 - VMC-05 (120 min remaining)");
            ImGui::BulletText("Part 205 - LASER-01 (10 min remaining)");
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Completed")) 
        {
            ImGui::Text("Recently completed parts:");
            ImGui::BulletText("Part 198 - Completed 15 min ago");
            ImGui::BulletText("Part 197 - Completed 45 min ago");
            ImGui::BulletText("Part 195 - Completed 2 hours ago");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
    ImGui::Begin("SnapshotCounter");

    // ImGui::Text("snapshot counter: %d", (snapshot.productionState.count*10));
    ImGui::Text("snapshot counter: %d", snapshot.productionState.count);
    
    ImGui::End();

}