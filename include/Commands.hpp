//
// Created by fjasis on 11/13/25.
//

#ifndef OPTIPRO_CNC_COMMANDS_HPP
#define OPTIPRO_CNC_COMMANDS_HPP
#include <variant>

#include "types.hpp"

struct AddMachineCommand {
    Machine machine;
};
struct AddToolCommand {
    Tool tool;
};
struct AddOperationCommand {
    Operation operation;
};
struct AddPartCommand {
    Part part;
};
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
    AddJobCommand,
    AddOperationCommand,
    AddPartCommand,
    AddToolCommand,
    GenerateRandomJobsCommand,
    StopEgnineCommand
    >;

#endif //OPTIPRO_CNC_COMMANDS_HPP