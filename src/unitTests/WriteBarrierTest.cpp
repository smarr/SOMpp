#include "WriteBarrierTest.h"
#include "../src/vmobjects/VMSymbol.h"
#include "../src/vmobjects/VMDouble.h"
#include "../src/vmobjects/VMClass.h"
#include "../src/vmobjects/VMBlock.h"
#define private public
#include "../src/vmobjects/VMMethod.h"
#include "../src/vmobjects/VMFrame.h"
#include "../src/vmobjects/VMEvaluationPrimitive.h"

#define TEST_WB_CALLED(msg, hld, ref) \
        CPPUNIT_ASSERT_MESSAGE(msg, \
                        _HEAP->writeBarrierCalledOn.find(make_pair(hld, ref)) != \
                        _HEAP->writeBarrierCalledOn.end());

void WriteBarrierTest::testWriteArray() {
#ifdef DEBUG
        //reset set...
        _HEAP->writeBarrierCalledOn.clear();
        pVMArray arr = _UNIVERSE->NewArray(3);
        pVMInteger newInt = _UNIVERSE->NewInteger(12345);
        pVMString str = _UNIVERSE->NewString("asdfghjkl");
        pVMDouble doub = _UNIVERSE->NewDouble(9876.654);
        pVMClass cloneClass = arrayClass->Clone();
        pVMClass clone2Class = cloneClass->Clone();
        arr->SetClass(cloneClass);
        arr->SetField(0, clone2Class);
        arr->SetIndexableField(0, newInt);
        arr->SetIndexableField(1, str);
        arr->SetIndexableField(2, doub);
        //test for obvious writeBarrier calls
        TEST_WB_CALLED("VMArray failed to call writeBarrier for an element", arr,
                        newInt);
        TEST_WB_CALLED("VMArray failed to call writeBarrier for an element", arr,
                        str);
        TEST_WB_CALLED("VMArray failed to call writeBarrier for an element", arr,
                        doub);
        TEST_WB_CALLED("VMArray failed to call writeBarrier for clazz", arr,
                        arrayClass);
        TEST_WB_CALLED("VMArray failed to call writeBarrier for clazz", arr,
                        cloneClass);
        TEST_WB_CALLED("VMArray failed to call writeBarrier for clazz", arr,
                        clone2Class);
        //nilObject is assigned in constructor
        TEST_WB_CALLED("VMArray failed to call writeBarrier for an element", arr,
                        nilObject);

#else
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
#endif
}

void WriteBarrierTest::testWriteBlock() {
#ifdef DEBUG
        //reset set...
        _HEAP->writeBarrierCalledOn.clear();

        pVMSymbol methodSymbol = _UNIVERSE->NewSymbol("someMethod");
        pVMMethod method = _UNIVERSE->NewMethod(methodSymbol, 0, 0);
        pVMBlock block = _UNIVERSE->NewBlock(method,
                        _UNIVERSE->GetInterpreter()->GetFrame(),
                        method->GetNumberOfArguments());
        TEST_WB_CALLED("VMBlock failed to call writeBarrier when creating", block,
                        block->GetClass());
        TEST_WB_CALLED("VMBlock failed to call writeBarrier when creating", block,
                        block->GetMethod());
        TEST_WB_CALLED("VMBlock failed to call writeBarrier when creating", block,
                        block->GetContext());

        block->SetMethod(method->Clone());
        TEST_WB_CALLED("VMBlock failed to call writeBarrier on SetMethod", block,
                        block->GetMethod());
        block->SetContext(block->GetContext()->Clone());
        TEST_WB_CALLED("VMBlock failed to call writeBarrier on SetContext", block,
                        block->GetContext());

#else
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
#endif
}

void WriteBarrierTest::testWriteFrame() {
#ifdef DEBUG
        //reset set...
        _HEAP->writeBarrierCalledOn.clear();

        pVMFrame frame = _UNIVERSE->GetInterpreter()->GetFrame()->Clone();
        TEST_WB_CALLED("VMFrame failed to call writeBarrier when cloning", frame, frame->GetPreviousFrame());
        TEST_WB_CALLED("VMFrame failed to call writeBarrier when cloning", frame, frame->GetContext());
        TEST_WB_CALLED("VMFrame failed to call writeBarrier when cloning", frame, frame->GetMethod());
        TEST_WB_CALLED("VMFrame failed to call writeBarrier when cloning", frame, frame->GetStackPointer());
        TEST_WB_CALLED("VMFrame failed to call writeBarrier when cloning", frame, frame->bytecodeIndex);
        TEST_WB_CALLED("VMFrame failed to call writeBarrier when cloning", frame, frame->localOffset);
        TEST_WB_CALLED("VMFrame failed to call writeBarrier when cloning", frame, frame->GetClass());
        for (int i = 0; i < _UNIVERSE->GetInterpreter()->GetFrame()->GetNumberOfIndexableFields(); i++)
                TEST_WB_CALLED("VMFrame failed to call writeBarrier when cloning", frame, frame->GetIndexableField(i));

        _HEAP->writeBarrierCalledOn.clear();

        frame->SetPreviousFrame(_UNIVERSE->GetInterpreter()->GetFrame());
        TEST_WB_CALLED("VMFrame failed to call writeBarrier on SetPreviousFrame", frame, _UNIVERSE->GetInterpreter()->GetFrame());
        frame->SetContext(frame->GetContext()->Clone());
        TEST_WB_CALLED("VMFrame failed to call writeBarrier on SetContext", frame, frame->GetContext());
        frame->ClearPreviousFrame();
        TEST_WB_CALLED("VMFrame failed to call writeBarrier on ClearPreviousFrame", frame, nilObject);
#else
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
#endif
}

void WriteBarrierTest::testWriteMethod() {
#ifdef DEBUG
        //reset set...
        _HEAP->writeBarrierCalledOn.clear();
        pVMMethod method = _UNIVERSE->GetInterpreter()->GetFrame()->GetMethod()->Clone();
        TEST_WB_CALLED("VMMethod failed to call writeBarrier when cloning", method, method->GetHolder());
        TEST_WB_CALLED("VMMethod failed to call writeBarrier when cloning", method, method->GetSignature());
        TEST_WB_CALLED("VMMethod failed to call writeBarrier when cloning", method, method->numberOfLocals);
        TEST_WB_CALLED("VMMethod failed to call writeBarrier when cloning", method, method->maximumNumberOfStackElements);
        TEST_WB_CALLED("VMMethod failed to call writeBarrier when cloning", method, method->bcLength);
        TEST_WB_CALLED("VMMethod failed to call writeBarrier when cloning", method, method->numberOfArguments);
        TEST_WB_CALLED("VMMethod failed to call writeBarrier when cloning", method, method->numberOfConstants);

        _HEAP->writeBarrierCalledOn.clear();

        method->SetHolder(integerClass);
        TEST_WB_CALLED("VMMethod failed to call writeBarrier on SetHolder", method, integerClass);
        method->SetSignature(method->GetSignature());
        TEST_WB_CALLED("VMMethod failed to call writeBarrier on SetSignature", method, method->GetSignature());

#else
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
#endif
}

void WriteBarrierTest::testWriteEvaluationPrimitive() {
#ifdef DEBUG
        //reset set...
        _HEAP->writeBarrierCalledOn.clear();
        pVMEvaluationPrimitive evPrim = new (_UNIVERSE->GetHeap()) VMEvaluationPrimitive(1);
        TEST_WB_CALLED("VMEvaluationPrimitive failed to call writeBarrier when creating", evPrim, evPrim->GetClass());
        TEST_WB_CALLED("VMEvaluationPrimitive failed to call writeBarrier when creating", evPrim, evPrim->numberOfArguments);
#else
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
#endif
}

void WriteBarrierTest::testWriteClass() {
#ifdef DEBUG
        //reset set...
        _HEAP->writeBarrierCalledOn.clear();
        pVMClass cl = integerClass->Clone();
        //after cloning several writebarriers should have been called
        TEST_WB_CALLED("VMClass failed to call writeBarrier when cloning", cl,
                        cl->GetClass());
        TEST_WB_CALLED("VMClass failed to call writeBarrier when cloning", cl,
                        cl->GetSuperClass());
        TEST_WB_CALLED("VMClass failed to call writeBarrier when cloning", cl,
                        cl->GetName());
        TEST_WB_CALLED("VMClass failed to call writeBarrier when cloning", cl,
                        cl->GetInstanceFields());
        TEST_WB_CALLED("VMClass failed to call writeBarrier when cloning", cl,
                        cl->GetInstanceInvokables());

        //now test all methods that change members
        cl->SetSuperClass(integerClass);
        TEST_WB_CALLED("VMClass failed to call writeBarrier on SetSuperClass", cl,
                        integerClass);
        pVMSymbol newName = _UNIVERSE->NewSymbol("andererName");
        cl->SetName(newName);
        TEST_WB_CALLED("VMClass failed to call writeBarrier on SetName", cl,
                        newName);
        pVMArray newInstFields = cl->GetInstanceFields()->Clone();
        cl->SetInstanceFields(newInstFields);
        TEST_WB_CALLED("VMClass failed to call writeBarrier on SetInstanceFields", cl,
                        newName);
        pVMArray newInstInvokables = cl->GetInstanceInvokables()->Clone();
        cl->SetInstanceInvokables(newInstInvokables);
        TEST_WB_CALLED("VMClass failed to call writeBarrier on SetInstanceInvokables", cl,
                        newName);
#else
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
#endif
}

