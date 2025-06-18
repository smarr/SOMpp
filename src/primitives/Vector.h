#pragma once

#include "../primitivesCore/PrimitiveContainer.h"
#include "../vmobjects/ObjectFormats.h"

class _Vector : public PrimitiveContainer {
public:
    _Vector();
    void LateInitialize(std::map<std::string, size_t>* hashes);
};