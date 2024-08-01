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
    2,  // BC_POP_FIELD_1
    2,  // BC_SEND
    2,  // BC_SUPER_SEND
    1,  // BC_RETURN_LOCAL
    1,  // BC_RETURN_NON_LOCAL

    3,  // BC_JUMP
    3,  // BC_JUMP_ON_TRUE_TOP_NIL
    3,  // BC_JUMP_ON_FALSE_TOP_NIL
    3,  // BC_JUMP_ON_TRUE_POP
    3,  // BC_JUMP_ON_FALSE_POP
    3,  // BC_JUMP_BACKWARDS

    3,  // BC_JUMP2
    3,  // BC_JUMP2_ON_TRUE_TOP_NIL
    3,  // BC_JUMP2_ON_FALSE_TOP_NIL
    3,  // BC_JUMP2_ON_TRUE_POP
    3,  // BC_JUMP2_ON_FALSE_POP
    3,  // BC_JUMP2_BACKWARDS
};

const char* Bytecode::bytecodeNames[] = {
    "HALT            ",          "DUP             ",
    "PUSH_LOCAL      ",          "PUSH_LOCAL_0    ",
    "PUSH_LOCAL_1    ",          "PUSH_LOCAL_2    ",
    "PUSH_ARGUMENT   ",          "PUSH_SELF       ",
    "PUSH_ARG_1      ",          "PUSH_ARG_2      ",
    "PUSH_FIELD      ",          "PUSH_FIELD_0    ",
    "PUSH_FIELD_1    ",          "PUSH_BLOCK      ",
    "PUSH_CONSTANT   ",          "PUSH_CONSTANT_0 ",
    "PUSH_CONSTANT_1 ",          "PUSH_CONSTANT_2 ",
    "PUSH_0          ",          "PUSH_1          ",
    "PUSH_NIL        ",          "PUSH_GLOBAL     ",
    "POP             ",          "POP_LOCAL       ",
    "POP_LOCAL_0     ",          "POP_LOCAL_1     ",
    "POP_LOCAL_2     ",          "POP_ARGUMENT    ",
    "POP_FIELD       ",          "POP_FIELD_0     ",
    "POP_FIELD_1     ",          "SEND            ",
    "SUPER_SEND      ",          "RETURN_LOCAL    ",
    "RETURN_NON_LOCAL",          "BC_JUMP         ",
    "BC_JUMP_ON_TRUE_TOP_NIL",   "BC_JUMP_ON_FALSE_TOP_NIL",
    "BC_JUMP_ON_TRUE_POP",       "BC_JUMP_ON_FALSE_POP",
    "BC_JUMP_BACKWARDS",

    "BC_JUMP2         ",         "BC_JUMP2_ON_TRUE_TOP_NIL",
    "BC_JUMP2_ON_FALSE_TOP_NIL", "BC_JUMP2_ON_TRUE_POP",
    "BC_JUMP2_ON_FALSE_POP",     "BC_JUMP2_BACKWARDS",
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
