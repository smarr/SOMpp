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
#define BC_PUSH_LOCAL_0      3
#define BC_PUSH_LOCAL_1      4
#define BC_PUSH_LOCAL_2      5
#define BC_PUSH_ARGUMENT     6
#define BC_PUSH_SELF         7
#define BC_PUSH_ARG_1        8
#define BC_PUSH_ARG_2        9
#define BC_PUSH_FIELD        10
#define BC_PUSH_FIELD_0      11
#define BC_PUSH_FIELD_1      12
#define BC_PUSH_BLOCK        13
#define BC_PUSH_CONSTANT     14
#define BC_PUSH_CONSTANT_0   15
#define BC_PUSH_CONSTANT_1   16
#define BC_PUSH_CONSTANT_2   17
#define BC_PUSH_0            18
#define BC_PUSH_1            19
#define BC_PUSH_NIL          20
#define BC_PUSH_GLOBAL       21
#define BC_POP               22
#define BC_POP_LOCAL         23
#define BC_POP_LOCAL_0       24
#define BC_POP_LOCAL_1       25
#define BC_POP_LOCAL_2       26
#define BC_POP_ARGUMENT      27
#define BC_POP_FIELD         28
#define BC_POP_FIELD_0       29
#define BC_POP_FIELD_1       30
#define BC_SEND              31
#define BC_SUPER_SEND        32
#define BC_RETURN_LOCAL      33
#define BC_RETURN_NON_LOCAL  34
#define BC_JUMP_IF_FALSE     35
#define BC_JUMP_IF_TRUE      36
#define BC_JUMP              37

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
