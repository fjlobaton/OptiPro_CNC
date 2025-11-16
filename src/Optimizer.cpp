
#include "Optimizer.hpp"
#include <algorithm>

void Optimizer::assignJobs(ProductionState& state) {
    for (auto& [jobId, job] : state.jobs) {
        bool jobAssigned = false;
        for (auto& [machineId, machine] : state.machines) {
            if (machine.status == MachineState::idle) {

                
                machine.status = MachineState::running;
                jobAssigned = true;
                break;
            }
        }
    }
}

void Optimizer::reassignJobs(ProductionState& state, MachineID failedMachineId) {

    // GRAPTH
    auto& failedMachine = state.machines[failedMachineId];
    
    while (!failedMachine.operations.empty()) {
        OperationID opId = failedMachine.operations.front();
        failedMachine.operations.pop();
        
        for (auto& [machineId, machine] : state.machines) {
            if (machineId != failedMachineId && machine.status != MachineState::error) {
                machine.operations.push(opId);
                break;
            }
        }
    }
}

void Optimizer::balanceLoad(ProductionState& state) {
  
}