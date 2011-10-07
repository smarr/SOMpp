#pragma once

#define MASK_OBJECT_IS_MARKED (1 << 0)
#define MASK_OBJECT_IS_OLD (1 << 1)
#define MASK_SEEN_BY_WRITE_BARRIER (1 << 2)

class VMObjectBase {
protected:
	int32_t gcfield;
public:
	inline int32_t GetGCField() const;
	inline void SetGCField(int32_t);
};

int32_t VMObjectBase::GetGCField() const {
    return gcfield;
}
void VMObjectBase::SetGCField(int32_t val) {
    gcfield = val;
}

