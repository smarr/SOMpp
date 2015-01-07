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
#include <mutex>
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
extern GCObject* nilObject;
extern GCObject* trueObject;
extern GCObject* falseObject;

extern GCClass* objectClass;
extern GCClass* classClass;
extern GCClass* metaClassClass;

extern GCClass* nilClass;
extern GCClass* integerClass;
extern GCClass* arrayClass;
extern GCClass* methodClass;
extern GCClass* symbolClass;
extern GCClass* primitiveClass;
extern GCClass* stringClass;
extern GCClass* systemClass;
extern GCClass* blockClass;
extern GCClass* doubleClass;

extern GCClass* trueClass;
extern GCClass* falseClass;

extern GCSymbol* symbolIfTrue;
extern GCSymbol* symbolIfFalse;

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

    VMSymbol* SymbolFor(const StdString&);
    VMSymbol* SymbolForChars(const char*);

    //VMObject instanciation methods. These should probably be refactored to a new class
    VMArray* NewArray(long) const;
    VMArray* NewArrayList(ExtendedList<vm_oop_t>& list) const;
    VMArray* NewArrayList(ExtendedList<VMInvokable*>& list) const;
    VMArray* NewArrayList(ExtendedList<VMSymbol*>& list) const;
    VMArray* NewArrayFromStrings(const vector<StdString>&) const;
    VMBlock* NewBlock(VMMethod*, VMFrame*, long);
    VMClass* NewClass(VMClass*) const;
    VMFrame* NewFrame(VMFrame*, VMMethod*) const;
    VMMethod* NewMethod(VMSymbol*, size_t, size_t) const;
    VMObject* NewInstance(VMClass*) const;
    VMInteger* NewInteger(int64_t) const;
    void WalkGlobals(walk_heap_fn);
    VMDouble* NewDouble(double) const;
    VMClass* NewMetaclassClass(void) const;
    VMString* NewString(const StdString&) const;
    VMSymbol* NewSymbol(const StdString&);
    VMString* NewString(const char*) const;
    VMSymbol* NewSymbol(const char*);
    VMClass* NewSystemClass(void) const;

    void InitializeSystemClass(VMClass*, VMClass*, const char*);

    vm_oop_t GetGlobal(VMSymbol*);
    void SetGlobal(VMSymbol* name, vm_oop_t val);
    bool HasGlobal(VMSymbol*);
    VMObject* InitializeGlobals();
    VMClass* GetBlockClass(void) const;
    VMClass* GetBlockClassWithArgs(long);

    VMClass* LoadClass(VMSymbol*);
    void LoadSystemClass(VMClass*);
    VMClass* LoadClassBasic(VMSymbol*, VMClass*);
    VMClass* LoadShellClass(StdString&);

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
    
    static bool IsValidObject(vm_oop_t obj);
    
    static void Print(StdString str);
    static void ErrorPrint(StdString str);

private:
    vector<StdString> handleArguments(long argc, char** argv);
    long getClassPathExt(vector<StdString>& tokens, const StdString& arg) const;

    VMMethod* createBootstrapMethod(VMClass* holder, long numArgsOfMsgSend);

    friend Universe* GetUniverse();
    static Universe* theUniverse;

    long setupClassPath(const StdString& cp);
    long addClassPath(const StdString& cp);
    void printUsageAndExit(char* executable) const;

    void initialize(long, char**);

    long heapSize;
    map<GCSymbol*, gc_oop_t> globals;
    map<StdString, GCSymbol*> symbolsMap;
    map<long, GCClass*> blockClassesByNoOfArgs;
    vector<StdString> classPath;

    Interpreter* interpreter;
    
    static mutex output_mutex;
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
