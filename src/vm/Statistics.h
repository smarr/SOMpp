#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <utility>

#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMInvokable.h"

#if STATS_INT_HIST || STATS_ALLOC || STATS_CALLS
  #define recordStat(feature, ...)          \
      if (Statistics::feature##Enabled) {   \
          Statistics::feature(__VA_ARGS__); \
      }
#else
  #define recordStat(feature, ...)
#endif

struct allocStatsData {
    int64_t noObjects;
    int64_t sizeObjects;
};

struct receiverStatData {
    int64_t noCalls;
    int64_t noPrimitiveCalls;
};

class Statistics {
private:
    static inline constexpr int64_t INT_HIST_SIZE = 1;

public:
    static inline constexpr bool IntegerHistogramEnabled = STATS_INT_HIST;
    static inline constexpr bool AllocationEnabled = STATS_ALLOC;
    static inline constexpr bool ReceiverTypeEnabled = STATS_CALLS;
    static inline constexpr bool CallStatsEnabled = STATS_CALLS;

    static void IntegerHistogram(int64_t value) {
        integerHist[value / INT_HIST_SIZE] =
            integerHist[value / INT_HIST_SIZE] + 1;
    }

    static void Allocation(const std::string& type, int64_t size) {
        struct allocStatsData tmp = allocationStats[type];
        tmp.noObjects += 1;
        tmp.sizeObjects += size;
        allocationStats[type] = tmp;
    }

    static void ReceiverType(VMClass* receiverClass) {
        std::string const name = receiverClass->GetName()->GetStdString();
        receiverTypes[name] += 1;
    }

    static void CallStats(VMClass* receiverClass, VMInvokable* invokable) {
        std::string const name = receiverClass->GetName()->GetStdString();
        if (callStats.find(name) == callStats.end()) {
            callStats[name] = {0, 0};
        }
        callStats[name].noCalls += 1;
        if (invokable->IsPrimitive()) {
            callStats[name].noPrimitiveCalls += 1;
        }
    }

    static void OutputStatistics();

    static void Initialize();
    static void SetMainName(std::string name) { mainName = std::move(name); }

private:
    static void outputIntegerHistogram();
    static void outputAllocations();
    static void outputReceiverTypes();
    static void outputSendTypes();

    static std::map<int64_t, int64_t> integerHist;
    static std::map<std::string, struct allocStatsData> allocationStats;
    static std::string mainName;

    static std::map<std::string, int64_t> receiverTypes;
    static std::map<std::string, struct receiverStatData> callStats;
};
