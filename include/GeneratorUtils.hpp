//
// Created by fjasis on 11/16/25.
//
#pragma once
#include <array>
#include <random>

#include "types.hpp"

inline MachineType randomMachineType(std::mt19937& rng) {
    constexpr std::array<MachineType,7> allTypes = {
        MachineType::LATHE,
        MachineType::LASER_CUTTER,
        MachineType::PRESS_BREAK,
        MachineType::TURN_MILL,
        MachineType::VMC_3AXIS,
        MachineType::VMC_4AXIS,
        MachineType::VMC_5AXIS
    };

    std::uniform_int_distribution<int> idxDist(0,static_cast<int>(allTypes.size())-1);
    return allTypes[idxDist(rng)];
}

inline SizeXYZ randomWorkEnvelope(std::mt19937& rng,MachineSizeClass size_class) {
    std::uniform_real_distribution<float> xDist;
    std::uniform_real_distribution<float> yDist;
    std::uniform_real_distribution<float> zDist;
    if (size_class == MachineSizeClass::Small) {
        xDist.param(std::uniform_real_distribution<float>::param_type((300.f,600.f)));
        yDist.param(std::uniform_real_distribution<float>::param_type((250.f,450.f)));
        zDist.param(std::uniform_real_distribution<float>::param_type((300.f,600.f)));

    } else if (size_class == MachineSizeClass::Medium) {
        xDist.param(std::uniform_real_distribution<float>::param_type((600.f,900.f)));
        yDist.param(std::uniform_real_distribution<float>::param_type((450.f,700.f)));
        zDist.param(std::uniform_real_distribution<float>::param_type((500.f,700.f)));
    } else if (size_class == MachineSizeClass::Large) {
        xDist.param(std::uniform_real_distribution<float>::param_type((1000.f,2500.f)));
        yDist.param(std::uniform_real_distribution<float>::param_type((900.f,2000.f)));
        zDist.param(std::uniform_real_distribution<float>::param_type((600.f,1300.f)));
    }
    return SizeXYZ{
        xDist(rng),
        yDist(rng),
        zDist(rng)
    };
}

inline std::set<MachineSpecs> randomMachineSpecs(std::mt19937& rng) {
    std::set<MachineSpecs> specs;
    std::bernoulli_distribution pick_spec(0.5);

    if (pick_spec(rng)) specs.insert(MachineSpecs::highspeed_spindle);
    if (pick_spec(rng)) specs.insert(MachineSpecs::long_tools);
    if (pick_spec(rng)) specs.insert(MachineSpecs::double_turret);

    if (specs.empty()) {
        specs.insert(MachineSpecs::NO_SPECS);
    }
    return specs;
}

inline MachineSizeClass randomMachineSizeClass(std::mt19937& rng) {
    std::bernoulli_distribution pick_spec(0.5);
    if (pick_spec(rng)) return MachineSizeClass::Small;
    if (pick_spec(rng)) return MachineSizeClass::Medium;
    if (pick_spec(rng)) return MachineSizeClass::Large;
}
//get random element from vector
template<typename T>
T getRandomElement(const std::vector<T>& vec,std::mt19937& rng) {
    std::uniform_int_distribution<T> dist(0, vec.size() - 1);
    return vec[dist(rng)];
}
//get random element from enu,
template<typename EnumType>
EnumType getRandomEnum(int maxValue,std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(0, maxValue);
    return static_cast<EnumType>(dist(rng));
}

inline Tool generateRandomTool(int toolId, std::mt19937& rng) {
    Tool tool;
    tool.toolId = toolId;
    //get random name from all names
    tool.name = getRandomElement(toolNames, rng);

    // select compatible machines randomly
    std::uniform_int_distribution<int>  machineCountDist(1,5);
    int numMachines = machineCountDist(rng);

    std::vector<MachineType> allMachines;
    for (int i=0; i < static_cast<int>(MachineType::count); ++i) {
        allMachines.push_back(static_cast<MachineType>(i));
    }

    for (int i=0 ; i < numMachines ; ++i) {
        tool.compatibleMachines.insert(getRandomElement(allMachines, rng));
    }

    //random tool life
    std::uniform_int_distribution<uint16_t> lifeDist(1000,6000);
    tool.maxToolLife = lifeDist(rng);

    //current tool life simulating having used tools
    std::uniform_int_distribution<uint16_t> cuurentLifeDist( tool.maxToolLife/3, tool.maxToolLife );
    tool.currentToolLife = cuurentLifeDist(rng);

    return tool;
}

inline std::vector<Operation> generateRandomOperations(std::mt19937& rng,PartID part_id,
    int startOpId,const std::map<ToolID,Tool>& toolLib) {
    std::vector<Operation> operatoins;

    //generate 1-5 ops for each part a good number
    std::uniform_int_distribution<int> opCountDist(1,5);
    int numOperations = opCountDist(rng);

    std::vector<Tool> availableTools;
    for (const auto& [id, tool] : toolLib) {
        availableTools.push_back(tool);
    }
    //start generating operations
    for (int i =0; i<numOperations; ++i) {
        Operation operation;
        operation.id = startOpId + i;

        //random Op quantity (1-100)tbd



    }

}