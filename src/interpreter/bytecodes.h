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

#include <cassert>

#include "../misc/defs.h"

// bytecode constants used by SOM++

// clang-format off
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
#define BC_JUMP                   35
#define BC_JUMP_ON_FALSE_POP      36
#define BC_JUMP_ON_TRUE_POP       37
#define BC_JUMP_ON_FALSE_TOP_NIL  38
#define BC_JUMP_ON_TRUE_TOP_NIL   39
#define BC_JUMP_BACKWARD          40
#define BC_JUMP2                  41
#define BC_JUMP2_ON_FALSE_POP     42
#define BC_JUMP2_ON_TRUE_POP      43
#define BC_JUMP2_ON_FALSE_TOP_NIL 44
#define BC_JUMP2_ON_TRUE_TOP_NIL  45
#define BC_JUMP2_BACKWARD         46

#define _LAST_BYTECODE BC_JUMP2_BACKWARD

#define BC_INVALID           255
// clang-format on

// TODO: port support for these bytecodes
//       they were already named in ported code, and it seemed nicer to just
//       already include that code
// clang-format off
#define BC_INC_FIELD         254
#define BC_INC_FIELD_PUSH    253
#define BC_INC               252
#define BC_DEC               251
#define BC_SEND_N            250
#define BC_SEND_3            249
#define BC_SEND_2            248
#define BC_SEND_1            247
#define BC_RETURN_FIELD_0    246
#define BC_RETURN_FIELD_1    245
#define BC_RETURN_FIELD_2    244
#define BC_RETURN_SELF       243
// clang-format on

// properties of the bytecodes

#define FIRST_DOUBLE_BYTE_JUMP_BYTECODE BC_JUMP2
#define NUM_SINGLE_BYTE_JUMP_BYTECODES ((BC_JUMP_BACKWARD - BC_JUMP) + 1)

class Bytecode {
public:
    static char* GetBytecodeName(uint8_t bc) {
        return (char*)bytecodeNames[bc];
    }

    inline static uint8_t GetBytecodeLength(uint8_t bc) {
        return bytecodeLengths[bc];  // Return the length of the given bytecode
    }

    static bool BytecodeDefinitionsAreConsistent();

private:
    static const uint8_t bytecodeLengths[];

    static const char* bytecodeNames[];
};

inline uint16_t ComputeOffset(uint8_t byte1, uint8_t byte2) {
    return ((uint16_t)byte1) | (((uint16_t)byte2) << 8);
}

bool IsJumpBytecode(uint8_t bc);
