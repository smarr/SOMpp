#pragma once

#include <string>

#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMSymbol.h"
#include "SourceCoordinate.h"

class Variable {
public:
    Variable(std::string& name, size_t index, bool isArgument,
             SourceCoordinate coord)
        : name(name), index(index), isArgument(isArgument), coord(coord) {
        assert(index != 0xff);
    }

    Variable() : name({}), index(0xff), isArgument(false) {}

    Variable(const Variable* old, size_t newIndex, bool isArgument)
        : name(old->name), index(newIndex), isArgument(isArgument),
          coord(old->coord) {}

    bool IsNamed(std::string& n) const { return name == n; }

    [[nodiscard]] bool IsSame(const Variable& other) const {
        return coord == other.coord;
    }

    [[nodiscard]] bool IsValid() const { return index != 0xff; }

    [[nodiscard]] inline uint8_t GetIndex() const { return index; }

    [[nodiscard]] const std::string* GetName() const { return &name; }

    [[nodiscard]] std::string MakeQualifiedName() const;

    [[nodiscard]] Variable CopyForInlining(size_t newIndex) const;

    [[nodiscard]] bool IsArgument() const { return isArgument; }

protected:
    std::string name;
    uint8_t index;
    bool isArgument;
    SourceCoordinate coord;
};
