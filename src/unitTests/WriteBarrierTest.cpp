#include "WriteBarrierTest.h"
#include "../src/vmobjects/VMSymbol.h"
#include "../src/vmobjects/VMDouble.h"
#include "../src/vmobjects/VMClass.h"
#include "../src/vmobjects/VMBlock.h"
#define private public
#include "../src/vmobjects/VMMethod.h"
#include "../src/vmobjects/VMFrame.h"
#include "../src/vmobjects/VMEvaluationPrimitive.h"

#if GC_TYPE==GENERATIONAL

#define TEST_WB_CALLED(msg, hld, ref) \
        CPPUNIT_ASSERT_MESSAGE(msg, \
                        GetHeap<HEAP_CLS>()->writeBarrierCalledOn.find(make_pair(hld, ref)) != \
                        GetHeap<HEAP_CLS>()->writeBarrierCalledOn.end());

void WriteBarrierTest::testWriteArray() {
#ifdef DEBUG
    //reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();
    pVMArray arr = GetUniverse()->NewArray(3);
    pVMInteger newInt = GetUniverse()->NewInteger(12345);
    pVMString str = GetUniverse()->NewString("asdfghjkl");
    pVMDouble doub = GetUniverse()->NewDouble(9876.654);
    pVMClass cloneClass = arrayClass->Clone();
    pVMClass clone2Class = cloneClass->Clone();
    arr->SetClass(cloneClass);
    arr->SetField(0, clone2Class);
    arr->SetIndexableField(0, newInt);
    arr->SetIndexableField(1, str);
    arr->SetIndexableField(2, doub);
    
    //test for obvious writeBarrier calls
    TEST_WB_CALLED("VMArray failed to call writeBarrier for an element", arr, newInt);
    TEST_WB_CALLED("VMArray failed to call writeBarrier for an element", arr, str);
    TEST_WB_CALLED("VMArray failed to call writeBarrier for an element", arr, doub);
    TEST_WB_CALLED("VMArray failed to call writeBarrier for clazz", arr, arrayClass);
    TEST_WB_CALLED("VMArray failed to call writeBarrier for clazz", arr, cloneClass);
    TEST_WB_CALLED("VMArray failed to call writeBarrier for clazz", arr, clone2Class);
    //nilObject is assigned in constructor
    TEST_WB_CALLED("VMArray failed to call writeBarrier for an element", arr, nilObject);
#else
    CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
#endif
}

void WriteBarrierTest::testWriteBlock() {
#ifdef DEBUG
    //reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();

    pVMSymbol methodSymbol = GetUniverse()->NewSymbol("someMethod");
    pVMMethod method = GetUniverse()->NewMethod(methodSymbol, 0, 0);
    pVMBlock block = GetUniverse()->NewBlock(method,
            GetUniverse()->GetInterpreter()->GetFrame(),
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
    // reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();

    pVMFrame frame = GetUniverse()->GetInterpreter()->GetFrame()->Clone();
    frame->SetContext(frame->Clone());

    frame->SetPreviousFrame(GetUniverse()->GetInterpreter()->GetFrame());
    TEST_WB_CALLED("VMFrame failed to call writeBarrier on SetPreviousFrame", frame, GetUniverse()->GetInterpreter()->GetFrame());
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
    // reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();
    pVMMethod method = GetUniverse()->GetInterpreter()->GetFrame()->GetMethod()->Clone();
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
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();
    pVMEvaluationPrimitive evPrim = new (GetHeap<HEAP_CLS>()) VMEvaluationPrimitive(1);
    TEST_WB_CALLED("VMEvaluationPrimitive failed to call writeBarrier when creating", evPrim, evPrim->GetClass());
    TEST_WB_CALLED("VMEvaluationPrimitive failed to call writeBarrier when creating", evPrim, evPrim->numberOfArguments);
#else
    CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
#endif
}

void WriteBarrierTest::testWriteClass() {
#ifdef DEBUG
    //reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();
    pVMClass cl = integerClass->Clone();
    //now test all methods that change members
    cl->SetSuperClass(integerClass);
    TEST_WB_CALLED("VMClass failed to call writeBarrier on SetSuperClass", cl,
            integerClass);
    pVMSymbol newName = GetUniverse()->NewSymbol("andererName");
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
#endif

