//
// Created by fjasis on 11/13/25.
//

#ifndef OPTIPRO_CNC_COMMANDS_HPP
#define OPTIPRO_CNC_COMMANDS_HPP
#include <variant>

#include "types.hpp"
//machine related commands
struct AddMachineCommand {
    Machine machine;
};
struct InitMachinesCommand {
    std::vector<Machine> machines;
};
struct GenerateRandomMachinesCommand {
    int count;
};
// tool related commands
struct AddToolCommand {
    Tool tool;
};

//operation related commands
struct AddOperationCommand {
    Operation operation;
};

//part related commands
struct AddPartCommand {
    Part part;
};

//job related commands
struct AddJobCommand {
    Job job;
};
struct GenerateRandomJobsCommand {
    int minJobs;
    int maxJobs;
};

struct StopEgnineCommand {};

using CommandVariant = std::variant<
    AddMachineCommand,
    GenerateRandomMachinesCommand,
    AddJobCommand,
    AddOperationCommand,
    AddPartCommand,
    AddToolCommand,
    GenerateRandomJobsCommand,
    StopEgnineCommand
    >;

#endif //OPTIPRO_CNC_COMMANDS_HPP