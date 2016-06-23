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
#include <assert.h>

#include "Lexer.h"

Lexer::Lexer(istream &file) : infile(file) {
    peekDone = false;
    bufp = 0;
    lineNumber = 0;
    
    text     = "";
    nextText = "";
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

size_t Lexer::fillBuffer(void) {
    if (!infile.good()) // file stream
        return -1;

    std::getline(infile, buf);
    ++lineNumber;
    bufp = 0;
    return buf.length();
}

//
// basic lexing
//

void Lexer::skipWhiteSpace(void) {
    while (isspace(_BC)) {
        bufp++;
        while (EOB) {
            if (fillBuffer() == -1) {
                return;
            }
        }
    }
}

void Lexer::skipComment(void) {

    if (_BC == '"') {
        do {
            bufp++;
            while (EOB) {
                if (fillBuffer() == -1) {
                    return;
                }
            }
        } while (_BC != '"');
        bufp++;
    }
}

#define _ISOP(C) \
    ((C) == '~' || (C) == '&' || (C) == '|' || (C) == '*' || (C) == '/' || \
     (C) == '\\' || (C) == '+' || (C) == '=' || (C) == '>' || (C) == '<' || \
     (C) == ',' || (C) == '@' || (C) == '%' || (C) == '-')
#define _MATCH(C, S) \
    if(_BC == (C)) { sym = (S); symc = _BC; text = _BC; bufp++;}
#define SEPARATOR StdString("----") //FIXME
#define PRIMITIVE StdString("primitive")

void Lexer::lexNumber() {
    sym = Integer;
    symc = 0;
    text.clear();

    bool sawDecimalMark = false;
    
    do {
        text += buf[bufp++];
        
        if (!sawDecimalMark         and
            '.' == _BC              and
            bufp + 1 < buf.length() and
            isdigit(buf[bufp + 1])) {
            sym = Double;
            sawDecimalMark = true;
            text += buf[bufp++];
        }
        
    } while (isdigit(_BC));
}


void Lexer::lexEscapeChar() {
    assert(!EOB);
    char current = _BC;
    switch (current) {
        case 't': text += '\t'; break;
        case 'b': text += '\b'; break;
        case 'n': text += '\n'; break;
        case 'r': text += '\r'; break;
        case 'f': text += '\f'; break;
        case '\'': text += '\''; break;
        case '\\': text += '\\'; break;
        default:
            assert(false);
            break;
    }
    bufp++;
}

void Lexer::lexStringChar() {
    if (_BC == '\\') {
        bufp++;
        lexEscapeChar();
    } else {
        text += buf[bufp++];
    }
}

void Lexer::lexString() {
    sym = STString;
    symc = 0;
    text.clear();
    bufp++;
    
    while (_BC != '\'') {
        lexStringChar();
        while (EOB) {
            if (fillBuffer() == -1) {
                return;
            }
        }
    }
    bufp++;
}

void Lexer::lexOperator() {
    if (_ISOP(buf[bufp + 1])) {
        sym = OperatorSequence;
        symc = 0;
        text.clear();
        while (_ISOP(_BC)) {
            text += buf[bufp++];
        }

    } else _MATCH('~', Not)  else _MATCH('&', And)   else _MATCH('|', Or)
      else _MATCH('*', Star) else _MATCH('/', Div)   else _MATCH('\\', Mod)
      else _MATCH('+', Plus) else _MATCH('=', Equal) else _MATCH('>', More)
      else _MATCH('<', Less) else _MATCH(',', Comma) else _MATCH('@', At)
      else _MATCH('%', Per)  else _MATCH('-', Minus)
}

Symbol Lexer::GetSym(void) {
    if (peekDone) {
        peekDone = false;
        sym = nextSym;
        symc = nextSymc;
        text = StdString(nextText);
        return sym;
    }

    do {
        if (!hasMoreInput()) {
            sym = NONE;
            symc = 0;
            return sym;
        }
        skipWhiteSpace();
        skipComment();
    } while ((EOB || isspace(_BC) || _BC == '"'));

    if (_BC == '\'') {
        lexString();
    } else _MATCH('[', NewBlock) else _MATCH(']', EndBlock) else if (_BC
            == ':') {
        if (buf[bufp + 1] == '=') {
            bufp += 2;
            sym = Assign;
            symc = 0;
            text = ":=";
        } else {
            bufp++;
            sym = Colon;
            symc = ':';
            text = ":";
        }
    } else _MATCH('(', NewTerm) else _MATCH(')', EndTerm) else _MATCH('#', Pound) else _MATCH('^', Exit) else _MATCH('.', Period) else if (_BC
            == '-') {
        if (!buf.substr(bufp, SEPARATOR.length()).compare(SEPARATOR)) {
            text.clear();
            while (_BC == '-') {
                text += buf[bufp++];
            }
            sym = Separator;
        } else {
            lexOperator();
        }
    } else if (_ISOP(_BC)) {
        lexOperator();
    } else if (nextWordInBufferIs(PRIMITIVE)) {
        bufp += PRIMITIVE.length();
        sym = Primitive;
        symc = 0;
        text = "primitive";
    } else if (isalpha(_BC)) {
        text.clear();
        symc = 0;
        while (isalpha(_BC) || isdigit(_BC) || _BC == '_') {
            text += buf[bufp++];
        }
        sym = Identifier;
        if (buf[bufp] == ':') {
            sym = Keyword;
            bufp++;
            text += ':';
            if (isalpha(_BC)) {
                sym = KeywordSequence;
                while (isalpha(_BC) || _BC == ':')
                    text += buf[bufp++];
            }
        }
    } else if (isdigit(_BC)) {
        lexNumber();
    } else {
        sym = NONE;
        symc = _BC;
        text = _BC;
    }

    return sym;
}

bool Lexer::hasMoreInput() {
    while (EOB) {
        if (fillBuffer() == -1) {
            return false;
        }
    }
    return true;
}

bool Lexer::nextWordInBufferIs(StdString word) {
    if (0 != buf.substr(bufp, PRIMITIVE.length()).compare(PRIMITIVE)) {
        return false;
    }
    if (bufp + PRIMITIVE.length() >= buf.length()) {
        return true;
    }
    return not isalnum(buf[bufp + PRIMITIVE.length()]);
}

Symbol Lexer::Peek(void) {
    Symbol saveSym = sym;
    char saveSymc = symc;
    StdString saveText = StdString(text);
    if (peekDone) {
        fprintf(stderr, "Cannot Peek twice!\n");
    }
    GetSym();
    nextSym = sym;
    nextSymc = symc;
    nextText = StdString(text);
    sym = saveSym;
    symc = saveSymc;
    text = StdString(saveText);
    peekDone = true;
    return nextSym;
}
