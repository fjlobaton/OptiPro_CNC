//
// Created by fjasis on 11/13/25.
//

#ifndef OPTIPRO_CNC_OPTIMIZATIONENGINE_HPP
#define OPTIPRO_CNC_OPTIMIZATIONENGINE_HPP
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <optional>
#include <random>
#include <thread>

#include "Commands.hpp"
#include "ConcurrentQueue.hpp"
#include "GeneratorUtils.hpp"
#include "utils.h"

class Engine {
public:
    Engine(std::chrono::milliseconds tick_period)
        : running_{false},
          tickPeriod_(tick_period),
          nextMachineId_(0),
          nextPartId_(0),
          nextJobId_(0),
          nextOperationId_(0),
          nextToolId_(0) {
    }

    ~Engine() {
        stop();
    }

    void start() {
        running_ = true;
        worker_ = std::thread(&Engine::run, this);
    }

    void stop() {
        if (!running_) return;
        running_ = false;
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    // function to send command that the ui uses
    void sendCommand(const CommandVariant &command) {
        commands_.push(command);
    }

    std::optional<StateSnapshot> pollUpdate() {
        return updates_.try_pop();
    }
    std::optional<ToolLib> pollTool() {
        tools_.push(ToolLib{state_.tools});
        return tools_.try_pop();
    }

private:
    void run() {
        using clock = std::chrono::steady_clock;
        auto nextTick = clock::now();
        while (true) {
            //see if there are any new commands to run
            processCommands();

            //check if the optimizer should be running and get out
            if (!running_) break;

            //run optimizer at 10hz
            auto now = clock::now();
            if (now >= nextTick) {
                //optimize, then publish the snapshot
                optimizeOnce();
                publishSnashot();
                nextTick += tickPeriod_;
            } else {
                //wait before trying to get commands again
                auto waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(nextTick - now);
                commands_.pop_for(waitTime);
            }
        }
    }

    void processCommands() {
        while (true) {
            auto command = commands_.try_pop();
            if (!command) break;
            std::visit([this](auto &&cmd) { handleCommand(cmd); }, *command);
        }
    }

    void handleCommand(const StopEgnineCommand &command) {
        running_ = false;
    }
    void handleCommand(const AddJobCommand &command) {
    }

    void handleCommand(const AddMachineCommand &command) {
    }

    void handleCommand(const GenerateRandomMachinesCommand &command) {
        generateRandomMachines(command.count);
    }

    void handleCommand(const AddOperationCommand &command) {
    }

    void handleCommand(const AddPartCommand &command) {
        addPart(command.part, command.operations);
    }

    void handleCommand(const AddToolCommand &command) {
        addTool(command.tool);
    }
    void handleCommand(const AddToolsCommand &command) {
        for (auto &tool : command.tools) {
            addTool(tool);
        }
    }
    void handleCommand(const GenerateRandomJobsCommand &command) {
        generateRandomJobs(command.minJobs, command.maxJobs);
    }



    void handleCommand(const GenerateRandomPartCommand &command)
    {
        GenerateRandomPart();
    }
    void handleCommand(const GenerateRandomToolsCommand &command) {
        generateRandomTools(command.count);
    }


    void optimizeOnce() {
        state_.count++;
        //std::cout << "optimizing!!!!!" << std::endl;
    }

    void publishSnashot() {
        std::cout << "publishing" << std::endl;
        for (const auto &op : state_.operations) {
            std::cout << "opId:" << op.second.id << " partId:" << op.second.partId << std::endl;
        }

        StateSnapshot snapshot{state_};
        updates_.push(std::move(snapshot));
    }

    void addPart(Part &part,std::vector<Operation> operations) {
        //creates a part with its associated operations
        //belives that the operations are correctly initialized
        part.id = nextPartId_++;
        state_.parts[part.id] = std::move(part);
        for (auto &op : part.operations) {
            operations[op].partId = part.id;
            state_.operations[nextOperationId_] = std::move(operations[op]);
            state_.parts[part.id].operations.push_back(nextOperationId_);
            nextOperationId_++;
        }

    }

    void addTool(Tool tool) {
        tool.toolId = ++nextToolId_;
        state_.tools[tool.toolId] = std::move(tool);

    }

    void generateRandomJobs(const int minJobs, const int maxJobs) {
        static thread_local std::mt19937 rng{std::random_device{}()};
        std::uniform_int_distribution<int> jobs(minJobs, maxJobs);

        const int numJobs = jobs(rng);
        for (int i = 1; i <= numJobs; i++) {
            auto [job , newParts, newOperations, lastJobId, lastPartId,lLastOpId]
            = GenerateRandomJob(rng,numJobs,
            "",nextJobId_, nextPartId_, nextOperationId_, state_.parts,
            state_.tools, state_.machines);
            state_.jobs[job.jobId] = std::move(job);
            for (auto &part: newParts) {
                state_.parts[part.first] = std::move(part.second);
            }
            for (auto &op: newOperations) {
                state_.operations[op.first] = std::move(op.second);
            }
            nextOperationId_ = lLastOpId;
            nextPartId_ = lastPartId;
            nextJobId_ = lastJobId;
        }

    }

    void generateRandomMachines(int count) {
        if (count <= 0) return;
        // generate a rng device
        static thread_local std::mt19937 rng{std::random_device{}()};

        for (int i = 0; i < count; ++i) {
            //create a new machine and assing next id
            Machine machine{};
            machine.id = nextMachineId_++;
            machine.status = MachineState::idle;

            //machineType and capabilities;
            machine.machineType = randomMachineType(rng);
            auto machineSizeClass = randomMachineSizeClass(rng);
            machine.sizeClass = machineSizeClass;
            machine.workEnvelope = randomWorkEnvelope(rng, machineSizeClass);
            machine.machineSpecs = randomMachineSpecs(rng);

            //selecting random tools from the tool library of the factory
            //map vector to tools
            std::vector<ToolID> to_select;
            for (const auto &[fst, snd]: state_.tools) {
                to_select.push_back(fst);
            }
            //shuffle the vector to get a random list
            std::shuffle(to_select.begin(), to_select.end(), rng);
            //select a random ammount of tools from the library
            std::uniform_int_distribution<int> distribution(1, state_.tools.size()-1);
            int numToSelect = distribution(rng);
            //push those tools into the machine tool lib and assigning them a internal tool id
            for (int j = 0; j < numToSelect; ++j) {
                machine.tools.insert({j,to_select[j]});
            }
            //push the machine back to the list of machines
            state_.machines[machine.id] = std::move(machine);
        }
    }
    void generateRandomTools(int count) {
        static thread_local std::mt19937 rng{std::random_device{}()};
        for (int i = 0; i < count; ++i) {
            state_.tools[nextToolId_] = std::move(generateRandomTool(nextToolId_,rng));
            ++nextToolId_;
        }
    }

    void GenerateRandomPart()
    {
        static thread_local std::mt19937 rng{std::random_device{}()};
        //std::cout << "generating part" << std::endl;
        //std::cout << "part id" << nextPartId_ << " opid:" << nextOperationId_ << std::endl;
        auto [part, ops, lastOpId] = GenerateRandomPartWithOperations(rng,nextPartId_,nextOperationId_,state_.tools, state_.machines);
        nextOperationId_ = lastOpId;
        state_.parts[part.id] = std::move(part);
        for (auto& op : ops)
        {
            state_.operations[op.id] = std::move(op);
        }
        ++nextPartId_;
        //std::cout << "after part generation" << std::endl;
        //std::cout << "part id" << nextPartId_ << " opid:" << nextOperationId_ << std::endl;
    }

    void GenerateRandomParts(const int amount)
    {
        //creates rng number and adds the parts and operations to the state
        static thread_local std::mt19937 rng{std::random_device{}()};
        auto [parts , operations, lastPartId, lastOpId] = generateRandomParts(rng,amount,nextPartId_,nextOperationId_,state_.tools,state_.machines);
        for (auto& [partId,part] : parts) {
            state_.parts[partId] = std::move(part);
        }
        for (auto& [opId,operation] : operations) {
            state_.operations[opId] = std::move(operation);
        }
        //updates the operation and part ids
        nextOperationId_ = lastOpId;
        nextPartId_ = lastPartId;
    }
    std::atomic<bool> running_;
    std::thread worker_;
    std::chrono::milliseconds tickPeriod_;

    ProductionState state_;

    ConcurrentQueue<CommandVariant> commands_;
    ConcurrentQueue<StateSnapshot> updates_;
    ConcurrentQueue<ToolLib> tools_;

    int nextMachineId_;
    int nextJobId_;
    int nextPartId_;
    int nextOperationId_;
    int nextToolId_;
};
#endif //OPTIPRO_CNC_OPTIMIZATIONENGINE_HPP
