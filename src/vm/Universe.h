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
#include "Print.h"

class SourcecodeCompiler;

// for runtime debug
extern short dumpBytecodes;
extern short gcVerbosity;

using namespace std;
class Universe {
public:
    // static methods
    static void Start(long argc, char** argv);
    static void BasicInit();

    static vm_oop_t interpret(StdString className, StdString methodName);

    static long setupClassPath(const StdString& cp);

    static void Assert(bool);

    // VMObject instanciation methods. These should probably be refactored to a
    // new class
    static VMArray* NewArray(size_t);

    static VMArray* NewArrayList(std::vector<vm_oop_t>& list);
    static VMArray* NewArrayList(std::vector<VMInvokable*>& list);
    static VMArray* NewArrayList(std::vector<VMSymbol*>& list);

    static VMArray* NewArrayFromStrings(const vector<StdString>&);
    static VMArray* NewArrayOfSymbolsFromStrings(const vector<StdString>&);

    static VMBlock* NewBlock(VMInvokable*, VMFrame*, long);
    static VMClass* NewClass(VMClass*);
    static VMFrame* NewFrame(VMFrame*, VMMethod*);
    static VMMethod* NewMethod(VMSymbol*, size_t numberOfBytecodes,
                               size_t numberOfConstants, size_t numLocals,
                               size_t maxStackDepth, LexicalScope*,
                               vector<BackJump>& inlinedLoops);
    static VMObject* NewInstance(VMClass*);
    static VMObject* NewInstanceWithoutFields();
    static VMInteger* NewInteger(int64_t);
    static void WalkGlobals(walk_heap_fn);
    static VMDouble* NewDouble(double);
    static VMClass* NewMetaclassClass();
    static VMString* NewString(const StdString&);
    static VMString* NewString(const size_t, const char*);
    static VMClass* NewSystemClass();

    static void InitializeSystemClass(VMClass*, VMClass*, const char*);

    static vm_oop_t GetGlobal(VMSymbol*);
    static void SetGlobal(VMSymbol* name, vm_oop_t val);
    static bool HasGlobal(VMSymbol*);
    static VMObject* InitializeGlobals();
    static VMClass* GetBlockClass();
    static VMClass* GetBlockClassWithArgs(long);

    static VMClass* LoadClass(VMSymbol*);
    static void LoadSystemClass(VMClass*);
    static VMClass* LoadClassBasic(VMSymbol*, VMClass*);
    static VMClass* LoadShellClass(StdString&);

    Universe() {}
    ~Universe();
#ifdef LOG_RECEIVER_TYPES
    struct stat_data {
        long noCalls;
        long noPrimitiveCalls;
    };
    static map<StdString, long> receiverTypes;
    static map<StdString, stat_data> callStats;
#endif
    //

    static void Shutdown();

private:
    static vm_oop_t interpretMethod(VMObject* receiver, VMInvokable* initialize,
                                    VMArray* argumentsArray);

    static vector<StdString> handleArguments(long argc, char** argv);
    static long getClassPathExt(vector<StdString>& tokens,
                                const StdString& arg);

    static VMMethod* createBootstrapMethod(VMClass* holder,
                                           long numArgsOfMsgSend);

    static long addClassPath(const StdString& cp);
    static void printUsageAndExit(char* executable);

    static void initialize(long, char**);

    static long heapSize;
    static map<GCSymbol*, gc_oop_t> globals;

    static map<long, GCClass*> blockClassesByNoOfArgs;
    static vector<StdString> classPath;
};
