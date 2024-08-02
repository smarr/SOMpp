#include "bytecodes.h"

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
#include <cstdint>

const uint8_t Bytecode::bytecodeLengths[] = {
    1,  // BC_HALT
    1,  // BC_DUP
    3,  // BC_PUSH_LOCAL
    1,  // BC_PUSH_LOCAL_0
    1,  // BC_PUSH_LOCAL_1
    1,  // BC_PUSH_LOCAL_2
    3,  // BC_PUSH_ARGUMENT
    1,  // BC_PUSH_SELF
    1,  // BC_PUSH_ARG_1
    1,  // BC_PUSH_ARG_2
    2,  // BC_PUSH_FIELD
    1,  // BC_PUSH_FIELD_0
    1,  // BC_PUSH_FIELD_1
    2,  // BC_PUSH_BLOCK
    2,  // BC_PUSH_CONSTANT
    1,  // BC_PUSH_CONSTANT_0
    1,  // BC_PUSH_CONSTANT_1
    1,  // BC_PUSH_CONSTANT_2
    1,  // BC_PUSH_0
    1,  // BC_PUSH_1
    1,  // BC_PUSH_NIL
    2,  // BC_PUSH_GLOBAL
    1,  // BC_POP
    3,  // BC_POP_LOCAL
    1,  // BC_POP_LOCAL_0
    1,  // BC_POP_LOCAL_1
    1,  // BC_POP_LOCAL_2
    3,  // BC_POP_ARGUMENT
    2,  // BC_POP_FIELD
    1,  // BC_POP_FIELD_0
    1,  // BC_POP_FIELD_1
    2,  // BC_SEND
    2,  // BC_SUPER_SEND
    1,  // BC_RETURN_LOCAL
    1,  // BC_RETURN_NON_LOCAL

    3,  // BC_JUMP
    3,  // BC_JUMP_ON_TRUE_TOP_NIL
    3,  // BC_JUMP_ON_FALSE_TOP_NIL
    3,  // BC_JUMP_ON_TRUE_POP
    3,  // BC_JUMP_ON_FALSE_POP
    3,  // BC_JUMP_BACKWARD

    3,  // BC_JUMP2
    3,  // BC_JUMP2_ON_TRUE_TOP_NIL
    3,  // BC_JUMP2_ON_FALSE_TOP_NIL
    3,  // BC_JUMP2_ON_TRUE_POP
    3,  // BC_JUMP2_ON_FALSE_POP
    3,  // BC_JUMP2_BACKWARD
};

const char* Bytecode::bytecodeNames[] = {
    "HALT            ",           // 0
    "DUP             ",           // 1
    "PUSH_LOCAL      ",           // 2
    "PUSH_LOCAL_0    ",           // 3
    "PUSH_LOCAL_1    ",           // 4
    "PUSH_LOCAL_2    ",           // 5
    "PUSH_ARGUMENT   ",           // 6
    "PUSH_SELF       ",           // 7
    "PUSH_ARG_1      ",           // 8
    "PUSH_ARG_2      ",           // 9
    "PUSH_FIELD      ",           // 10
    "PUSH_FIELD_0    ",           // 11
    "PUSH_FIELD_1    ",           // 12
    "PUSH_BLOCK      ",           // 13
    "PUSH_CONSTANT   ",           // 14
    "PUSH_CONSTANT_0 ",           // 15
    "PUSH_CONSTANT_1 ",           // 16
    "PUSH_CONSTANT_2 ",           // 17
    "PUSH_0          ",           // 18
    "PUSH_1          ",           // 19
    "PUSH_NIL        ",           // 20
    "PUSH_GLOBAL     ",           // 21
    "POP             ",           // 22
    "POP_LOCAL       ",           // 23
    "POP_LOCAL_0     ",           // 24
    "POP_LOCAL_1     ",           // 25
    "POP_LOCAL_2     ",           // 26
    "POP_ARGUMENT    ",           // 27
    "POP_FIELD       ",           // 28
    "POP_FIELD_0     ",           // 29
    "POP_FIELD_1     ",           // 30
    "SEND            ",           // 31
    "SUPER_SEND      ",           // 32
    "RETURN_LOCAL    ",           // 33
    "RETURN_NON_LOCAL",           // 34
    "BC_JUMP         ",           // 35
    "BC_JUMP_ON_FALSE_POP",       // 36
    "BC_JUMP_ON_TRUE_POP",        // 37
    "BC_JUMP_ON_FALSE_TOP_NIL",   // 38
    "BC_JUMP_ON_TRUE_TOP_NIL",    // 39
    "BC_JUMP_BACKWARD",           // 40
    "BC_JUMP2         ",          // 41
    "BC_JUMP2_ON_FALSE_POP",      // 42
    "BC_JUMP2_ON_TRUE_POP",       // 43
    "BC_JUMP2_ON_FALSE_TOP_NIL",  // 44
    "BC_JUMP2_ON_TRUE_TOP_NIL",   // 45
    "BC_JUMP2_BACKWARD",          // 46
};

bool IsJumpBytecode(uint8_t bc) {
    assert(BC_JUMP < BC_JUMP2_BACKWARD);
    assert((BC_JUMP2_BACKWARD - BC_JUMP) == 11);

    return BC_JUMP <= bc && bc <= BC_JUMP2_BACKWARD;
}

bool Bytecode::BytecodeDefinitionsAreConsistent() {
    bool namesAndLengthMatch =
        (sizeof(Bytecode::bytecodeNames) / sizeof(char*)) ==
        (sizeof(Bytecode::bytecodeLengths) / sizeof(uint8_t));
    bool lastBytecodeLinesUp =
        _LAST_BYTECODE ==
        (sizeof(Bytecode::bytecodeLengths) - 1);  // -1 because null terminated

    return namesAndLengthMatch && lastBytecodeLinesUp;
}
