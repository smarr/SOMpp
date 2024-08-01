#pragma once

#ifdef GENERATE_ALLOCATION_STATISTICS
struct alloc_data {
    long noObjects;
    long sizeObjects;
};
std::map<std::string, struct alloc_data> allocationStats;
  #define LOG_ALLOCATION(TYPE, SIZE)                     \
      {                                                  \
          struct alloc_data tmp = allocationStats[TYPE]; \
          tmp.noObjects++;                               \
          tmp.sizeObjects += (SIZE);                     \
          allocationStats[TYPE] = tmp;                   \
      }
#else
  #define LOG_ALLOCATION(TYPE, SIZE)
#endif

void InitializeAllocationLog();
void OutputAllocationLogFile();
