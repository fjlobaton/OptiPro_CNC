#pragma once
#include "imgui.h"
#include "Engine.hpp"



class GuiManager{
    private:
        Engine& engine_;

    public:

    explicit GuiManager(Engine& engine) : engine_(engine){
        
    }

    void renderGui(const StateSnapshot snapchot){
        ImGui::Begin("New GUI");

        if(ImGui::Button("GenerateRandomJobsCommand")){
            auto genJobs = GenerateRandomJobsCommand{2,20};
            engine_.sendCommand(genJobs);
            
        }
        if(ImGui::Button("GenerateRandomMachinesCommand")){
            auto genMachine = GenerateRandomMachinesCommand{5};
            engine_.sendCommand(genMachine);
            
        }
        ImGui::End();
    }
    


};