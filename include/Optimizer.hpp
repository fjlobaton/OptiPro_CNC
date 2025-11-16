//
// Created by fjasis on 11/13/25.
//

#ifndef OPTIPRO_CNC_OPTIMIZER_HPP
#define OPTIPRO_CNC_OPTIMIZER_HPP

#include "types.hpp"

class Optimizer {    
public:

    static void assignJobs(ProductionState& state);
    static void reassignJobs(ProductionState& state, MachineID machineId);
    static void balanceLoad(ProductionState& state);
};

#endif //OPTIPRO_CNC_OPTIMIZER_HPP