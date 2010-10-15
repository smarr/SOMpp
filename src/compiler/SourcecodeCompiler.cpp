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
#include <fstream>

#include "SourcecodeCompiler.h"
#include "ClassGenerationContext.h"
#include "Parser.h"

#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMSymbol.h"

//#define COMPILER_DEBUG


SourcecodeCompiler::SourcecodeCompiler() {
    parser = NULL;
}


SourcecodeCompiler::~SourcecodeCompiler() {
    if (parser != NULL) delete(parser);
}


pVMClass SourcecodeCompiler::CompileClass( const StdString& path, 
                                          const StdString& file, 
                                          pVMClass systemClass ) {
    pVMClass result = systemClass;

    StdString fname = path + fileSeparator + file + ".som";

    ifstream* fp = new ifstream();
    fp->open(fname.c_str(), std::ios_base::in);
	if (!fp->is_open()) {
		return NULL;
	}

    if (parser != NULL) delete(parser);
    parser = new Parser(*fp);
    result = compile(systemClass);

    pVMSymbol cname = result->GetName();
    StdString cnameC = cname->GetStdString();

    if (file != cnameC) {
        
        ostringstream Str;
        Str << "Filename: " << file << " does not match class name " << cnameC;

        showCompilationError(file, Str.str().c_str());
        return NULL;
    }
    delete(parser);
    parser = NULL;
    delete(fp);
#ifdef COMPILER_DEBUG
    std::cout << "Compilation finished" << endl;
#endif
    return result;
}


pVMClass SourcecodeCompiler::CompileClassString( const StdString& stream, 
                                                pVMClass systemClass ) {
    istringstream* ss = new istringstream(stream);
    if (parser != NULL) delete(parser);
    parser = new Parser(*ss);
    
    pVMClass result = compile(systemClass);
    delete(parser);
    parser = NULL;
    delete(ss);

    return result;
}


void SourcecodeCompiler::showCompilationError( const StdString& filename, 
                                              const char* message ) {
    cout << "Error when compiling " << filename << ":" << endl;
    cout << message << endl;
}


pVMClass SourcecodeCompiler::compile( pVMClass systemClass ) {
    if (parser == NULL) {
        cout << "Parser not initiated" << endl;
        _UNIVERSE->ErrorExit("Compiler error");
    }
    ClassGenerationContext* cgc = new ClassGenerationContext();

    pVMClass result = systemClass;

    parser->Classdef(cgc);

    if (systemClass == NULL) result = cgc->Assemble();
    else cgc->AssembleSystemClass(result);

    delete(cgc);

    
    return result;
}

