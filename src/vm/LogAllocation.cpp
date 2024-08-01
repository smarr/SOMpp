#ifdef GENERATE_ALLOCATION_STATISTICS
  #include "LogAllocation.h"

  #include <map>
  #include <string>

std::map<std::string, struct alloc_data> allocationStats;
#endif

void InitializeAllocationLog() {
#ifdef GENERATE_ALLOCATION_STATISTICS
    allocationStats["VMArray"] = {0, 0};
#endif
}

void OutputAllocationLogFile() {
#ifdef GENERATE_ALLOCATION_STATISTICS
    std::string file_name_allocation = std::string(bm_name);
    file_name_allocation.append("_allocation_statistics.csv");

    fstream file_alloc_stats(file_name_allocation.c_str(), ios::out);
    map<std::string, struct alloc_data>::iterator iter;
    for (iter = allocationStats.begin(); iter != allocationStats.end();
         iter++) {
        file_alloc_stats << iter->first << ", " << iter->second.noObjects
                         << ", " << iter->second.sizeObjects << std::endl;
    }
#endif
}
