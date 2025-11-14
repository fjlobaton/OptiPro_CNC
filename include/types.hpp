//
// Created by fjasis on 11/6/25.
//

#pragma once
#include <string_view>
#include <string>
#include <cstdint>
#include <unordered_set>

//X-macro patter to make enum declaration easier to automatically generate tostrings for ui viewing
#define STATE_LIST(X) X(pending) X(running) X(completed) X(stopped) X(cancelled)
#define MACHINE_STATE_LIST(X) X(idle) X(running) X(stopped) X(error)
#define MACHINE_TYPE_LIST(X) X(VMC_3AXIS) X(VMC_4AXIS) X(VMC_5AXIS) X(LATHE) X(TURN_MILL) X(LASER_CUTTER) X(PRESS_BREAK)
#define PRIORITY_LIST(X) X(low) X(high) X(normal) X(urgent)
#define MACHINE_SPECS_LIST(X) X(highspeed_spindle) X(double_turret) X(long_tools)

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

struct Tool {
    std::string name;
    std::unordered_set<MachineType> compatibleMachines;
    uint16_t maxToolLife;
    uint16_t currentToolLife;
    int toolId;
};