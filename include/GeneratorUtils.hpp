//
// Created by fjasis on 11/16/25.
//
#pragma once
#include <array>
#include <random>

#include "types.hpp"

inline MachineType randomMachineType(std::mt19937& rng) {
    constexpr std::array<MachineType,7> allTypes = {
        MachineType::LATHE,
        MachineType::LASER_CUTTER,
        MachineType::PRESS_BREAK,
        MachineType::TURN_MILL,
        MachineType::VMC_3AXIS,
        MachineType::VMC_4AXIS,
        MachineType::VMC_5AXIS
    };

    std::uniform_int_distribution<int> idxDist(0,static_cast<int>(allTypes.size())-1);
    return allTypes[idxDist(rng)];
}

inline SizeXYZ randomWorkEnvelope(std::mt19937& rng,MachineSizeClass size_class) {
    std::uniform_real_distribution<float> xDist;
    std::uniform_real_distribution<float> yDist;
    std::uniform_real_distribution<float> zDist;
    if (size_class == MachineSizeClass::Small) {
        std::uniform_real_distribution<float>::param_type xDist_param(300.f,600.f);
        std::uniform_real_distribution<float>::param_type yDist_param(250.f,450.f);
        std::uniform_real_distribution<float>::param_type zDist_param(300.f,600.f);
        xDist.param(xDist_param);
        yDist.param(yDist_param);
        zDist.param(zDist_param);

    } else if (size_class == MachineSizeClass::Medium) {
        std::uniform_real_distribution<float>::param_type xDist_param(600.f,900.f);
        std::uniform_real_distribution<float>::param_type yDist_param(450.f,700.f);
        std::uniform_real_distribution<float>::param_type zDist_param(500.f,700.f);
        xDist.param(xDist_param);
        yDist.param(yDist_param);
        zDist.param(zDist_param);
    } else if (size_class == MachineSizeClass::Large) {
        std::uniform_real_distribution<float>::param_type xDist_param(1000.f,2500.f);
        std::uniform_real_distribution<float>::param_type yDist_param(900.f,2000.f);
        std::uniform_real_distribution<float>::param_type zDist_param(600.f,1300.f);
        xDist.param(xDist_param);
        yDist.param(yDist_param);
        zDist.param(zDist_param);
    }
    return SizeXYZ{
        xDist(rng),
        yDist(rng),
        zDist(rng)
    };
}

inline std::set<MachineSpecs> randomMachineSpecs(std::mt19937& rng) {
    std::set<MachineSpecs> specs;
    std::bernoulli_distribution pick_spec(0.5);

    if (pick_spec(rng)) specs.insert(MachineSpecs::highspeed_spindle);
    if (pick_spec(rng)) specs.insert(MachineSpecs::long_tools);
    if (pick_spec(rng)) specs.insert(MachineSpecs::double_turret);

    if (specs.empty()) {
        specs.insert(MachineSpecs::NO_SPECS);
    }
    return specs;
}

inline MachineSizeClass randomMachineSizeClass(std::mt19937& rng) {
    std::bernoulli_distribution pick_spec(0.5);
    if (pick_spec(rng)) return MachineSizeClass::Small;
    if (pick_spec(rng)) return MachineSizeClass::Medium;
    if (pick_spec(rng)) return MachineSizeClass::Large;
    return MachineSizeClass::Medium;
}
//get random element from vector
template<typename T>
T getRandomElement(const std::vector<T>& vec,std::mt19937& rng) {
    std::uniform_int_distribution<size_t> dist(0, vec.size() - 1);
    return vec[dist(rng)];
}
//get random element from enu,
template<typename EnumType>
EnumType getRandomEnum(int maxValue,std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(0, maxValue);
    return static_cast<EnumType>(dist(rng));
}

inline Tool generateRandomTool(int toolId, std::mt19937& rng) {
    Tool tool;
    tool.toolId = toolId;
    //get random name from all names
    tool.name = getRandomElement(toolNames, rng);

    // select compatible machines randomly
    std::uniform_int_distribution<int>  machineCountDist(1,5);
    int numMachines = machineCountDist(rng);

    std::vector<MachineType> allMachines;
    for (int i=0; i < static_cast<int>(MachineType::count); ++i) {
        allMachines.push_back(static_cast<MachineType>(i));
    }

    for (int i=0 ; i < numMachines ; ++i) {
        tool.compatibleMachines.insert(getRandomElement(allMachines, rng));
    }

    //random tool life
    std::uniform_int_distribution<uint16_t> lifeDist(1000,6000);
    tool.maxToolLife = lifeDist(rng);

    //current tool life simulating having used tools
    std::uniform_int_distribution<uint16_t> cuurentLifeDist( tool.maxToolLife/3, tool.maxToolLife );
    tool.currentToolLife = cuurentLifeDist(rng);

    return tool;
}

inline std::pair<std::vector<Operation>, OperationID> generateRandomOperations(std::mt19937& rng,PartID part_id,
    int startOpId,const std::map<ToolID,Tool>& toolLib) {
    std::vector<Operation> operations;

    auto baseOpId = startOpId;
    // //generate 1-5 ops for each part a good number
    // std::uniform_int_distribution<int> opCountDist(1,5);
    // int numOperations = opCountDist(rng);
    int numOperations = 1;
    std::vector<Tool> availableTools;
    for (const auto& [id, tool] : toolLib) {
        availableTools.push_back(tool);
    }
    //start generating operations
    //std::cout << "number of ops to generate" << numOperations << std::endl;
    for (int i =0; i< numOperations; ++i) {
        Operation operation;
        operation.id = baseOpId;
        ++baseOpId;
        operation.partId = part_id;
        //random Op quantity (1-100)tbd
        //TODO implement bin packing on a random machine size and see if theres an avaialable machine
        //TODO check the ammount of parts needed for said job to see if its worthed
        //std::uniform_int_distribution<uint32_t> qtyDist(1,50);
        //operation.quantity = qtyDist(rng);
        operation.quantity = 1;

        //select random tools needed of the same type of machine
        std::uniform_int_distribution<int> toolCountDist(1,std::min(4, static_cast<int>(availableTools.size())));
        const int numTools = toolCountDist(rng);

        std::vector<Tool> shuffledTools = availableTools;
        std::shuffle(shuffledTools.begin(), shuffledTools.end(), rng);

        auto machineType = randomMachineType(rng);
        operation.requiredMachine = machineType;
        int addedTools = 0;
        for (const auto& tool:shuffledTools)
        {
            if (tool.compatibleMachines.count(operation.requiredMachine) && addedTools < numTools)
            {
                operation.tools.insert(tool.toolId);
                ++addedTools;
            }
        }
        //set up time
        std::uniform_int_distribution<int> setupDist(5,60);
        operation.setupTime = setupDist(rng);

        //machine time
        std::uniform_int_distribution<int> machineTimeDist(1,120);
        operation.machineTime = machineTimeDist(rng);
        //total time
        operation.totalTime = operation.setupTime + (operation.machineTime * operation.quantity);

        //randomSpecs
        std::uniform_int_distribution<int> specsCountDist(0,2);
        int specCount = specsCountDist(rng);

        std::vector<MachineSpecs> allSpecs = {
            MachineSpecs::highspeed_spindle,
            MachineSpecs::double_turret,
            MachineSpecs::long_tools
        };

        for (int j =0; j < specCount && j < allSpecs.size(); ++j)
        {
            operation.requiredMachineSpces.insert(allSpecs[j]);
        }
        operations.push_back(operation);
    }
    return {operations, baseOpId};
}

inline std::tuple<Part, std::vector<Operation> , int> GenerateRandomPartWithOperations(std::mt19937& rng, PartID partId,
    int startOpId,const std::map<ToolID, Tool>& toolLib, const std::map<MachineID, Machine>& machines)
{
    Part part;
    part.id = partId;

    //convert map to vector to shuffle machines
    std::vector<Machine> availableMachines;
    for (const auto& [machineId_, machine_ ] : machines)
    {
        availableMachines.push_back(machine_);
    }
    std::shuffle(availableMachines.begin(), availableMachines.end(), rng);
    //get first machine from the shuffle and use its size class to get the random size
    const auto machine = availableMachines.front();
    //generate random work envelope then check if the part is larger than the machine and change the value if it is
    auto size = randomWorkEnvelope(rng, machine.sizeClass);
    if (size.X > machine.workEnvelope.X) size.X = machine.workEnvelope.X;
    if (size.Y > machine.workEnvelope.Y) size.Y = machine.workEnvelope.Y;
    if (size.Z > machine.workEnvelope.Z) size.Z = machine.workEnvelope.Z;

    part.partSize = size;
    //generate the ammount of operations needed for said part
    auto [operations, lastOPId] = generateRandomOperations(rng, partId,startOpId,toolLib);
    part.baseMachineTime = 0;
    for (const auto& op: operations)
    {
        part.operations.push_back(op.id);
        part.baseMachineTime += op.totalTime;
    }

    return {part, operations, lastOPId};
}

inline std::pair<ToolLib,int> generateToolLibrary(std::mt19937& rng, int numTools, ToolID baseToolId)
{
    ToolLib toolLib;
    for (int i = 0; i < numTools; ++i)
    {
        Tool tool = generateRandomTool(baseToolId, rng);
        toolLib.tools[tool.toolId] = tool;
        ++baseToolId;
    }
    return {toolLib, baseToolId};
}
//generates random parts with its operations, also returns the last part and operation id used
inline std::tuple<std::map<PartID,Part>, std::map<OperationID, Operation>, int , int> generateRandomParts(std::mt19937& rng,
    int numParts, int startPartId, int startOpId, const std::map<ToolID, Tool>& toolLib, const std::map<MachineID, Machine>& machines)
{
    std::map<PartID,Part> parts;
    std::map<OperationID,Operation> operations;

    int currentOpId = startOpId;

    for (int i = 0; i < numParts; ++i)
    {
        PartID partId = startPartId;
        auto [part, ops , lastOpId] = GenerateRandomPartWithOperations(rng,partId,
            startPartId,toolLib, machines);

        parts[partId] = part;
        for (const auto& op : ops)
        {
            operations[op.id] = op;
        }
        currentOpId = lastOpId;
        ++startPartId;

    }
    return {parts, operations, startPartId, startOpId};
}
//generates a random job, and returns a job, map of new parts and new ops, the updated jobid, partid and opid
inline std::tuple<Job , std::map<PartID, Part> , std::map<OperationID, Operation> , int, int, int> GenerateRandomJob(std::mt19937& rng,
    int numJobs, const std::string& jobNameId, const int nextJobId
    ,int nextPartId, int nextOpId,const std::map<PartID,Part>& parts,  const std::map<ToolID, Tool>& toolLib, const std::map<MachineID, Machine>& machines) {
    Job job;
    std::map<PartID, Part> newParts;
    std::map<OperationID, Operation> newOperations;
    std::vector<PartID> availableParts;
    float newPartProbability = 1.0f;
    int newOpId = nextOpId;
    int newPartId = nextPartId;

    for (auto const& [partId , part] : parts) {
        availableParts.push_back(part.id);
    }
    //assing id
    job.jobId = nextJobId;
    //assign job priority
    job.priority = getRandomEnum<Priority>(4, rng);
    //created time
    job.createdTime = std::chrono::system_clock::now();

    job.startedTime = std::chrono::system_clock::time_point{};
    job.finishedTime = std::chrono::system_clock::time_point{};

    std::uniform_int_distribution<int> partCountDist(1,3);
    int numParts = partCountDist(rng);

    //new part probability
    std::uniform_real_distribution<float> partDist(0,1);
    //quantity distribution for each part
    std::uniform_int_distribution<uint32_t> qtyDist(0,300);
    for (int i = 0; i < numParts; ++i) {
        PartID partId;

        bool createdNew = false;
        if (parts.empty() || partDist(rng) < newPartProbability) {
            partId = newPartId;
            ++newPartId;
            auto [newPart , newOps , lastOpId] = GenerateRandomPartWithOperations(rng,
                partId,newOpId,toolLib, machines);
            //add new parts to the result
            newParts[partId] = newPart;
            for (const auto& op : newOps) {
                newOperations[op.id] = op;
            }
            newOpId = lastOpId;
        }else {
            //use exising part
            std::uniform_int_distribution<size_t> partIndexDist(0,parts.size()-1);
            partId = availableParts[partIndexDist(rng)];
        }
        //add part id to the jobs part map and its quantity
        uint32_t qty = qtyDist(rng);
        job.parts[partId] = qty;
    }


    return {job, newParts, newOperations ,job.jobId + 1,newPartId, newOpId };
}
