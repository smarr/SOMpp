#pragma once

class SourceCoordinate {
public:
    SourceCoordinate(size_t line, size_t column) : line(line), column(column) {}
    SourceCoordinate() : line(0), column(0) {}

    [[nodiscard]] inline size_t GetLine() const { return line; }
    [[nodiscard]] inline size_t GetColumn() const { return column; }

    inline bool operator==(const SourceCoordinate& other) const {
        return line == other.line && column == other.column;
    }

private:
    /* 1-based */
    size_t line;

    /* 1-based */
    size_t column;
};
