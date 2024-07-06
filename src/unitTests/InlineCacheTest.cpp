#include "InlineCacheTest.h"

#include <cppunit/TestAssert.h>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "../compiler/LexicalScope.h"
#include "../interpreter/bytecodes.h"
#include "../vm/Globals.h"
#include "../vm/IsValidObject.h"
#include "../vm/Symbols.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMDouble.h"
#include "../vmobjects/VMMethod.h"
#include "WalkObjectsTest.h"

void InlineCacheTest::testCaching() {
    cachingAtBytecodeIndex(3);
    cachingAtBytecodeIndex(8);
}

void InlineCacheTest::cachingAtBytecodeIndex(size_t bytecodeIndex) {
    ClearWalkedObjects();

    const size_t numBytecodes = 10;

    VMSymbol* methodSymbol = SymbolFor("frameMethod");
    LexicalScope scope{nullptr, {}, {}};
    vector<BackJump> jumps;
    VMMethod* method = GetUniverse()->NewMethod(methodSymbol, numBytecodes, 5,
                                                0, 0, &scope, jumps);

    for (size_t i = 0; i < numBytecodes; i += 1) {
        // this does not really matter, and is not used
        // I just want to put something semi-sensible here, and make sure
        // I can possibly see writing over boundaries
        method->SetBytecode(i, BC_SEND);  // this is 31 at the moment
    }

    for (size_t i = 0; i < 5; i += 1) {
        method->SetIndexableField(i, GetUniverse()->NewDouble(i));
    }

    VMSymbol* toDoSym = SymbolFor("to:do:");
    VMInvokable* ivkInt =
        method->LookupWithCache(toDoSym, load_ptr(integerClass), bytecodeIndex);
    CPPUNIT_ASSERT_MESSAGE("Expected an invokable", ivkInt != nullptr);

    VMInvokable* ivkDbl =
        method->LookupWithCache(toDoSym, load_ptr(doubleClass), bytecodeIndex);
    CPPUNIT_ASSERT_MESSAGE("Expected an invokable", ivkDbl != nullptr);

    CPPUNIT_ASSERT_MESSAGE("Expected two different ones", ivkInt != ivkDbl);

    VMInvokable* ivkInt2 =
        method->LookupWithCache(toDoSym, load_ptr(integerClass), bytecodeIndex);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Expected same", ivkInt, ivkInt2);

    VMInvokable* ivkDbl2 =
        method->LookupWithCache(toDoSym, load_ptr(doubleClass), bytecodeIndex);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Expected same", ivkDbl, ivkDbl2);

    for (size_t i = 0; i < numBytecodes; i += 1) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Expected bytecodes to be unchanged",
                                     method->GetBytecode(i), (uint8_t)BC_SEND);
    }

    for (size_t i = 0; i < 5; i += 1) {
        vm_oop_t ptr = method->GetIndexableField(i);
        CPPUNIT_ASSERT_MESSAGE("Expected a double", IsVMDouble(ptr));
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
            "Expect the double I created.", i,
            ((VMDouble*)ptr)->GetEmbeddedDouble(), 0.0);
    }

    for (size_t i = 0; i < numBytecodes * 2; i += 1) {
        size_t lowEndOfCacheUsed = bytecodeIndex * 2;
        size_t highEndOfCacheUsed = (bytecodeIndex + 2) * 2;
        if (i < lowEndOfCacheUsed) {
            CPPUNIT_ASSERT_MESSAGE("Expected a nullptr",
                                   nullptr == method->inlineCache[i]);
        } else if (i < highEndOfCacheUsed) {
            CPPUNIT_ASSERT_MESSAGE("Expected a cache entry",
                                   nullptr != method->inlineCache[i]);
        } else {
            CPPUNIT_ASSERT_MESSAGE("Expected a nullptr",
                                   nullptr == method->inlineCache[i]);
        }
    }

    method->WalkObjects(collectMembers);

    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(methodSymbol)));
    CPPUNIT_ASSERT(WalkerHasFound(integerClass));
    CPPUNIT_ASSERT(WalkerHasFound(doubleClass));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(ivkInt)));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(ivkInt2)));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(ivkDbl)));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(ivkDbl2)));
}
