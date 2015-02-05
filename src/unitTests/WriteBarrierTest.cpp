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
    if (!DEBUG) {
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
    }
    
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    //reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();
    VMArray* arr = GetUniverse()->NewArray(3, page);
    VMInteger* newInt = GetUniverse()->NewInteger(12345, page);
    VMString* str = GetUniverse()->NewString("asdfghjkl", page);
    VMDouble* doub = GetUniverse()->NewDouble(9876.654, page);
    VMClass* cloneClass = load_ptr(arrayClass)->Clone(page);
    VMClass* clone2Class = cloneClass->Clone(page);
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
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    Interpreter interp(page);
    page->SetInterpreter(&interp);
    
    //reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();

    VMSymbol* methodSymbol = GetUniverse()->NewSymbol("someMethod", page);
    VMMethod* method = GetUniverse()->NewMethod(methodSymbol, 0, 0, page);
    VMBlock* block = GetUniverse()->NewBlock(method,
            GetUniverse()->NewFrame(nullptr, method, page),
            method->GetNumberOfArguments(), page);
    TEST_WB_CALLED("VMBlock failed to call writeBarrier when creating", block,
            block->GetClass());
    TEST_WB_CALLED("VMBlock failed to call writeBarrier when creating", block,
            block->GetMethod());
    TEST_WB_CALLED("VMBlock failed to call writeBarrier when creating", block,
            block->GetContext());

    block->SetMethod(method->Clone(page));
    TEST_WB_CALLED("VMBlock failed to call writeBarrier on SetMethod", block,
            block->GetMethod());
    block->SetContext(block->GetContext()->Clone(page));
    TEST_WB_CALLED("VMBlock failed to call writeBarrier on SetContext", block,
            block->GetContext());
}

void WriteBarrierTest::testWriteFrame() {
    if (!DEBUG) {
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
    }
    
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    Interpreter interp(page);
    page->SetInterpreter(&interp);
    
    // reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();
    
    VMSymbol* methodSymbol = GetUniverse()->NewSymbol("someMethod", page);
    VMMethod* method = GetUniverse()->NewMethod(methodSymbol, 0, 0, page);

    VMFrame* frame = GetUniverse()->NewFrame(nullptr, method, page);
    frame->SetContext(frame->Clone(page));

    frame->SetPreviousFrame(GetUniverse()->NewFrame(nullptr, method, page));
    TEST_WB_CALLED("VMFrame failed to call writeBarrier on SetPreviousFrame", frame, GetUniverse()->NewFrame(nullptr, method, page));
    frame->SetContext(frame->GetContext()->Clone(page));
    TEST_WB_CALLED("VMFrame failed to call writeBarrier on SetContext", frame, frame->GetContext());
    frame->ClearPreviousFrame();
    TEST_WB_CALLED("VMFrame failed to call writeBarrier on ClearPreviousFrame", frame, load_ptr(nilObject));
}

void WriteBarrierTest::testWriteMethod() {
    if (!DEBUG) {
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
    }
    
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    // reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();

    VMSymbol* methodSymbol = GetUniverse()->NewSymbol("someMethod", page);
    VMMethod* method = GetUniverse()->NewMethod(methodSymbol, 0, 0, page);

    method->SetHolder(load_ptr(integerClass));
    TEST_WB_CALLED("VMMethod failed to call writeBarrier on SetHolder", method, load_ptr(integerClass));
    method->SetSignature(method->GetSignature(), page);
    TEST_WB_CALLED("VMMethod failed to call writeBarrier on SetSignature", method, method->GetSignature());
}

void WriteBarrierTest::testWriteEvaluationPrimitive() {
    if (!DEBUG) {
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
    }
    
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    //reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();
    VMEvaluationPrimitive* evPrim = new (page) VMEvaluationPrimitive(1, page);
    TEST_WB_CALLED("VMEvaluationPrimitive failed to call writeBarrier when creating", evPrim, evPrim->GetClass());
    TEST_WB_CALLED("VMEvaluationPrimitive failed to call writeBarrier when creating", evPrim, load_ptr(evPrim->numberOfArguments));
}

void WriteBarrierTest::testWriteClass() {
    if (!DEBUG) {
        CPPUNIT_FAIL("WriteBarrier tests only work in DEBUG builds for speed reasons");
    }
    
    Page* page = GetHeap<HEAP_CLS>()->RegisterThread();
    
    //reset set...
    GetHeap<HEAP_CLS>()->writeBarrierCalledOn.clear();
    VMClass* cl = load_ptr(integerClass)->Clone(page);
    //now test all methods that change members
    cl->SetSuperClass(load_ptr(integerClass));
    TEST_WB_CALLED("VMClass failed to call writeBarrier on SetSuperClass", cl,
            load_ptr(integerClass));
    VMSymbol* newName = GetUniverse()->NewSymbol("andererName", page);
    cl->SetName(newName);
    TEST_WB_CALLED("VMClass failed to call writeBarrier on SetName", cl,
            newName);
    VMArray* newInstFields = cl->GetInstanceFields()->Clone(page);
    cl->SetInstanceFields(newInstFields);
    TEST_WB_CALLED("VMClass failed to call writeBarrier on SetInstanceFields", cl,
            newName);
    VMArray* newInstInvokables = cl->GetInstanceInvokables()->Clone(page);
    cl->SetInstanceInvokables(newInstInvokables);
    TEST_WB_CALLED("VMClass failed to call writeBarrier on SetInstanceInvokables", cl,
            newName);
}
#endif

