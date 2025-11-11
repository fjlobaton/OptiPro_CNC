//
// Created by fjasis on 11/6/25.
//

#pragma once
#include <cstdint>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "types.hpp"


class Operation;

class Machine {
private:
    std::string machineId;
    MachineType machineType;
    SizeXYZ workEnvelope;
    std::map<uint16_t,Tool> tools;
    MachineSpecs machineSpecs;
    MachineState status;
    std::queue<Operation*> operations;
};

