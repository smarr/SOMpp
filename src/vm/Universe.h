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

#include <map>
#include <vector>

#include "../interpreter/Interpreter.h"
#include "../memory/Heap.h"
#include "../misc/Timer.h"
#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"

class SourcecodeCompiler;

// for runtime debug
extern short dumpBytecodes;
extern short gcVerbosity;

using namespace std;
class Universe {
public:
    inline Universe* operator->();

    // static methods
    static void Start(long argc, char** argv);
    static void BasicInit();
    __attribute__((noreturn)) static void Quit(long);
    __attribute__((noreturn)) static void ErrorExit(const char*);

    vm_oop_t interpret(StdString className, StdString methodName);

    long setupClassPath(const StdString& cp);

    Interpreter* GetInterpreter() { return interpreter; }

    void Assert(bool) const;

    // VMObject instanciation methods. These should probably be refactored to a
    // new class
    VMArray* NewArray(size_t) const;

    VMArray* NewArrayList(std::vector<vm_oop_t>& list) const;
    VMArray* NewArrayList(std::vector<VMInvokable*>& list) const;
    VMArray* NewArrayList(std::vector<VMSymbol*>& list) const;

    VMArray* NewArrayFromStrings(const vector<StdString>&) const;
    VMArray* NewArrayOfSymbolsFromStrings(const vector<StdString>&) const;

    VMBlock* NewBlock(VMInvokable*, VMFrame*, long);
    VMClass* NewClass(VMClass*) const;
    VMFrame* NewFrame(VMFrame*, VMMethod*) const;
    VMMethod* NewMethod(VMSymbol*, size_t numberOfBytecodes,
                        size_t numberOfConstants, size_t numLocals,
                        size_t maxStackDepth, LexicalScope*,
                        vector<BackJump>& inlinedLoops) const;
    VMObject* NewInstance(VMClass*) const;
    VMObject* NewInstanceWithoutFields() const;
    VMInteger* NewInteger(int64_t) const;
    void WalkGlobals(walk_heap_fn);
    VMDouble* NewDouble(double) const;
    VMClass* NewMetaclassClass(void) const;
    VMString* NewString(const StdString&) const;
    VMString* NewString(const size_t, const char*) const;
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

private:
    vm_oop_t interpretMethod(VMObject* receiver, VMInvokable* initialize,
                             VMArray* argumentsArray);

    vector<StdString> handleArguments(long argc, char** argv);
    long getClassPathExt(vector<StdString>& tokens, const StdString& arg) const;

    VMMethod* createBootstrapMethod(VMClass* holder, long numArgsOfMsgSend);

    friend Universe* GetUniverse();
    static Universe* theUniverse;

    long addClassPath(const StdString& cp);
    void printUsageAndExit(char* executable) const;

    void initialize(long, char**);

    long heapSize;
    map<GCSymbol*, gc_oop_t> globals;

    map<long, GCClass*> blockClassesByNoOfArgs;
    vector<StdString> classPath;

    Interpreter* interpreter;
};

// Singleton accessor
inline Universe* GetUniverse() __attribute__((always_inline));
Universe* GetUniverse() {
    if (DEBUG && Universe::theUniverse == nullptr) {
        Universe::ErrorExit(
            "Trying to access uninitialized Universe, exiting.");
    }
    return Universe::theUniverse;
}

Universe* Universe::operator->() {
    if (DEBUG && theUniverse == nullptr) {
        ErrorExit("Trying to access uninitialized Universe, exiting.");
    }
    return theUniverse;
}
