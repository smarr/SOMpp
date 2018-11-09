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

#include <sstream> 
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <iomanip>

#include "Universe.h"
#include "Shell.h"

#include <vmobjects/VMSymbol.h>
#include <vmobjects/VMObject.h>
#include <vmobjects/VMMethod.h>
#include <vmobjects/VMClass.h>
#include <vmobjects/VMFrame.h>
#include <vmobjects/VMArray.h>
#include <vmobjects/VMBlock.h>
#include <vmobjects/VMDouble.h>
#include <vmobjects/VMInteger.h>
#include <vmobjects/VMString.h>
#include <vmobjects/VMEvaluationPrimitive.h>

#include <interpreter/bytecodes.h>

#include <compiler/Disassembler.h>
#include <compiler/SourcecodeCompiler.h>

#include "../vmobjects/IntegerBox.h"

#if GC_TYPE == OMR_GARBAGE_COLLECTION
#include "fvtest/compilertest/Jit.hpp"
#include "ilgen/TypeDictionary.hpp"
#include "../../omrglue/SOMppMethod.hpp"
#include "../../omr/include_core/omrvm.h"
#include "../../omr/include_core/omrlinkedlist.h"
#endif

#if CACHE_INTEGER
gc_oop_t prebuildInts[INT_CACHE_MAX_VALUE - INT_CACHE_MIN_VALUE + 1];
#endif

#define INT_HIST_SIZE 1

// Here we go:

short dumpBytecodes;
short gcVerbosity;
#if GC_TYPE == OMR_GARBAGE_COLLECTION
bool enableJIT;
#endif

Universe* Universe::theUniverse = nullptr;

GCObject* nilObject;
GCObject* trueObject;
GCObject* falseObject;

GCClass* objectClass;
GCClass* classClass;
GCClass* metaClassClass;

GCClass* nilClass;
GCClass* integerClass;
GCClass* arrayClass;
GCClass* methodClass;
GCClass* symbolClass;
GCClass* primitiveClass;
GCClass* stringClass;
GCClass* systemClass;
GCClass* blockClass;
GCClass* doubleClass;

#if GC_TYPE == OMR_GARBAGE_COLLECTION
GCClass* booleanClass;
#endif
GCClass* trueClass;
GCClass* falseClass;

GCSymbol* symbolIfTrue;
GCSymbol* symbolIfFalse;

std::string bm_name;

#ifdef GENERATE_ALLOCATION_STATISTICS
struct alloc_data {long noObjects; long sizeObjects;};
std::map<std::string, struct alloc_data> allocationStats;
#define LOG_ALLOCATION(TYPE,SIZE) {struct alloc_data tmp=allocationStats[TYPE];tmp.noObjects++;tmp.sizeObjects+=(SIZE);allocationStats[TYPE]=tmp;}
#else
#define LOG_ALLOCATION(TYPE,SIZE)
#endif

map<int64_t, int64_t> integerHist;
mutex Universe::output_mutex;

void Universe::Start(long argc, char** argv) {
    theUniverse = new Universe();
    theUniverse->initialize(argc, argv);
}

__attribute__((noreturn)) void Universe::Quit(long err) {
    Universe::ErrorPrint("Time spent in GC: [" + to_string(Timer::GCTimer->GetTotalTime()) + "] msec for " + to_string(Timer::GCTimer->GetTotalCount()) + " GCs\n");
#ifdef GENERATE_INTEGER_HISTOGRAM
    std::string file_name_hist = std::string(bm_name);
    file_name_hist.append("_integer_histogram.csv");
    fstream hist_csv(file_name_hist.c_str(), ios::out);

    for (map<long, long>::iterator it = integerHist.begin(); it != integerHist.end(); it++) {
        hist_csv << it->first << ", " << it->second << endl;
    }
#endif

#ifdef LOG_RECEIVER_TYPES
    std::string file_name_receivers = std::string(bm_name);
    file_name_receivers.append("_receivers.csv");
    fstream receivers(file_name_receivers.c_str(), ios::out);
    for (map<StdString, long>::iterator it = theUniverse->receiverTypes.begin(); it != theUniverse->receiverTypes.end(); it++)
    receivers << it->first << ",  " << it->second << endl;

    std::string file_name_send_types = std::string(bm_name);
    file_name_send_types.append("_send_types.csv");
    fstream send_stat(file_name_send_types.c_str(), ios::out);
    send_stat << "#name, percentage_primitive_calls, no_primitive_calls, no_non_primitive_calls" << endl;
    for (map<StdString, Universe::stat_data>::iterator it = theUniverse->callStats.begin(); it != theUniverse->callStats.end(); it++)
    send_stat << it->first << ", " << setiosflags(ios::fixed) << setprecision(2) << (double)(it->second.noPrimitiveCalls) / (double)(it->second.noCalls) <<
    ", " << it->second.noPrimitiveCalls << ", " << it->second.noCalls - it->second.noPrimitiveCalls << endl;
#endif

#ifdef GENERATE_ALLOCATION_STATISTICS
    std::string file_name_allocation = std::string(bm_name);
    file_name_allocation.append("_allocation_statistics.csv");

    fstream file_alloc_stats(file_name_allocation.c_str(), ios::out);
    map<std::string, struct alloc_data>::iterator iter;
    for (iter = allocationStats.begin(); iter != allocationStats.end(); iter++)
    {
        file_alloc_stats << iter->first << ", " << iter->second.noObjects << ", " << iter->second.sizeObjects << std::endl;
    }
#endif
    if (theUniverse)
        delete (theUniverse);

    exit((int) err);
}

__attribute__((noreturn)) void Universe::ErrorExit(const char* err) {
    Universe::ErrorPrint("Runtime error: " + StdString(err) + "\n");
    Quit(ERR_FAIL);
}

vector<StdString> Universe::handleArguments(long argc, char** argv) {
    vector<StdString> vmArgs = vector<StdString>();
    dumpBytecodes = 0;
    gcVerbosity   = 0;
#if GC_TYPE == OMR_GARBAGE_COLLECTION
#if DEFAULT_OMR_JIT_ON
    enableJIT     = true;
#else
    enableJIT     = false;
#endif
#endif

    for (long i = 1; i < argc; ++i) {

        if (strncmp(argv[i], "-cp", 3) == 0) {
            if ((argc == i + 1) || classPath.size() > 0)
                printUsageAndExit(argv[0]);
            setupClassPath(StdString(argv[++i]));
        } else if (strncmp(argv[i], "-d", 2) == 0) {
            ++dumpBytecodes;
        } else if (strncmp(argv[i], "-g", 2) == 0) {
            ++gcVerbosity;
        } else if (strncmp(argv[i], "-H", 2) == 0) {
            long heap_size = 0;
            char unit[3];
            if (sscanf(argv[i], "-H%ld%2s", &heap_size, unit) == 2) {
                if (strcmp(unit, "KB") == 0)
                    heapSize = heap_size * 1024;
                else if (strcmp(unit, "MB") == 0)
                    heapSize = heap_size * 1024 * 1024;
            } else
                printUsageAndExit(argv[0]);

        } else if ((strncmp(argv[i], "-h", 2) == 0)
                || (strncmp(argv[i], "--help", 6) == 0)) {
            printUsageAndExit(argv[0]);
        }
#if GC_TYPE == OMR_GARBAGE_COLLECTION
        else if (strncmp(argv[i], "-jit", 4) == 0) {
        	enableJIT = true;
        } else if (strncmp(argv[i], "-int", 4) == 0) {
        	enableJIT = false;
        }
#endif
        else {
            vector<StdString> extPathTokens = vector<StdString>(2);
            StdString tmpString = StdString(argv[i]);
            if (getClassPathExt(extPathTokens, tmpString) == ERR_SUCCESS) {
                addClassPath(extPathTokens[0]);
            }
            //Different from CSOM!!!:
            //In CSOM there is an else, where the original filename is pushed into the vm_args.
            //But unlike the class name in extPathTokens (extPathTokens[1]) that could
            //still have the .som suffix though.
            //So in SOM++ getClassPathExt will strip the suffix and add it to extPathTokens
            //even if there is no new class path present. So we can in any case do the following:
            vmArgs.push_back(extPathTokens[1]);
        }
    }
    addClassPath(StdString("."));

    return vmArgs;
}

long Universe::getClassPathExt(vector<StdString>& tokens,
        const StdString& arg) const {
#define EXT_TOKENS 2
    long result = ERR_SUCCESS;
    long fpIndex = arg.find_last_of(fileSeparator);
    long ssepIndex = arg.find(".som");

    if (fpIndex == StdString::npos) { //no new path
        //different from CSOM (see also HandleArguments):
        //we still want to strip the suffix from the filename, so
        //we set the start to -1, in order to start the substring
        //from character 0. npos is -1 too, but this is to make sure
        fpIndex = -1;
        //instead of returning here directly, we have to remember that
        //there is no new class path and return it later
        result = ERR_FAIL;
    } else
        tokens[0] = arg.substr(0, fpIndex);

    //adding filename (minus ".som" if present) to second slot
    ssepIndex =
            ((ssepIndex != StdString::npos) && (ssepIndex > fpIndex)) ?
                    (ssepIndex - 1) : arg.length();
    tokens[1] = arg.substr(fpIndex + 1, ssepIndex - (fpIndex));
    return result;
}

long Universe::setupClassPath(const StdString& cp) {
    try {
        std::stringstream ss(cp);
        StdString token;

        long i = 0;
        while (getline(ss, token, pathSeparator)) {
            classPath.push_back(token);
            ++i;
        }

        return ERR_SUCCESS;
    } catch (std::exception e) {
        return ERR_FAIL;
    }
}

long Universe::addClassPath(const StdString& cp) {
    classPath.push_back(cp);
    return ERR_SUCCESS;
}

void Universe::printUsageAndExit(char* executable) const {
    cout << "Usage: " << executable << " [-options] [args...]" << endl << endl;
    cout << "where options include:" << endl;
    cout << "    -cp <directories separated by " << pathSeparator << ">"
         << endl;
    cout << "        set search path for application classes" << endl;
    cout << "    -d  enable disassembling (twice for tracing)" << endl;
    cout << "    -g  enable garbage collection details:" << endl
         << "        1x - print statistics when VM shuts down" << endl
         << "        2x - print statistics upon each collection" << endl
         << "        3x - print statistics and dump heap upon each " << endl
         << "collection" << endl;
    cout << "    -HxMB set the heap size to x MB (default: 1 MB)" << endl;
    cout << "    -HxKB set the heap size to x KB (default: 1 MB)" << endl;
#if GC_TYPE == OMR_GARBAGE_COLLECTION
    cout << "    -jit  enable JIT compilation" << endl;
    cout << "    -int  enable interpreter only" << endl;
#endif
    cout << "    -h  show this help" << endl;

    Quit(ERR_SUCCESS);
}

Universe::Universe() {
    interpreter = nullptr;
}

VMMethod* Universe::createBootstrapMethod(VMClass* holder, long numArgsOfMsgSend) {
    VMMethod* bootstrapMethod = NewMethod(SymbolForChars("bootstrap"), 1, 0);
    bootstrapMethod->SetBytecode(0, BC_HALT);
    bootstrapMethod->SetNumberOfLocals(0);
    bootstrapMethod->SetMaximumNumberOfStackElements(numArgsOfMsgSend);
    bootstrapMethod->SetHolder(holder);
    return bootstrapMethod;
}

#if GC_TYPE == OMR_GARBAGE_COLLECTION
int Universe::jitCompilationEntryPoint(void *arg) {
    SOM_VM *vm = (SOM_VM *) arg;
    OMR_VMThread *omrVMThread = NULL;

    omr_error_t rc = OMR_Glue_BindCurrentThread(vm->omrVM, "JIT Compilation Thread", &omrVMThread);
    if (OMR_ERROR_NONE != rc) {
        omrthread_monitor_enter(vm->jitCompilationQueueMonitor);
        vm->jitCompilationState = 2;
        omrthread_monitor_notify_all(vm->jitCompilationQueueMonitor);
        omrthread_exit(vm->jitCompilationQueueMonitor);
        return 0;
    }

    omrthread_monitor_enter(vm->jitCompilationQueueMonitor);
    vm->jitCompilationState = 0;
    omrthread_monitor_notify_all(vm->jitCompilationQueueMonitor);

    while (2 != vm->jitCompilationState) {
        while (0 == vm->jitCompilationState) {
            omrthread_monitor_wait(vm->jitCompilationQueueMonitor);
        }

        if (1 == vm->jitCompilationState) {
        	uint32_t rc = 0;
            OMR_CompilationQueueNode *node = NULL;
            if (J9_LINKED_LIST_IS_EMPTY(vm->jitCompilationQueue)) {
                vm->jitCompilationState = 0;
                continue;
            }
            J9_LINKED_LIST_REMOVE_FIRST(vm->jitCompilationQueue, node);

            omrthread_monitor_exit(vm->jitCompilationQueueMonitor);

			TR::TypeDictionary types;

			/* TODO what is keeping the method alive if a GC happens during compilation? */
			/* We need a way to interrupt the JIT compile or block the GC from happening during compilation */
			SOMppMethod methodBuilder(&types, node->vmMethod, true);
			uint8_t *entry=0;

			rc = (*compileMethodBuilder)(&methodBuilder, &entry);
			if (0 == rc) {
				node->vmMethod->compiledMethod = (SOMppFunctionType *)entry;
			}

        	OMRPORT_ACCESS_FROM_OMRVM(vm->omrVM);
        	omrmem_free_memory(node);

        	omrthread_monitor_enter(vm->jitCompilationQueueMonitor);
    	}

    }
    omrthread_monitor_exit(vm->jitCompilationQueueMonitor);

    OMR_Glue_UnbindCurrentThread(omrVMThread);

    omrthread_monitor_enter(vm->jitCompilationQueueMonitor);
    vm->jitCompilationState = 3;
    omrthread_monitor_notify_all(vm->jitCompilationQueueMonitor);
    omrthread_exit(vm->jitCompilationQueueMonitor);

	return 0;
}

uintptr_t Universe::createJITAsyncCompileThread(SOM_VM *vm) {
    omrthread_t self = (omrthread_t)NULL;
    omrthread_t jitCompilationThread = (omrthread_t)NULL;
    uintptr_t rc = 0;

    vm->jitCompilationQueue = NULL;

    if (0 == omrthread_attach_ex(&self, J9THREAD_ATTR_DEFAULT)) {
        vm->jitCompilationQueueMonitor = (omrthread_monitor_t)NULL;

        if (0 != omrthread_monitor_init(&vm->jitCompilationQueueMonitor, 0)) {
        	omrthread_detach(self);
        	Universe::ErrorPrint("JIT Compilation failed to create monitor\n");
            return 1;
        }

        omrthread_monitor_enter(vm->jitCompilationQueueMonitor);

        vm->jitCompilationState = 0;

        intptr_t rc = omrthread_create(&jitCompilationThread, 128 * 1024, J9THREAD_PRIORITY_NORMAL, 0, &jitCompilationEntryPoint, vm);
        if (0 != rc) {
            omrthread_monitor_exit(vm->jitCompilationQueueMonitor);
            omrthread_detach(self);
            Universe::ErrorPrint("JIT Compilation thread failed to start\n");
            return 1;
        }

        omrthread_monitor_wait(vm->jitCompilationQueueMonitor);
        if (2 == vm->jitCompilationState) {
        	Universe::ErrorPrint("JIT Compilation state incorrect\n");
            rc = 1;
        }
        omrthread_monitor_exit(vm->jitCompilationQueueMonitor);

        omrthread_detach(self);
    } else {
        Universe::ErrorPrint("JIT Compilation failed to attach main thread\n");
        return 1;
    }
    return rc;
}

void Universe::shutdownJITAsyncCompileThread(SOM_VM *vm) {
    omrthread_monitor_enter(vm->jitCompilationQueueMonitor);

    vm->jitCompilationState = 2;
    omrthread_monitor_notify_all(vm->jitCompilationQueueMonitor);
    while (3 != vm->jitCompilationState) {
    	omrthread_monitor_wait(vm->jitCompilationQueueMonitor);
    }

    omrthread_monitor_exit(vm->jitCompilationQueueMonitor);
}
#endif

void Universe::initialize(long _argc, char** _argv) {
#ifdef GENERATE_ALLOCATION_STATISTICS
    allocationStats["VMArray"] = {0,0};
#endif

    heapSize = 1 * 1024 * 1024;

    vector<StdString> argv = handleArguments(_argc, _argv);
    
    // remember file that was executed (for writing statistics)
    if (argv.size() > 0)
        bm_name = argv[0];

    Heap<HEAP_CLS>::InitializeHeap(heapSize);

#if GC_TYPE == OMR_GARBAGE_COLLECTION
    SOM_VM *vm = GetHeap<OMRHeap>()->getVM();
    SOM_Thread *thread = GetHeap<OMRHeap>()->getThread();
    if (OMR_ERROR_NONE != OMR_Initialize_VM(&vm->omrVM, &thread->omrVMThread, vm, thread)) {
        Universe::ErrorPrint("Failed startup OMR\n");
        GetUniverse()->Quit(-1);
    }

    if (enableJIT) {
    	enableJIT = initializeJit();
        if (!enableJIT) {
    	    Universe::ErrorPrint("Could not initialize JIT\n");
            GetUniverse()->Quit(-1);
       } else {
    	   if (0 != createJITAsyncCompileThread(vm)) {
    		   Universe::ErrorPrint("Could not initialize JIT thread\n");
    		   GetUniverse()->Quit(-1);
    	   }
       }
    }
#endif

    interpreter = new Interpreter();

#if CACHE_INTEGER
# warning is _store_ptr sufficient/correct here?
    // create prebuilt integers
    for (long it = INT_CACHE_MIN_VALUE; it <= INT_CACHE_MAX_VALUE; ++it) {
        prebuildInts[(unsigned long)(it - INT_CACHE_MIN_VALUE)] = _store_ptr(new (GetHeap<HEAP_CLS>()) VMInteger(it));
    }
#endif

    VMObject* systemObject = InitializeGlobals();

    
    VMMethod* bootstrapMethod = createBootstrapMethod(load_ptr(systemClass), 2);

    if (argv.size() == 0) {
        Shell* shell = new Shell(bootstrapMethod);
        shell->Start(interpreter);
        return;
    }

    /* only trace bootstrap if the number of cmd-line "-d"s is > 2 */
    short trace = 2 - dumpBytecodes;
    if (!(trace > 0))
        dumpBytecodes = 1;

    VMArray* argumentsArray = NewArrayFromStrings(argv);

    VMFrame* bootstrapFrame = interpreter->PushNewFrame(bootstrapMethod);
    bootstrapFrame->Push(systemObject);
    bootstrapFrame->Push(argumentsArray);

    VMInvokable* initialize = load_ptr(systemClass)->LookupInvokable(
                                            SymbolForChars("initialize:"));
    initialize->Invoke(interpreter, bootstrapFrame);

    // reset "-d" indicator
    if (!(trace > 0))
        dumpBytecodes = 2 - trace;

    interpreter->Start();
}

Universe::~Universe() {
    if (interpreter)
        delete (interpreter);

#if GC_TYPE == OMR_GARBAGE_COLLECTION
    SOM_VM *vm = GetHeap<OMRHeap>()->getVM();
    if (enableJIT) {
        shutdownJITAsyncCompileThread(vm);
        shutdownJit();
    }
    SOM_Thread *thread = GetHeap<OMRHeap>()->getThread();
    omr_error_t rc = OMR_Shutdown_VM(vm->omrVM, thread->omrVMThread);
    if (OMR_ERROR_NONE != rc) {
    	Universe::ErrorPrint("Error shutting down OMR\n");
    }

#endif
    // check done inside
    Heap<HEAP_CLS>::DestroyHeap();
}

#if !DEBUG
    static void set_vt_to_null() {}
    static void obtain_vtables_of_known_classes(VMSymbol* className) {}
    bool Universe::IsValidObject(vm_oop_t obj) {
        return true;
    }
#else
    void* vt_array;
    void* vt_block;
    void* vt_class;
    void* vt_double;
    void* vt_eval_primitive;
    void* vt_frame;
    void* vt_integer;
    void* vt_method;
    void* vt_object;
    void* vt_primitive;
    void* vt_string;
    void* vt_symbol;

    bool Universe::IsValidObject(vm_oop_t obj) {
        if (IS_TAGGED(obj))
            return true;

        if (obj == INVALID_VM_POINTER
            // || obj == nullptr
            ) {
            assert(false);
            return false;
        }
        
        if (obj == nullptr)
            return true;
        
        
        if (vt_symbol == nullptr) // initialization not yet completed
            return true;
        
        void* vt = *(void**) obj;
        bool b = vt == vt_array    ||
               vt == vt_block      ||
               vt == vt_class      ||
               vt == vt_double     ||
               vt == vt_eval_primitive ||
               vt == vt_frame      ||
               vt == vt_integer    ||
               vt == vt_method     ||
               vt == vt_object     ||
               vt == vt_primitive  ||
               vt == vt_string     ||
               vt == vt_symbol;
        assert(b);
        return b;
    }

    static void set_vt_to_null() {
        vt_array      = nullptr;
        vt_block      = nullptr;
        vt_class      = nullptr;
        vt_double     = nullptr;
        vt_eval_primitive = nullptr;
        vt_frame      = nullptr;
        vt_integer    = nullptr;
        vt_method     = nullptr;
        vt_object     = nullptr;
        vt_primitive  = nullptr;
        vt_string     = nullptr;
        vt_symbol     = nullptr;
    }

    static void obtain_vtables_of_known_classes(VMSymbol* className) {
        VMArray* arr  = new (GetHeap<HEAP_CLS>()) VMArray(0, 0);
        vt_array      = *(void**) arr;
        
        VMBlock* blck = new (GetHeap<HEAP_CLS>()) VMBlock();
        vt_block      = *(void**) blck;
        
        vt_class      = *(void**) symbolClass;
        
        VMDouble* dbl = new (GetHeap<HEAP_CLS>()) VMDouble(0.0);
        vt_double     = *(void**) dbl;
        
        VMEvaluationPrimitive* ev = new (GetHeap<HEAP_CLS>()) VMEvaluationPrimitive(1);
        vt_eval_primitive = *(void**) ev;
        
        VMFrame* frm  = new (GetHeap<HEAP_CLS>()) VMFrame(0, 0);
        vt_frame      = *(void**) frm;
        
        VMInteger* i  = new (GetHeap<HEAP_CLS>()) VMInteger(0);
        vt_integer    = *(void**) i;
        
        VMMethod* mth = new (GetHeap<HEAP_CLS>()) VMMethod(0, 0, 0);
        vt_method     = *(void**) mth;
        vt_object     = *(void**) nilObject;
        
        VMPrimitive* prm = new (GetHeap<HEAP_CLS>()) VMPrimitive(className);
        vt_primitive  = *(void**) prm;
        
        VMString* str = new (GetHeap<HEAP_CLS>()) VMString("");
        vt_string     = *(void**) str;
        vt_symbol     = *(void**) className;
    }
#endif

VMObject* Universe::InitializeGlobals() {
    set_vt_to_null();
    
# warning is _store_ptr sufficient?
    
    //
    //allocate nil object
    //
    VMObject* nil = new (GetHeap<HEAP_CLS>()) VMObject;
    nilObject = _store_ptr(nil);
    nil->SetClass((VMClass*) nil);

    metaClassClass = _store_ptr(NewMetaclassClass());

    objectClass     = _store_ptr(NewSystemClass());
    nilClass        = _store_ptr(NewSystemClass());
    classClass      = _store_ptr(NewSystemClass());
    arrayClass      = _store_ptr(NewSystemClass());
    symbolClass     = _store_ptr(NewSystemClass());
    methodClass     = _store_ptr(NewSystemClass());
    integerClass    = _store_ptr(NewSystemClass());
    primitiveClass  = _store_ptr(NewSystemClass());
    stringClass     = _store_ptr(NewSystemClass());
    doubleClass     = _store_ptr(NewSystemClass());

    nil->SetClass(load_ptr(nilClass));

    InitializeSystemClass(load_ptr(objectClass),                  nullptr, "Object");
    InitializeSystemClass(load_ptr(classClass),     load_ptr(objectClass), "Class");
    InitializeSystemClass(load_ptr(metaClassClass),  load_ptr(classClass), "Metaclass");
    InitializeSystemClass(load_ptr(nilClass),       load_ptr(objectClass), "Nil");
    InitializeSystemClass(load_ptr(arrayClass),     load_ptr(objectClass), "Array");
    InitializeSystemClass(load_ptr(methodClass),     load_ptr(arrayClass), "Method");
    InitializeSystemClass(load_ptr(stringClass),    load_ptr(objectClass), "String");
    InitializeSystemClass(load_ptr(symbolClass),    load_ptr(stringClass), "Symbol");
    InitializeSystemClass(load_ptr(integerClass),   load_ptr(objectClass), "Integer");
    InitializeSystemClass(load_ptr(primitiveClass), load_ptr(objectClass), "Primitive");
    InitializeSystemClass(load_ptr(doubleClass),    load_ptr(objectClass), "Double");

    // Fix up objectClass
    load_ptr(objectClass)->SetSuperClass((VMClass*) nil);

    obtain_vtables_of_known_classes(nil->GetClass()->GetName());
    
#if USE_TAGGING
    GlobalBox::updateIntegerBox(NewInteger(1));
#endif

    LoadSystemClass(load_ptr(objectClass));
    LoadSystemClass(load_ptr(classClass));
    LoadSystemClass(load_ptr(metaClassClass));
    LoadSystemClass(load_ptr(nilClass));
    LoadSystemClass(load_ptr(arrayClass));
    LoadSystemClass(load_ptr(methodClass));
    LoadSystemClass(load_ptr(symbolClass));
    LoadSystemClass(load_ptr(integerClass));
    LoadSystemClass(load_ptr(primitiveClass));
    LoadSystemClass(load_ptr(stringClass));
    LoadSystemClass(load_ptr(doubleClass));

    blockClass = _store_ptr(LoadClass(SymbolForChars("Block")));

#if GC_TYPE == OMR_GARBAGE_COLLECTION
    booleanClass = _store_ptr(LoadClass(SymbolForChars("Boolean")));
#endif

    VMSymbol* trueClassName = SymbolForChars("True");
    trueClass  = _store_ptr(LoadClass(trueClassName));
    trueObject = _store_ptr(NewInstance(load_ptr(trueClass)));
    
    VMSymbol* falseClassName = SymbolForChars("False");
    falseClass  = _store_ptr(LoadClass(falseClassName));
    falseObject = _store_ptr(NewInstance(load_ptr(falseClass)));

    systemClass = _store_ptr(LoadClass(SymbolForChars("System")));
    
    
    VMObject* systemObject = NewInstance(load_ptr(systemClass));
    
    SetGlobal(SymbolForChars("nil"),    load_ptr(nilObject));
    SetGlobal(SymbolForChars("true"),   load_ptr(trueObject));
    SetGlobal(SymbolForChars("false"),  load_ptr(falseObject));
    SetGlobal(SymbolForChars("system"), systemObject);
    SetGlobal(SymbolForChars("System"), load_ptr(systemClass));
    SetGlobal(SymbolForChars("Block"),  load_ptr(blockClass));
    
    symbolIfTrue  = _store_ptr(SymbolForChars("ifTrue:"));
    symbolIfFalse = _store_ptr(SymbolForChars("ifFalse:"));
    
    return systemObject;
}

void Universe::Assert(bool value) const {
    if (!value) {
        Universe::ErrorPrint("Universe::Assert Assertion failed\n");
    }
}

VMClass* Universe::GetBlockClass() const {
    return load_ptr(blockClass);
}

VMClass* Universe::GetBlockClassWithArgs(long numberOfArguments) {
    map<long, GCClass*>::iterator it =
    blockClassesByNoOfArgs.find(numberOfArguments);
    if (it != blockClassesByNoOfArgs.end())
        return load_ptr(it->second);

    Assert(numberOfArguments < 10);

    ostringstream Str;
    Str << "Block" << numberOfArguments;
    VMSymbol* name = SymbolFor(Str.str());
    VMClass* result = LoadClassBasic(name, nullptr);

    result->AddInstancePrimitive(new (GetHeap<HEAP_CLS>()) VMEvaluationPrimitive(numberOfArguments) );

    SetGlobal(name, result);
# warning is _store_ptr sufficient here?
    blockClassesByNoOfArgs[numberOfArguments] = _store_ptr(result);

    return result;
}

vm_oop_t Universe::GetGlobal(VMSymbol* name) {
    # warning is _store_ptr correct here? it relies on _store_ptr not to be really changed...
    auto it = globals.find(_store_ptr(name));
    if (it == globals.end()) {
        return nullptr;
    } else {
        return load_ptr(it->second);
    }
}

bool Universe::HasGlobal(VMSymbol* name) {
    # warning is _store_ptr correct here? it relies on _store_ptr not to be really changed...
    auto it = globals.find(_store_ptr(name));
    if (it == globals.end()) {
        return false;
    } else {
        return true;
    }
}

void Universe::InitializeSystemClass(VMClass* systemClass,
VMClass* superClass, const char* name) {
    StdString s_name(name);

    if (superClass != nullptr) {
        systemClass->SetSuperClass(superClass);
        VMClass* sysClassClass = systemClass->GetClass();
        VMClass* superClassClass = superClass->GetClass();
        sysClassClass->SetSuperClass(superClassClass);
    } else {
        VMClass* sysClassClass = systemClass->GetClass();
        sysClassClass->SetSuperClass(load_ptr(classClass));
    }

    VMClass* sysClassClass = systemClass->GetClass();

    systemClass->SetInstanceFields(NewArray(0));
    sysClassClass->SetInstanceFields(NewArray(0));

    systemClass->SetInstanceInvokables(NewArray(0));
    sysClassClass->SetInstanceInvokables(NewArray(0));

    systemClass->SetName(SymbolFor(s_name));
    ostringstream Str;
    Str << s_name << " class";
    StdString classClassName(Str.str());
    sysClassClass->SetName(SymbolFor(classClassName));

    SetGlobal(systemClass->GetName(), systemClass);
}

VMClass* Universe::LoadClass(VMSymbol* name) {
    VMClass* result = static_cast<VMClass*>(GetGlobal(name));
    
    if (result != nullptr)
        return result;

    result = LoadClassBasic(name, nullptr);

    if (!result) {
		// we fail silently, it is not fatal that loading a class failed
		return (VMClass*) nilObject;
    }

    if (result->HasPrimitives() || result->GetClass()->HasPrimitives())
        result->LoadPrimitives(classPath);
    
    SetGlobal(name, result);

    return result;
}

VMClass* Universe::LoadClassBasic(VMSymbol* name, VMClass* systemClass) {
    StdString s_name = name->GetStdString();
    VMClass* result;

    for (vector<StdString>::iterator i = classPath.begin();
            i != classPath.end(); ++i) {
        SourcecodeCompiler compiler;
        result = compiler.CompileClass(*i, name->GetStdString(), systemClass);
        if (result) {
            if (dumpBytecodes) {
                Disassembler::Dump(result->GetClass());
                Disassembler::Dump(result);
            }
            return result;
        }
    }
    return nullptr;
}

VMClass* Universe::LoadShellClass(StdString& stmt) {
    SourcecodeCompiler compiler;
    VMClass* result = compiler.CompileClassString(stmt, nullptr);
    if(dumpBytecodes)
        Disassembler::Dump(result);
    return result;
}

void Universe::LoadSystemClass(VMClass* systemClass) {
    VMClass* result = LoadClassBasic(systemClass->GetName(), systemClass);
    StdString s = systemClass->GetName()->GetStdString();

    if (!result) {
        Universe::ErrorPrint("Can't load system class: " + s + "\n");
        Universe::Quit(ERR_FAIL);
    }

    if (result->HasPrimitives() || result->GetClass()->HasPrimitives())
        result->LoadPrimitives(classPath);
}

VMArray* Universe::NewArray(long size) const {
    long additionalBytes = size * sizeof(VMObject*);
    
    bool outsideNursery;
    
#if GC_TYPE == GENERATIONAL
    // if the array is too big for the nursery, we will directly allocate a
    // mature object
    outsideNursery = additionalBytes + sizeof(VMArray) > GetHeap<HEAP_CLS>()->GetMaxNurseryObjectSize();
#endif

    VMArray* result = new (GetHeap<HEAP_CLS>(), additionalBytes ALLOC_OUTSIDE_NURSERY(outsideNursery)) VMArray(size);
    if ((GC_TYPE == GENERATIONAL) && outsideNursery)
        result->SetGCField(MASK_OBJECT_IS_OLD);

    result->SetClass(load_ptr(arrayClass));
    
    LOG_ALLOCATION("VMArray", result->GetObjectSize());
    return result;
}

VMArray* Universe::NewArrayFromStrings(const vector<StdString>& argv) const {
    VMArray* result = NewArray(argv.size());
    long j = 0;
    for (vector<StdString>::const_iterator i = argv.begin();
            i != argv.end(); ++i) {
        result->SetIndexableField(j, NewString(*i));
        ++j;
    }

    return result;
}

VMArray* Universe::NewArrayList(ExtendedList<VMSymbol*>& list) const {
    ExtendedList<vm_oop_t>& objList = (ExtendedList<vm_oop_t>&) list;
    return NewArrayList(objList);
}

VMArray* Universe::NewArrayList(ExtendedList<VMInvokable*>& list) const {
    ExtendedList<vm_oop_t>& objList = (ExtendedList<vm_oop_t>&) list;
    return NewArrayList(objList);
}

VMArray* Universe::NewArrayList(ExtendedList<vm_oop_t>& list) const {
    long size = list.Size();
    VMArray* result = NewArray(size);

    if (result) {
        for (long i = 0; i < size; ++i) {
            vm_oop_t elem = list.Get(i);
            result->SetIndexableField(i, elem);
        }
    }
    return result;
}

VMBlock* Universe::NewBlock(VMMethod* method, VMFrame* context, long arguments) {
    VMBlock* result = new (GetHeap<HEAP_CLS>()) VMBlock;
    result->SetClass(GetBlockClassWithArgs(arguments));

    result->SetMethod(method);
    result->SetContext(context);

    LOG_ALLOCATION("VMBlock", result->GetObjectSize());
    return result;
}

VMClass* Universe::NewClass(VMClass* classOfClass) const {
    long numFields = classOfClass->GetNumberOfInstanceFields();
    VMClass* result;
    long additionalBytes = numFields * sizeof(VMObject*);
    if (numFields) result = new (GetHeap<HEAP_CLS>(), additionalBytes) VMClass(numFields);
    else result = new (GetHeap<HEAP_CLS>()) VMClass;

    result->SetClass(classOfClass);

    LOG_ALLOCATION("VMClass", result->GetObjectSize());
    return result;
}

VMDouble* Universe::NewDouble(double value) const {
    LOG_ALLOCATION("VMDouble", sizeof(VMDouble));
    return new (GetHeap<HEAP_CLS>()) VMDouble(value);
}

VMFrame* Universe::NewFrame(VMFrame* previousFrame, VMMethod* method) const {
    VMFrame* result = nullptr;
#ifdef UNSAFE_FRAME_OPTIMIZATION
    result = method->GetCachedFrame();
    if (result != nullptr) {
        method->SetCachedFrame(nullptr);
        result->SetPreviousFrame(previousFrame);
        return result;
    }
#endif
    long length = method->GetNumberOfArguments() +
                  method->GetNumberOfLocals() +
                  method->GetMaximumNumberOfStackElements();

    long additionalBytes = length * sizeof(VMObject*);
    result = new (GetHeap<HEAP_CLS>(), additionalBytes) VMFrame(length);
    result->clazz = nullptr;
# warning I think _store_ptr is sufficient here, but...
    result->method        = _store_ptr(method);
    result->previousFrame = _store_ptr(previousFrame);
    result->ResetStackPointer();
    
    LOG_ALLOCATION("VMFrame", result->GetObjectSize());
    return result;
}

VMObject* Universe::NewInstance(VMClass* classOfInstance) const {
    long numOfFields = classOfInstance->GetNumberOfInstanceFields();
    //the additional space needed is calculated from the number of fields
    long additionalBytes = numOfFields * sizeof(VMObject*);
    VMObject* result = new (GetHeap<HEAP_CLS>(), additionalBytes) VMObject(numOfFields);
    result->SetClass(classOfInstance);

    LOG_ALLOCATION(classOfInstance->GetName()->GetStdString(), result->GetObjectSize());
    return result;
}

VMInteger* Universe::NewInteger(int64_t value) const {

#ifdef GENERATE_INTEGER_HISTOGRAM
    integerHist[value/INT_HIST_SIZE] = integerHist[value/INT_HIST_SIZE]+1;
#endif

#if CACHE_INTEGER
    size_t index = (size_t) value - (size_t)INT_CACHE_MIN_VALUE;
    if (index < (size_t)(INT_CACHE_MAX_VALUE - INT_CACHE_MIN_VALUE)) {
        return static_cast<VMInteger*>(load_ptr(prebuildInts[index]));
    }
#endif

    LOG_ALLOCATION("VMInteger", sizeof(VMInteger));
    return new (GetHeap<HEAP_CLS>()) VMInteger(value);
}

VMClass* Universe::NewMetaclassClass() const {
    VMClass* result = new (GetHeap<HEAP_CLS>()) VMClass;
    result->SetClass(new (GetHeap<HEAP_CLS>()) VMClass);

    VMClass* mclass = result->GetClass();
    mclass->SetClass(result);

    LOG_ALLOCATION("VMClass", result->GetObjectSize());
    return result;
}

void Universe::WalkGlobals(walk_heap_fn walk) {
    nilObject   = static_cast<GCObject*>(walk(nilObject));
    trueObject  = static_cast<GCObject*>(walk(trueObject));
    falseObject = static_cast<GCObject*>(walk(falseObject));

#if USE_TAGGING
    GlobalBox::WalkGlobals(walk);
#endif

    objectClass    = static_cast<GCClass*>(walk(objectClass));
    classClass     = static_cast<GCClass*>(walk(classClass));
    metaClassClass = static_cast<GCClass*>(walk(metaClassClass));

    nilClass        = static_cast<GCClass*>(walk(nilClass));
    integerClass    = static_cast<GCClass*>(walk(integerClass));
    arrayClass      = static_cast<GCClass*>(walk(arrayClass));
    methodClass     = static_cast<GCClass*>(walk(methodClass));
    symbolClass     = static_cast<GCClass*>(walk(symbolClass));
    primitiveClass  = static_cast<GCClass*>(walk(primitiveClass));
    stringClass     = static_cast<GCClass*>(walk(stringClass));
    systemClass     = static_cast<GCClass*>(walk(systemClass));
    blockClass      = static_cast<GCClass*>(walk(blockClass));
    doubleClass     = static_cast<GCClass*>(walk(doubleClass));
    
#if GC_TYPE == OMR_GARBAGE_COLLECTION
    booleanClass  = static_cast<GCClass*>(walk(booleanClass));
#endif
    trueClass  = static_cast<GCClass*>(walk(trueClass));
    falseClass = static_cast<GCClass*>(walk(falseClass));

#if CACHE_INTEGER
    for (unsigned long i = 0; i < (INT_CACHE_MAX_VALUE - INT_CACHE_MIN_VALUE); i++)
#if USE_TAGGING
        prebuildInts[i] = TAG_INTEGER(INT_CACHE_MIN_VALUE + i);
#else
        prebuildInts[i] = walk(prebuildInts[i]);
#endif
#endif

    // walk all entries in globals map
    map<GCSymbol*, gc_oop_t> globs = globals;
    globals.clear();
    map<GCSymbol*, gc_oop_t>::iterator iter;
    for (iter = globs.begin(); iter != globs.end(); iter++) {
        assert(iter->second != nullptr);

        GCSymbol* key = static_cast<GCSymbol*>(walk(iter->first));
        gc_oop_t val = walk(iter->second);
        globals[key] = val;
    }
    
    // walk all entries in symbols map
    map<StdString, GCSymbol*>::iterator symbolIter;
    for (symbolIter = symbolsMap.begin();
         symbolIter != symbolsMap.end();
         symbolIter++) {
        //insert overwrites old entries inside the internal map
        symbolIter->second = static_cast<GCSymbol*>(walk(symbolIter->second));
    }

    map<long, GCClass*>::iterator bcIter;
    for (bcIter = blockClassesByNoOfArgs.begin();
         bcIter != blockClassesByNoOfArgs.end();
         bcIter++) {
        bcIter->second = static_cast<GCClass*>(walk(bcIter->second));
    }

    //reassign ifTrue ifFalse Symbols
    symbolIfTrue  = symbolsMap["ifTrue:"];
    symbolIfFalse = symbolsMap["ifFalse:"];
    
#if GC_TYPE == OMR_GARBAGE_COLLECTION
    if (enableJIT) {
        SOM_VM *vm = GetHeap<OMRHeap>()->getVM();
        omrthread_monitor_enter(vm->jitCompilationQueueMonitor);

        OMR_CompilationQueueNode *node = J9_LINKED_LIST_START_DO(vm->jitCompilationQueue);
        while (NULL != node) {
    	    node->vmMethod = (VMMethod *)walk((gc_oop_t)node->vmMethod);
    	    node = J9_LINKED_LIST_NEXT_DO(vm->jitCompilationQueue, node);
        }

        omrthread_monitor_exit(vm->jitCompilationQueueMonitor);
    }
#endif

    interpreter->WalkGlobals(walk);
}

VMMethod* Universe::NewMethod(VMSymbol* signature,
        size_t numberOfBytecodes, size_t numberOfConstants) const {
    //Method needs space for the bytecodes and the pointers to the constants
    long additionalBytes = PADDED_SIZE(numberOfBytecodes + numberOfConstants*sizeof(VMObject*));
//#if GC_TYPE==GENERATIONAL
//    VMMethod* result = new (GetHeap<HEAP_CLS>(),additionalBytes, true) 
//                VMMethod(numberOfBytecodes, numberOfConstants);
//#else
    VMMethod* result = new (GetHeap<HEAP_CLS>(),additionalBytes)
    VMMethod(numberOfBytecodes, numberOfConstants);
//#endif
    result->SetClass(load_ptr(methodClass));

    result->SetSignature(signature);

    LOG_ALLOCATION("VMMethod", result->GetObjectSize());
    return result;
}

VMString* Universe::NewString(const StdString& str) const {
    return NewString(str.c_str());
}

VMString* Universe::NewString(const char* str) const {
    VMString* result = new (GetHeap<HEAP_CLS>(), PADDED_SIZE(strlen(str) + 1)) VMString(str);

    LOG_ALLOCATION("VMString", result->GetObjectSize());
    return result;
}

VMSymbol* Universe::NewSymbol(const StdString& str) {
    return NewSymbol(str.c_str());
}

VMSymbol* Universe::NewSymbol(const char* str) {
    VMSymbol* result = new (GetHeap<HEAP_CLS>(), PADDED_SIZE(strlen(str)+1)) VMSymbol(str);
# warning is _store_ptr sufficient here?
    symbolsMap[str] = _store_ptr(result);

    LOG_ALLOCATION("VMSymbol", result->GetObjectSize());
    return result;
}

VMClass* Universe::NewSystemClass() const {
    VMClass* systemClass = new (GetHeap<HEAP_CLS>()) VMClass();

    systemClass->SetClass(new (GetHeap<HEAP_CLS>()) VMClass());
    VMClass* mclass = systemClass->GetClass();

    mclass->SetClass(load_ptr(metaClassClass));

    LOG_ALLOCATION("VMClass", systemClass->GetObjectSize());
    return systemClass;
}

VMSymbol* Universe::SymbolFor(const StdString& str) {
    map<string,GCSymbol*>::iterator it = symbolsMap.find(str);
    return (it == symbolsMap.end()) ? NewSymbol(str) : load_ptr(it->second);
}

VMSymbol* Universe::SymbolForChars(const char* str) {
    return SymbolFor(str);
}

void Universe::SetGlobal(VMSymbol* name, vm_oop_t val) {
# warning is _store_ptr correct here? it relies on _store_ptr not to be really changed...
    globals[_store_ptr(name)] = _store_ptr(val);
}

void Universe::Print(StdString str) {
    lock_guard<mutex> lock(output_mutex);
    cout << str << flush;
}

void Universe::ErrorPrint(StdString str) {
    lock_guard<mutex> lock(output_mutex);
    cerr << str << flush;
}
