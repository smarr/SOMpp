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

#include <misc/ParseInteger.h>

#include <vmobjects/VMMethod.h>
#include <vmobjects/VMPrimitive.h>
#include <vmobjects/VMObject.h>
#include <vmobjects/VMDouble.h>
#include <vmobjects/VMSymbol.h>
#include <vmobjects/VMClass.h>

#include <vm/Universe.h>

#include <iostream>
#include <cctype>
#include <sstream>
#include <cstring>
#include <string>

#include <assert.h>

void Parser::GetSym() {
    sym  = lexer->GetSym();
    text = lexer->GetText();
}

void Parser::Peek() {
    nextSym = lexer->Peek();
	nextText = lexer->GetNextText();
}

void Parser::PeekForNextSymbolFromLexerIfNecessary() {
    if (!lexer->GetPeekDone()) {
        nextSym = lexer->Peek();
    }
}

Parser::Parser(istream& file, StdString& fname): fname(fname) {
    sym = NONE;
    lexer = new Lexer(file);
    nextSym = NONE;

    GetSym();
}

Parser::~Parser() {
    delete (lexer);
}

//
// parsing
//

bool Parser::symIn(Symbol* ss) {
    while (*ss)
        if (*ss++ == sym)
            return true;
    return false;
}

bool Parser::accept(Symbol s) {
    if (sym == s) {
        GetSym();
        return true;
    }
    return false;
}

bool Parser::acceptOneOf(Symbol* ss) {
    if (symIn(ss)) {
        GetSym();
        return true;
    }
    return false;
}

#define _PRINTABLE_SYM (sym == Integer || sym >= STString)

bool Parser::symIsIdentifier() {
    return sym == Identifier || sym == Primitive;
}

bool Parser::expect(Symbol s) {
    if (accept(s))
        return true;
    
    parseError("Unexpected symbol. Expected %(expected)s, but found %(found)s\n", s);
    return false;
}

bool Parser::expectOneOf(Symbol* ss) {
    if (acceptOneOf(ss))
        return true;
    parseError("Unexpected symbol. Expected one of %(expected)s, but found %(found)s\n", ss);
    
    return false;
}

void Parser::genPushVariable(MethodGenerationContext* mgenc,
        VMSymbol* var) {
    // The purpose of this function is to find out whether the variable to be
    // pushed on the stack is a local variable, argument, or object field. This
    // is done by examining all available lexical contexts, starting with the
    // innermost (i.e., the one represented by mgenc).
    size_t index = 0;
    int context = 0;
    bool is_argument = false;

    if (mgenc->FindVar(var, &index, &context, &is_argument)) {
        if (is_argument) {
            EmitPUSHARGUMENT(mgenc, index, context);
        } else {
            EmitPUSHLOCAL(mgenc, index, context);
        }
    } else {
        if (mgenc->HasField(var)) {
            EmitPUSHFIELD(mgenc, var);
        } else {
            EmitPUSHGLOBAL(mgenc, var);
        }
    }
}

void Parser::genPopVariable(MethodGenerationContext* mgenc, VMSymbol* var) {
    // The purpose of this function is to find out whether the variable to be
    // popped off the stack is a local variable, argument, or object field. This
    // is done by examining all available lexical contexts, starting with the
    // innermost (i.e., the one represented by mgenc).
    size_t index = 0;
    int context = 0;
    bool is_argument = false;

    if (mgenc->FindVar(var, &index, &context, &is_argument)) {
        if (is_argument)
            EmitPOPARGUMENT(mgenc, index, context);
        else
            EmitPOPLOCAL(mgenc, index, context);
    } else
        EmitPOPFIELD(mgenc, var);
    }

//
// grammar
//

Symbol singleOpSyms[] = { Not, And, Or, Star, Div, Mod, Plus, Equal, More, Less,
        Comma, At, Per, Minus, NONE };

Symbol binaryOpSyms[] = { Or, Comma, Minus, Equal, Not, And, Or, Star, Div, Mod,
        Plus, Equal, More, Less, Comma, At, Per, NONE };

Symbol keywordSelectorSyms[] = { Keyword, KeywordSequence };

void Parser::Classdef(ClassGenerationContext* cgenc) {
    cgenc->SetName(GetUniverse()->SymbolFor(text));
    expect(Identifier);

    expect(Equal);

    superclass(cgenc);

    expect(NewTerm);
    instanceFields(cgenc);
    while (symIsIdentifier() || sym == Keyword || sym == OperatorSequence ||
           symIn(binaryOpSyms)) {

        MethodGenerationContext mgenc;
        mgenc.SetHolder(cgenc);
        mgenc.AddArgument("self");

        method(&mgenc);

        if(mgenc.IsPrimitive())
            cgenc->AddInstanceMethod(mgenc.AssemblePrimitive(false));
        else
            cgenc->AddInstanceMethod(mgenc.Assemble());
    }

    if (accept(Separator)) {
        cgenc->SetClassSide(true);
        classFields(cgenc);
        while (symIsIdentifier() || sym == Keyword || sym == OperatorSequence ||
        symIn(binaryOpSyms)) {
            MethodGenerationContext mgenc;
            mgenc.SetHolder(cgenc);
            mgenc.AddArgument("self");

            method(&mgenc);

            if(mgenc.IsPrimitive())
                cgenc->AddClassMethod(mgenc.AssemblePrimitive(true));
            else
                cgenc->AddClassMethod(mgenc.Assemble());
        }
    }
    expect(EndTerm);
}

void Parser::superclass(ClassGenerationContext *cgenc) {
    VMSymbol* superName;
    if (sym == Identifier) {
        superName = GetUniverse()->SymbolFor(text);
        accept(Identifier);
    } else {
        superName = GetUniverse()->SymbolFor("Object");
    }
    cgenc->SetSuperName(superName);
    
    // Load the super class, if it is not nil (break the dependency cycle)
    if (superName != GetUniverse()->SymbolFor("nil")) {
        VMClass* superClass = GetUniverse()->LoadClass(superName);
        cgenc->SetInstanceFieldsOfSuper(superClass->GetInstanceFields());
        cgenc->SetClassFieldsOfSuper(superClass->GetClass()->GetInstanceFields());
    } else {
        // we hardcode here the field names for Class
        // since Object class superclass = Class
        // We avoid here any kind of dynamic solution to avoid further complexity.
        // However, that makes it static, it is going to make it harder to
        // change the definition of Class and Object
        vector<StdString> fieldNamesOfClass{ "class", "superClass", "name",
            "instanceFields", "instanceInvokables" };
        VMArray* fieldNames = GetUniverse()->NewArrayFromStrings(fieldNamesOfClass);
        cgenc->SetClassFieldsOfSuper(fieldNames);
    }
}

void Parser::instanceFields(ClassGenerationContext* cgenc) {
    if (accept(Or)) {
        while (symIsIdentifier()) {
            StdString var = variable();
            cgenc->AddInstanceField(GetUniverse()->SymbolFor(var));
        }
        expect(Or);
    }
}

void Parser::classFields(ClassGenerationContext* cgenc) {
    if (accept(Or)) {
        while (symIsIdentifier()) {
            StdString var = variable();
            cgenc->AddClassField(GetUniverse()->SymbolFor(var));
        }
        expect(Or);
    }
}

void Parser::method(MethodGenerationContext* mgenc) {
    pattern(mgenc);

    expect(Equal);
    if (sym == Primitive) {
        mgenc->SetPrimitive(true);
        primitiveBlock();
    } else
        methodBlock(mgenc);
}

void Parser::primitiveBlock(void) {
    expect(Primitive);
}

void Parser::pattern(MethodGenerationContext* mgenc) {
    switch (sym) {
    case Identifier:
    case Primitive:
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
    } while (sym == Keyword);

    mgenc->SetSignature(GetUniverse()->SymbolFor(kw));
}

void Parser::methodBlock(MethodGenerationContext* mgenc) {
    expect(NewTerm);
    blockContents(mgenc, false);
    // if no return has been generated so far, we can be sure there was no .
    // terminating the last expression, so the last expression's value must be
    // popped off the stack and a ^self be generated
    if (!mgenc->IsFinished()) {
        EmitPOP(mgenc);
        EmitPUSHARGUMENT(mgenc, 0, 0);
        EmitRETURNLOCAL(mgenc);
        mgenc->SetFinished();
    }

    expect(EndTerm);
}

VMSymbol* Parser::unarySelector(void) {
    return GetUniverse()->SymbolFor(identifier());
}

VMSymbol* Parser::binarySelector(void) {
    StdString s(text);

    if(acceptOneOf(singleOpSyms))
    ;
    else if(accept(OperatorSequence))
    ;
    else
    expect(NONE);

    VMSymbol* symb = GetUniverse()->SymbolFor(s);
    return symb;
}

StdString Parser::identifier(void) {
    StdString s(text);
    if (accept(Primitive))
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

void Parser::blockContents(MethodGenerationContext* mgenc, bool is_inlined) {
    if (accept(Or)) {
        locals(mgenc);
        expect(Or);
    }
    blockBody(mgenc, false, is_inlined);
}

void Parser::locals(MethodGenerationContext* mgenc) {
    while (symIsIdentifier())
        mgenc->AddLocalIfAbsent(variable());
}

void Parser::blockBody(MethodGenerationContext* mgenc, bool seen_period, bool is_inlined) {
    if (accept(Exit))
        result(mgenc);
    else if (sym == EndBlock) {
        if (seen_period) {
            // a POP has been generated which must be elided (blocks always
            // return the value of the last expression, regardless of whether it
            // was terminated with a . or not)
            mgenc->RemoveLastBytecode();
        }
        if (!is_inlined) {
            // if the block is empty, we need to return nil
            if (mgenc->IsBlockMethod() && !mgenc->HasBytecodes()) {
                EmitPUSHCONSTANT(mgenc, load_ptr(nilObject));
            }
            EmitRETURNLOCAL(mgenc);
            mgenc->SetFinished();
        }
    } else if (sym == EndTerm) {
        // it does not matter whether a period has been seen, as the end of the
        // method has been found (EndTerm) - so it is safe to emit a "return
        // self"
        EmitPUSHARGUMENT(mgenc, 0, 0);
        EmitRETURNLOCAL(mgenc);
        mgenc->SetFinished();
    } else {
        expression(mgenc);
        if (accept(Period)) {
            EmitPOP(mgenc);
            blockBody(mgenc, true, is_inlined);
        }
    }
}

void Parser::result(MethodGenerationContext* mgenc) {
    expression(mgenc);

    if (mgenc->IsBlockMethod())
        EmitRETURNNONLOCAL(mgenc);
    else
        EmitRETURNLOCAL(mgenc);

    mgenc->SetFinished(true);
    accept(Period);
}

void Parser::expression(MethodGenerationContext* mgenc) {
    Peek();
    if (nextSym == Assign)
        assignation(mgenc);
    else
        evaluation(mgenc);
}

void Parser::assignation(MethodGenerationContext* mgenc) {
    list<VMSymbol*> l;

    assignments(mgenc, l);
    evaluation(mgenc);
    list<VMSymbol*>::iterator i;
    for (i = l.begin(); i != l.end(); ++i)
        EmitDUP(mgenc);
    for (i = l.begin(); i != l.end(); ++i)
        genPopVariable(mgenc, (*i));

}

void Parser::assignments(MethodGenerationContext* mgenc, list<VMSymbol*>& l) {
    if (symIsIdentifier()) {
        l.push_back(assignment(mgenc));
        Peek();
        
        if (nextSym == Assign)
            assignments(mgenc, l);
    }
}

VMSymbol* Parser::assignment(MethodGenerationContext* mgenc) {
    StdString v = variable();

    expect(Assign);

    return GetUniverse()->SymbolFor(v);
}

void Parser::evaluation(MethodGenerationContext* mgenc) {
    bool super = primary(mgenc);
    if (symIsIdentifier() || sym == Keyword || sym == OperatorSequence
            || symIn(binaryOpSyms)) {
        messages(mgenc, super);
    }
}

bool Parser::primary(MethodGenerationContext* mgenc) {
    bool super = false;
    switch (sym) {
    case Primitive:
    case Identifier: {
        StdString v = variable();
        if (v == "super") {
            super = true;
            // sends to super push self as the receiver
            v = StdString("self");
        }

        genPushVariable(mgenc, GetUniverse()->SymbolFor(v));
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

        VMMethod* block_method = bgenc->Assemble();
        EmitPUSHBLOCK(mgenc, block_method);
        delete (bgenc);
        break;
    }
    default:
        literal(mgenc);
        break;
    }

    return super;
}

StdString Parser::variable(void) {
    return identifier();
}

void Parser::messages(MethodGenerationContext* mgenc, bool super) {
    if (symIsIdentifier()) {
        do {
            // only the first message in a sequence can be a super send
            unaryMessage(mgenc, super);
            super = false;
        } while (symIsIdentifier());

        while (sym == OperatorSequence || symIn(binaryOpSyms)) {
            binaryMessage(mgenc, false);
        }

        if (sym == Keyword) {
            keywordMessage(mgenc, false);
        }
    } else if (sym == OperatorSequence || symIn(binaryOpSyms)) {
        do {
            // only the first message in a sequence can be a super send
            binaryMessage(mgenc, super);
            super = false;
        } while (sym == OperatorSequence || symIn(binaryOpSyms));

        if (sym == Keyword) {
            keywordMessage(mgenc, false);
        }
    } else {
        keywordMessage(mgenc, super);
    }
}

void Parser::unaryMessage(MethodGenerationContext* mgenc, bool super) {
    VMSymbol* msg = unarySelector();

    if (super) {
        EmitSUPERSEND(mgenc, msg);
    } else {
        EmitSEND(mgenc, msg);
    }
}

void Parser::binaryMessage(MethodGenerationContext* mgenc, bool super) {
    VMSymbol* msg = binarySelector();

    bool tmp_bool = false;
    binaryOperand(mgenc);

    if (super)
        EmitSUPERSEND(mgenc, msg);
    else
        EmitSEND(mgenc, msg);

}

bool Parser::binaryOperand(MethodGenerationContext* mgenc) {
    bool super = primary(mgenc);

    while (symIsIdentifier()) {
        unaryMessage(mgenc, super);
        super = false;
    }

    return super;
}


void Parser::keywordMessage(MethodGenerationContext* mgenc, bool super) {
    StdString kw = keyword();
    
    formula(mgenc);
    while (sym == Keyword) {
        kw.append(keyword());
        formula(mgenc);
    }

    VMSymbol* msg = GetUniverse()->SymbolFor(kw);

    if (super)
        EmitSUPERSEND(mgenc, msg);
    else
        EmitSEND(mgenc, msg);

}

void Parser::formula(MethodGenerationContext* mgenc) {
    bool super = binaryOperand(mgenc);

    // only the first message in a sequence can be a super send
    if (sym == OperatorSequence || symIn(binaryOpSyms)) {
        binaryMessage(mgenc, super);
    }

    while (sym == OperatorSequence || symIn(binaryOpSyms)) {
        binaryMessage(mgenc, false);
    }
}

void Parser::nestedTerm(MethodGenerationContext* mgenc) {
    expect(NewTerm);
    expression(mgenc);
    expect(EndTerm);
}

void Parser::literal(MethodGenerationContext* mgenc) {
    switch (sym) {
    case Pound:
        PeekForNextSymbolFromLexerIfNecessary();
        if (nextSym == NewTerm) {
            literalArray(mgenc);
        } else {
            literalSymbol(mgenc);
        }
        break;
    case STString:
        literalString(mgenc);
        break;
    default:
        literalNumber(mgenc);
        break;
    }
}

vm_oop_t Parser::literalNumberOop() {
    if (sym == Minus)
        return negativeDecimal();
    else
        return literalDecimal(false);
}

void Parser::literalNumber(MethodGenerationContext* mgenc) {
    vm_oop_t lit = literalNumberOop();
    EmitPUSHCONSTANT(mgenc, lit);
}

vm_oop_t Parser::literalDecimal(bool negateValue) {
    if (sym == Integer) {
        return literalInteger(negateValue);
    } else {
        return literalDouble(negateValue);
    }
}

vm_oop_t Parser::negativeDecimal(void) {
    expect(Minus);
    return literalDecimal(true);
}

vm_oop_t Parser::literalInteger(bool negateValue) {
    vm_oop_t i = ParseInteger(text.c_str(), 10, negateValue);
    expect(Integer);
    return i;
}

vm_oop_t Parser::literalDouble(bool negateValue) {
    double d = std::strtod(text.c_str(), nullptr);
    if (negateValue) {
        d = 0 - d;
    }
    expect(Double);
    return GetUniverse()->NewDouble(d);
}

void Parser::literalSymbol(MethodGenerationContext* mgenc) {
    VMSymbol* symb;
    expect(Pound);
    if (sym == STString) {
        StdString s = _string();
        symb = GetUniverse()->SymbolFor(s);

    } else {
        symb = selector();
    }
    EmitPUSHCONSTANT(mgenc, symb);
}

void Parser::literalArray(MethodGenerationContext* mgenc) {
    expect(Pound);
    expect(NewTerm);

    VMSymbol* arrayClassName       = GetUniverse()->SymbolFor("Array");
    VMSymbol* arraySizePlaceholder = GetUniverse()->SymbolFor("ArraySizeLiteralPlaceholder");
    VMSymbol* newMessage           = GetUniverse()->SymbolFor("new:");
    VMSymbol* atPutMessage         = GetUniverse()->SymbolFor("at:put:");

    const uint8_t arraySizeLiteralIndex = mgenc->AddLiteral(arraySizePlaceholder);

    // create bytecode sequence for instantiating new array
    EmitPUSHGLOBAL(mgenc, arrayClassName);
    EmitPUSHCONSTANT(mgenc, arraySizeLiteralIndex);
    EmitSEND(mgenc, newMessage);

    size_t i = 1;

    while (sym != EndTerm) {
        vm_oop_t pushIndex = NEW_INT(i);
        EmitPUSHCONSTANT(mgenc, pushIndex);
        literal(mgenc);
        EmitSEND(mgenc, atPutMessage);
        i += 1;
    }

    // replace the placeholder with the actual array size
    mgenc->UpdateLiteral(arraySizePlaceholder, arraySizeLiteralIndex, NEW_INT(i - 1));

    expect(EndTerm);
}

void Parser::literalString(MethodGenerationContext* mgenc) {
    StdString s = _string();

    VMString* str = GetUniverse()->NewString(s);
    EmitPUSHCONSTANT(mgenc, str);
}

VMSymbol* Parser::selector(void) {
    if(sym == OperatorSequence || symIn(singleOpSyms))
    return binarySelector();
    else if(sym == Keyword || sym == KeywordSequence)
    return keywordSelector();
    else
    return unarySelector();
}

VMSymbol* Parser::keywordSelector(void) {
    StdString s(text);
    expectOneOf(keywordSelectorSyms);
    VMSymbol* symb = GetUniverse()->SymbolFor(s);
    return symb;
}

StdString Parser::_string(void) {
    StdString s(text);
    expect(STString);
    return s; // <-- Literal strings are At Most BUFSIZ chars long.
}

void Parser::nestedBlock(MethodGenerationContext* mgenc) {
    mgenc->AddArgumentIfAbsent("$block self");

    expect(NewBlock);
    if (sym == Colon)
        blockPattern(mgenc);

    // generate Block signature
    StdString block_sig = "$blockMethod@" + to_string(lexer->GetCurrentLineNumber());
    size_t arg_size = mgenc->GetNumberOfArguments();
    for (size_t i = 1; i < arg_size; i++)
        block_sig += ":";

    mgenc->SetSignature(GetUniverse()->SymbolFor(block_sig));

    blockContents(mgenc, false);

    // if no return has been generated, we can be sure that the last expression
    // in the block was not terminated by ., and can generate a return
    if (!mgenc->IsFinished()) {
        if (!mgenc->HasBytecodes()) {
          // if the block is empty, we need to return nil
          EmitPUSHCONSTANT(mgenc, load_ptr(nilObject));
        }
        EmitRETURNLOCAL(mgenc);
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

    } while (sym == Colon);
}

static bool replace(StdString& str, const char* pattern, StdString& replacement) {
    size_t pos = str.find(pattern);
    if (pos == std::string::npos) {
        return false;
    }
    
    str.replace(pos, strlen(pattern), replacement);
    return true;
}

__attribute__((noreturn)) void Parser::parseError(const char* msg, Symbol expected) {
    StdString expectedStr(symnames[expected]);
    parseError(msg, expectedStr);
}


__attribute__((noreturn)) void Parser::parseError(const char* msg, StdString expected) {
    StdString msgWithMeta = "%(file)s:%(line)d:%(column)d: error: " + StdString(msg);
    
    StdString foundStr;
    if (_PRINTABLE_SYM) {
        foundStr = symnames[sym] + StdString(" (") + text + ")";
    } else {
        foundStr = symnames[sym];
    }
    
    replace(msgWithMeta, "%(file)s", fname);

    StdString line = std::to_string(lexer->GetCurrentLineNumber());
    replace(msgWithMeta, "%(line)d", line);
    
    StdString column = std::to_string(lexer->getCurrentColumn());
    replace(msgWithMeta, "%(column)d", column);
    replace(msgWithMeta, "%(expected)s", expected);
    replace(msgWithMeta, "%(found)s", foundStr);
    
    ErrorPrint(msgWithMeta);
    GetUniverse()->Quit(ERR_FAIL);
}

__attribute__((noreturn)) void Parser::parseError(const char* msg, Symbol* expected) {
    bool first = true;
    StdString expectedStr = "";
    
    Symbol* next = expected;
    while (*next) {
        if (first) {
            first = false;
        } else {
            expectedStr += ", ";
        }
        
        expectedStr += symnames[*next];
        next += 1;
    }
    
    parseError(msg, expectedStr);
}
