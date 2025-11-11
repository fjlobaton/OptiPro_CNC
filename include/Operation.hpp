//
// Created by fjasis on 11/6/25.
//

#pragma once
#include <cstdint>
#include <set>

#include "types.hpp"


class Part;

class Operation {
public:
    explicit Operation(Part* part_);
    void set_tools(std::set<Tool> tools_);
private:
    Part* part;
    uint32_t quantity;
    std::set<Tool> tools;
    uint32_t totalTime;
    uint32_t setupTime;
    uint32_t machineTime;
    MachineType requiredMachine;
    MachineSpecs requiredMachineSpces;
    // to define nesting grahp???

};
