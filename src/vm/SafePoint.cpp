#include "SafePoint.h"

std::mutex              SafePoint::mutex;
std::condition_variable SafePoint::condvar;

int64_t                 SafePoint::safepoint_i = 0;

int32_t                 SafePoint::numTotalMutators  = 0;
int32_t                 SafePoint::numActiveMutators = 0;

std::function<void()>   SafePoint::action_fn;
