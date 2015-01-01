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

#include <stdio.h>
#include <string.h>

#include "Disassembler.h"

#include "../vm/Universe.h"

#include "../interpreter/bytecodes.h"
#include "../interpreter/Interpreter.h"

#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMBigInteger.h"
#include "../vmobjects/VMBlock.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMDouble.h"
#include "../vmobjects/VMEvaluationPrimitive.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMInteger.h"
#include "../vmobjects/VMInvokable.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMString.h"
#include "../vmobjects/VMSymbol.h"
#include "../vmobjects/Signature.h"

#include "../misc/debug.h"
#include "../misc/defs.h"

/** 
 * Dispatch an object to its content and write out
 */
void Disassembler::dispatch(oop_t o) {
    //dispatch
    // can't switch() objects, so:
    if (o == nullptr)
        return; // nullptr isn't interesting.
    else if(o == nilObject)
        DebugPrint("{Nil}");
    else if(o == trueObject)
        DebugPrint("{True}");
    else if(o == falseObject)
        DebugPrint("{False}");
    else if(o == systemClass)
        DebugPrint("{System Class object}");
    else if(o == blockClass)
        DebugPrint("{Block Class object}");
    else if(o == GetUniverse()->GetGlobal(GetUniverse()->SymbolForChars("system")))
        DebugPrint("{System}");
    else {
        VMClass* c = CLASS_OF(o);
        if (c == stringClass) {
            DebugPrint("\"%s\"", static_cast<pVMString>(o)->GetChars());
        } else if(c == doubleClass)
            DebugPrint("%g", static_cast<VMDouble*>(o)->GetEmbeddedDouble());
        else if(c == bigIntegerClass)
            DebugPrint("%lld", static_cast<VMBigInteger*>(o)->GetEmbeddedInteger());
        else if(c == integerClass)
            DebugPrint("%d", INT_VAL(o));
        else if(c == symbolClass) {
            DebugPrint("#%s", static_cast<pVMSymbol>(o)->GetChars());
        } else
            DebugPrint("address: %p", (void*)o);
    }
}

/**
 * Dump a class and all subsequent methods.
 */
void Disassembler::Dump(VMClass* cl) {
    long numInvokables = cl->GetNumberOfInstanceInvokables();
    for (long i = 0; i < numInvokables; ++i) {
        pVMInvokable inv = static_cast<pVMInvokable>(cl->GetInstanceInvokable(i));
        // output header and skip if the Invokable is a Primitive
        pVMSymbol sig = inv->GetSignature();
        pVMSymbol cname = cl->GetName();
        DebugDump("%s>>%s = ", cname->GetChars(), sig->GetChars());
        if (inv->IsPrimitive()) {
            DebugPrint("<primitive>\n");
            continue;
        }
        // output actual method
        DumpMethod(static_cast<pVMMethod>(inv), "\t");
    }
}

/**
 * Bytecode Index Accessor macros
 */
#define BC_0 method->GetBytecode(bc_idx)
#define BC_1 method->GetBytecode(bc_idx+1)
#define BC_2 method->GetBytecode(bc_idx+2)

/**
 * Dump all Bytecode of a method.
 */
void Disassembler::DumpMethod(pVMMethod method, const char* indent) {
    DebugPrint("(\n");
    {   // output stack information
        long locals = method->GetNumberOfLocals();
        long max_stack = method->GetMaximumNumberOfStackElements();
        DebugDump("%s<%d locals, %d stack, %d bc_count>\n", indent, locals,
        max_stack, method->GetNumberOfBytecodes());
    }
#ifdef _DEBUG
    cout << "bytecodes: ";
    long numBytecodes = method->GetNumberOfBytecodes();
    for (long i = 0; i < numBytecodes; ++i) {
        cout << (int)(*method)[i] << " ";
    }
    cout << endl;
#endif
    // output bytecodes
    long numBytecodes = method->GetNumberOfBytecodes();
    for (long bc_idx = 0;
         bc_idx < numBytecodes;
         bc_idx += Bytecode::GetBytecodeLength(method->GetBytecode(bc_idx))) {
        // the bytecode.
        uint8_t bytecode = BC_0;
        // indent, bytecode index, bytecode mnemonic
        DebugDump("%s%4d:%s  ", indent, bc_idx,
        Bytecode::GetBytecodeName(bytecode));
        // parameters (if any)
        if(Bytecode::GetBytecodeLength(bytecode) == 1) {
            DebugPrint("\n");
            continue;
        }
        switch(bytecode) {
            case BC_PUSH_LOCAL:
                DebugPrint("local: %d, context: %d\n", BC_1, BC_2); break;
            case BC_PUSH_ARGUMENT:
                DebugPrint("argument: %d, context %d\n", BC_1, BC_2); break;
            case BC_PUSH_FIELD: {
                long fieldIdx = BC_1;
                VMClass* holder = dynamic_cast<VMClass*>((pVMObject) method->GetHolder());
                if (holder) {
                    pVMSymbol name = holder->GetInstanceFieldName(fieldIdx);
                    if (name != nullptr) {
                        DebugPrint("(index: %d) field: %s\n", BC_1, name->GetChars());
                        break;
                    }
                }
                
                DebugPrint("(index: %d) field: !nullptr!: error!\n", BC_1);
                break;
            }
            case BC_PUSH_BLOCK: {
                char* nindent = new char[strlen(indent)+1+1];
                DebugPrint("block: (index: %d) ", BC_1);
                sprintf(nindent, "%s\t", indent);

                Disassembler::DumpMethod(
                static_cast<pVMMethod>(method->GetConstant(bc_idx)), nindent);
                break;
            }
            case BC_PUSH_CONSTANT: {
                oop_t constant = method->GetConstant(bc_idx);
                VMClass* cl = CLASS_OF(constant);
                pVMSymbol cname = cl->GetName();

                DebugPrint("(index: %d) value: (%s) ",
                BC_1, cname->GetChars());
                dispatch(constant); DebugPrint("\n");
                break;
            }
            case BC_PUSH_GLOBAL: {
                oop_t cst = method->GetConstant(bc_idx);

                if (cst != nullptr) {
                    pVMSymbol name = static_cast<pVMSymbol>(cst);
                    if (name != nullptr) {
                        DebugPrint("(index: %d) value: %s\n", BC_1,
                        name->GetChars());
                        break;
                    }
                } else
                DebugPrint("(index: %d) value: !nullptr!: error!\n", BC_1);

                break;
            }
            case BC_POP_LOCAL:
                DebugPrint("local: %d, context: %d\n", BC_1, BC_2);
                break;
            case BC_POP_ARGUMENT:
                DebugPrint("argument: %d, context: %d\n", BC_1, BC_2);
                break;
            case BC_POP_FIELD: {
                long fieldIdx = BC_1;
                VMClass* holder = dynamic_cast<VMClass*>((pVMObject) method->GetHolder());
                if (holder) {
                    pVMSymbol name = holder->GetInstanceFieldName(fieldIdx);
                    DebugPrint("(index: %d) field: %s\n", fieldIdx, name->GetChars());
                } else {
                    DebugPrint("(index: %d) block holder is not a class!!\n", fieldIdx);
                }
                break;
            }
            case BC_SEND: {
                pVMSymbol name = static_cast<pVMSymbol>(method->GetConstant(bc_idx));

                DebugPrint("(index: %d) signature: %s\n", BC_1,
                name->GetChars());
                break;
            }
            case BC_SUPER_SEND: {
                pVMSymbol name = static_cast<pVMSymbol>(method->GetConstant(bc_idx));

                DebugPrint("(index: %d) signature: %s\n", BC_1,
                name->GetChars());
                break;
            }
            case BC_JUMP_IF_FALSE:
            case BC_JUMP_IF_TRUE:
            case BC_JUMP: {
                int target = 0;
                target |= method->GetBytecode(bc_idx + 1);
                target |= method->GetBytecode(bc_idx + 2) << 8;
                target |= method->GetBytecode(bc_idx + 3) << 16;
                target |= method->GetBytecode(bc_idx + 4) << 24;
                DebugPrint("(target: %d)\n", target);
                break;
            }
            default:
                DebugPrint("<incorrect bytecode>\n");
        }
    }
    DebugDump("%s)\n", indent);
}

/**
 * Dump bytecode from the frame running
 */
void Disassembler::DumpBytecode(VMFrame* frame, pVMMethod method, long bc_idx) {
    static long long indentc = 0;
    static char ikind = '@';
    uint8_t bc = BC_0;
    VMClass* cl = method->GetHolder();

    // Determine Context: Class or Block?
    if (cl != nullptr) {
        pVMSymbol cname = cl->GetName();
        pVMSymbol sig = method->GetSignature();

        DebugTrace("%20s>>%-20s% 10lld %c %04d: %s\t",
        cname->GetChars(), sig->GetChars(),
        indentc, ikind, bc_idx,
        Bytecode::GetBytecodeName(bc));
    } else {
        pVMSymbol sig = method->GetSignature();

        DebugTrace("%-42s% 10lld %c %04d: %s\t",
        sig->GetChars(),
        indentc, ikind, bc_idx,
        Bytecode::GetBytecodeName(bc));
    }
    // reset send indicator
    if(ikind != '@') ikind = '@';

    switch(bc) {
        case BC_HALT: {
            DebugPrint("<halting>\n\n\n");
            break;
        }
        case BC_DUP: {
            oop_t o = frame->GetStackElement(0);
            if (o) {
                VMClass* c = CLASS_OF(o);
                pVMSymbol cname = c->GetName();

                DebugPrint("<to dup: (%s) ", cname->GetChars());
                //dispatch
                dispatch(o);
            } else
                DebugPrint("<to dup: address: %p", (void*)o);
            DebugPrint(">\n");
            break;
        }
        case BC_PUSH_LOCAL: {
            uint8_t bc1 = BC_1, bc2 = BC_2;
            oop_t o = frame->GetLocal(bc1, bc2);
            VMClass* c = CLASS_OF(o);
            pVMSymbol cname = c->GetName();

            DebugPrint("local: %d, context: %d <(%s) ",
            BC_1, BC_2, cname->GetChars());
            //dispatch
            dispatch(o);
            DebugPrint(">\n");
            break;
        }
        case BC_PUSH_ARGUMENT: {
            uint8_t bc1 = BC_1, bc2 = BC_2;
            oop_t o = frame->GetArgument(bc1, bc2);
            DebugPrint("argument: %d, context: %d", bc1, bc2);

            if (cl != nullptr) {
                VMClass* c = CLASS_OF(o);
                pVMSymbol cname = c->GetName();

                DebugPrint("<(%s) ", cname->GetChars());
                //dispatch
                dispatch(o);
                DebugPrint(">");
            }
            DebugPrint("\n");
            break;
        }
        case BC_PUSH_FIELD: {
            VMFrame* ctxt = frame->GetOuterContext();
            oop_t arg = ctxt->GetArgument(0, 0);
            uint8_t field_index = BC_1;
            
            oop_t o = ((pVMObject) arg)->GetField(field_index);
            VMClass* c = CLASS_OF(o);
            pVMSymbol cname = c->GetName();
            long fieldIdx = BC_1;
            pVMSymbol name = method->GetHolder()->GetInstanceFieldName(fieldIdx);
            DebugPrint("(index: %d) field: %s <(%s) ", BC_1, name->GetChars(),
                       cname->GetChars());
            //dispatch
            dispatch(o);
            DebugPrint(">\n");
            break;
        }
        case BC_PUSH_BLOCK: {
            DebugPrint("block: (index: %d) ", BC_1);
            pVMMethod meth = dynamic_cast<pVMMethod>((pVMAbstract)method->GetConstant(bc_idx));
            DumpMethod(meth, "$");
            break;
        }
        case BC_PUSH_CONSTANT: {
            oop_t constant = method->GetConstant(bc_idx);
            VMClass* c = CLASS_OF(constant);
            pVMSymbol cname = c->GetName();

            DebugPrint("(index: %d) value: (%s) ", BC_1,
            cname->GetChars());
            dispatch(constant);
            DebugPrint("\n");
            break;
        }
        case BC_PUSH_GLOBAL: {
            pVMSymbol name = static_cast<pVMSymbol>(method->GetConstant(bc_idx));
            oop_t o = GetUniverse()->GetGlobal(name);
            pVMSymbol cname;

            const char* c_cname;
            if (o) {
                VMClass* c = CLASS_OF(o);
                cname = c->GetName();

                c_cname = cname->GetChars();
            } else
                c_cname = "nullptr";

            DebugPrint("(index: %d)value: %s <(%s) ", BC_1,
            name->GetChars(), c_cname);
            dispatch(o);
            DebugPrint(">\n");
            break;
        }
        case BC_POP: {
            oop_t o = frame->GetStackElement(0);
            VMClass* c = CLASS_OF(o);
            pVMSymbol cname = c->GetName();

            DebugPrint("popped <(%s) ", cname->GetChars());
            //dispatch
            dispatch(o);
            DebugPrint(">\n");
            break;
        }
        case BC_POP_LOCAL: {
            oop_t o = frame->GetStackElement(0);
            VMClass* c = CLASS_OF(o);
            pVMSymbol cname = c->GetName();

            DebugPrint("popped local: %d, context: %d <(%s) ", BC_1, BC_2,
            cname->GetChars());
            //dispatch
            dispatch(o);
            DebugPrint(">\n");
            break;
        }
        case BC_POP_ARGUMENT: {
            oop_t o = frame->GetStackElement(0);
            VMClass* c = CLASS_OF(o);
            pVMSymbol cname = c->GetName();
            DebugPrint("argument: %d, context: %d <(%s) ", BC_1, BC_2,
            cname->GetChars());
            //dispatch
            dispatch(o);
            DebugPrint(">\n");
            break;
        }
        case BC_POP_FIELD: {
            oop_t o = frame->GetStackElement(0);
            VMClass* c = CLASS_OF(o);
            long fieldIdx = BC_1;
            pVMSymbol name = method->GetHolder()->GetInstanceFieldName(fieldIdx);
            pVMSymbol cname = c->GetName();
            DebugPrint("(index: %d) field: %s <(%s) ", fieldIdx, name->GetChars(),
                       cname->GetChars());
            dispatch(o);
            DebugPrint(">\n");
            break;
        }
        case BC_SUPER_SEND:
        case BC_SEND: {
            pVMSymbol sel = static_cast<pVMSymbol>(method->GetConstant(bc_idx));

            DebugPrint("(index: %d) signature: %s (", BC_1,
            sel->GetChars());
            //handle primitives, they don't increase call-depth
            oop_t elem = GetUniverse()->GetInterpreter()->GetFrame()->
            GetStackElement(
            Signature::GetNumberOfArguments(sel)-1);
            VMClass* elemClass = CLASS_OF(elem);
            pVMInvokable inv = dynamic_cast<pVMInvokable>(elemClass->LookupInvokable(sel));

            if(inv != nullptr && inv->IsPrimitive())
                DebugPrint("*)\n");
            else {
                DebugPrint("\n");
                indentc++; ikind='>'; // visual
            }
            break;
        }
        case BC_RETURN_LOCAL:
        case BC_RETURN_NON_LOCAL: {
            DebugPrint(")\n");
            indentc--; ikind='<'; //visual
            break;
        }
        case BC_JUMP_IF_FALSE:
        case BC_JUMP_IF_TRUE:
        case BC_JUMP: {
            int target = 0;
            target |= method->GetBytecode(bc_idx + 1);
            target |= method->GetBytecode(bc_idx + 2) << 8;
            target |= method->GetBytecode(bc_idx + 3) << 16;
            target |= method->GetBytecode(bc_idx + 4) << 24;
            DebugPrint("(target: %d)\n", target);
            break;
        }
        default:
            DebugPrint("<incorrect bytecode>\n");
            break;
    }
}
