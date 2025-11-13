//
// Created by fjasis on 11/6/25.
//

#pragma once
#include <chrono>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <string_view>
#include <string>
#include <cstdint>
#include <unordered_set>

//X-macro patter to make enum declaration easier to automatically generate tostrings for ui viewing
#define STATE_LIST(X) X(pending) X(running) X(completed) X(stopped) X(cancelled)
#define MACHINE_STATE_LIST(X) X(idle) X(running) X(stopped) X(error)
#define MACHINE_TYPE_LIST(X) X(DEFAULT) X(VMC_3AXIS) X(VMC_4AXIS) X(VMC_5AXIS) X(LATHE) X(TURN_MILL) X(LASER_CUTTER) X(PRESS_BREAK)
#define PRIORITY_LIST(X) X(low) X(high) X(normal) X(urgent)
#define MACHINE_SPECS_LIST(X) X(NO_SPECS) X(highspeed_spindle) X(double_turret) X(long_tools)

//macro to generate enum value
#define AS_ENUM(Name) Name,
//macro to generate enum cases for switch with outer scope EnumType
#define AS_CASE(Name) case EnumType::Name: return #Name;
//macro to generate enum and tostring that takes that macro as parameter
#define DEFINE_ENUM(EnumName, VALUES_MACRO)\
    enum class EnumName{\
    VALUES_MACRO(AS_ENUM)\
    };\
    constexpr std::string_view toString(EnumName value){\
        using EnumType = EnumName;\
        switch (value){\
            VALUES_MACRO(AS_CASE)\
            default: return "Invalid";\
        }\
    }

//enum declaration
DEFINE_ENUM(State, STATE_LIST);
DEFINE_ENUM(MachineState, MACHINE_STATE_LIST);
DEFINE_ENUM(MachineType, MACHINE_TYPE_LIST);
DEFINE_ENUM(Priority, PRIORITY_LIST);
DEFINE_ENUM(MachineSpecs, MACHINE_SPECS_LIST)
// Use strong types for IDs for clarity and safety
using JobID = std::string;
using PartID = int;
using ToolID = int;
using MachineID = int;
using OperationID = int; // Added this



struct Tool;
struct SizeXYZ;
struct Machine;
struct Part;
struct Operation;
struct Job;

struct SizeXYZ {
    float X,Y,Z;
};
struct Job {
    JobID Id;
    std::map<PartID,uint32_t> parts;
    Priority priority;
    std::chrono::system_clock::time_point createdTime;
    std::chrono::system_clock::time_point startedTime;
    std::chrono::system_clock::time_point finishedTime;
};

struct Part {
    PartID id;
    std::vector<std::unique_ptr<Operation>> operations;
    SizeXYZ partSize; //in mm
    uint32_t baseMachineTime;
};
struct Operation {
    OperationID id;
    PartID partId;
    uint32_t quantity;
    std::set<Tool> tools;
    uint32_t totalTime;
    uint32_t setupTime;
    uint32_t machineTime;
    MachineType requiredMachine;
    std::set<MachineSpecs> requiredMachineSpces;
};
struct Machine {
    MachineID id;
    std::map<uint16_t,ToolID> tools;
    MachineState status;
    std::queue<OperationID> operations;
    MachineType machineType;
    SizeXYZ workEnvelope;
    std::set<MachineSpecs> machineSpecs;
};
struct Tool {
    std::string_view name;
    std::unordered_set<MachineType> compatibleMachines;
    uint16_t maxToolLife;
    uint16_t currentToolLife;
    int toolId;
};

struct ProductionState {
    std::map<JobID,Job> jobs;
    std::map<PartID,Job> partJobs;
    std::map<ToolID,Tool> tools;
    std::map<MachineID,Machine> machines;
    std::map<OperationID, Operation> operations;
};

struct StateSnapshot {
    ProductionState productionState;
};
