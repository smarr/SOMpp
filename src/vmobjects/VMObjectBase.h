#pragma once

#ifndef VMBASEOBJECT_H_
#define VMBASEOBJECT_H_

#define MASK_OBJECT_IS_MARKED (1 << 0)
#define MASK_OBJECT_IS_OLD (1 << 1)
#define MASK_SEEN_BY_WRITE_BARRIER (1 << 2)
#define MASK_BITS_ALL (MASK_OBJECT_IS_MARKED | MASK_OBJECT_IS_OLD | MASK_SEEN_BY_WRITE_BARRIER)


#if GC_TYPE==PAUSELESS
#define READBARRIER(reference) (ReadBarrier(&reference))
#else
#define READBARRIER(reference) (reference)
#endif

#if GC_TYPE==PAUSELESS
#define WRITEBARRIER(reference) (WriteBarrier(reference))
//#elif GC_TYPE==GENERATIONAL
//#define WRITEBARRIER(reference) (_HEAP->WriteBarrier(this, reference))
#else
#define WRITEBARRIER(reference) (reference)
#endif

class VMObjectBase {
protected:
    volatile size_t gcfield;
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
    assert(GCFIELD_IS_NOT_FORWARDING_POINTER || val > MASK_BITS_ALL);
    gcfield = val;
}

#endif
