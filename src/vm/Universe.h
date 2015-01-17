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
#include <unordered_set>

#include <misc/defs.h>
#include <misc/Timer.h>
#include <misc/ExtendedList.h>

#include <vmobjects/ObjectFormats.h>

#include <interpreter/Interpreter.h>

#include <memory/Heap.h>

class SourcecodeCompiler;

// for runtime debug
extern short dumpBytecodes;
extern short gcVerbosity;

// global VMObjects
extern GCObject* nilObject;
extern GCObject* trueObject;
extern GCObject* falseObject;
extern GCObject* systemObject;

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

extern GCClass* conditionClass;
extern GCClass* mutexClass;
extern GCClass* threadClass;

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
    
    void Assert(bool) const;

    VMSymbol* SymbolFor(const StdString&, Page*);
    VMSymbol* SymbolForChars(const char*, Page*);

    //VMObject instanciation methods. These should probably be refactored to a new class
    VMArray* NewArray(long, Page* page) const;
    VMArray* NewArrayList(ExtendedList<vm_oop_t>& list, Page* page) const;
    VMArray* NewArrayList(ExtendedList<VMInvokable*>& list, Page* page) const;
    VMArray* NewArrayList(ExtendedList<VMSymbol*>& list, Page* page) const;
    VMArray* NewArrayFromStrings(const vector<StdString>&, Page* page) const;
    VMBlock* NewBlock(VMMethod*, VMFrame*, long, Page* page);
    VMClass* NewClass(VMClass*, Page* page) const;
    VMFrame* NewFrame(VMFrame*, VMMethod*, Page* page) const;
    VMMethod* NewMethod(VMSymbol*, size_t, size_t, Page* page) const;
    VMObject* NewInstance(VMClass*, Page* page) const;
    VMInteger* NewInteger(int64_t, Page* page) const;
    void WalkGlobals(walk_heap_fn, Page*);
    VMDouble* NewDouble(double, Page* page) const;
    VMClass* NewMetaclassClass(Page* page) const;
    VMString* NewString(const StdString&, Page* page) const;
    VMSymbol* NewSymbol(const StdString&, Page* page);
    VMString* NewString(const char*, Page* page) const;
    VMSymbol* NewSymbol(const char*, Page* page);
    VMClass* NewSystemClass(Page* page) const;

    VMCondition* NewCondition(VMMutex*, Page* page) const;
    VMMutex*     NewMutex(Page* page) const;
    VMThread*    NewThread(VMBlock* block, vm_oop_t arguments, Page* page);

    void InitializeSystemClass(VMClass*, VMClass*, const char*, Page*);

    vm_oop_t GetGlobal(VMSymbol*);
    void SetGlobal(VMSymbol* name, vm_oop_t val);
    bool HasGlobal(VMSymbol*);
    VMObject* InitializeGlobals(Page* page);
    VMClass* GetBlockClass(void) const;
    VMClass* GetBlockClassWithArgs(long, Page*);

    VMClass* LoadClass(VMSymbol*, Page*);
    void LoadSystemClass(VMClass*, Page*);
    VMClass* LoadClassBasic(VMSymbol*, VMClass*, Page*);
    VMClass* LoadShellClass(StdString&, Page*);
    
    unordered_set<Interpreter*>* GetInterpreters() { return &interpreters; }

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

    VMMethod* createBootstrapMethod(VMClass* holder, long numArgsOfMsgSend, Page*);
    void startInterpreterInThread(VMThread* thread, VMBlock* block, vm_oop_t arguments);
    
    void registerInterpreter(Interpreter*);
    void unregisterInterpreter(Interpreter*);

    friend Universe* GetUniverse();
    static Universe* theUniverse;

    long setupClassPath(const StdString& cp);
    long addClassPath(const StdString& cp);
    void printUsageAndExit(char* executable) const;

    void initialize(long, char**);
    
    bool isPowerOfTwo(size_t val) {
        return !(val == 0) && !(val & (val - 1));
    }

    size_t heapSize;
    size_t pageSize;

    map<GCSymbol*, gc_oop_t> globals;
    map<StdString, GCSymbol*> symbolsMap;
    recursive_mutex globalsAndSymbols_mutex;
    
    
    map<long, GCClass*> blockClassesByNoOfArgs;
    vector<StdString> classPath;

    unordered_set<Interpreter*> interpreters;
    mutex interpreters_mutex;
    
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
