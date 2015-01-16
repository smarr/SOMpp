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

#include <misc/defs.h>
#include <misc/Timer.h>
#include <misc/ExtendedList.h>

#include <vmobjects/ObjectFormats.h>

#include <memory/PagedHeap.h>

#if GC_TYPE==PAUSELESS
#include <memory/pauseless/Worklist.h>
#endif

class Interpreter;

class SourcecodeCompiler;


//Convenience macro to get access to an interpreters memory page
#define _PAGE GetUniverse()->GetInterpreter()->GetPage()

// for runtime debug
extern short dumpBytecodes;
extern short gcVerbosity;

// global VMObjects
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

extern GCClass* threadClass;
extern GCClass* mutexClass;
extern GCClass* signalClass;

using namespace std;
class Universe {
public:
    //static methods
    static void Start(long argc, char** argv);
    static void Quit(long);
    static void ErrorExit(const char*);

    //Globals accessor (only for GC, could be considered be
    //declared as a private friend method for the GC)
    map<GCSymbol*, gc_oop_t> GetGlobals() {
        return globals;
    }
    PagedHeap* GetHeap() {
        return heap;
    }
    
    FORCE_INLINE Interpreter* GetInterpreter() { return (Interpreter*)pthread_getspecific(interpreterKey); }
    Interpreter* NewInterpreter();
    void RemoveInterpreter();
    
#if GC_TYPE==PAUSELESS
    unique_ptr<vector<Interpreter*>> GetInterpretersCopy();
#else
    vector<Interpreter*>* GetInterpreters();
#endif

    void Assert(bool) const;

    VMSymbol* SymbolFor(const StdString&);
    VMSymbol* SymbolForChars(const char*);

    //VMObject instanciation methods. These should probably be refactored to a new class
    VMArray* NewArray(long) const;
    VMArray* NewArrayList(ExtendedList<VMObject*>& list) const;
    VMArray* NewArrayList(ExtendedList<VMSymbol*>& list) const;
    VMArray* NewArrayFromStrings(const vector<StdString>&) const;
    VMBlock* NewBlock(VMMethod*, VMFrame*, long);
    VMClass* NewClass(VMClass*) const;
    VMFrame* NewFrame(VMFrame*, VMMethod*) const;
    VMMethod* NewMethod(VMSymbol*, size_t, size_t) const;
    VMObject* NewInstance(VMClass*) const;
    VMInteger* NewInteger(long) const;
    VMDouble* NewDouble(double) const;
    VMClass* NewMetaclassClass(void) const;
    VMString* NewString(const StdString&) const;
    VMSymbol* NewSymbol(const StdString&);
    VMString* NewString(const char*) const;
    VMSymbol* NewSymbol(const char*);
    VMClass* NewSystemClass(void) const;

    VMMutex* NewMutex() const;
    VMSignal* NewSignal() const;
    VMThread* NewThread() const;
   
#if GC_TYPE==PAUSELESS
    void MarkGlobals();
    void  CheckMarkingGlobals(void (AbstractVMObject*));
#else
    void WalkGlobals(VMOBJECT_PTR (*walk)(VMOBJECT_PTR));
#endif

    void InitializeSystemClass(VMClass*, VMClass*, const char*);

    vm_oop_t GetGlobal(VMSymbol*);
    void SetGlobal(VMSymbol* name, vm_oop_t val);
    void InitializeGlobals();
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
    
    //void PrintGlobals();
    
private:
    
    pthread_mutex_t testMutex;
    
    pthread_mutex_t interpreterMutex;
    pthread_key_t interpreterKey;
    vector<Interpreter*> interpreters;

    pthread_mutexattr_t attrclassLoading;
    pthread_mutex_t classLoading;
    
    vector<StdString> handleArguments(long argc, char** argv);
    long getClassPathExt(vector<StdString>& tokens, const StdString& arg) const;

    friend Universe* GetUniverse();
    static Universe* theUniverse;

    long setupClassPath(const StdString& cp);
    long addClassPath(const StdString& cp);
    void printUsageAndExit(char* executable) const;

    void initialize(long, char**);

    PagedHeap* heap;
    long heapSize;
    long pageSize;

    map<GCSymbol*, gc_oop_t> globals;
    map<long, GCClass*> blockClassesByNoOfArgs;
    vector<StdString> classPath;
};


//Singleton accessor
inline Universe* GetUniverse() __attribute__ ((always_inline));
Universe* GetUniverse() {
    /*if (DEBUG && !Universe::theUniverse) {
        Universe::ErrorExit("Trying to access uninitialized Universe, exiting.");
    }*/
    return Universe::theUniverse;
}
