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

//#define __DEBUG
#include <map>
#include <vector>

#include "../misc/defs.h"
#include "../misc/Timer.h"
#include "../misc/ExtendedList.h"

#include "../vmobjects/ObjectFormats.h"

#include "../interpreter/Interpreter.h"

#include "../memory/Heap.h"

class SourcecodeCompiler;

// for runtime debug
extern short dumpBytecodes;
extern short gcVerbosity;

//global VMObjects
extern pVMObject nilObject;
extern pVMObject trueObject;
extern pVMObject falseObject;

extern pVMClass objectClass;
extern pVMClass classClass;
extern pVMClass metaClassClass;

extern pVMClass nilClass;
extern pVMClass integerClass;
extern pVMClass bigIntegerClass;
extern pVMClass arrayClass;
extern pVMClass methodClass;
extern pVMClass symbolClass;
extern pVMClass primitiveClass;
extern pVMClass stringClass;
extern pVMClass systemClass;
extern pVMClass blockClass;
extern pVMClass doubleClass;

extern pVMClass trueClass;
extern pVMClass falseClass;

extern pVMSymbol symbolIfTrue;
extern pVMSymbol symbolIfFalse;

using namespace std;
class Universe {
public:
    inline Universe* operator->();

    //static methods
    static void Start(long argc, char** argv);
    static void Quit(long);
    static void ErrorExit(const char*);

    Interpreter* GetInterpreter() {
        return interpreter;
    }
    
    void Assert(bool) const;

    pVMSymbol SymbolFor(const StdString&);
    pVMSymbol SymbolForChars(const char*);

    //VMObject instanciation methods. These should probably be refactored to a new class
    VMArray* NewArray(long) const;
    VMArray* NewArrayList(ExtendedList<oop_t>& list) const;
    VMArray* NewArrayList(ExtendedList<pVMSymbol>& list) const;
    VMArray* NewArrayFromStrings(const vector<StdString>&) const;
    VMBlock* NewBlock(pVMMethod, pVMFrame, long);
    pVMClass NewClass(pVMClass) const;
    pVMFrame NewFrame(pVMFrame, pVMMethod) const;
    pVMMethod NewMethod(pVMSymbol, size_t, size_t) const;
    pVMObject NewInstance(pVMClass) const;
    pVMInteger NewInteger(long) const;
    void WalkGlobals(oop_t (*walk)(oop_t));
    VMBigInteger* NewBigInteger(int64_t) const;
    VMDouble* NewDouble(double) const;
    pVMClass NewMetaclassClass(void) const;
    pVMString NewString(const StdString&) const;
    pVMSymbol NewSymbol(const StdString&);
    pVMString NewString(const char*) const;
    pVMSymbol NewSymbol(const char*);
    pVMClass NewSystemClass(void) const;

    void InitializeSystemClass(pVMClass, pVMClass, const char*);

    oop_t GetGlobal(pVMSymbol);
    void SetGlobal(pVMSymbol name, oop_t val);
    bool HasGlobal(pVMSymbol);
    void InitializeGlobals();
    pVMClass GetBlockClass(void) const;
    pVMClass GetBlockClassWithArgs(long);

    pVMClass LoadClass(pVMSymbol);
    void LoadSystemClass(pVMClass);
    pVMClass LoadClassBasic(pVMSymbol, pVMClass);
    pVMClass LoadShellClass(StdString&);

    Universe();
    ~Universe();
#ifdef LOG_RECEIVER_TYPES
    struct stat_data {
        long noCalls;
        long noPrimitiveCalls;
    };
    map<StdString, long> receiverTypes;
    map<StdString, stat_data> callStats;
#endif
    //
    
    static bool IsValidObject(oop_t obj);
private:
    vector<StdString> handleArguments(long argc, char** argv);
    long getClassPathExt(vector<StdString>& tokens, const StdString& arg) const;

    friend Universe* GetUniverse();
    static Universe* theUniverse;

    long setupClassPath(const StdString& cp);
    long addClassPath(const StdString& cp);
    void printUsageAndExit(char* executable) const;

    void initialize(long, char**);

    long heapSize;
    map<pVMSymbol, oop_t> globals;
    map<long,pVMClass> blockClassesByNoOfArgs;
    vector<StdString> classPath;

    Interpreter* interpreter;
};

//Singleton accessor
inline Universe* GetUniverse() __attribute__ ((always_inline));
Universe* GetUniverse() {
    if (DEBUG && !Universe::theUniverse) {
        Universe::ErrorExit("Trying to access uninitialized Universe, exiting.");
    }
    return Universe::theUniverse;
}

Universe* Universe::operator->() {
    if (DEBUG && !theUniverse) {
        ErrorExit("Trying to access uninitialized Universe, exiting.");
    }
    return theUniverse;
}
