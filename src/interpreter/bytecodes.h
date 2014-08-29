#pragma once

/*
 *
 *
 Copyright (c) 2007 Michael Haupt, Tobias Pape, Arne Bergmann
 Software Architecture Group, Hasso Plattner Institute, Potsdam, Germany
 http://www.hpi.uni-potsdam.de/swa/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include "../misc/defs.h"

// bytecode constants used by SOM++

#define BC_HALT              0
#define BC_DUP               1
#define BC_PUSH_LOCAL        2
#define BC_PUSH_ARGUMENT     3
#define BC_PUSH_FIELD        4
#define BC_PUSH_BLOCK        5
#define BC_PUSH_CONSTANT     6
#define BC_PUSH_GLOBAL       7
#define BC_POP               8
#define BC_POP_LOCAL         9
#define BC_POP_ARGUMENT      10
#define BC_POP_FIELD         11
#define BC_SEND              12
#define BC_SUPER_SEND        13
#define BC_RETURN_LOCAL      14
#define BC_RETURN_NON_LOCAL  15
#define BC_JUMP_IF_FALSE     16
#define BC_JUMP_IF_TRUE      17
#define BC_JUMP              18

// bytecode lengths

class Bytecode {

public:
    static char* GetBytecodeName(uint8_t bc) {
        return (char*) bytecodeNames[bc];
    }

    inline static uint8_t GetBytecodeLength(uint8_t bc) {
        return bytecodeLengths[bc]; // Return the length of the given bytecode
    }

private:

    static const uint8_t bytecodeLengths[];

    static const char* bytecodeNames[];
};
