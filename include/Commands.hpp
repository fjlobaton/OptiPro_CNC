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
struct AddToolsCommand {
    std::vector<Tool> tools;
};


//operation related commands
struct AddOperationCommand {
    Operation operation;
};

//part related commands
struct AddPartCommand {
    Part &part;
    std::vector<Operation> &operations;
};

//job related commands
struct AddJobCommand {
    Job job;
};
struct GenerateRandomToolsCommand {
    int count;
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
    AddToolsCommand,
    GenerateRandomJobsCommand,
    GenerateRandomToolsCommand,
    StopEgnineCommand
    >;

#endif //OPTIPRO_CNC_COMMANDS_HPP