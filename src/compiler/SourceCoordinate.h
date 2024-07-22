#pragma once

class SourceCoordinate {
public:
    SourceCoordinate(size_t line, size_t column) : line(line), column(column) {}
    SourceCoordinate() : line(0), column(0) {}
    
    inline size_t GetLine() const { return line; }
    inline size_t GetColumn() const { return column; }
    
private:
    /* 1-based */
    size_t line;
    
    /* 1-based */
    size_t column;
};
