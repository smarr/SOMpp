#pragma once

#define MASK_OBJECT_IS_MARKED (1 << 0)
#define MASK_OBJECT_IS_OLD (1 << 1)
#define MASK_SEEN_BY_WRITE_BARRIER (1 << 2)
#define MASK_BITS_ALL (MASK_OBJECT_IS_MARKED | MASK_OBJECT_IS_OLD | MASK_SEEN_BY_WRITE_BARRIER)

#include <assert.h>

class VMObjectBase : public VMOop {
protected:
    size_t gcfield;
public:
    inline size_t GetGCField() const;
    inline void SetGCField(size_t);
};

size_t VMObjectBase::GetGCField() const {
    return gcfield;
}
void VMObjectBase::SetGCField(size_t val) {
    // if gcfield is used as a forwarding pointer it should not be overwritten
    // with simple mark bits, because the object itself is garbage but the
    // forwarding address needs to be maintained incase any object still points
    // to the garbage object.
    #define GCFIELD_IS_NOT_FORWARDING_POINTER (gcfield <= MASK_BITS_ALL)
#if GC_TYPE != MARK_SWEEP
    assert(GCFIELD_IS_NOT_FORWARDING_POINTER || val > MASK_BITS_ALL);
#endif
    gcfield = val;
}
