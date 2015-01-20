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

#include <misc/defs.h>
#include <misc/Timer.h>
#include <misc/ExtendedList.h>

#include <vmobjects/ObjectFormats.h>

#include <memory/PagedHeap.h>
#include <memory/Worklist.h>

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

extern GCClass* threadClass;
extern GCClass* mutexClass;
extern GCClass* signalClass;

extern GCSymbol* symbolIfTrue;
extern GCSymbol* symbolIfFalse;

using namespace std;
class Universe {
public:
    //static methods
    static void Start(long argc, char** argv);
    static void Quit(long);
    static void ErrorExit(StdString);

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
    VMDouble* NewDouble(double, Page* page) const;
    VMClass* NewMetaclassClass(Page* page) const;
    VMString* NewString(const StdString&, Page* page) const;
    VMSymbol* NewSymbol(const StdString&, Page* page);
    VMString* NewString(const char*, Page* page) const;
    VMSymbol* NewSymbol(const char*, Page* page);
    VMClass* NewSystemClass(Page* page) const;

    VMMutex* NewMutex(Page* page) const;
    VMSignal* NewSignal(Page* page) const;
    VMThread* NewThread(Page* page) const;
   
#if GC_TYPE==PAUSELESS
    void MarkGlobals();
    void  CheckMarkingGlobals(void (vm_oop_t));
#else
    void WalkGlobals(walk_heap_fn);
#endif

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
    size_t heapSize;
    size_t pageSize;

    map<GCSymbol*, gc_oop_t> globals;
    map<long, GCClass*> blockClassesByNoOfArgs;
    vector<StdString> classPath;

    static mutex output_mutex;
};

//Singleton accessor
inline Universe* GetUniverse() __attribute__ ((always_inline));
Universe* GetUniverse() {
    /*if (DEBUG && !Universe::theUniverse) {
        Universe::ErrorExit("Trying to access uninitialized Universe, exiting.");
    }*/
    return Universe::theUniverse;
}
