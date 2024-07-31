#include "../misc/defs.h"

#if GC_TYPE==GENERATIONAL

#include <cppunit/TestAssert.h>
#include <utility>

#include "../memory/Heap.h"
#include "../vm/Globals.h"
#include "../vm/Symbols.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMBlock.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMDouble.h"
#include "../vmobjects/VMEvaluationPrimitive.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMInteger.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMString.h"
#include "../vmobjects/VMSymbol.h"
#include "WriteBarrierTest.h"


#define TEST_WB_CALLED(msg, hld, ref) \
        CPPUNIT_ASSERT_MESSAGE(msg, \
                        GetHeap<HEAP_CLS>()->writeBarrierCalledOn.find(make_pair(hld, ref)) != \
                        GetHeap<HEAP_CLS>()->writeBarrierCalledOn.end());

void WriteBarrierTest::testWriteArray() {
    if (!DEBUG) {
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
    }

    //reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();
    VMArray* arr = GetUniverse()->NewArray(3);
    VMInteger* newInt = GetUniverse()->NewInteger(12345);
    VMString* str = GetUniverse()->NewString("asdfghjkl");
    VMDouble* doub = GetUniverse()->NewDouble(9876.654);
    VMClass* cloneClass = load_ptr(arrayClass)->CloneForMovingGC();
    VMClass* clone2Class = cloneClass->CloneForMovingGC();
    arr->SetClass(cloneClass);
    arr->SetField(0, clone2Class);
    arr->SetIndexableField(0, newInt);
    arr->SetIndexableField(1, str);
    arr->SetIndexableField(2, doub);

    //test for obvious writeBarrier calls
    TEST_WB_CALLED("VMArray failed to call writeBarrier for an element", arr, newInt);
    TEST_WB_CALLED("VMArray failed to call writeBarrier for an element", arr, str);
    TEST_WB_CALLED("VMArray failed to call writeBarrier for an element", arr, doub);
    TEST_WB_CALLED("VMArray failed to call writeBarrier for clazz", arr, load_ptr(arrayClass));
    TEST_WB_CALLED("VMArray failed to call writeBarrier for clazz", arr, cloneClass);
    TEST_WB_CALLED("VMArray failed to call writeBarrier for clazz", arr, clone2Class);
    //nilObject is assigned in constructor
    TEST_WB_CALLED("VMArray failed to call writeBarrier for an element", arr, load_ptr(nilObject));
}

void WriteBarrierTest::testWriteBlock() {
    if (!DEBUG) {
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
    }

    //reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();

    VMSymbol* methodSymbol = NewSymbol("someMethod");
    VMMethod* method = GetUniverse()->NewMethod(methodSymbol, 0, 0, 0, 0, new LexicalScope(nullptr, {}, {}));
    VMBlock* block = GetUniverse()->NewBlock(method,
            GetUniverse()->GetInterpreter()->GetFrame(),
            method->GetNumberOfArguments());
    TEST_WB_CALLED("VMBlock failed to call writeBarrier when creating", block,
            block->GetClass());
    TEST_WB_CALLED("VMBlock failed to call writeBarrier when creating", block,
            block->GetMethod());
    TEST_WB_CALLED("VMBlock failed to call writeBarrier when creating", block,
            block->GetContext());
}

void WriteBarrierTest::testWriteFrame() {
    if (!DEBUG) {
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
    }

    // reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();

    VMFrame* frame = GetUniverse()->GetInterpreter()->GetFrame()->CloneForMovingGC();
    frame->SetContext(frame->CloneForMovingGC());

    frame->SetPreviousFrame(GetUniverse()->GetInterpreter()->GetFrame());
    TEST_WB_CALLED("VMFrame failed to call writeBarrier on SetPreviousFrame", frame, GetUniverse()->GetInterpreter()->GetFrame());
    frame->SetContext(frame->GetContext()->CloneForMovingGC());
    TEST_WB_CALLED("VMFrame failed to call writeBarrier on SetContext", frame, frame->GetContext());
    frame->ClearPreviousFrame();
    TEST_WB_CALLED("VMFrame failed to call writeBarrier on ClearPreviousFrame", frame, load_ptr(nilObject));
}

void WriteBarrierTest::testWriteMethod() {
    if (!DEBUG) {
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
    }

    // reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();
    VMMethod* method = GetUniverse()->GetInterpreter()->GetFrame()->GetMethod()->CloneForMovingGC();
    method->SetHolder(load_ptr(integerClass));
    TEST_WB_CALLED("VMMethod failed to call writeBarrier on SetHolder", method, load_ptr(integerClass));
}

void WriteBarrierTest::testWriteEvaluationPrimitive() {
    if (!DEBUG) {
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
    }

    //reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();
    VMEvaluationPrimitive* evPrim = new (GetHeap<HEAP_CLS>(), 0) VMEvaluationPrimitive(1);
    TEST_WB_CALLED("VMEvaluationPrimitive failed to call writeBarrier when creating", evPrim, evPrim->GetClass());
}

void WriteBarrierTest::testWriteClass() {
    if (!DEBUG) {
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
    }

    //reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();
    VMClass* cl = load_ptr(integerClass)->CloneForMovingGC();
    //now test all methods that change members
    cl->SetSuperClass(load_ptr(integerClass));
    TEST_WB_CALLED("VMClass failed to call writeBarrier on SetSuperClass", cl,
            load_ptr(integerClass));
    VMSymbol* newName = NewSymbol("andererName");
    cl->SetName(newName);
    TEST_WB_CALLED("VMClass failed to call writeBarrier on SetName", cl,
            newName);
    VMArray* newInstFields = cl->GetInstanceFields()->CloneForMovingGC();
    cl->SetInstanceFields(newInstFields);
    TEST_WB_CALLED("VMClass failed to call writeBarrier on SetInstanceFields", cl,
            newName);
    VMArray* newInstInvokables = cl->GetInstanceInvokables()->CloneForMovingGC();
    cl->SetInstanceInvokables(newInstInvokables);
    TEST_WB_CALLED("VMClass failed to call writeBarrier on SetInstanceInvokables", cl,
            newName);
}
#endif
