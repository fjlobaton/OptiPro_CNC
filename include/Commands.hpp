//
// Created by fjasis on 11/13/25.
//
#pragma once

#include <variant>

#include "types.hpp"
//machine related commands
struct AddMachineCommand {
    Machine machine;
};
struct InitMachinesCommand {
    std::vector<Machine> machines;
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

//generator commands
struct GenerateRandomMachinesCommand {
    int count;
};
struct GenerateRandomToolsCommand {
    int count;
};
struct GenerateRandomJobsCommand {
    int minJobs;
    int maxJobs;
};
struct GenerateRandomPartCommand
{

};

struct StopEgnineCommand {};

using CommandVariant = std::variant<
    AddMachineCommand,
    AddJobCommand,
    AddOperationCommand,
    AddPartCommand,
    AddToolCommand,
    AddToolsCommand,
    GenerateRandomMachinesCommand,
    GenerateRandomJobsCommand,
    GenerateRandomToolsCommand,
    GenerateRandomPartCommand,
    StopEgnineCommand
    >;
