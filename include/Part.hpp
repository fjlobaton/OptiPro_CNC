//
// Created by fjasis on 11/6/25.
//

#pragma once

#include <memory>
#include <string>
#include <vector>
#include "types.hpp"


class Operation;

class Part {
public:
    ~Part();


    //TODO DEFINE CLASS STRUCTURE AND ITS BEHAVIOR
private:
    std::string PartId;
    std::vector<std::unique_ptr<Operation>> operations_;
    SizeXYZ partSize; //in mm
    uint32_t baseMachineTime;

};


