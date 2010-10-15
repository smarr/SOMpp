#pragma once
#ifndef UNIVERSE_H_
#define UNIVERSE_H_

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

#include "../misc/defs.h"
#include "../misc/ExtendedList.h"

#include "../vmobjects/ObjectFormats.h"

#include "../interpreter/Interpreter.h"

#include "../memory/Heap.h"


class VMObject;
class VMSymbol;
class VMClass;
class VMFrame;
class VMArray;
class VMBlock;
class VMDouble;
class VMInteger;
class VMMethod;
class VMString;
class VMBigInteger;
class Symboltable;
class SourcecodeCompiler;

//Convenience macro for Singleton access
#define _UNIVERSE Universe::GetUniverse()

// for runtime debug
extern short dumpBytecodes;
extern short gcVerbosity;

//global VMObjects
extern pVMObject nilObject;
extern pVMObject trueObject;
extern pVMObject falseObject;
      
extern pVMClass objectClass;
extern pVMClass classClass;
extern pVMClass metaClassClass;
  
extern pVMClass nilClass;
extern pVMClass integerClass;
extern pVMClass bigIntegerClass;
extern pVMClass arrayClass;
extern pVMClass methodClass;
extern pVMClass symbolClass;
extern pVMClass frameClass;
extern pVMClass primitiveClass;
extern pVMClass stringClass;
extern pVMClass systemClass;
extern pVMClass blockClass;
extern pVMClass doubleClass;

using namespace std;
class Universe
{
public:

    Universe* operator->();

    //static methods
	static Universe* GetUniverse();
    static void Start(int argc, char** argv);
    static void Quit(int);
    static void ErrorExit(const char*);

    //Globals accessor (only for GC, could be considered be 
    //declared as a private friend method for the GC)
	map<pVMSymbol, pVMObject>  GetGlobals() {return globals;}
	Heap* GetHeap() {return heap;}
    Interpreter* GetInterpreter() {return interpreter;}

    //

    void          Assert(bool) const;

    pVMSymbol     SymbolFor(const StdString&);
    pVMSymbol     SymbolForChars(const char*);

    //VMObject instanciation methods. These should probably be refactored to a new class
    pVMArray      NewArray(int) const;
    pVMArray      NewArrayList(ExtendedList<pVMObject>& list) const;
    pVMArray      NewArrayFromArgv(const vector<StdString>&) const;
    pVMBlock      NewBlock(pVMMethod, pVMFrame, int);
    pVMClass      NewClass(pVMClass) const;
    pVMFrame      NewFrame(pVMFrame, pVMMethod) const;
    pVMMethod     NewMethod(pVMSymbol, size_t, size_t) const;
    pVMObject     NewInstance(pVMClass) const;
    pVMInteger    NewInteger(int32_t) const;
    pVMBigInteger NewBigInteger(int64_t) const;
    pVMDouble     NewDouble(double) const;
    pVMClass      NewMetaclassClass(void) const;
    pVMString     NewString(const StdString&) const;
    pVMSymbol     NewSymbol(const StdString&);
    pVMString     NewString(const char*) const;
    pVMSymbol     NewSymbol(const char*);
    pVMClass      NewSystemClass(void) const;

    void          InitializeSystemClass(pVMClass, pVMClass, const char*);

    pVMObject     GetGlobal(pVMSymbol);
    void          SetGlobal(pVMSymbol name, pVMObject val);
    bool          HasGlobal(pVMSymbol);
    void          InitializeGlobals();
    pVMClass      GetBlockClass(void) const;
    pVMClass      GetBlockClassWithArgs(int);

    pVMClass      LoadClass(pVMSymbol);
    void          LoadSystemClass(pVMClass);
    pVMClass      LoadClassBasic(pVMSymbol, pVMClass);
    pVMClass      LoadShellClass(StdString&);
    
    void          FullGC();
    
    Universe();
	~Universe();
    //
private:
    vector<StdString>  handleArguments(int argc, char** argv) ;
    int getClassPathExt(vector<StdString>& tokens, const StdString& arg) const;
    
    static Universe* theUniverse;
    
    int setupClassPath(const StdString& cp);
    int addClassPath(const StdString& cp);
    void printUsageAndExit(char* executable) const;
	

    void initialize(int, char**);

	Heap* heap;
    int heapSize;
	map<pVMSymbol, pVMObject> globals;
    vector<StdString> classPath;
    
    Symboltable* symboltable;
    SourcecodeCompiler* compiler;
    Interpreter* interpreter;
};


#endif
