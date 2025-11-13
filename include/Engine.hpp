//
// Created by fjasis on 11/13/25.
//

#ifndef OPTIPRO_CNC_OPTIMIZATIONENGINE_HPP
#define OPTIPRO_CNC_OPTIMIZATIONENGINE_HPP
#include <atomic>
#include <chrono>
#include <iostream>
#include <optional>
#include <random>
#include <thread>

#include "Commands.hpp"
#include "ConcurrentQueue.hpp"

class Engine {
    public:
    Engine()
        : running_{false},
            tickPeriod_(std::chrono::milliseconds{100}){}
    ~Engine() {
        stop();
    }
    void start() {
        running_ = true;
        worker_ = std::thread(&Engine::run, this);
    }
    void stop() {
        if (!running_) return;
        sendCommand(StopEgnineCommand{});
        if (worker_.joinable()) {
            worker_.join();
        }
        running_ = false;
    }
    // function to send command that the ui uses
    void sendCommand(const CommandVariant& command) {
        commands_.push(command);
    }
    std::optional<StateSnapshot> pollUpdate() {
        return updates_.try_pop();
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
                optimizeOnce();
                publishSnashot();
                nextTick -= tickPeriod_;
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
            std::visit([this](auto&& cmd) {handleCommand(cmd); }, *command);
        }
    }
    void handleCommand(const StopEgnineCommand& command) {
        running_ = false;
    }
    void handleCommand(const AddJobCommand& command) {

    }
    void handleCommand(const AddMachineCommand& command) {

    }
    void handleCommand(const AddOperationCommand& command) {

    }
    void handleCommand(const AddPartCommand& command) {

    }
    void handleCommand(const AddToolCommand& command) {

    }
    void handleCommand(const GenerateRandomJobsCommand& command) {
        generateRandomJobs(command.minJobs,command.maxJobs);
    }
    void generateRandomJobs(const int minJobs, const int maxJobs) {
        static thread_local std::mt19937 rng{std::random_device{}()};

    }

    void optimizeOnce() {
        std::cout << "optimizing!!!!!" << std::endl;
    }

    void publishSnashot() {
        StateSnapshot snapshot{state_};
        updates_.push(std::move(snapshot));
    }

    std::atomic<bool> running_;
    std::thread worker_;
    std::chrono::milliseconds tickPeriod_;

    ProductionState state_;

    ConcurrentQueue<CommandVariant> commands_;
    ConcurrentQueue<StateSnapshot> updates_;
};
#endif //OPTIPRO_CNC_OPTIMIZATIONENGINE_HPP