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
#include "../vmobjects/Symboltable.h"

#include "../interpreter/bytecodes.h"

#include "../compiler/Disassembler.h"
#include "../compiler/SourcecodeCompiler.h"

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
        } else if (argv[i][0] == '-' && argv[i][1] == 'H') {
            int heap_size = atoi(argv[i] + 2);
            heapSize = heap_size;
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
    cout << "    -Hx set the _HEAP size to x MB (default: 1 MB)" << endl;
    cout << "    -h  show this help" << endl;

    Quit(ERR_SUCCESS);
}


Universe::Universe(){
	this->compiler = NULL;
	this->symboltable = NULL;
	this->interpreter = NULL;
};


void Universe::initialize(int _argc, char** _argv) {
    heapSize = 1048576;

    vector<StdString> argv = this->handleArguments(_argc, _argv);

    Heap::InitializeHeap(heapSize);
    heap = _HEAP;

    symboltable = new Symboltable();
    compiler = new SourcecodeCompiler();
    interpreter = new Interpreter();
    
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
        dynamic_cast<pVMInvokable>(systemClass->LookupInvokable(this->SymbolForChars("initialize:")));
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
    if (symboltable) 
        delete(symboltable);

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
    this->Assert(numberOfArguments < 10);

    ostringstream Str;
    Str << "Block" << numberOfArguments ;
    StdString blockName(Str.str());
    pVMSymbol name = SymbolFor(blockName);

    if (HasGlobal(name))
        return (pVMClass)GetGlobal(name);

    pVMClass result = LoadClassBasic(name, NULL);

    result->AddInstancePrimitive(new (_HEAP) VMEvaluationPrimitive(numberOfArguments) );

    SetGlobal(name, (pVMObject) result);

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
       return dynamic_cast<pVMClass>(GetGlobal(name));

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
    pVMArray result = new (_HEAP, additionalBytes) VMArray(size);
    result->SetClass(arrayClass);
    return result;
}


pVMArray Universe::NewArrayFromArgv( const vector<StdString>& argv) const {
    pVMArray result = NewArray(argv.size());
    int j = 0;
    for (vector<StdString>::const_iterator i = argv.begin();
         i != argv.end(); ++i) {
        (*result)[j] =  NewString(*i);
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
            
            (*result)[i] = elem;
        }
    }
    return result;
}


pVMBigInteger Universe::NewBigInteger( int64_t value) const {
    pVMBigInteger result = new (_HEAP) VMBigInteger(value);
    result->SetClass(bigIntegerClass);

    return result;
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
    pVMDouble result = new (_HEAP) VMDouble(value);
    result->SetClass(doubleClass);
    return result;
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

pVMInteger Universe::NewInteger( int32_t value) const {
    pVMInteger result = new (_HEAP) VMInteger(value);
    result->SetClass(integerClass);
    return result;
}

pVMClass Universe::NewMetaclassClass() const {
    pVMClass result = new (_HEAP) VMClass;
    result->SetClass(new (_HEAP) VMClass);

    pVMClass mclass = result->GetClass();
    mclass->SetClass(result);

    return result;
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
    //string needs space for str.length characters plus one byte for '\0'
    int additionalBytes = strlen(str) + 1;
    pVMString result = new (_HEAP, additionalBytes) VMString(str);
    result->SetClass(stringClass);

    return result;
}

pVMSymbol Universe::NewSymbol( const StdString& str) {
    return NewSymbol(str.c_str());
}

pVMSymbol Universe::NewSymbol( const char* str ) {
    //symbol needs space for str.length characters plus one byte for '\0'
    int additionalBytes = strlen(str) + 1;
    pVMSymbol result = new (_HEAP, additionalBytes) VMSymbol(str);
    result->SetClass(symbolClass);

    symboltable->insert(result);

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
    return SymbolForChars(str.c_str());
    
}


pVMSymbol Universe::SymbolForChars( const char* str) {
    pVMSymbol result = symboltable->lookup(str);
    
    return (result != NULL) ?
           result :
           NewSymbol(str);
}


void Universe::SetGlobal(pVMSymbol name, VMObject *val) {
    globals[name] = val;
}

void Universe::FullGC() {
    heap->FullGC();
}

