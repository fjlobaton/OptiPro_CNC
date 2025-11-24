#include "Gui.hpp"

#include <string>
#include <algorithm>
#include <cstring>
#include "NewGui.hpp"

void renderGui(StateSnapshot snapshot, std::function<void(const CommandVariant&)> sendCommand)
{
    static int counter = 0;

    if (ImGuiViewport* vp = ImGui::GetMainViewport())
        ImGui::DockSpaceOverViewport(vp->ID);


    ImGui::Begin("Machine Dashboard");
    ImGui::Columns(4);
    ImGui::Text("Total: 8");
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(0,1,0,1), "Online: 6");
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(1,1,0,1), "Processing: 3");
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(1,0,0,1), "Offline: 2");
    ImGui::Columns(1);
    
    ImGui::Separator();

    static char search[128] = "";
    ImGui::InputText("Search by name", search, IM_ARRAYSIZE(search));

    auto icontains = [](const char* hay, const char* needle) -> bool 
    {
        if (!needle || !*needle) 
        {
            return true;
        } 
        std::string h(hay), n(needle);
        std::transform(h.begin(), h.end(), h.begin(), ::tolower);
        std::transform(n.begin(), n.end(), n.begin(), ::tolower);
        return h.find(n) != std::string::npos;
    };
    if (ImGui::BeginTable("Machines", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Queue");
        ImGui::TableHeadersRow();
        
        //datos de ejemplo 
        struct MachineData { int id; const char* name; MachineType type; const char* status; int queue; };
        static MachineData machines[] = 
        {
            {1, "Machine1", MachineType::VMC_3AXIS, "Running", 2},
            {2, "Machine2", MachineType::VMC_4AXIS, "Idle", 0},
            {3, "Machine3", MachineType::VMC_5AXIS, "Running", 1},
            {4, "Machine4", MachineType::LATHE, "Offline", 3},
            {5, "Machine5", MachineType::LASER_CUTTER, "Running", 1},
            {6, "Machine6", MachineType::TURN_MILL, "Idle", 0},
            {7, "Machine7", MachineType::VMC_3AXIS, "Offline", 5},
            {8, "Machine8", MachineType::PRESS_BREAK, "Idle", 0},
        };
        
        for (const auto& machine : machines) 
        {
            if (!icontains(machine.name, search))
            {
                continue;
            }

            ImGui::TableNextRow();
            
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", machine.id);
            
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", machine.name);
            
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", toString(machine.type).data());
            
            ImGui::TableSetColumnIndex(3);

            // Color 
            ImVec4 statusColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            if (strcmp(machine.status, "Running") == 0)
            {
                statusColor = ImVec4(0,1,0,1);
            }
            else if (strcmp(machine.status, "Offline") == 0) 
            {
                statusColor = ImVec4(1,0,0,1);
            }
            else if (strcmp(machine.status, "Idle") == 0) 
            {
                statusColor = ImVec4(1,1,0,1);
            }
            
            ImGui::TextColored(statusColor, "%s", machine.status);
            
            ImGui::TableSetColumnIndex(4);
            if (machine.queue > 0) 
            {
                ImGui::TextColored(ImVec4(1,0.5f,0,1), "%d parts", machine.queue);
            } 
            else 
            {
                ImGui::Text("Empty");
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
    
    
    ImGui::Begin("Part Queue");
    
    if (ImGui::BeginTabBar("PartQueueTabs")) 
    {
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
                    ImGui::TableSetColumnIndex(0); 
                    ImGui::Text("%d", part.id);

                    ImGui::TableSetColumnIndex(1); 
                    ImGui::Text("%s", part.desc);

                    ImGui::TableSetColumnIndex(2); 
                    ImGui::Text("%s", toString(part.machine).data());
                    
                    ImGui::TableSetColumnIndex(3); 
                    ImGui::Text("%.1f min", part.time);
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

    ImGui::Text("snapshot counter: %d", snapshot.productionState.count);
    ImGui::End();

}