#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <utility>
#include <unistd.h>
#include <GlobalModules/Logger/Logger.hpp>

class SystemStat{
public:
    static long busyMemory();
    static long busyCPU();
private:
    static std::pair<unsigned long, unsigned long> __getActualCPU();
};