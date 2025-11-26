//
// Created by fjasis on 11/13/25.
//
#pragma once
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
#include "Optimizer.hpp"
#include <mutex>
#include <unordered_set>

class Engine
{
public:
    explicit Engine(const std::chrono::milliseconds tick_period)
        : running_{false},
          tickPeriod_(tick_period),
          nextMachineId_(0),
          nextJobId_(0),
          nextPartId_(0),
          nextOperationId_(0),
          nextToolId_(0)
    {
    }

    ~Engine()
    {
        stop();
    }

    void start()
    {
        running_ = true;
        worker_ = std::thread(&Engine::run, this);
    }

    void stop()
    {
        if (!running_) return;
        running_ = false;
        if (worker_.joinable())
        {
            worker_.join();
        }
    }

    // function to send command that the ui uses
    void sendCommand(const CommandVariant& command)
    {
        commands_.push(command);
    }

    std::optional<StateSnapshot> pollUpdate()
    {
        return updates_.try_pop();
    }

    std::optional<ToolLib> pollTool()
    {
        tools_.push(ToolLib{state_.tools});
        return tools_.try_pop();
    }

    // Trigger the optimizer
    void runOptimizeNow()
    {
        optimizeOnce();
    }

    //VELOCIDAD DE PRODUCCION
    void setTimerMultiplier(double multiplier)
    {
        timer_multiplier_ = multiplier;
    }

    double getTimerMultiplier() const
    {
        return timer_multiplier_;
    }


    // Return a copy of the latest schedule
    std::vector<OptiProSimple::ScheduledOp> getCurrentSchedule()
    {
        std::lock_guard<std::mutex> lk(schedule_mutex_);
        return current_schedule_;
    }

    // Apply a schedule to the runtime state
    void applySchedule(const std::vector<OptiProSimple::ScheduledOp>& schedule)
    {
        std::unordered_map<MachineID, std::vector<OperationID>> assignments;
        for (const auto& s : schedule)
        {
            assignments[s.machine_id].push_back(s.op_id);
        }

        for (auto& [mid, m] : state_.machines)
        {
            std::queue<OperationID> q;
            auto it = assignments.find(mid);
            if (it != assignments.end())
            {
                for (auto opid : it->second) q.push(opid);
            }
            m.operations = std::move(q);
            if (!m.operations.empty())
            {
                m.status = MachineState::running;
                if (machine_current_op_.find(mid) == machine_current_op_.end())
                {
                    OperationID next = m.operations.front();
                    m.operations.pop();
                    machine_current_op_[mid] = next;
                    machine_remaining_time_[mid] = static_cast<double>(state_.operations[next].totalTime);
                }
            }
            else if (m.status != MachineState::error)
            {
                m.status = MachineState::idle;
                machine_current_op_.erase(mid);
                machine_remaining_time_.erase(mid);
            }
        }
    }

    // Simulate a machine failure, mark machine error and replan

    void simulateMachineFailure(MachineID mid)
    {
        if (failed_handled_.find(mid) != failed_handled_.end()) return;
        auto it = state_.machines.find(mid);
        if (it == state_.machines.end()) return;

        it->second.status = MachineState::error;
        failed_handled_.insert(mid);


        std::thread([this, mid]()
        {
            std::this_thread::sleep_for(std::chrono::seconds(15));

            // Después de 15s → Recuperar la máquina
            state_.machines[mid].status = MachineState::running;
        }).detach();


        std::vector<OperationID> toReassign;
        auto itcur = machine_current_op_.find(mid);
        if (itcur != machine_current_op_.end())
        {
            toReassign.push_back(itcur->second);
            machine_current_op_.erase(itcur);
        }
        machine_remaining_time_.erase(mid);

        auto& mops = it->second.operations;
        while (!mops.empty())
        {
            toReassign.push_back(mops.front());
            mops.pop();
        }

        // rebuild optimizer structures from current state
        ProductionState snapshot = state_;
        opt_graph_.clear();
        OptiProSimple::build_graph_from_state(snapshot, opt_graph_);

        opt_machines_.clear();
        opt_machines_.reserve(snapshot.machines.size());
        for (const auto& [mid2, mdata] : snapshot.machines)
        {
            OptiProSimple::OptMachine om;
            om.machine_id = mid2;
            om.available = (mdata.status != MachineState::error);
            om.available_time = 0.0;
            opt_machines_.push_back(std::move(om));
        }

        std::vector<OptiProSimple::ScheduledOp> prior;
        {
            std::lock_guard<std::mutex> lk(schedule_mutex_);
            prior = current_schedule_;
        }

        using clock = std::chrono::steady_clock;
        double now = std::chrono::duration<double>(clock::now().time_since_epoch()).count();

        auto new_schedule = OptiProSimple::handle_machine_failure(opt_graph_, opt_machines_, prior, snapshot, mid, now);

        {
            std::lock_guard<std::mutex> lk(schedule_mutex_);
            current_schedule_ = new_schedule;
        }

        applySchedule(new_schedule);
    }

private:
    // Random failure injector: simulates random machine stops
    void monitorAndInjectFailures()
    {
        static thread_local std::mt19937 rng{std::random_device{}()};


        // Random machine stops, with probability
        if (!state_.machines.empty())
        {
            std::uniform_real_distribution<double> prob(0.0, 1.0);
            double p = 0.01;
            if (prob(rng) < p)
            {
                std::uniform_int_distribution<size_t> pickm(0, state_.machines.size() - 1);
                size_t idx = pickm(rng);
                auto it = state_.machines.begin();
                std::advance(it, idx);
                if (it != state_.machines.end())
                {
                    simulateMachineFailure(it->first);
                }
            }
        }
    }

    // `seconds` is the elapsed time in seconds since last advance.
    void advanceProcessing(double seconds)
    {
        if (seconds <= 0.0) return;

        for (auto& [mid, m] : state_.machines)
        {
            if (machine_current_op_.find(mid) == machine_current_op_.end())
            {
                if (!m.operations.empty() && m.status != MachineState::error)
                {
                    OperationID next = m.operations.front();
                    m.operations.pop();

                    machine_current_op_[mid] = next;
                    auto& op = state_.operations[next];
                    double duration = static_cast<double>(op.totalTime);
                    machine_remaining_time_[mid] = duration;
                    m.status = MachineState::running;
                    state_.operations[op.id].state = State::running;

                    std::cout << "Máquina " << mid << " comenzó operación " << next << " (duración: " << duration <<
                        "s)" << std::endl;
                }
            }


            auto itcur = machine_current_op_.find(mid);
            if (itcur == machine_current_op_.end()) continue;

            OperationID curOp = itcur->second;
            double& rem = machine_remaining_time_[mid];

            rem -= seconds;

            if (rem <= 0.0)
            {
                std::cout << "Máquina " << mid << " COMPLETÓ operación " << curOp << std::endl;


                state_.operations[curOp].state = State::completed;
                auto part_id = state_.operations[curOp].partId;
                state_.parts[part_id].operations;
                if (state_.operations[curOp].state == State::completed)
                {
                    //verificar las operations
                    bool part_completed = false;
                    for (auto operation : state_.parts[part_id].operations)
                    {
                        part_completed = state_.operations[operation].state == State::completed;
                    }
                    if (part_completed)
                    {
                        state_.parts[part_id].state = State::completed;
                    }


                    for (auto job : state_.jobs)
                    {
                        bool job_completed = false;

                        bool job_in_part = false;
                        for (auto part : job.second.parts)
                        {
                            job_completed = state_.parts[part.first].state == State::completed;
                            job_in_part = part.first == part_id;
                        }

                        if (job_in_part && job_completed)
                        {
                            job.second.state = State::completed;
                        }
                    }
                }


                auto itop = state_.operations.find(curOp);
                if (itop != state_.operations.end())
                {
                    itop->second.completed = true;
                }

                machine_current_op_.erase(mid);
                machine_remaining_time_.erase(mid);

                if (!m.operations.empty() && m.status != MachineState::error)
                {
                    OperationID next = m.operations.front();
                    m.operations.pop();
                    machine_current_op_[mid] = next;
                    machine_remaining_time_[mid] = static_cast<double>(state_.operations[next].totalTime);
                    m.status = MachineState::running;

                    std::cout << "Máquina " << mid << " comenzó operación " << next << " (cola restante: " << m.
                        operations.size() << ")" << std::endl;
                    state_.operations[curOp].state = State::running;
                    for (auto [PartId , part] : state_.parts)
                    {
                        if (part.operations.at(curOp))
                        {
                        }
                    }
                }
                else
                {
                    if (m.status != MachineState::error)
                    {
                        m.status = MachineState::idle;
                        std::cout << "Máquina " << mid << " ahora IDLE" << std::endl;
                    }
                }

                // If machine recovered from a previous handled failure, clear the handled flag
                if (m.status != MachineState::error)
                {
                    auto fit = failed_handled_.find(mid);
                    if (fit != failed_handled_.end()) failed_handled_.erase(fit);
                }
            }
        }
    }


    int jobs_size_mem = 0;

    void run()
    {
        using clock = std::chrono::steady_clock;
        auto nextTick = clock::now();
        while (true)
        {
            //see if there are any new commands to run
            processCommands();

            //check if the optimizer should be running and get out
            if (!running_) break;

            //run optimizer at configured tick
            if (auto now = clock::now(); now >= nextTick)
            {
                // advance processing by tickPeriod, inject random failures, optimize and publish snapshot
                double dt = std::chrono::duration<double>(tickPeriod_).count();
                //Optimze One

                //Ver si las maquina fallaron, si es asi replanificar.


                // Advance processing
                //Ver si Una operacion se completo
                //Avanzar Tiempo de las operaciones en curso
                //Restando Tiempo


                //Publish snapshot
                //Pasarle el Frame al GUI


                advanceProcessing(dt);
                monitorAndInjectFailures();
                optimizeOnce();
                publishSnashot();
                nextTick += tickPeriod_;
            }
            else
            {
                //wait before trying to get commands again
                auto waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(nextTick - now);
                //Capture the popped command!
                auto earlyCommand = commands_.pop_for(waitTime);

                if (earlyCommand)
                {
                    std::visit([this](auto&& cmd) { handleCommand(cmd); }, *earlyCommand);

                    publishSnashot();
                }
            }
        }
    }

    void processCommands()
    {
        while (true)
        {
            auto command = commands_.try_pop();
            if (!command) break;
            std::visit([this](auto&& cmd) { handleCommand(cmd); }, *command);
        }
    }

    void handleCommand(const StopEgnineCommand& command)
    {
        running_ = false;
    }

    void handleCommand(const AddJobCommand& command)
    {
    }

    void handleCommand(const AddMachineCommand& command)
    {
    }

    void handleCommand(const AddOperationCommand& command)
    {
    }

    void handleCommand(const AddPartCommand& command)
    {
        addPart(command.part, command.operations);
    }

    void handleCommand(const AddToolCommand& command)
    {
        addTool(command.tool);
    }

    void handleCommand(const AddToolsCommand& command)
    {
        for (auto& tool : command.tools)
        {
            addTool(tool);
        }
    }

    void handleCommand(const GenerateRandomMachinesCommand& command)
    {
        generateRandomMachines(command.count);
    }

    void handleCommand(const GenerateRandomJobsCommand& command)
    {
        generateRandomJobs(command.minJobs, command.maxJobs);
    }

    void optimizeOnce()
    {
        state_.count++;
        std::vector<Job> ordered;
        for (auto i = state_.machines.begin(); i != state_.machines.end(); ++i)
        {
            if (i->second.status == MachineState::error)
            {
                //REPLANIFICAR
                simulateMachineFailure(i->first);

                // std::cout << "Engine: detected machine " << i->first << " in error; invoking replan\n";
            }
        }

        //NUEVO JOB
        if (jobs_size_mem != state_.jobs.size())
        {
            //PRIORIDAD DE JOBS:

            static const std::unordered_map<Priority, int> prioOrder = {
                {Priority::low, 0},
                {Priority::normal, 1},
                {Priority::high, 2},
                {Priority::urgent, 3},
            };

            auto cmp = [&](const Job& a, const Job& b)
            {
                // 1. initialized=true primero
                if (a.initialized != b.initialized)
                    return a.initialized; // true primero

                // 2. prioridad
                if (a.priority != b.priority)
                    return static_cast<int>(a.priority) > static_cast<int>(b.priority);

                // 3. desempate para evitar inestabilidad → por ID
                return a.Id < b.Id; // usa lo que tengas como identificador único
            };

            // Crear vector temporal ordenable
            std::vector<Job> ordered;

            ordered.reserve(state_.jobs.size());

            for (auto& [id, job] : state_.jobs)
                ordered.push_back(job);

            std::sort(ordered.begin(), ordered.end(), cmp);

            for (auto& job : ordered)
            {
                for (auto part : job.parts)
                {
                    part.first; //ID DE PARTES ORDENADA
                    for (auto ope : state_.operations)
                    {
                        if (part.first == ope.second.partId && ope.second.state == State::pending)
                        {
                            //Con mi lista ordenada de jobs, asignar las operaciones a las maquinas
                            assignOperationToMachine(ope.second.id);
                            ope.second.state = State::pending;
                            if (job.state == State::pending)
                            {
                                job.state = State::running;
                            }
                        }
                    }
                }
            }
            jobs_size_mem = state_.jobs.size();
        }
    }

    void publishSnashot()
    {
        StateSnapshot snapshot;
        snapshot.productionState = state_;

        // per-machine runtime
        for (const auto& [mid, m] : state_.machines)
        {
            MachineRuntime rt;
            auto itcur = machine_current_op_.find(mid);
            if (itcur != machine_current_op_.end())
            {
                rt.current_op = itcur->second;
                auto itrem = machine_remaining_time_.find(mid);
                if (itrem != machine_remaining_time_.end()) rt.remaining_time = itrem->second;
            }
            else
            {
                rt.current_op = std::nullopt;
                rt.remaining_time = 0.0;
            }
            snapshot.runtime[mid] = rt;
        }

        updates_.push(std::move(snapshot));
    }

    void addPart(Part& part, std::vector<Operation> operations)
    {
        //creates a part with its associated operations
        //belives that the operations are correctly initialized
        part.id = nextPartId_++;
        state_.parts[part.id] = std::move(part);
        for (auto& op : part.operations)
        {
            operations[op].partId = part.id;
            state_.operations[nextOperationId_] = std::move(operations[op]);
            state_.parts[part.id].operations.push_back(nextOperationId_);
            nextOperationId_++;
        }
    }

    void addTool(Tool tool)
    {
        tool.toolId = ++nextToolId_;
        state_.tools[tool.toolId] = std::move(tool);
    }

    void handleCommand(const GenerateRandomPartCommand& command)
    {
        GenerateRandomPart();
    }

    void handleCommand(const GenerateRandomToolsCommand& command)
    {
        generateRandomTools(command.count);
    }

    void generateRandomJobs(const int minJobs, const int maxJobs)
    {
        static thread_local std::mt19937 rng{std::random_device{}()};
        std::uniform_int_distribution<int> jobs(minJobs, maxJobs);

        const int numJobs = jobs(rng);
        for (int i = 1; i <= numJobs; ++i)
        {
            auto [job , newParts, newOperations, lastJobId, lastPartId,lLastOpId]
                = GenerateRandomJob(rng, numJobs,
                                    "", nextJobId_, nextPartId_, nextOperationId_, state_.parts,
                                    state_.tools, state_.machines);

            state_.jobs[job.jobId] = std::move(job);

            for (auto& part : newParts)
            {
                state_.parts[part.first] = std::move(part.second);
            }

            for (auto& op : newOperations)
            {
                state_.operations[op.first] = std::move(op.second);
            }

            // Assign created operations to machines
            for (const auto& op : newOperations)
            {
                assignOperationToMachine(op.first);
            }

            nextOperationId_ = lLastOpId;
            nextPartId_ = lastPartId;
            nextJobId_ = lastJobId;
        }
    }

    void generateRandomMachines(int count)
    {
        if (count <= 0) return;
        // generate a rng device
        static thread_local std::mt19937 rng{std::random_device{}()};

        for (int i = 0; i < count; ++i)
        {
            //create a new machine and assing next id
            Machine machine{};
            machine.id = ++nextMachineId_;
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
            for (const auto& [fst, snd] : state_.tools)
            {
                to_select.push_back(fst);
            }
            //shuffle the vector to get a random list
            std::shuffle(to_select.begin(), to_select.end(), rng);
            //select a random ammount of tools from the library
            std::uniform_int_distribution<int> distribution(1, state_.tools.size() - 1);
            int numToSelect = distribution(rng);
            //push those tools into the machine tool lib and assigning them a internal tool id
            for (int j = 0; j < numToSelect; ++j)
            {
                machine.tools.insert({j, to_select[j]});
            }
            //push the machine back to the list of machines
            state_.machines[machine.id] = std::move(machine);
        }
    }

    void generateRandomTools(int count)
    {
        static thread_local std::mt19937 rng{std::random_device{}()};
        for (int i = 0; i < count; ++i)
        {
            state_.tools[nextToolId_] = std::move(generateRandomTool(nextToolId_, rng));
            ++nextToolId_;
        }
    }

    void GenerateRandomPart()
    {
        static thread_local std::mt19937 rng{std::random_device{}()};
        //std::cout << "generating part" << std::endl;
        //std::cout << "part id" << nextPartId_ << " opid:" << nextOperationId_ << std::endl;
        auto [part, ops, lastOpId] = GenerateRandomPartWithOperations(rng, nextPartId_, nextOperationId_, state_.tools,
                                                                      state_.machines);
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

    // Assign a single operation to the first compatible machine (simple heuristic)
    void assignOperationToMachine(OperationID opid)
    {
        auto itop = state_.operations.find(opid);
        if (itop == state_.operations.end()) return;
        const auto& op = itop->second;
        for (auto& [mid, m] : state_.machines)
        {
            if (m.machineType != op.requiredMachine) continue;
            bool ok = true;
            for (const auto& spec : op.requiredMachineSpces)
            {
                if (m.machineSpecs.find(spec) == m.machineSpecs.end())
                {
                    ok = false;
                    break;
                }
            }
            if (!ok) continue;

            if (m.status != MachineState::error)
            {
                m.status = MachineState::running;
                m.operations.push(opid);
            }
            return;
        }
    }

    void GenerateRandomParts(const int amount)
    {
        //creates rng number and adds the parts and operations to the state
        static thread_local std::mt19937 rng{std::random_device{}()};
        auto [parts , operations, lastPartId, lastOpId] = generateRandomParts(
            rng, amount, nextPartId_, nextOperationId_, state_.tools, state_.machines);
        for (auto& [partId,part] : parts)
        {
            state_.parts[partId] = std::move(part);
        }
        for (auto& [opId,operation] : operations)
        {
            state_.operations[opId] = std::move(operation);
        }
        // assign generated operations to machines
        for (const auto& [opId, operation] : operations)
        {
            assignOperationToMachine(opId);
        }
        //updates the operation and part ids
        nextOperationId_ = lastOpId;
        nextPartId_ = lastPartId;
    }

    std::atomic<bool> running_;
    std::thread worker_;
    std::chrono::milliseconds tickPeriod_;

    ProductionState state_;

    // Optimizer runtime structures
    OptiProSimple::Graph opt_graph_;
    std::vector<OptiProSimple::OptMachine> opt_machines_;
    std::vector<OptiProSimple::ScheduledOp> current_schedule_;
    std::mutex schedule_mutex_;

    ConcurrentQueue<CommandVariant> commands_;
    ConcurrentQueue<StateSnapshot> updates_;
    ConcurrentQueue<ToolLib> tools_;

    // Runtime execution tracking
    std::unordered_map<MachineID, OperationID> machine_current_op_;
    std::unordered_map<MachineID, double> machine_remaining_time_;
    std::unordered_map<ToolID, double> tool_wear_accum_;
    std::unordered_set<MachineID> failed_handled_;

    int nextMachineId_;
    int nextJobId_;
    int nextPartId_;
    int nextOperationId_;
    int nextToolId_;

    double timer_multiplier_ = 1.0;
};
