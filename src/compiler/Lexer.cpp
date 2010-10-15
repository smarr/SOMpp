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


#include <string.h>

#include "Lexer.h"


Lexer::Lexer(istream &file) : infile(file) {
	peekDone = false;
	bufp = 0;
    lineNumber = 0;
}

Lexer::~Lexer() {
}


StdString Lexer::GetText(void) {
	return StdString(text);
}

StdString Lexer::GetNextText(void) {
	return StdString(nextText);
}

StdString Lexer::GetRawBuffer(void) {
	//for debug
	return StdString(buf);
}

#define _BC (buf[bufp])
#define EOB (bufp >= buf.length())

int Lexer::fillBuffer(void) {
	if(!infile.good()) // file stream
        return 0;
    
    std::getline(infile, buf);
    ++lineNumber;
    bufp = 0;
    return buf.length();
}


//
// basic lexing
//

void Lexer::skipWhiteSpace(void) {
    while(isspace(_BC)) {
        bufp++;
        if(EOB)
            fillBuffer();
    }
}


void Lexer::skipComment(void) {
	
    if(_BC == '"') {
        do {
            bufp++;
            if(EOB)
                fillBuffer();
        } while(_BC != '"');
        bufp++;
    }
}


#define _ISOP(C) \
    ((C) == '~' || (C) == '&' || (C) == '|' || (C) == '*' || (C) == '/' || \
     (C) == '\\' || (C) == '+' || (C) == '=' || (C) == '>' || (C) == '<' || \
     (C) == ',' || (C) == '@' || (C) == '%')
#define _MATCH(C, S) \
    if(_BC == (C)) { sym = (S); symc = _BC; sprintf(text, "%c", _BC); bufp++;}
#define SEPARATOR StdString("----") //FIXME
#define PRIMITIVE StdString("primitive")

Symbol Lexer::GetSym(void) {
    if(peekDone) {
        peekDone = false;
        sym = nextSym;
        symc = nextSymc;
        strncpy(text, nextText, 512);
        return sym;
    }

    do {
        if(EOB)
            fillBuffer();
        skipWhiteSpace();
        skipComment();
		
    } while((EOB || isspace(_BC) || _BC == '"') && infile.good());
    
    if(_BC == '\'') {
        sym = STString;
        symc = 0;
        char* t = text;
        do {
            *t++ = buf[++bufp];
			
        } while(_BC != '\'');
		
        bufp++;
        *--t = 0;
    }
    else _MATCH('[', NewBlock)
    else _MATCH(']', EndBlock)
    else if(_BC == ':') {
        if(buf[bufp+1] == '=') {
            bufp += 2;
            sym = Assign;
            symc = 0;
            sprintf(text, ":=");
        } else {
            bufp++;
            sym = Colon;
            symc = ':';
            sprintf(text, ":");
        }
    }
    else _MATCH('(', NewTerm)
    else _MATCH(')', EndTerm)
    else _MATCH('#', Pound)
    else _MATCH('^', Exit)
    else _MATCH('.', Period)
    else if(_BC == '-') {
		if(!buf.substr(bufp, SEPARATOR.length()).compare(SEPARATOR)) {
            char* t = text;
            while(_BC == '-')
                *t++ = buf[bufp++];
            *t = 0;
            sym = Separator;
        } else {
            bufp++;
            sym = Minus;
            symc = '-';
            sprintf(text, "-");
        }
    }
    else if(_ISOP(_BC)) {
        if(_ISOP(buf[bufp + 1])) {
            sym = OperatorSequence;
            symc = 0;
            char* t = text;
            while(_ISOP(_BC))
                *t++ = buf[bufp++];
            *t = 0;
        }
        else _MATCH('~', Not)
        else _MATCH('&', And)
        else _MATCH('|', Or)
        else _MATCH('*', Star)
        else _MATCH('/', Div)
        else _MATCH('\\', Mod)
        else _MATCH('+', Plus)
        else _MATCH('=', Equal)
        else _MATCH('>', More)
        else _MATCH('<', Less)
        else _MATCH(',', Comma)
        else _MATCH('@', At)
        else _MATCH('%', Per)
    }
    else if (!buf.substr(bufp, PRIMITIVE.length()).compare(PRIMITIVE)) {
        bufp += PRIMITIVE.length();
        sym = Primitive;
        symc = 0;
        sprintf(text, "primitive");
    }
    else if(isalpha(_BC)) {
        char* t = text;
        symc = 0;
        while(isalpha(_BC) || isdigit(_BC) || _BC == '_')
            *t++ = buf[bufp++];
        sym = Identifier;
        if(buf[bufp] == ':') {
            sym = Keyword;
            bufp++;
            *t++ = ':';
            if(isalpha(_BC)) {
                sym = KeywordSequence;
                while(isalpha(_BC) || _BC == ':')
                    *t++ = buf[bufp++];
            }
        }
        *t = 0;
    }
    else if(isdigit(_BC)) {
        sym = Integer;
        symc = 0;
        char* t = text;
        do {
            *t++ = buf[bufp++];
        } while(isdigit(_BC));
        *t = 0;
    }
    else {
        sym = NONE;
        symc = _BC;
        sprintf(text, "%c", _BC);
    }
	
	return sym;
}


Symbol Lexer::Peek(void) {
    Symbol saveSym = sym;
    char saveSymc = symc;
    char saveText[256];
    strcpy(saveText, text);
    if(peekDone)
        fprintf(stderr, "Cannot Peek twice!\n");
    GetSym();
    nextSym = sym;
    nextSymc = symc;
    strcpy(nextText, text);
    sym = saveSym;
    symc = saveSymc;
    strcpy(text, saveText);
    peekDone = true;
	return nextSym;
}
