#pragma once

#define MASK_OBJECT_IS_MARKED (1 << 0)
#define MASK_OBJECT_IS_OLD (1 << 1)
#define MASK_SEEN_BY_WRITE_BARRIER (1 << 2)

class VMObjectBase {
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
    gcfield = val;
}

