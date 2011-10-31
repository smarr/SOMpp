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

#include "Universe.h"
#include "Shell.h"

#include "../vmobjects/VMSymbol.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMBlock.h"
#include "../vmobjects/VMDouble.h"
#include "../vmobjects/VMInteger.h"
#include "../vmobjects/VMString.h"
#include "../vmobjects/VMBigInteger.h"
#include "../vmobjects/VMEvaluationPrimitive.h"
#ifdef USE_TAGGING
#include "../vmobjects/VMPointerConverter.h"
#endif

#include "../interpreter/bytecodes.h"

#include "../compiler/Disassembler.h"
#include "../compiler/SourcecodeCompiler.h"

#ifdef CACHE_INTEGER
#define INT_CACHE_MIN_VALUE (-5)
#define INT_CACHE_MAX_VALUE (100)
pVMInteger prebuildInts[INT_CACHE_MAX_VALUE - INT_CACHE_MIN_VALUE + 1];
#endif

// Here we go:

short dumpBytecodes;
short gcVerbosity;


Universe* Universe::theUniverse = NULL;

pVMObject nilObject;
pVMObject trueObject;
pVMObject falseObject;
     
pVMClass objectClass;
pVMClass classClass;
pVMClass metaClassClass;
  
pVMClass nilClass;
pVMClass integerClass;
pVMClass bigIntegerClass;
pVMClass arrayClass;
pVMClass methodClass;
pVMClass symbolClass;
pVMClass frameClass;
pVMClass primitiveClass;
pVMClass stringClass;
pVMClass systemClass;
pVMClass blockClass;
pVMClass doubleClass;


std::map<std::string, pVMSymbol> symbolsMap;

//Singleton accessor
Universe* Universe::GetUniverse() {
    if (!theUniverse) {
        ErrorExit("Trying to access uninitialized Universe, exiting.");
    }
	return theUniverse;
}


Universe* Universe::operator->() {
    if (!theUniverse) {
        ErrorExit("Trying to access uninitialized Universe, exiting.");
    }
	return theUniverse;
}


void Universe::Start(int argc, char** argv) {
	theUniverse = new Universe();
    theUniverse->initialize(argc, argv);
}


void Universe::Quit(int err) {
	cout << "Time spent in GC: [" << Timer::GCTimer->GetTotalTime() << "] msec" << endl;
    if (theUniverse) delete(theUniverse);

    exit(err);
}


void Universe::ErrorExit( const char* err) {
    cout << "Runtime error: " << err << endl;
    Quit(ERR_FAIL);
}

vector<StdString> Universe::handleArguments( int argc, char** argv ) {

    vector<StdString> vmArgs = vector<StdString>();
    dumpBytecodes = 0;
    gcVerbosity = 0;
    for (int i = 1; i < argc ; ++i) {
        
        if (strncmp(argv[i], "-cp", 3) == 0) {
            if ((argc == i + 1) || classPath.size() > 0)
                printUsageAndExit(argv[0]);
            setupClassPath(StdString(argv[++i]));
        } else if (strncmp(argv[i], "-d", 2) == 0) {
            ++dumpBytecodes;
        } else if (strncmp(argv[i], "-g", 2) == 0) {
            ++gcVerbosity;
        } else if (strncmp(argv[i], "-H",2) == 0) {
            int heap_size = 0;
			if (sscanf(argv[i],"-H%dMB", &heap_size) == 1)
				heapSize = heap_size * 1024 * 1024;
			else if (sscanf(argv[i],"-H%dKB", &heap_size) == 1)
				heapSize = heap_size * 1024;
			else
				printUsageAndExit(argv[0]);

        } else if ((strncmp(argv[i], "-h", 2) == 0) ||
            (strncmp(argv[i], "--help", 6) == 0)) {
                printUsageAndExit(argv[0]);
        } else {
            vector<StdString> extPathTokens = vector<StdString>(2);
            StdString tmpString = StdString(argv[i]);
            if (this->getClassPathExt(extPathTokens, tmpString) ==
                                        ERR_SUCCESS) {
                this->addClassPath(extPathTokens[0]);
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

int Universe::getClassPathExt(vector<StdString>& tokens,const StdString& arg ) const {
#define EXT_TOKENS 2
    int result = ERR_SUCCESS;
    int fpIndex = arg.find_last_of(fileSeparator);
    int ssepIndex = arg.find(".som");

    if (fpIndex == StdString::npos) { //no new path
        //different from CSOM (see also HandleArguments):
        //we still want to strip the suffix from the filename, so
        //we set the start to -1, in order to start the substring
        //from character 0. npos is -1 too, but this is to make sure
        fpIndex = -1;
        //instead of returning here directly, we have to remember that
        //there is no new class path and return it later
        result = ERR_FAIL;
    } else tokens[0] = arg.substr(0, fpIndex);
    
    //adding filename (minus ".som" if present) to second slot
    ssepIndex = ( (ssepIndex != StdString::npos) && (ssepIndex > fpIndex)) ?
                 (ssepIndex - 1) :
                 arg.length();
    tokens[1] = arg.substr(fpIndex + 1, ssepIndex - (fpIndex));
    return result;
}


int Universe::setupClassPath( const StdString& cp ) {
    try {
        std::stringstream ss ( cp );
        StdString token;

        int i = 0;
        while( getline(ss, token, pathSeparator) ) {
            classPath.push_back(token);
            ++i;
        }

        return ERR_SUCCESS;
    } catch(std::exception e){ 
        return ERR_FAIL;
    }
}


int Universe::addClassPath( const StdString& cp ) {
    classPath.push_back(cp);
    return ERR_SUCCESS;
}


void Universe::printUsageAndExit( char* executable ) const {
    cout << "Usage: " << executable << " [-options] [args...]" << endl << endl;
    cout << "where options include:" << endl;
    cout << "    -cp <directories separated by " << pathSeparator 
         << ">" << endl;
    cout << "        set search path for application classes" << endl;
    cout << "    -d  enable disassembling (twice for tracing)" << endl;
    cout << "    -g  enable garbage collection details:" << endl <<
                    "        1x - print statistics when VM shuts down" << endl <<
                    "        2x - print statistics upon each collection" << endl <<
                    "        3x - print statistics and dump _HEAP upon each "  << endl <<
                    "collection" << endl;
    cout << "    -HxMB set the _HEAP size to x MB (default: 1 MB)" << endl;
    cout << "    -HxKB set the _HEAP size to x KB (default: 1 MB)" << endl;
    cout << "    -h  show this help" << endl;

    Quit(ERR_SUCCESS);
}


Universe::Universe(){
	this->compiler = NULL;
	this->interpreter = NULL;
};


void Universe::initialize(int _argc, char** _argv) {

    heapSize = 1 * 1024 * 1024;

    vector<StdString> argv = this->handleArguments(_argc, _argv);

    Heap::InitializeHeap(heapSize);

    heap = _HEAP;

    compiler = new SourcecodeCompiler();
    interpreter = new Interpreter();

#ifdef CACHE_INTEGER
  //create prebuilt integers
  for (int32_t it = INT_CACHE_MIN_VALUE; it <= INT_CACHE_MAX_VALUE; ++it) {
    prebuildInts[(uint32_t)(it - INT_CACHE_MIN_VALUE)] = new (_HEAP) VMInteger(it);
  }
#endif
    
    InitializeGlobals();

    pVMObject systemObject = NewInstance(systemClass);

    this->SetGlobal(SymbolForChars("nil"), nilObject);
    this->SetGlobal(SymbolForChars("true"), trueObject);
    this->SetGlobal(SymbolForChars("false"), falseObject);
    this->SetGlobal(SymbolForChars("system"), systemObject);
    this->SetGlobal(SymbolForChars("System"), systemClass);
    this->SetGlobal(SymbolForChars("Block"), blockClass);
    
    pVMMethod bootstrapMethod = NewMethod(SymbolForChars("bootstrap"), 1, 0);
    bootstrapMethod->SetBytecode(0, BC_HALT);
    bootstrapMethod->SetNumberOfLocals(0);

    bootstrapMethod->SetMaximumNumberOfStackElements(2);
    bootstrapMethod->SetHolder(systemClass);
    
    if (argv.size() == 0) {
        Shell* shell = new Shell(bootstrapMethod);
        shell->Start();
        return;
    }

    /* only trace bootstrap if the number of cmd-line "-d"s is > 2 */
    short trace = 2 - dumpBytecodes;
    if(!(trace > 0)) dumpBytecodes = 1;

    pVMArray argumentsArray = _UNIVERSE->NewArrayFromArgv(argv);
    
    pVMFrame bootstrapFrame = interpreter->PushNewFrame(bootstrapMethod);
    bootstrapFrame->Push(systemObject);
    bootstrapFrame->Push((pVMObject)argumentsArray);

    pVMInvokable initialize = 
#ifdef USE_TAGGING
        DynamicConvert<VMInvokable, VMObject>(
            systemClass->LookupInvokable(this->SymbolForChars("initialize:")));
#else
        dynamic_cast<pVMInvokable>(systemClass->LookupInvokable(this->SymbolForChars("initialize:")));
#endif
    (*initialize)(bootstrapFrame);
    
    // reset "-d" indicator
    if(!(trace>0)) dumpBytecodes = 2 - trace;

    interpreter->Start();
}


Universe::~Universe() {
    if (interpreter) 
        delete(interpreter);
    if (compiler) 
        delete(compiler);

	// check done inside
    Heap::DestroyHeap();
}

void Universe::InitializeGlobals() {
    //
    //allocate nil object
    //
    nilObject = new (_HEAP) VMObject;
    nilObject->SetField(0, nilObject);

    metaClassClass = NewMetaclassClass();

    objectClass     = NewSystemClass();
    nilClass        = NewSystemClass();
    classClass      = NewSystemClass();
    arrayClass      = NewSystemClass();
    symbolClass     = NewSystemClass();
    methodClass     = NewSystemClass();
    integerClass    = NewSystemClass();
    bigIntegerClass = NewSystemClass();
    frameClass      = NewSystemClass();
    primitiveClass  = NewSystemClass();
    stringClass     = NewSystemClass();
    doubleClass     = NewSystemClass();
    
    nilObject->SetClass(nilClass);

    InitializeSystemClass(objectClass, NULL, "Object");
    InitializeSystemClass(classClass, objectClass, "Class");
    InitializeSystemClass(metaClassClass, classClass, "Metaclass");
    InitializeSystemClass(nilClass, objectClass, "Nil");
    InitializeSystemClass(arrayClass, objectClass, "Array");
    InitializeSystemClass(methodClass, arrayClass, "Method");
    InitializeSystemClass(symbolClass, objectClass, "Symbol");
    InitializeSystemClass(integerClass, objectClass, "Integer");
    InitializeSystemClass(bigIntegerClass, objectClass,
                                     "BigInteger");
    InitializeSystemClass(frameClass, arrayClass, "Frame");
    InitializeSystemClass(primitiveClass, objectClass,
                                     "Primitive");
    InitializeSystemClass(stringClass, objectClass, "String");
    InitializeSystemClass(doubleClass, objectClass, "Double");

    LoadSystemClass(objectClass);
    LoadSystemClass(classClass);
    LoadSystemClass(metaClassClass);
    LoadSystemClass(nilClass);
    LoadSystemClass(arrayClass);
    LoadSystemClass(methodClass);
    LoadSystemClass(symbolClass);
    LoadSystemClass(integerClass);
    LoadSystemClass(bigIntegerClass);
    LoadSystemClass(frameClass);
    LoadSystemClass(primitiveClass);
    LoadSystemClass(stringClass);
    LoadSystemClass(doubleClass);

    blockClass = LoadClass(_UNIVERSE->SymbolForChars("Block"));

    trueObject = NewInstance(_UNIVERSE->LoadClass(_UNIVERSE->SymbolForChars("True")));
    falseObject = NewInstance(_UNIVERSE->LoadClass(_UNIVERSE->SymbolForChars("False")));

    systemClass = LoadClass(_UNIVERSE->SymbolForChars("System"));
}

void Universe::Assert( bool value) const {
    if (!value)  {
        cout << "Assertion failed" << endl;
    }

}


pVMClass Universe::GetBlockClass() const {
    return blockClass;
}


pVMClass Universe::GetBlockClassWithArgs( int numberOfArguments) {
	map<int, pVMClass>::iterator it =
		blockClassesByNoOfArgs.find(numberOfArguments);
	if (it != blockClassesByNoOfArgs.end())
		return it->second;


    this->Assert(numberOfArguments < 10);
    ostringstream Str;
    Str << "Block" << numberOfArguments ;
    pVMSymbol name = SymbolFor(Str.str());
    pVMClass result = LoadClassBasic(name, NULL);
    result->AddInstancePrimitive(new (_HEAP) VMEvaluationPrimitive(numberOfArguments) );
    SetGlobal(name, (pVMObject) result);

	blockClassesByNoOfArgs[numberOfArguments] = result;

    return result;
}



pVMObject Universe::GetGlobal( pVMSymbol name) {
    if (HasGlobal(name))
        return (pVMObject)globals[name];

    return NULL;
}


bool Universe::HasGlobal( pVMSymbol name) {
    if (globals[name] != NULL) return true;
    else return false;
}


void Universe::InitializeSystemClass( pVMClass systemClass, 
                                     pVMClass superClass, const char* name) {
    StdString s_name(name);

    if (superClass != NULL) {
        systemClass->SetSuperClass(superClass);
        pVMClass sysClassClass = systemClass->GetClass();
        pVMClass superClassClass = superClass->GetClass();
        sysClassClass->SetSuperClass(superClassClass);
    } else {
        pVMClass sysClassClass = systemClass->GetClass();
        sysClassClass->SetSuperClass(classClass);
    }

    pVMClass sysClassClass = systemClass->GetClass();

    systemClass->SetInstanceFields(NewArray(0));
    sysClassClass->SetInstanceFields(NewArray(0));

    systemClass->SetInstanceInvokables(NewArray(0));
    sysClassClass->SetInstanceInvokables(NewArray(0));

    systemClass->SetName(SymbolFor(s_name));
    ostringstream Str;
    Str << s_name << " class";
    StdString classClassName(Str.str());
    sysClassClass->SetName(SymbolFor(classClassName));

    SetGlobal(systemClass->GetName(), (pVMObject)systemClass);


}


pVMClass Universe::LoadClass( pVMSymbol name) {
   if (HasGlobal(name))
#ifdef USE_TAGGING
       return DynamicConvert<VMClass, VMObject>(GetGlobal(name));
#else
       return dynamic_cast<pVMClass>(GetGlobal(name));
#endif

   pVMClass result = LoadClassBasic(name, NULL);

   if (!result) {
       cout << "can\'t load class " << name->GetStdString() << endl;
       Universe::Quit(ERR_FAIL);
   }

   if (result->HasPrimitives() || result->GetClass()->HasPrimitives())
       result->LoadPrimitives(classPath);
    
   return result;
}


pVMClass Universe::LoadClassBasic( pVMSymbol name, pVMClass systemClass) {
    StdString s_name = name->GetStdString();
    //cout << s_name.c_str() << endl;
    pVMClass result;

    for (vector<StdString>::iterator i = classPath.begin();
         i != classPath.end(); ++i) {
        result = compiler->CompileClass(*i, name->GetStdString(), systemClass);
        if (result) {
            if (dumpBytecodes) {
                Disassembler::Dump(result->GetClass());
                Disassembler::Dump(result);
            }
            return result;
        }

    }
    return NULL;
}


pVMClass Universe::LoadShellClass( StdString& stmt) {
    pVMClass result = compiler->CompileClassString(stmt, NULL);
     if(dumpBytecodes)
         Disassembler::Dump(result);
    return result;
}


void Universe::LoadSystemClass( pVMClass systemClass) {
    pVMClass result =
        LoadClassBasic(systemClass->GetName(), systemClass);
    StdString s = systemClass->GetName()->GetStdString();

    if (!result) {
        cout << "Can\'t load system class: " << s;
        Universe::Quit(ERR_FAIL);
    }

    if (result->HasPrimitives() || result->GetClass()->HasPrimitives())
        result->LoadPrimitives(classPath);
}


pVMArray Universe::NewArray( int size) const {
    int additionalBytes = size*sizeof(pVMObject);
#if GC_TYPE==GENERATIONAL
	//if the array is too big for the nursery, we will directly allocate a
	// mature object
	bool outsideNursery = 
		additionalBytes+sizeof(VMArray) > _HEAP->GetMaxNurseryObjectSize();

    pVMArray result = new (_HEAP, additionalBytes, outsideNursery) VMArray(size);
	if (outsideNursery)
		result->SetGCField(MASK_OBJECT_IS_OLD);
#else
    pVMArray result = new (_HEAP, additionalBytes) VMArray(size);
#endif
    result->SetClass(arrayClass);
    return result;
}


pVMArray Universe::NewArrayFromArgv( const vector<StdString>& argv) const {
    pVMArray result = NewArray(argv.size());
    int j = 0;
    for (vector<StdString>::const_iterator i = argv.begin();
         i != argv.end(); ++i) {
        result->SetIndexableField(j, NewString(*i));
        ++j;
    }

    return result;
}


pVMArray Universe::NewArrayList(ExtendedList<pVMObject>& list ) const {
    int size = list.Size();
    pVMArray result = NewArray(size);

    if (result)  {
        for (int i = 0; i < size; ++i) {
            pVMObject elem = list.Get(i);
            result->SetIndexableField(i, elem);
        }
    }
    return result;
}


pVMBigInteger Universe::NewBigInteger( int64_t value) const {
    return new (_HEAP) VMBigInteger(value);
}


pVMBlock Universe::NewBlock( pVMMethod method, pVMFrame context, int arguments) {
    pVMBlock result = new (_HEAP) VMBlock;
    result->SetClass(this->GetBlockClassWithArgs(arguments));

    result->SetMethod(method);
    result->SetContext(context);

    return result;
}


pVMClass Universe::NewClass( pVMClass classOfClass) const {
    int numFields = classOfClass->GetNumberOfInstanceFields();
    pVMClass result;
    int additionalBytes = numFields * sizeof(pVMObject);
    if (numFields) result = new (_HEAP, additionalBytes) VMClass(numFields);
    else result = new (_HEAP) VMClass;

    result->SetClass(classOfClass);

    return result;
}


pVMDouble Universe::NewDouble( double value) const {
    return new (_HEAP) VMDouble(value);
}


pVMFrame Universe::NewFrame( pVMFrame previousFrame, pVMMethod method) const {
    int length = method->GetNumberOfArguments() +
                 method->GetNumberOfLocals()+
                 method->GetMaximumNumberOfStackElements(); 
   
    int additionalBytes = length * sizeof(pVMObject);
    pVMFrame result = new (_HEAP, additionalBytes) VMFrame(length);
    result->SetClass(frameClass);

    result->SetMethod(method);

    if (previousFrame != NULL) 
        result->SetPreviousFrame(previousFrame);

    result->ResetStackPointer();
    result->SetBytecodeIndex(0);

    return result;
}


pVMObject Universe::NewInstance( pVMClass  classOfInstance) const {
    //the number of fields for allocation. We have to calculate the clazz
    //field out of this, because it is already taken care of by VMObject
    int numOfFields = classOfInstance->GetNumberOfInstanceFields() - 1;
    //the additional space needed is calculated from the number of fields
    int additionalBytes = numOfFields * sizeof(pVMObject);
    pVMObject result = new (_HEAP, additionalBytes) VMObject(numOfFields);
    result->SetClass(classOfInstance);
    return result;
}

#ifdef USE_TAGGING
VMPointer<VMInteger> Universe::NewInteger( int32_t value) const {
#else
  pVMInteger Universe::NewInteger( int32_t value) const {
#endif

#ifdef CACHE_INTEGER
    uint32_t index = (uint32_t)value - (uint32_t)INT_CACHE_MIN_VALUE;
    if (index < (uint32_t)(INT_CACHE_MAX_VALUE - INT_CACHE_MIN_VALUE)) {
      return prebuildInts[index];
    }
#endif
    return new (_HEAP) VMInteger(value);
  }

pVMClass Universe::NewMetaclassClass() const {
    pVMClass result = new (_HEAP) VMClass;
    result->SetClass(new (_HEAP) VMClass);

    pVMClass mclass = result->GetClass();
    mclass->SetClass(result);

    return result;
}

#ifdef USE_TAGGING
void Universe::WalkGlobals(AbstractVMObject* (*walk)(AbstractVMObject*)) {
#else
void Universe::WalkGlobals(pVMObject (*walk)(pVMObject)) {
#endif
	nilObject = walk(nilObject);
	trueObject = walk(trueObject);
	falseObject = walk(falseObject);

#ifdef USE_TAGGING
	GlobalBox::updateIntegerBox(dynamic_cast<VMInteger*>(walk(GlobalBox::IntegerBox())));

	objectClass = DynamicConvert<VMClass, AbstractVMObject>(walk(objectClass));
	classClass = DynamicConvert<VMClass,AbstractVMObject>(walk(classClass));
	metaClassClass =  DynamicConvert<VMClass,AbstractVMObject>(walk(metaClassClass));

	nilClass = DynamicConvert<VMClass,AbstractVMObject>(walk(nilClass));
	integerClass = DynamicConvert<VMClass,AbstractVMObject>(walk(integerClass));
	bigIntegerClass = DynamicConvert<VMClass,AbstractVMObject>(walk(bigIntegerClass));
	arrayClass = DynamicConvert<VMClass,AbstractVMObject>(walk(arrayClass));
	methodClass = DynamicConvert<VMClass,AbstractVMObject>(walk(methodClass));
	symbolClass = DynamicConvert<VMClass,AbstractVMObject>(walk(symbolClass));
	frameClass = DynamicConvert<VMClass,AbstractVMObject>(walk(frameClass));
	primitiveClass = DynamicConvert<VMClass,AbstractVMObject>(walk(primitiveClass));
	stringClass = DynamicConvert<VMClass,AbstractVMObject>(walk(stringClass));
	systemClass = DynamicConvert<VMClass,AbstractVMObject>(walk(systemClass));
	blockClass = DynamicConvert<VMClass,AbstractVMObject>(walk(blockClass));
	doubleClass = DynamicConvert<VMClass,AbstractVMObject>(walk(doubleClass));
#else

	objectClass = (pVMClass)(walk(objectClass));
	classClass = (pVMClass)(walk(classClass));
	metaClassClass =  (pVMClass)(walk(metaClassClass));

	nilClass = (pVMClass)(walk(nilClass));
	integerClass = (pVMClass)(walk(integerClass));
	bigIntegerClass = (pVMClass)(walk(bigIntegerClass));
	arrayClass = (pVMClass)(walk(arrayClass));
	methodClass = (pVMClass)(walk(methodClass));
	symbolClass = (pVMClass)(walk(symbolClass));
	frameClass = (pVMClass)(walk(frameClass));
	primitiveClass = (pVMClass)(walk(primitiveClass));
	stringClass = (pVMClass)(walk(stringClass));
	systemClass = (pVMClass)(walk(systemClass));
	blockClass = (pVMClass)(walk(blockClass));
	doubleClass = (pVMClass)(walk(doubleClass));
#endif

#ifdef CACHE_INTEGER
  for (int32_t i = 0; i < (INT_CACHE_MAX_VALUE - INT_CACHE_MIN_VALUE); i++)
#ifdef USE_TAGGING
    prebuildInts[i] = INT_CACHE_MIN_VALUE + i;
#else
    prebuildInts[i] = (pVMInteger)walk(prebuildInts[i]);
#endif
#endif

	//walk all entries in globals map
	map<pVMSymbol, pVMObject> globs = globals;
	globals.clear();
	map<pVMSymbol, pVMObject>::iterator iter;
	for (iter = globs.begin(); iter != globs.end(); iter++) {
		if (iter->second == NULL)
			continue;

#ifdef USE_TAGGING
		pVMSymbol key =
			DynamicConvert<VMSymbol,AbstractVMObject>(walk(iter->first.GetPointer()));
#else
		pVMSymbol key = (pVMSymbol)(walk(iter->first));
#endif
		pVMObject val = walk(iter->second);
		globals[key] = val;
	}
	//walk all entries in symbols map
	map<StdString, pVMSymbol>::iterator symbolIter;
	for (symbolIter = symbolsMap.begin(); symbolIter !=
			symbolsMap.end(); symbolIter++) {
		//insert overwrites old entries inside the internal map
#ifdef USE_TAGGING
		symbolIter->second = DynamicConvert<VMSymbol,AbstractVMObject>(walk(symbolIter->second));
#else
		symbolIter->second = (pVMSymbol)walk(symbolIter->second);
#endif
	}

	map<int, pVMClass>::iterator bcIter;
	for (bcIter = blockClassesByNoOfArgs.begin(); bcIter !=
			blockClassesByNoOfArgs.end(); bcIter++) {
#ifdef USE_TAGGING
		bcIter->second = DynamicConvert<VMClass, AbstractVMObject>(walk(bcIter->second));
#else
		bcIter->second = (pVMClass)walk(bcIter->second);
#endif
	}
}


pVMMethod Universe::NewMethod( pVMSymbol signature, 
                    size_t numberOfBytecodes, size_t numberOfConstants) const {
    //Method needs space for the bytecodes and the pointers to the constants
    int additionalBytes = numberOfBytecodes + 
                numberOfConstants*sizeof(pVMObject);
    pVMMethod result = new (_HEAP,additionalBytes) 
                VMMethod(numberOfBytecodes, numberOfConstants);
    result->SetClass(methodClass);

    result->SetSignature(signature);

    return result;
}

pVMString Universe::NewString( const StdString& str) const {
    return NewString(str.c_str());
}

pVMString Universe::NewString( const char* str) const {
    pVMString result = new (_HEAP, strlen(str) + 1) VMString(str);
    return result;
}

pVMSymbol Universe::NewSymbol( const StdString& str) {
    return NewSymbol(str.c_str());
}

pVMSymbol Universe::NewSymbol( const char* str ) {
    pVMSymbol result = new (_HEAP, strlen(str)+1) VMSymbol(str);
	symbolsMap[str] = result;
    return result;
}


pVMClass Universe::NewSystemClass() const {
    pVMClass systemClass = new (_HEAP) VMClass();

    systemClass->SetClass(new (_HEAP) VMClass());
    pVMClass mclass = systemClass->GetClass();
    
    mclass->SetClass(metaClassClass);

    return systemClass;
}


pVMSymbol Universe::SymbolFor( const StdString& str) {
	map<string,pVMSymbol>::iterator it = symbolsMap.find(str);
    return (it == symbolsMap.end()) ? NewSymbol(str) : it->second;
}


pVMSymbol Universe::SymbolForChars( const char* str) {
	return SymbolFor(str);
}


void Universe::SetGlobal(pVMSymbol name, pVMObject val) {
    globals[name] = val;
}
