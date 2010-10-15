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


#include "Parser.h"
#include "BytecodeGenerator.h"

#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMSymbol.h"

#include "../vm/Universe.h"

#include <iostream>
#include <cctype>
#include <sstream>
#include <stdlib.h>
#include <string.h>


#define GETSYM sym = lexer->GetSym(); \
			   text = lexer->GetText()

#define PEEK nextSym = lexer->Peek(); \
			 nextText = lexer->GetNextText()

Parser::Parser(istream& file) {
    sym = NONE;
    lexer = new Lexer(file);
    bcGen = new BytecodeGenerator();
    nextSym = NONE;

    GETSYM;
}

Parser::~Parser() {
    delete(lexer);
    delete(bcGen);
}

//
// parsing
//


bool Parser::symIn(Symbol* ss) {
    while(*ss)
        if(*ss++ == sym)
            return true;
    return false;
}


bool Parser::accept(Symbol s) {
    if(sym == s) {
        GETSYM;
        return true;
    }
    return false;
}


bool Parser::acceptOneOf(Symbol* ss) {
    if(symIn(ss)) {
        GETSYM;
        return true;
    }
    return false;
}


#define _PRINTABLE_SYM (sym == Integer || sym >= STString)


bool Parser::expect(Symbol s) {
    if(accept(s))
        return true;
    fprintf(stderr, "Error: unexpected symbol in line %d. Expected %s, but found %s", 
            lexer->GetCurrentLineNumber(), symnames[s], symnames[sym]);
    if(_PRINTABLE_SYM)
        fprintf(stderr, " (%s)", text.c_str());
	fprintf(stderr, ": %s\n", lexer->GetRawBuffer().c_str());
    return false;
}


bool Parser::expectOneOf(Symbol* ss) {
    if(acceptOneOf(ss))
        return true;
    fprintf(stderr, "Error: unexpected symbol in line %d. Expected one of ",
            lexer->GetCurrentLineNumber());
    while(*ss)
        fprintf(stderr, "%s, ", symnames[*ss++]);
    fprintf(stderr, "but found %s", symnames[sym]);
    if(_PRINTABLE_SYM)
        fprintf(stderr, " (%s)", text.c_str());
	fprintf(stderr, ": %s\n", lexer->GetRawBuffer().c_str());
    return false;
}




void Parser::genPushVariable(MethodGenerationContext* mgenc, const StdString& var) {
    // The purpose of this function is to find out whether the variable to be
    // pushed on the stack is a local variable, argument, or object field. This
    // is done by examining all available lexical contexts, starting with the
    // innermost (i.e., the one represented by mgenc).
    int index = 0;
    int context = 0;
    bool is_argument = false;
    
    if(mgenc->FindVar(var, &index, &context, &is_argument)) {
		if(is_argument) 
            bcGen->EmitPUSHARGUMENT(mgenc, index, context);
        else 
            bcGen->EmitPUSHLOCAL(mgenc, index, context);
    } else if(mgenc->FindField(var)) {
        pVMSymbol fieldName = _UNIVERSE->SymbolFor(var);
		mgenc->AddLiteralIfAbsent((pVMObject)fieldName);
        bcGen->EmitPUSHFIELD(mgenc, fieldName);
    } else {
        
        pVMSymbol global = _UNIVERSE->SymbolFor(var);
		mgenc->AddLiteralIfAbsent((pVMObject)global);
        
        bcGen->EmitPUSHGLOBAL(mgenc, global);
    }
}


void Parser::genPopVariable(MethodGenerationContext* mgenc, const StdString& var){
    // The purpose of this function is to find out whether the variable to be
    // popped off the stack is a local variable, argument, or object field. This
    // is done by examining all available lexical contexts, starting with the
    // innermost (i.e., the one represented by mgenc).
    int index = 0;
    int context = 0;
    bool is_argument = false;
	
    if(mgenc->FindVar(var, &index, &context, &is_argument)) {
        if(is_argument) bcGen->EmitPOPARGUMENT(mgenc, index, context);
        else bcGen->EmitPOPLOCAL(mgenc, index, context);
    } else bcGen->EmitPOPFIELD(mgenc, _UNIVERSE->SymbolFor(var));
}


//
// grammar
//


Symbol singleOpSyms[] = {
    Not, And, Or, Star, Div, Mod, Plus, Equal, More, Less, Comma, At, Per, NONE
};


Symbol binaryOpSyms[] = {
    Or, Comma, Minus, Equal, Not, And, Or, Star, Div, Mod, Plus, Equal,
    More, Less, Comma, At, Per, NONE
};


Symbol keywordSelectorSyms[] = { Keyword, KeywordSequence };




void Parser::Classdef(ClassGenerationContext* cgenc) {
	cgenc->SetName(_UNIVERSE->SymbolFor(text));
    expect(Identifier);
    
    expect(Equal);
    
    if(sym == Identifier) {
		cgenc->SetSuperName(_UNIVERSE->SymbolFor(text));
        accept(Identifier);
    } else cgenc->SetSuperName(_UNIVERSE->SymbolFor("Object"));
    
    expect(NewTerm);
    instanceFields(cgenc);
    while(sym == Identifier || sym == Keyword || sym == OperatorSequence ||
        symIn(binaryOpSyms)) {
		
        MethodGenerationContext* mgenc = new MethodGenerationContext();
		mgenc->SetHolder(cgenc);
		mgenc->AddArgument("self");
    
        method(mgenc);
        
		if(mgenc->IsPrimitive())
            cgenc->AddInstanceMethod((pVMObject)(mgenc->AssemblePrimitive()));
        else
			cgenc->AddInstanceMethod((pVMObject)(mgenc->Assemble()));
        delete(mgenc);
    }
    
    if(accept(Separator)) {
        cgenc->SetClassSide(true);
        classFields(cgenc);
        while(sym == Identifier || sym == Keyword || sym == OperatorSequence ||
                symIn(binaryOpSyms)) {
            MethodGenerationContext* mgenc = new MethodGenerationContext();
			mgenc->SetHolder(cgenc);
			mgenc->AddArgument("self");
            
            method(mgenc);
            
			if(mgenc->IsPrimitive())
                cgenc->AddClassMethod((pVMObject)mgenc->AssemblePrimitive());
            else
				cgenc->AddClassMethod((pVMObject)mgenc->Assemble());
            delete(mgenc);
        }    
    }
    expect(EndTerm);
}


void Parser::instanceFields(ClassGenerationContext* cgenc) {
    if(accept(Or)) {
        while(sym == Identifier) {
            StdString var = variable();
            cgenc->AddInstanceField((pVMObject)_UNIVERSE->SymbolFor(var));
        }
        expect(Or);
    }
}


void Parser::classFields(ClassGenerationContext* cgenc) {
    if(accept(Or)) {
        while(sym == Identifier) {
            StdString var = variable();
			cgenc->AddClassField((pVMObject)_UNIVERSE->SymbolFor(var));
        }
        expect(Or);
    }
}


void Parser::method(MethodGenerationContext* mgenc) {
    pattern(mgenc);
    
    expect(Equal);
    if(sym == Primitive) {
		mgenc->SetPrimitive(true);
        primitiveBlock();
    } else
        methodBlock(mgenc);
}


void Parser::primitiveBlock(void) {
    expect(Primitive);
}


void Parser::pattern(MethodGenerationContext* mgenc) {
    switch(sym) {
        case Identifier: 
            unaryPattern(mgenc);
            break;
        case Keyword: 
            keywordPattern(mgenc);
            break;
        default: 
            binaryPattern(mgenc);
            break;
    }
}


void Parser::unaryPattern(MethodGenerationContext* mgenc) {
    mgenc->SetSignature(unarySelector());
}


void Parser::binaryPattern(MethodGenerationContext* mgenc) {
    mgenc->SetSignature(binarySelector());
	mgenc->AddArgumentIfAbsent(argument());
}


void Parser::keywordPattern(MethodGenerationContext* mgenc) {
    StdString kw;
    do {
        kw.append(keyword());
		mgenc->AddArgumentIfAbsent(argument());
    } while(sym == Keyword);
    
    mgenc->SetSignature(_UNIVERSE->SymbolFor(kw));
}


void Parser::methodBlock(MethodGenerationContext* mgenc){
    expect(NewTerm);
    blockContents(mgenc);
    // if no return has been generated so far, we can be sure there was no .
    // terminating the last expression, so the last expression's value must be
    // popped off the stack and a ^self be generated
    if(!mgenc->IsFinished()) {
        bcGen->EmitPOP(mgenc);
        bcGen->EmitPUSHARGUMENT(mgenc, 0, 0);
        bcGen->EmitRETURNLOCAL(mgenc);
        mgenc->SetFinished();
    }
    
    expect(EndTerm);
}


pVMSymbol Parser::unarySelector(void) {
    return _UNIVERSE->SymbolFor(identifier());
}


pVMSymbol Parser::binarySelector(void) {
    StdString s(text);
    
    if(accept(Or))
        ;
    else if(accept(Comma))
        ;
    else if(accept(Minus))
        ;
    else if(accept(Equal))
        ;
    else if(acceptOneOf(singleOpSyms))
        ;
    else if(accept(OperatorSequence))
        ;
    else
        expect(NONE);
    
    pVMSymbol symb = _UNIVERSE->SymbolFor(s);
    return symb;
}


StdString Parser::identifier(void) {
    StdString s(text);
    if(accept(Primitive))
        ; // text is set
    else
        expect(Identifier);
    
    return s;
}


StdString Parser::keyword(void) {
    StdString s(text);
    expect(Keyword);
    
    return s;
}


StdString Parser::argument(void) {
    return variable();
}


void Parser::blockContents(MethodGenerationContext* mgenc) {
    if(accept(Or)) {
        locals(mgenc);
        expect(Or);
    }
    blockBody(mgenc, false);
}


void Parser::locals(MethodGenerationContext* mgenc)
{
    while(sym == Identifier) 
		mgenc->AddLocalIfAbsent(variable());
}


void Parser::blockBody(MethodGenerationContext* mgenc, bool seen_period) {
    if(accept(Exit))
        result(mgenc);
    else if(sym == EndBlock) {
		if(seen_period) {
            // a POP has been generated which must be elided (blocks always
            // return the value of the last expression, regardless of whether it
            // was terminated with a . or not)
            mgenc->RemoveLastBytecode();
		}
        bcGen->EmitRETURNLOCAL(mgenc);
		
		mgenc->SetFinished();
    } else if(sym == EndTerm) {
        // it does not matter whether a period has been seen, as the end of the
        // method has been found (EndTerm) - so it is safe to emit a "return
        // self"
        bcGen->EmitPUSHARGUMENT(mgenc, 0, 0);
		bcGen->EmitRETURNLOCAL(mgenc);
		mgenc->SetFinished();
    } else {
        expression(mgenc);
        if(accept(Period)) {
            bcGen->EmitPOP(mgenc);
            blockBody(mgenc, true);
        }
    }
}


void Parser::result(MethodGenerationContext* mgenc) {
    expression(mgenc);
	
	if(mgenc->IsBlockMethod()) bcGen->EmitRETURNNONLOCAL(mgenc);
	else bcGen->EmitRETURNLOCAL(mgenc);
    
    mgenc->SetFinished(true);
	accept(Period);
}


void Parser::expression(MethodGenerationContext* mgenc) {
    PEEK;
    if(nextSym == Assign)
        assignation(mgenc);
    else
        evaluation(mgenc);
}


void Parser::assignation(MethodGenerationContext* mgenc) {
	list<StdString> l;
    
    assignments(mgenc, l);
    evaluation(mgenc);
    list<StdString>::iterator i;
    for(i=l.begin(); i != l.end(); ++i)
        bcGen->EmitDUP(mgenc);
    for(i=l.begin(); i != l.end(); ++i)
        genPopVariable(mgenc, (*i));
    
}


void Parser::assignments(MethodGenerationContext* mgenc, list<StdString>& l) {
    if(sym == Identifier) {
        l.push_back(assignment(mgenc));
        PEEK;
        if(nextSym == Assign)
            assignments(mgenc, l);
    }
}


StdString Parser::assignment(MethodGenerationContext* mgenc) {
    StdString v = variable();
    pVMSymbol var = _UNIVERSE->SymbolFor(v);
	mgenc->AddLiteralIfAbsent((pVMObject)var);
    
    expect(Assign);
    
    return v;
}


void Parser::evaluation(MethodGenerationContext* mgenc) {
    bool super;
    primary(mgenc, &super);
    if(sym == Identifier || sym == Keyword || sym == OperatorSequence ||
        symIn(binaryOpSyms)) {       
        messages(mgenc, super);
    }
}


void Parser::primary(MethodGenerationContext* mgenc, bool* super) {
    *super = false;
    switch(sym) {
        case Identifier: {
            StdString v = variable();
			if(v == "super") { 
                *super = true;
                // sends to super push self as the receiver
                v = StdString("self");
            }
            
            genPushVariable(mgenc, v);
            break;
        }
        case NewTerm:
            nestedTerm(mgenc);
            break;
        case NewBlock: {
            MethodGenerationContext* bgenc = new MethodGenerationContext();
			bgenc->SetIsBlockMethod(true);
			bgenc->SetHolder(mgenc->GetHolder());
			bgenc->SetOuter(mgenc);
            
            nestedBlock(bgenc);
            
            pVMMethod block_method = bgenc->Assemble();
			mgenc->AddLiteral(block_method);
            bcGen->EmitPUSHBLOCK(mgenc, block_method);
			delete(bgenc);
            break;
        }
        default:
            literal(mgenc);
            break;
    }
}


StdString Parser::variable(void) {
    return identifier();
}


void Parser::messages(MethodGenerationContext* mgenc, bool super) {
    if(sym == Identifier) {
        do {
            // only the first message in a sequence can be a super send
            unaryMessage(mgenc, super);
            super = false;
        } while(sym == Identifier);
        
        while(sym == OperatorSequence || symIn(binaryOpSyms)) {
            binaryMessage(mgenc, false);
        }
        
        if(sym == Keyword) {
            keywordMessage(mgenc, false);
        }
    } else if(sym == OperatorSequence || symIn(binaryOpSyms)) {
        do {
            // only the first message in a sequence can be a super send
            binaryMessage(mgenc, super);
            super = false;
        } while(sym == OperatorSequence || symIn(binaryOpSyms));
        
        if(sym == Keyword) {
            keywordMessage(mgenc, false);
        }
    } else
        keywordMessage(mgenc, super);
}


void Parser::unaryMessage(MethodGenerationContext* mgenc, bool super) {
    pVMSymbol msg = unarySelector();
	mgenc->AddLiteralIfAbsent((pVMObject)msg);
    
    if(super) bcGen->EmitSUPERSEND(mgenc, msg);
    else bcGen->EmitSEND(mgenc, msg);
	
}


void Parser::binaryMessage(MethodGenerationContext* mgenc, bool super) {
    pVMSymbol msg = binarySelector();
	mgenc->AddLiteralIfAbsent((pVMObject)msg);
    
    
    bool tmp_bool = false;
    binaryOperand(mgenc, &tmp_bool);
    
    if(super)
        bcGen->EmitSUPERSEND(mgenc, msg);
    else
        bcGen->EmitSEND(mgenc, msg);
	
}


void Parser::binaryOperand(MethodGenerationContext* mgenc, bool* super) {
    primary(mgenc, super);
    
    while(sym == Identifier)
        unaryMessage(mgenc, *super);
}


void Parser::keywordMessage(MethodGenerationContext* mgenc, bool super) {
    StdString kw;
    do {
        kw.append(keyword());
        formula(mgenc);
    } while(sym == Keyword);
    
    pVMSymbol msg = _UNIVERSE->SymbolFor(kw);
    
	mgenc->AddLiteralIfAbsent((pVMObject)msg);
    
    
    if(super) bcGen->EmitSUPERSEND(mgenc, msg);
    else bcGen->EmitSEND(mgenc, msg);
	
}


void Parser::formula(MethodGenerationContext* mgenc) {
    bool super;
    binaryOperand(mgenc, &super);
    
    // only the first message in a sequence can be a super send
    if(sym == OperatorSequence || symIn(binaryOpSyms))
        binaryMessage(mgenc, super);
    while(sym == OperatorSequence || symIn(binaryOpSyms))
        binaryMessage(mgenc, false);
}


void Parser::nestedTerm(MethodGenerationContext* mgenc) {
    expect(NewTerm);
    expression(mgenc);
    expect(EndTerm);
}


void Parser::literal(MethodGenerationContext* mgenc) {
    switch(sym) {
        case Pound: 
            literalSymbol(mgenc);
            break;
        case STString: 
            literalString(mgenc);
            break;
        default: 
            literalNumber(mgenc);
            break;
    }
}


void Parser::literalNumber(MethodGenerationContext* mgenc) {
    int32_t val;
    if(sym == Minus)
        val = negativeDecimal();
    else
        val = literalDecimal();
    
    pVMInteger lit = _UNIVERSE->NewInteger(val);
	mgenc->AddLiteralIfAbsent((pVMObject)lit);
    bcGen->EmitPUSHCONSTANT(mgenc, (pVMObject)lit);
}


uint32_t Parser::literalDecimal(void) {
    return literalInteger();
}


int32_t Parser::negativeDecimal(void) {
    expect(Minus);
    return  -((int32_t)literalInteger());
}


uint32_t Parser::literalInteger(void) {
    uint32_t i = (uint32_t) strtoul(text.c_str(), NULL, 10);
    expect(Integer);
    return i;
}


void Parser::literalSymbol(MethodGenerationContext* mgenc) {
    pVMSymbol symb;
    expect(Pound);
    if(sym == STString) {
        StdString s = _string();
        symb = _UNIVERSE->SymbolFor(s);
        
    } else
        symb = selector();
	mgenc->AddLiteralIfAbsent((pVMObject)symb);
    
    bcGen->EmitPUSHCONSTANT(mgenc, (pVMObject)symb);
}


void Parser::literalString(MethodGenerationContext* mgenc) {
    StdString s = _string();
	
    pVMString str = _UNIVERSE->NewString(s);
    mgenc->AddLiteralIfAbsent((pVMObject)str);
    
    bcGen->EmitPUSHCONSTANT(mgenc,(pVMObject)str);
    
}


pVMSymbol Parser::selector(void) {
    if(sym == OperatorSequence || symIn(singleOpSyms))
        return binarySelector();
    else if(sym == Keyword || sym == KeywordSequence)
        return keywordSelector();
    else
        return unarySelector();
}


pVMSymbol Parser::keywordSelector(void) {
    StdString s(text);
    expectOneOf(keywordSelectorSyms);
    pVMSymbol symb = _UNIVERSE->SymbolFor(s);
    return symb;
}


StdString Parser::_string(void) {
    StdString s(text); 
    expect(STString);    
    return s; // <-- Literal strings are At Most BUFSIZ chars long.
}


void Parser::nestedBlock(MethodGenerationContext* mgenc) {
    #define BLOCK_METHOD_S "$block method"
    #define BLOCK_METHOD_LEN (13)
	mgenc->AddArgumentIfAbsent("$block self");
    
    expect(NewBlock);
    if(sym == Colon)
        blockPattern(mgenc);
    
    // generate Block signature
    StdString block_sig = StdString(BLOCK_METHOD_S);
	size_t arg_size = mgenc->GetNumberOfArguments();
    for(size_t i = 1; i < arg_size; i++)
		block_sig += ":";

    mgenc->SetSignature(_UNIVERSE->SymbolFor(block_sig));
    
    blockContents(mgenc);
    
    // if no return has been generated, we can be sure that the last expression
    // in the block was not terminated by ., and can generate a return
    if(!mgenc->IsFinished()) {
		bcGen->EmitRETURNLOCAL(mgenc);
        mgenc->SetFinished(true);
    }
    
    expect(EndBlock);
}


void Parser::blockPattern(MethodGenerationContext* mgenc) {
    blockArguments(mgenc);
    expect(Or);
}


void Parser::blockArguments(MethodGenerationContext* mgenc) {
    do {
        expect(Colon);
		mgenc->AddArgumentIfAbsent(argument());
        
    } while(sym == Colon);
}


