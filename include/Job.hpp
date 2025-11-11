//
// Created by fjasis on 11/6/25.
//

#pragma once
#include <chrono>
#include <map>
#include <memory>
#include <string>

#include "types.hpp"


class Part;

class Job {
private:
    std::string JobId;
    std::map<Part*,uint32_t> parts;
    Priority priority;
    std::chrono::system_clock::time_point createdTime;
    std::chrono::system_clock::time_point startedTime;
    std::chrono::system_clock::time_point finishedTime;


};

