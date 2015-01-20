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

#include <interpreter/Interpreter.h>

#include <misc/debug.h>

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

#include <vmobjects/VMThread.h>
#include <vmobjects/VMMutex.h>
#include <vmobjects/VMSignal.h>

#include <interpreter/bytecodes.h>

#include <compiler/Disassembler.h>
#include <compiler/SourcecodeCompiler.h>

#include <vmobjects/IntegerBox.h>

#include <vmobjects/VMBlock.inline.h>
#include <vmobjects/VMMethod.inline.h>

#if CACHE_INTEGER
gc_oop_t prebuildInts[INT_CACHE_MAX_VALUE - INT_CACHE_MIN_VALUE + 1];
#endif

#define INT_HIST_SIZE 1

// Here we go:

short dumpBytecodes;
short gcVerbosity;

Universe* Universe::theUniverse = nullptr;

GCObject* nilObject;
GCObject* trueObject;
GCObject* falseObject;
GCObject* systemObject;

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

GCClass* trueClass;
GCClass* falseClass;

GCClass* threadClass;
GCClass* mutexClass;
GCClass* signalClass;

GCSymbol* symbolIfTrue;
GCSymbol* symbolIfFalse;

std::map<std::string, GCSymbol*> symbolsMap;

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

void Universe::Quit(long err) {
    cout << "Time spent in GC: [" << Timer::GCTimer->GetTotalTime() << "] msec"
            << endl;
#if GC_TYPE!=PAUSELESS
    cout << "Number of GC cycles performed: " << _HEAP->GetNumberOfCycles() << endl;
#endif
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
    //if (theUniverse)
    //    delete (theUniverse);

    exit((int) err);
}

__attribute__((noreturn)) void Universe::ErrorExit(StdString err) {
    Universe::ErrorPrint("Runtime error: " + err + "\n");
    Quit(ERR_FAIL);
}

vector<StdString> Universe::handleArguments(long argc, char** argv) {
    vector<StdString> vmArgs = vector<StdString>();
    dumpBytecodes = 0;
    gcVerbosity   = 0;

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
            size_t heap_size = 0;
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
        } else {
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
    cout << endl;
    cout << "    -d  enable disassembling (twice for tracing)" << endl;
    cout << "    -g  enable garbage collection details:" << endl
         << "        1x - print statistics when VM shuts down" << endl
         << "        2x - print statistics upon each collection" << endl
         << "        3x - print statistics and dump heap upon each " << endl
         << "collection" << endl;
    cout << endl;
    cout << "    -HxMB set the heap size to x MB (default: 1 MB)" << endl;
    cout << "    -HxKB set the heap size to x KB (default: 1 MB)" << endl;
    cout << endl;
    cout << "    -h  show this help" << endl;

    Quit(ERR_SUCCESS);
}

Universe::Universe() {
    pthread_key_create(&interpreterKey, nullptr);
    pthread_mutex_init(&interpreterMutex, nullptr);
    pthread_mutexattr_init(&attrclassLoading);
    pthread_mutexattr_settype(&attrclassLoading, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&classLoading, &attrclassLoading);
    
    pthread_mutex_init(&testMutex, nullptr);
}

void Universe::initialize(long _argc, char** _argv) {
#ifdef GENERATE_ALLOCATION_STATISTICS
    allocationStats["VMArray"] = {0,0};
#endif

    //heapSize = 240 * 1024 * 1024;
    //pageSize = 4 * 8192;

    vector<StdString> argv = handleArguments(_argc, _argv);
    
    // remember file that was executed (for writing statistics)
    if (argv.size() > 0)
        bm_name = argv[0];

    PagedHeap::InitializeHeap(HEAP_SIZE, PAGE_SIZE);

    heap = _HEAP;

    interpreters = vector<Interpreter*>();
    Interpreter* interpreter = NewInterpreter();
    Page* page = interpreter->GetPage();
    assert(page);

#if CACHE_INTEGER
    //create prebuilt integers
    for (long it = INT_CACHE_MIN_VALUE; it <= INT_CACHE_MAX_VALUE; ++it) {
#if GC_TYPE==GENERATIONAL
        prebuildInts[(unsigned long)(it - INT_CACHE_MIN_VALUE)] = new (_HEAP, _PAGE) VMInteger(it);
#elif GC_TYPE==PAUSELESS
        prebuildInts[(unsigned long)(it - INT_CACHE_MIN_VALUE)] = new (page) VMInteger(it);
#else
        prebuildInts[(unsigned long)(it - INT_CACHE_MIN_VALUE)] = new (_HEAP) VMInteger(it);
#endif
    }
#endif

    VMObject* systemObject = InitializeGlobals(page);

    
    VMMethod* bootstrapMethod = NewMethod(SymbolForChars("bootstrap", page), 1, 0, page);
    bootstrapMethod->SetBytecode(0, BC_HALT);
    bootstrapMethod->SetNumberOfLocals(0, page);
    
    bootstrapMethod->SetMaximumNumberOfStackElements(2, page);
    bootstrapMethod->SetHolder(load_ptr(systemClass));
    
    VMThread* thread = NewThread(page);
    VMSignal* signal = NewSignal(page);
    thread->SetResumeSignal(signal);
    thread->SetShouldStop(false);
    interpreter->SetThread(thread);

    if (argv.size() == 0) {
        Shell* shell = new Shell(bootstrapMethod);
        shell->Start(interpreter);
        return;
    }

    /* only trace bootstrap if the number of cmd-line "-d"s is > 2 */
    short trace = 2 - dumpBytecodes;
    if (!(trace > 0))
        dumpBytecodes = 1;

    VMArray* argumentsArray = NewArrayFromStrings(argv, page);

    VMFrame* bootstrapFrame = interpreter->PushNewFrame(bootstrapMethod);
    bootstrapFrame->Push(systemObject);
    bootstrapFrame->Push(argumentsArray);

    VMInvokable* initialize = load_ptr(systemClass)->LookupInvokable(
                                            SymbolForChars("initialize:", page));
    initialize->Invoke(interpreter, bootstrapFrame);

    // reset "-d" indicator
    if (!(trace > 0))
        dumpBytecodes = 2 - trace;
    
#if GC_TYPE==PAUSELESS
    _HEAP->Start();
#endif
    interpreter->Start();
    //pthread_exit(0);
}

Universe::~Universe() {
    pthread_key_delete(interpreterKey);
    pthread_mutex_destroy(&interpreterMutex);
    PagedHeap::DestroyHeap();
}

#if !DEBUG
    static void set_vt_to_null() {}
    static void obtain_vtables_of_known_classes(VMSymbol* className, Page*) {}
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
    void* vt_thread;
    void* vt_mutex;
    void* vt_signal;

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
        assert(vt != nullptr);
        
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
               vt == vt_symbol     ||
               vt == vt_thread     ||
               vt == vt_mutex      ||
               vt == vt_signal;
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
        vt_thread     = nullptr;
        vt_mutex      = nullptr;
        vt_signal     = nullptr;
    }

    static void obtain_vtables_of_known_classes(VMSymbol* className, Page* page) {
#if GC_TYPE==GENERATIONAL
        VMArray* arr  = new (_HEAP, _PAGE) VMArray(0, 0);
#elif GC_TYPE==PAUSELESS
        VMArray* arr  = new (page) VMArray(0, 0);
#else
        VMArray* arr  = new (_HEAP) VMArray(0, 0);
#endif
        vt_array      = *(void**) arr;
        
#if GC_TYPE==GENERATIONAL
        VMBlock* blck = new (_HEAP, _PAGE) VMBlock();
#elif GC_TYPE==PAUSELESS
        VMBlock* blck = new (page) VMBlock();
#else
        VMBlock* blck = new (_HEAP) VMBlock();
#endif
        vt_block      = *(void**) blck;
        
        vt_class      = *(void**) symbolClass;
        
#if GC_TYPE==GENERATIONAL
        VMDouble* dbl = new (_HEAP, _PAGE) VMDouble(0.0);
#elif GC_TYPE==PAUSELESS
        VMDouble* dbl = new (page) VMDouble(0.0);
#else
        VMDouble* dbl = new (_HEAP) VMDouble(0.0);
#endif
        vt_double     = *(void**) dbl;
        
#if GC_TYPE==GENERATIONAL
        VMEvaluationPrimitive* ev = new (_HEAP, _PAGE) VMEvaluationPrimitive(1);
#elif GC_TYPE==PAUSELESS
        VMEvaluationPrimitive* ev = new (page) VMEvaluationPrimitive(1, page);
#else
        VMEvaluationPrimitive* ev = new (_HEAP) VMEvaluationPrimitive(1);
#endif
        vt_eval_primitive = *(void**) ev;
        
#if GC_TYPE==GENERATIONAL
        VMFrame* frm  = new (_HEAP, _PAGE) VMFrame(0, 0);
#elif GC_TYPE==PAUSELESS
        VMFrame* frm  = new (page) VMFrame(0, 0);
#else
        VMFrame* frm  = new (_HEAP) VMFrame(0, 0);
#endif
        vt_frame      = *(void**) frm;
        
#if GC_TYPE==GENERATIONAL
        VMInteger* i  = new (_HEAP, _PAGE) VMInteger(0);
#elif GC_TYPE==PAUSELESS
        VMInteger* i  = new (page) VMInteger(0);
#else
        VMInteger* i  = new (_HEAP) VMInteger(0);
#endif
        vt_integer    = *(void**) i;
        
#if GC_TYPE==GENERATIONAL
        VMMethod* mth = new (_HEAP, _PAGE) VMMethod(0, 0, 0);
#elif GC_TYPE==PAUSELESS
        VMMethod* mth = new (page) VMMethod(0, 0, 0, page);
#else
        VMMethod* mth = new (_HEAP) VMMethod(0, 0, 0);
#endif
        vt_method     = *(void**) mth;
        vt_object     = *(void**) nilObject;
        
#if GC_TYPE==GENERATIONAL
        VMPrimitive* prm = new (_HEAP, _PAGE) VMPrimitive(className);
#elif GC_TYPE==PAUSELESS
        VMPrimitive* prm = new (page) VMPrimitive(className);
#else
        VMPrimitive* prm = new (_HEAP) VMPrimitive(className);
#endif
        vt_primitive  = *(void**) prm;
        
#if GC_TYPE==GENERATIONAL
        VMString* str = new (_HEAP, _PAGE, PADDED_SIZE(7)) VMString("foobar");
#elif GC_TYPE==PAUSELESS
        VMString* str = new (page) VMString("");
#else
        VMString* str = new (_HEAP, PADDED_SIZE(7)) VMString("foobar");
#endif
        vt_string     = *(void**) str;
        vt_symbol     = *(void**) className;
        
#if GC_TYPE==GENERATIONAL
        VMThread* thr = new (_HEAP, _PAGE) VMThread();
#elif GC_TYPE==PAUSELESS
        VMThread* thr = new (page) VMThread();
#else
        VMThread* thr = new (_HEAP) VMThread();
#endif
        vt_thread     = *(void**) thr;
        
#if GC_TYPE==GENERATIONAL
        VMMutex* mtx  = new (_HEAP, _PAGE) VMMutex();
#elif GC_TYPE==PAUSELESS
        VMMutex* mtx  = new (page) VMMutex();
#else
        VMMutex* mtx  = new (_HEAP) VMMutex();
#endif
        vt_mutex      = *(void**) mtx;
        
#if GC_TYPE==GENERATIONAL
        VMSignal* sgnl = new (_HEAP, _PAGE) VMSignal();
#elif GC_TYPE==PAUSELESS
        VMSignal* sgnl = new (page) VMSignal();
#else
        VMSignal* sgnl = new (_HEAP) VMSignal();
#endif
        vt_signal      = *(void**) sgnl;
    }
#endif

VMObject* Universe::InitializeGlobals(Page* page) {
    set_vt_to_null();
    
    //
    //allocate nil object
    //
#if GC_TYPE==GENERATIONAL
    VMObject* nil = new (_HEAP, _PAGE) VMObject;
#elif GC_TYPE==PAUSELESS
    VMObject* nil = new (page) VMObject;
#else
    VMObject* nil = new (_HEAP) VMObject;
#endif
    nilObject = _store_ptr(nil);
    nil->SetClass((VMClass*) nil);

    metaClassClass = _store_ptr(NewMetaclassClass(page));

    objectClass     = _store_ptr(NewSystemClass(page));
    nilClass        = _store_ptr(NewSystemClass(page));
    classClass      = _store_ptr(NewSystemClass(page));
    arrayClass      = _store_ptr(NewSystemClass(page));
    symbolClass     = _store_ptr(NewSystemClass(page));
    methodClass     = _store_ptr(NewSystemClass(page));
    integerClass    = _store_ptr(NewSystemClass(page));
    primitiveClass  = _store_ptr(NewSystemClass(page));
    stringClass     = _store_ptr(NewSystemClass(page));
    doubleClass     = _store_ptr(NewSystemClass(page));

    signalClass     = _store_ptr(NewSystemClass(page));
    mutexClass      = _store_ptr(NewSystemClass(page));
    threadClass     = _store_ptr(NewSystemClass(page));

    load_ptr(nilObject)->SetClass(load_ptr(nilClass));

    InitializeSystemClass(load_ptr(objectClass),                  nullptr, "Object",    page);
    InitializeSystemClass(load_ptr(classClass),     load_ptr(objectClass), "Class",     page);
    InitializeSystemClass(load_ptr(metaClassClass),  load_ptr(classClass), "Metaclass", page);
    InitializeSystemClass(load_ptr(nilClass),       load_ptr(objectClass), "Nil",       page);
    InitializeSystemClass(load_ptr(arrayClass),     load_ptr(objectClass), "Array",     page);
    InitializeSystemClass(load_ptr(methodClass),     load_ptr(arrayClass), "Method",    page);
    InitializeSystemClass(load_ptr(stringClass),    load_ptr(objectClass), "String",    page);
    InitializeSystemClass(load_ptr(symbolClass),    load_ptr(stringClass), "Symbol",    page);
    InitializeSystemClass(load_ptr(integerClass),   load_ptr(objectClass), "Integer",   page);
    InitializeSystemClass(load_ptr(primitiveClass), load_ptr(objectClass), "Primitive", page);
    InitializeSystemClass(load_ptr(doubleClass),    load_ptr(objectClass), "Double",    page);

    InitializeSystemClass(load_ptr(signalClass),    load_ptr(objectClass), "Signal",    page);
    InitializeSystemClass(load_ptr(mutexClass),     load_ptr(objectClass), "Mutex",     page);
    InitializeSystemClass(load_ptr(threadClass),    load_ptr(objectClass), "Thread",    page);

    // Fix up objectClass
    load_ptr(objectClass)->SetSuperClass((VMClass*) load_ptr(nilObject));
    
    obtain_vtables_of_known_classes(nil->GetClass()->GetName(), page);

    LoadSystemClass(load_ptr(objectClass),    page);
    LoadSystemClass(load_ptr(classClass),     page);
    LoadSystemClass(load_ptr(metaClassClass), page);
    LoadSystemClass(load_ptr(nilClass),       page);
    LoadSystemClass(load_ptr(arrayClass),     page);
    LoadSystemClass(load_ptr(methodClass),    page);
    LoadSystemClass(load_ptr(symbolClass),    page);
    LoadSystemClass(load_ptr(integerClass),   page);
    LoadSystemClass(load_ptr(primitiveClass), page);
    LoadSystemClass(load_ptr(stringClass),    page);
    LoadSystemClass(load_ptr(doubleClass),    page);

    LoadSystemClass(load_ptr(signalClass),    page);
    LoadSystemClass(load_ptr(mutexClass),     page);
    LoadSystemClass(load_ptr(threadClass),    page);

    blockClass = _store_ptr(LoadClass(SymbolForChars("Block", page), page));

    VMSymbol* trueClassName = SymbolForChars("True", page);
    trueClass  = _store_ptr(LoadClass(trueClassName, page));
    trueObject = _store_ptr(NewInstance(load_ptr(trueClass), page));
    
    VMSymbol* falseClassName = SymbolForChars("False", page);
    falseClass  = _store_ptr(LoadClass(falseClassName, page));
    falseObject = _store_ptr(NewInstance(load_ptr(falseClass), page));

    systemClass = _store_ptr(LoadClass(SymbolForChars("System", page), page));

    VMObject* systemObj = NewInstance(load_ptr(systemClass), page);
    systemObject = _store_ptr(systemObj);
    
    
    SetGlobal(SymbolForChars("nil",    page), load_ptr(nilObject));
    SetGlobal(SymbolForChars("true",   page), load_ptr(trueObject));
    SetGlobal(SymbolForChars("false",  page), load_ptr(falseObject));
    SetGlobal(SymbolForChars("system", page), systemObj);
    SetGlobal(SymbolForChars("System", page), load_ptr(systemClass));
    SetGlobal(SymbolForChars("Block",  page), load_ptr(blockClass));
    
    symbolIfTrue  = _store_ptr(SymbolForChars("ifTrue:", page));
    symbolIfFalse = _store_ptr(SymbolForChars("ifFalse:", page));

    return systemObj;
}

void Universe::Assert(bool value) const {
    if (!value) {
        Universe::ErrorPrint("Universe::Assert Assertion failed\n");
    }
}

VMClass* Universe::GetBlockClass() const {
    return load_ptr(blockClass);
}

VMClass* Universe::GetBlockClassWithArgs(long numberOfArguments, Page* page) {
    map<long, GCClass*>::iterator it =
    blockClassesByNoOfArgs.find(numberOfArguments);
    if (it != blockClassesByNoOfArgs.end())
        return load_ptr(it->second);

    Assert(numberOfArguments < 10);

    ostringstream Str;
    Str << "Block" << numberOfArguments;
    VMSymbol* name = SymbolFor(Str.str(), page);
    VMClass* result = LoadClassBasic(name, nullptr, page);

#if GC_TYPE==GENERATIONAL
    result->AddInstancePrimitive(new (_HEAP, _PAGE) VMEvaluationPrimitive(numberOfArguments) );
#elif GC_TYPE==PAUSELESS
    result->AddInstancePrimitive(new (page, 0, true) VMEvaluationPrimitive(numberOfArguments, page), page);
#else
    result->AddInstancePrimitive(new (_HEAP) VMEvaluationPrimitive(numberOfArguments) );
#endif

    SetGlobal(name, result);
# warning is _store_ptr sufficient here?
    blockClassesByNoOfArgs[numberOfArguments] = _store_ptr(result);

    return result;
}

#if GC_TYPE==PAUSELESS
vm_oop_t Universe::GetGlobal(VMSymbol* name) {
    pthread_mutex_lock(&testMutex);
    map<GCSymbol*, gc_oop_t>::iterator it;
    it = globals.find((GCSymbol*) name);
    if (it == globals.end()) {
        it = globals.find(Flip((GCSymbol*) name));
    }
    if (it == globals.end()) {
        pthread_mutex_unlock(&testMutex);
        return nullptr;
    } else {
        pthread_mutex_unlock(&testMutex);
        return load_ptr(it->second);
    }
}
#else
vm_oop_t Universe::GetGlobal(VMSymbol* name) {
    auto it = globals.find(_store_ptr(name));
    if (it == globals.end())
        return nullptr;
    else
        return load_ptr(it->second);
}
#endif

#if GC_TYPE==PAUSELESS
bool Universe::HasGlobal(VMSymbol* name) {
    pthread_mutex_lock(&testMutex);
    map<GCSymbol*, gc_oop_t>::iterator it;
    it = globals.find((GCSymbol*) name);
    if (it == globals.end()) {
        it = globals.find(Flip((GCSymbol*) name));
    }
    if (it == globals.end()) {
        pthread_mutex_unlock(&testMutex);
        return false;
    }
    return true;
}
#else
bool Universe::HasGlobal(VMSymbol* name) {
    pthread_mutex_lock(&testMutex);
    auto it = globals.find(_store_ptr(name));
    if (it == globals.end()) {
        pthread_mutex_unlock(&testMutex);
        return false;
    }
    return true;
}
#endif

void Universe::InitializeSystemClass(VMClass* systemClass,
VMClass* superClass, const char* name, Page* page) {
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

    systemClass->SetInstanceFields(NewArray(0, page));
    sysClassClass->SetInstanceFields(NewArray(0, page));

    systemClass->SetInstanceInvokables(NewArray(0, page));
    sysClassClass->SetInstanceInvokables(NewArray(0, page));

    systemClass->SetName(SymbolFor(s_name, page));
    ostringstream Str;
    Str << s_name << " class";
    StdString classClassName(Str.str());
    sysClassClass->SetName(SymbolFor(classClassName, page));

    SetGlobal(systemClass->GetName(), systemClass);
}

VMClass* Universe::LoadClass(VMSymbol* name, Page* page) {
    pthread_mutex_lock(&classLoading);
    VMClass* result = static_cast<VMClass*>(GetGlobal(name));
    
    if (result != nullptr) {
        pthread_mutex_unlock(&classLoading);
        return result;
    }

    result = LoadClassBasic(name, nullptr, page);

    if (!result) {
		// we fail silently, it is not fatal that loading a class failed
        pthread_mutex_unlock(&classLoading);
		return static_cast<VMClass*>(load_ptr(nilObject));
    }

    if (result->HasPrimitives() || result->GetClass()->HasPrimitives())
        result->LoadPrimitives(classPath, page);
    
    SetGlobal(name, result);

    pthread_mutex_unlock(&classLoading);
    return result;
}

VMClass* Universe::LoadClassBasic(VMSymbol* name, VMClass* systemClass, Page* page) {
    StdString s_name = name->GetStdString();
    //sync_out(ostringstream() << "LoadClassBasic: " << name->GetChars());
    // assert(0 != strcmp(name->GetChars(), "nil")); // NOTE: name can be nil. During assembling we do a load again, unconditionally, also for nil symbol. Should be fixed...
    
    VMClass* result;

    for (vector<StdString>::iterator i = classPath.begin();
            i != classPath.end(); ++i) {
        SourcecodeCompiler compiler;
        result = compiler.CompileClass(*i, name->GetStdString(), systemClass, page);
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

VMClass* Universe::LoadShellClass(StdString& stmt, Page* page) {
    SourcecodeCompiler compiler;
    VMClass* result = compiler.CompileClassString(stmt, nullptr, page);
    if(dumpBytecodes)
        Disassembler::Dump(result);
    return result;
}

void Universe::LoadSystemClass(VMClass* systemClass, Page* page) {
    VMClass* result = LoadClassBasic(systemClass->GetName(), systemClass, page);
    StdString s = systemClass->GetName()->GetStdString();

    if (!result) {
        ErrorExit("Can't load system class: " + s + "\n");
    }

    if (result->HasPrimitives() || result->GetClass()->HasPrimitives())
        result->LoadPrimitives(classPath, page);
}

VMArray* Universe::NewArray(long size, Page* page) const {
    long additionalBytes = size * sizeof(VMObject*);
    
#if GC_TYPE==GENERATIONAL
    // if the array is too big for the nursery, we will directly allocate a
    // mature object
    bool outsideNursery = additionalBytes + sizeof(VMArray) > _HEAP->GetMaxObjectSize();

    VMArray* result = new (_HEAP, _PAGE, additionalBytes, outsideNursery) VMArray(size);
    if (outsideNursery)
        result->SetGCField(MASK_OBJECT_IS_OLD);
#elif GC_TYPE==PAUSELESS
    VMArray* result = new (page, additionalBytes) VMArray(size);
#else
    VMArray* result = new (_HEAP, additionalBytes) VMArray(size);
#endif

    result->SetClass(load_ptr(arrayClass));
    
    LOG_ALLOCATION("VMArray", result->GetObjectSize());
    return result;
}

VMArray* Universe::NewArrayFromStrings(const vector<StdString>& argv, Page* page) const {
    VMArray* result = NewArray(argv.size(), page);
    long j = 0;
    for (vector<StdString>::const_iterator i = argv.begin();
            i != argv.end(); ++i) {
        result->SetIndexableField(j, NewString(*i, page));
        ++j;
    }

    return result;
}

VMArray* Universe::NewArrayList(ExtendedList<VMSymbol*>& list, Page* page) const {
    ExtendedList<vm_oop_t>& objList = (ExtendedList<vm_oop_t>&) list;
    return NewArrayList(objList, page);
}

VMArray* Universe::NewArrayList(ExtendedList<VMInvokable*>& list, Page* page) const {
    ExtendedList<vm_oop_t>& objList = (ExtendedList<vm_oop_t>&) list;
    return NewArrayList(objList, page);
}

VMArray* Universe::NewArrayList(ExtendedList<vm_oop_t>& list, Page* page) const {
    long size = list.Size();
    VMArray* result = NewArray(size, page);

    if (result) {
        for (long i = 0; i < size; ++i) {
            vm_oop_t elem = list.Get(i);
            result->SetIndexableField(i, elem);
        }
    }
    return result;
}

VMBlock* Universe::NewBlock(VMMethod* method, VMFrame* context, long arguments, Page* page) {
#if GC_TYPE==GENERATIONAL
    VMBlock* result = new (_HEAP, _PAGE) VMBlock;
#elif GC_TYPE==PAUSELESS
    VMBlock* result = new (page) VMBlock;
#else
    VMBlock* result = new (_HEAP) VMBlock;
#endif
    result->SetClass(GetBlockClassWithArgs(arguments, page));

    result->SetMethod(method);
    result->SetContext(context);

    LOG_ALLOCATION("VMBlock", result->GetObjectSize());
    return result;
}

VMClass* Universe::NewClass(VMClass* classOfClass, Page* page) const {
    long numFields = classOfClass->GetNumberOfInstanceFields();
    VMClass* result;
    long additionalBytes = numFields * sizeof(VMObject*);
    if (numFields)
#if GC_TYPE==GENERATIONAL
    result = new (_HEAP, _PAGE, additionalBytes) VMClass(numFields);
#elif GC_TYPE==PAUSELESS
    result = new (page, additionalBytes, true) VMClass(numFields);
#else
    result = new (_HEAP, additionalBytes) VMClass(numFields);
#endif
    else
#if GC_TYPE==GENERATIONAL
        result = new (_HEAP, _PAGE) VMClass;
#elif GC_TYPE==PAUSELESS
        result = new (page, 0, true) VMClass;
#else
        result = new (_HEAP) VMClass;
#endif

    result->SetClass(classOfClass);

    LOG_ALLOCATION("VMClass", result->GetObjectSize());
    return result;
}

VMDouble* Universe::NewDouble(double value, Page* page) const {
    LOG_ALLOCATION("VMDouble", sizeof(VMDouble));
#if GC_TYPE==GENERATIONAL
    return new (_HEAP, _PAGE) VMDouble(value);
#elif GC_TYPE==PAUSELESS
    return new (page) VMDouble(value);
#else
    return new (_HEAP) VMDouble(value);
#endif
}

VMFrame* Universe::NewFrame(VMFrame* previousFrame, VMMethod* method, Page* page) const {
    VMFrame* result = nullptr;
    
    /*
#ifdef UNSAFE_FRAME_OPTIMIZATION
    result = method->GetCachedFrame();
    if (result != nullptr) {
        method->SetCachedFrame(nullptr);
        result->SetPreviousFrame(previousFrame);
        return result;
    }
#endif
    */
    
    long length = method->GetNumberOfArguments() +
                  method->GetNumberOfLocals() +
                  method->GetMaximumNumberOfStackElements();

    long additionalBytes = length * sizeof(VMObject*);
#if GC_TYPE==GENERATIONAL
    result = new (_HEAP, _PAGE, additionalBytes) VMFrame(length);
#elif GC_TYPE==PAUSELESS
    result = new (page, additionalBytes) VMFrame(length);
#else
    result = new (_HEAP, additionalBytes) VMFrame(length);
#endif
    result->clazz = nullptr;
# warning I think _store_ptr is sufficient here, but...
    result->method        = _store_ptr(method);
    result->previousFrame = _store_ptr(previousFrame);
    result->ResetStackPointer();

    LOG_ALLOCATION("VMFrame", result->GetObjectSize());
    return result;
}

VMObject* Universe::NewInstance(VMClass* classOfInstance, Page* page) const {
    long numOfFields = classOfInstance->GetNumberOfInstanceFields();
    //the additional space needed is calculated from the number of fields
    long additionalBytes = numOfFields * sizeof(VMObject*);
#if GC_TYPE==GENERATIONAL
    VMObject* result = new (_HEAP, _PAGE, additionalBytes) VMObject(numOfFields);
#elif GC_TYPE==PAUSELESS
    VMObject* result = new (page, additionalBytes) VMObject(numOfFields);
#else
    VMObject* result = new (_HEAP, additionalBytes) VMObject(numOfFields);
#endif
    result->SetClass(classOfInstance);

    LOG_ALLOCATION(classOfInstance->GetName()->GetStdString(), result->GetObjectSize());
    return result;
}

VMInteger* Universe::NewInteger(int64_t value, Page* page) const {

#ifdef GENERATE_INTEGER_HISTOGRAM
    integerHist[value/INT_HIST_SIZE] = integerHist[value/INT_HIST_SIZE]+1;
#endif

#if CACHE_INTEGER
    unsigned long index = (unsigned long)value - (unsigned long)INT_CACHE_MIN_VALUE;
    if (index < (unsigned long)(INT_CACHE_MAX_VALUE - INT_CACHE_MIN_VALUE)) {
        return prebuildInts[index];
    }
#endif

    LOG_ALLOCATION("VMInteger", sizeof(VMInteger));
#if GC_TYPE==GENERATIONAL
    return new (_HEAP, _PAGE) VMInteger(value);
#elif GC_TYPE==PAUSELESS
    return new (page) VMInteger(value);
#else
    return new (_HEAP) VMInteger(value);
#endif
}

VMClass* Universe::NewMetaclassClass(Page* page) const {
#if GC_TYPE==GENERATIONAL
    VMClass* result = new (_HEAP, _PAGE) VMClass;
    result->SetClass(new (_HEAP, _PAGE) VMClass);
#elif GC_TYPE==PAUSELESS
    VMClass* result = new (page, 0, true) VMClass;
    result->SetClass(new (page, 0, true) VMClass);
#else
    VMClass* result = new (_HEAP) VMClass;
    result->SetClass(new (_HEAP) VMClass);
#endif
    VMClass* mclass = result->GetClass();
    mclass->SetClass(result);

    LOG_ALLOCATION("VMClass", result->GetObjectSize());
    return result;
}

#if GC_TYPE==PAUSELESS
void Universe::MarkGlobals() {
    ReadBarrierForGCThread(&nilObject, true);
    ReadBarrierForGCThread(&trueObject, true);
    ReadBarrierForGCThread(&falseObject, true);
    
    ReadBarrierForGCThread(&objectClass, true);
    ReadBarrierForGCThread(&classClass, true);
    ReadBarrierForGCThread(&metaClassClass, true);
    
    ReadBarrierForGCThread(&nilClass, true);
    ReadBarrierForGCThread(&integerClass, true);
    ReadBarrierForGCThread(&arrayClass, true);
    ReadBarrierForGCThread(&methodClass, true);
    ReadBarrierForGCThread(&symbolClass, true);
    ReadBarrierForGCThread(&primitiveClass, true);
    ReadBarrierForGCThread(&stringClass, true);
    ReadBarrierForGCThread(&systemClass, true);
    ReadBarrierForGCThread(&blockClass, true);
    ReadBarrierForGCThread(&doubleClass, true);
    
    ReadBarrierForGCThread(&threadClass, true);
    ReadBarrierForGCThread(&mutexClass, true);
    ReadBarrierForGCThread(&signalClass, true);
    
    ReadBarrierForGCThread(&trueClass, true);
    ReadBarrierForGCThread(&falseClass, true);
    
    
    pthread_mutex_lock(&testMutex);
    
    // walk all entries in globals map
    map<GCSymbol*, gc_oop_t> globs;
    map<GCSymbol*, gc_oop_t>::iterator iter;
    for (iter = globals.begin(); iter != globals.end(); iter++) {
        vm_oop_t val = ReadBarrierForGCThread(&iter->second, true);
        if (val == nullptr)
            continue;
        GCSymbol* key = iter->first;
        //sync_out(ostringstream() << "GLOB OLD: " << iter->first);
        //globs[key] = WriteBarrierForGCThread(val);
        
        GCSymbol* new_ptr = WriteBarrierForGCThread(ReadBarrierForGCThread(&key,true));
        globs[new_ptr] = WriteBarrierForGCThread(val);
        
        //sync_out(ostringstream() << "GLOB NEW: " << new_ptr);
    }
    globals = globs;
    
    //cout << "Mark symbol map" << endl;
    // walk all entries in symbols map
    map<StdString, GCSymbol*>::iterator symbolIter;
    for (symbolIter = symbolsMap.begin();
         symbolIter != symbolsMap.end();
         symbolIter++) {
        //insert overwrites old entries inside the internal map
        symbolIter->second = WriteBarrierForGCThread(ReadBarrierForGCThread(&symbolIter->second, true));
    }
    //cout << "Mark block classes" << endl;
    map<long, GCClass*>::iterator bcIter;
    for (bcIter = blockClassesByNoOfArgs.begin();
         bcIter != blockClassesByNoOfArgs.end();
         bcIter++) {
        bcIter->second = WriteBarrierForGCThread(ReadBarrierForGCThread(&bcIter->second, true));
    }
    
    //reassign ifTrue ifFalse Symbols
    symbolIfTrue  = symbolsMap["ifTrue:"];
    symbolIfFalse = symbolsMap["ifFalse:"];

/*
    map<string, GCSymbol*>::iterator it = symbolsMap.find("true");
    //VMSymbol* trueSym = (VMSymbol*) ReadBarrierForGCThread(&it->second);
    VMSymbol* trueSym = Untag(it->second);
    
    
    GCAbstractObject* raw_glob = globals[(GCSymbol*) trueSym];  // Cast is Performance HACK to avoid barrier!!!
    if (raw_glob == nullptr)
        raw_glob = globals[(GCSymbol*) Flip(trueSym)];
    
    //VMObject* glob_ptr_val = ReadBarrierForGCThread(&raw_glob);
    VMObject* glob_ptr_val = Untag(raw_glob);

    assert(glob_ptr_val == Untag(trueObject));
*/
    
    pthread_mutex_unlock(&testMutex);
    
    
}
void  Universe::CheckMarkingGlobals(void (*walk)(vm_oop_t)) {
    walk(Untag(nilObject));
    walk(Untag(trueObject));
    walk(Untag(falseObject));

    walk(Untag(objectClass));
    walk(Untag(classClass));
    walk(Untag(metaClassClass));
    
    walk(Untag(nilClass));
    walk(Untag(integerClass));
    walk(Untag(arrayClass));
    walk(Untag(methodClass));
    walk(Untag(symbolClass));
    walk(Untag(primitiveClass));
    walk(Untag(stringClass));
    walk(Untag(systemClass));
    walk(Untag(blockClass));
    walk(Untag(doubleClass));
    
    walk(Untag(threadClass));
    walk(Untag(mutexClass));
    walk(Untag(signalClass));
    
    walk(Untag(trueClass));
    walk(Untag(falseClass));
    
    // walk all entries in globals map
    map<GCSymbol*, gc_oop_t>::iterator iter;
    for (iter = globals.begin(); iter != globals.end(); iter++) {
        if (iter->second == nullptr)
            continue;
        walk(Untag(iter->first));
        walk(Untag(iter->second));
    }
    
    // walk all entries in symbols map
    map<StdString, GCSymbol*>::iterator symbolIter;
    for (symbolIter = symbolsMap.begin();
         symbolIter != symbolsMap.end();
         symbolIter++) {
        //insert overwrites old entries inside the internal map
        walk(Untag(symbolIter->second));
    }
    
    map<long, GCClass*>::iterator bcIter;
    for (bcIter = blockClassesByNoOfArgs.begin();
         bcIter != blockClassesByNoOfArgs.end();
         bcIter++) {
        walk(Untag(bcIter->second));
    }
}
#else
void Universe::WalkGlobals(walk_heap_fn walk, Page* page) {
    nilObject   = static_cast<GCObject*>(walk(nilObject,    page));
    trueObject  = static_cast<GCObject*>(walk(trueObject,   page));
    falseObject = static_cast<GCObject*>(walk(falseObject,  page));
    systemObject= static_cast<GCObject*>(walk(systemObject, page));

#if USE_TAGGING
    GlobalBox::WalkGlobals(walk, page);
#endif

    objectClass    = static_cast<GCClass*>(walk(objectClass,     page));
    classClass     = static_cast<GCClass*>(walk(classClass,      page));
    metaClassClass = static_cast<GCClass*>(walk(metaClassClass,  page));

    nilClass        = static_cast<GCClass*>(walk(nilClass,       page));
    integerClass    = static_cast<GCClass*>(walk(integerClass,   page));
    arrayClass      = static_cast<GCClass*>(walk(arrayClass,     page));
    methodClass     = static_cast<GCClass*>(walk(methodClass,    page));
    symbolClass     = static_cast<GCClass*>(walk(symbolClass,    page));
    primitiveClass  = static_cast<GCClass*>(walk(primitiveClass, page));
    stringClass     = static_cast<GCClass*>(walk(stringClass,    page));
    systemClass     = static_cast<GCClass*>(walk(systemClass,    page));
    blockClass      = static_cast<GCClass*>(walk(blockClass,     page));
    doubleClass     = static_cast<GCClass*>(walk(doubleClass,    page));
    
    signalClass     = static_cast<GCClass*>(walk(signalClass,    page));
    mutexClass      = static_cast<GCClass*>(walk(mutexClass,     page));
    threadClass     = static_cast<GCClass*>(walk(threadClass,    page));
    
    trueClass  = static_cast<GCClass*>(walk(trueClass,  page));
    falseClass = static_cast<GCClass*>(walk(falseClass, page));

#if CACHE_INTEGER
    for (unsigned long i = 0; i < (INT_CACHE_MAX_VALUE - INT_CACHE_MIN_VALUE); i++)
#if USE_TAGGING
        prebuildInts[i] = TAG_INTEGER(INT_CACHE_MIN_VALUE + i, page);
#else
        prebuildInts[i] = walk(prebuildInts[i], page);
#endif
#endif

    // walk all entries in globals map
    map<GCSymbol*, gc_oop_t> globs = globals;
    globals.clear();
    for (auto iter = globs.begin(); iter != globs.end(); iter++) {
        if (iter->second == nullptr)
            continue;

        GCSymbol* key = static_cast<GCSymbol*>(walk(iter->first, page));
        gc_oop_t val = walk(iter->second, page);
        globals[key] = val;
    }
    
    // walk all entries in symbols map
    map<StdString, GCSymbol*>::iterator symbolIter;
    for (symbolIter = symbolsMap.begin();
         symbolIter != symbolsMap.end();
         symbolIter++) {
        //insert overwrites old entries inside the internal map
        symbolIter->second = static_cast<GCSymbol*>(walk(symbolIter->second, page));
    }

    map<long, GCClass*>::iterator bcIter;
    for (bcIter = blockClassesByNoOfArgs.begin();
         bcIter != blockClassesByNoOfArgs.end();
         bcIter++) {
        bcIter->second = static_cast<GCClass*>(walk(bcIter->second, page));
    }

    //reassign ifTrue ifFalse Symbols
    symbolIfTrue  = symbolsMap["ifTrue:"];
    symbolIfFalse = symbolsMap["ifFalse:"];
    
}
#endif

VMMethod* Universe::NewMethod(VMSymbol* signature,
        size_t numberOfBytecodes, size_t numberOfConstants, Page* page) const {
    //Method needs space for the bytecodes and the pointers to the constants
    long additionalBytes = numberOfBytecodes + numberOfConstants*sizeof(VMObject*);

#if GC_TYPE==GENERATIONAL
    VMMethod* result = new (_HEAP, _PAGE, additionalBytes)
#elif GC_TYPE==PAUSELESS
    VMMethod* result = new (page, additionalBytes, true)
#else
    VMMethod* result = new (_HEAP,additionalBytes)
#endif
                VMMethod(numberOfBytecodes, numberOfConstants, 0, page);

    result->SetClass(load_ptr(methodClass));
    result->SetSignature(signature, page);

    LOG_ALLOCATION("VMMethod", result->GetObjectSize());
    return result;
}

VMMutex* Universe::NewMutex(Page* page) const {
#if GC_TYPE==GENERATIONAL
    VMMutex* result = new (_HEAP, _PAGE) VMMutex();
#elif GC_TYPE==PAUSELESS
    VMMutex* result = new (page) VMMutex();
#else
    VMMutex* result = new (_HEAP) VMMutex();
#endif
    result->SetClass(load_ptr(mutexClass));

    LOG_ALLOCATION("VMMutex", sizeof(VMMutex));
    return result;
}

VMSignal* Universe::NewSignal(Page* page) const {
#if GC_TYPE==GENERATIONAL
    VMSignal* result = new (_HEAP, _PAGE) VMSignal();
#elif GC_TYPE==PAUSELESS
    VMSignal* result = new (page) VMSignal();
#else
    VMSignal* result = new (_HEAP) VMSignal();
#endif
    result->SetClass(load_ptr(signalClass));

    LOG_ALLOCATION("VMSignal", sizeof(VMSignal));
    return result;
}

VMThread* Universe::NewThread(Page* page) const {
#if GC_TYPE==GENERATIONAL
    VMThread* result = new (_HEAP, _PAGE) VMThread();
#elif GC_TYPE==PAUSELESS
    VMThread* result = new (page) VMThread();
#else
    VMThread* result = new (_HEAP) VMThread();
#endif
    //result->SetThreadId(threadCounter);
    //threadCounter += 1;
    result->SetClass(load_ptr(threadClass));

    LOG_ALLOCATION("VMThread", sizeof(VMThread));
    return result;
}

VMString* Universe::NewString(const StdString& str, Page* page) const {
    return NewString(str.c_str(), page);
}

VMString* Universe::NewString(const char* str, Page* page) const {
#if GC_TYPE==GENERATIONAL
    VMString* result = new (_HEAP, _PAGE, PADDED_SIZE(strlen(str) + 1)) VMString(str);
#elif GC_TYPE==PAUSELESS
    VMString* result = new (page, PADDED_SIZE(strlen(str) + 1)) VMString(str);
#else
    VMString* result = new (_HEAP, PADDED_SIZE(strlen(str) + 1)) VMString(str);
#endif

    LOG_ALLOCATION("VMString", result->GetObjectSize());
    return result;
}

VMSymbol* Universe::NewSymbol(const StdString& str, Page* page) {
    return NewSymbol(str.c_str(), page);
}

VMSymbol* Universe::NewSymbol(const char* str, Page* page) {
#if GC_TYPE==GENERATIONAL
    VMSymbol* result = new (_HEAP, _PAGE, PADDED_SIZE(strlen(str)+1)) VMSymbol(str);
#elif GC_TYPE==PAUSELESS
    VMSymbol* result = new (page, PADDED_SIZE(strlen(str)+1), true) VMSymbol(str);
#else
    VMSymbol* result = new (_HEAP, PADDED_SIZE(strlen(str)+1)) VMSymbol(str);
#endif
    symbolsMap[str] = _store_ptr(result);

    LOG_ALLOCATION("VMSymbol", result->GetObjectSize());
    return result;
}

VMClass* Universe::NewSystemClass(Page* page) const {
#if GC_TYPE==GENERATIONAL
    VMClass* systemClass = new (_HEAP, _PAGE) VMClass();
    systemClass->SetClass(new (_HEAP, _PAGE) VMClass());
#elif GC_TYPE==PAUSELESS
    VMClass* systemClass = new (page, 0, true) VMClass();
    systemClass->SetClass(new (page, 0, true) VMClass());
#else
    VMClass* systemClass = new (_HEAP) VMClass();
    systemClass->SetClass(new (_HEAP) VMClass());
#endif

    VMClass* mclass = systemClass->GetClass();

    mclass->SetClass(load_ptr(metaClassClass));

    LOG_ALLOCATION("VMClass", systemClass->GetObjectSize());
    return systemClass;
}

VMSymbol* Universe::SymbolFor(const StdString& str, Page* page) {
    map<string, GCSymbol*>::iterator it = symbolsMap.find(str);
    
    if (it == symbolsMap.end()) {
        //sync_out(ostringstream() << "Create new symbol: " << str.c_str());
        return NewSymbol(str, page);
    } else {
        return load_ptr(it->second);
    }
    //return (it == symbolsMap.end()) ? NewSymbol(str) : it->second;
}

VMSymbol* Universe::SymbolForChars(const char* str, Page* page) {
    return SymbolFor(str, page);
}

void Universe::SetGlobal(VMSymbol* name, vm_oop_t val) {
    pthread_mutex_lock(&testMutex);

# warning is _store_ptr correct here? it relies on _store_ptr not to be really changed...
    globals[_store_ptr(name)] = _store_ptr(val);
    pthread_mutex_unlock(&testMutex);
}

void Universe::RemoveInterpreter() {
    pthread_mutex_lock(&interpreterMutex);
    interpreters.erase(std::remove(interpreters.begin(), interpreters.end(), this->GetInterpreter()), interpreters.end());
    pthread_mutex_unlock(&interpreterMutex);
}

#if GC_TYPE!=PAUSELESS
Interpreter* Universe::NewInterpreter() {
    Interpreter* interpreter = new Interpreter();
    pthread_setspecific(this->interpreterKey, interpreter);
    pthread_mutex_lock(&interpreterMutex);
    interpreters.push_back(interpreter);
    pthread_mutex_unlock(&interpreterMutex);
    return interpreter;
}

vector<Interpreter*>* Universe::GetInterpreters() {
    return &interpreters;
}
#else
Interpreter* Universe::NewInterpreter() {
    pthread_mutex_lock(_HEAP->GetNewInterpreterMutex());
    Interpreter* interpreter;
    pthread_mutex_lock(&interpreterMutex);
    if (interpreters.empty())
        interpreter = new Interpreter(false, true);
    else
        interpreter = new Interpreter(interpreters.back()->GetExpectedNMT(), interpreters.back()->GCTrapEnabled());
    pthread_setspecific(this->interpreterKey, interpreter);
    interpreters.push_back(interpreter);
    pthread_mutex_unlock(&interpreterMutex);
    pthread_mutex_unlock(_HEAP->GetNewInterpreterMutex());
    return interpreter;
}

unique_ptr<vector<Interpreter*>> Universe::GetInterpretersCopy() {
    pthread_mutex_lock(&interpreterMutex);
    unique_ptr<vector<Interpreter*>> copy(new vector<Interpreter*>(interpreters.begin(),interpreters.end()));
    pthread_mutex_unlock(&interpreterMutex);
    return copy;
}
#endif

// FOR DEBUGGING PURPOSES
/*
void Universe::PrintGlobals() {
    map<GCSymbol*, GCAbstractObject*>::iterator it;
    for (it = globals.begin(); it != globals.end(); it++) {
        sync_out(ostringstream() << "[GLOBALS] symbol: " << Untag(it->first)->GetChars()
                 << " ptr: " << it->first << " value ptr: " << it->second);
    }
} */

void Universe::Print(StdString str) {
    lock_guard<mutex> lock(output_mutex);
    cout << str << flush;
}

void Universe::ErrorPrint(StdString str) {
    lock_guard<mutex> lock(output_mutex);
    cerr << str << flush;
}
