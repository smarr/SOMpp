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
#include "Lexer.h"

#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <istream>
#include <string>

#include "../vm/Print.h"

Lexer::Lexer(istream& file) : infile(file), peekDone(false) {}

#define _BC (buf[state.bufp])
#define EOB (state.bufp >= buf.length())

int64_t Lexer::fillBuffer() {
    if (!infile.good()) {  // file stream
        return -1;
    }

    std::getline(infile, buf);
    state.lineNumber += 1;
    state.bufp = 0;
    return buf.length();
}

//
// basic lexing
//

void Lexer::skipWhiteSpace() {
    while (isspace(_BC) != 0) {
        state.incPtr();
        while (EOB) {
            if (fillBuffer() == -1) {
                return;
            }
        }
    }
}

void Lexer::skipComment() {
    if (_BC == '"') {
        do {
            state.incPtr();
            while (EOB) {
                if (fillBuffer() == -1) {
                    return;
                }
            }
        } while (_BC != '"');
        state.incPtr();
    }
}

#define _ISOP(C)                                                            \
    ((C) == '~' || (C) == '&' || (C) == '|' || (C) == '*' || (C) == '/' ||  \
     (C) == '\\' || (C) == '+' || (C) == '=' || (C) == '>' || (C) == '<' || \
     (C) == ',' || (C) == '@' || (C) == '%' || (C) == '-')
#define _MATCH(C, S)      \
    if (_BC == (C)) {     \
        state.sym = (S);  \
        state.symc = _BC; \
        state.text = _BC; \
        state.incPtr();   \
    }
#define SEPARATOR std::string("----")  // FIXME
#define PRIMITIVE std::string("primitive")

void Lexer::lexNumber() {
    state.sym = Integer;
    state.symc = 0;
    state.text.clear();

    bool sawDecimalMark = false;

    do {
        state.text += _BC;
        state.incPtr();

        if (!sawDecimalMark and '.' == _BC and state.bufp + 1 < buf.length() and
            isdigit(buf[state.bufp + 1]) != 0) {
            state.sym = Double;
            sawDecimalMark = true;
            state.text += buf[state.bufp];
            state.incPtr();
        }

    } while (isdigit(_BC) != 0);
}

void Lexer::lexEscapeChar() {
    assert(!EOB);
    const char current = _BC;
    switch (current) {
        case 't':
            state.text += '\t';
            break;
        case 'b':
            state.text += '\b';
            break;
        case 'n':
            state.text += '\n';
            break;
        case 'r':
            state.text += '\r';
            break;
        case 'f':
            state.text += '\f';
            break;
        case '0':
            state.text += '\0';
            break;
        case '\'':
            state.text += '\'';
            break;
        case '\\':
            state.text += '\\';
            break;
        default:
            assert(false);
            break;
    }
    state.incPtr();
}

void Lexer::lexStringChar() {
    if (_BC == '\\') {
        state.incPtr();
        lexEscapeChar();
    } else {
        state.text += buf[state.bufp];
        state.incPtr();
    }
}

void Lexer::lexString() {
    state.sym = STString;
    state.symc = 0;
    state.text.clear();
    state.incPtr();

    while (_BC != '\'') {
        while (EOB) {
            if (fillBuffer() == -1) {
                return;
            }
            state.text += '\n';
        }
        if (_BC != '\'') {
            lexStringChar();
        }
    }
    state.incPtr();
}

void Lexer::lexOperator() {
    if (_ISOP(buf[state.bufp + 1])) {
        state.sym = OperatorSequence;
        state.symc = 0;
        state.text.clear();
        while (_ISOP(_BC)) {
            state.text += buf[state.bufp];
            state.incPtr();
        }
    }
    // clang-format off
    else _MATCH('~', Not)  else _MATCH('&', And)   else _MATCH('|', Or)
    else _MATCH('*', Star) else _MATCH('/', Div)   else _MATCH('\\', Mod)
    else _MATCH('+', Plus) else _MATCH('=', Equal) else _MATCH('>', More)
    else _MATCH('<', Less) else _MATCH(',', Comma) else _MATCH('@', At)
    else _MATCH('%', Per)  else _MATCH('-', Minus)
    // clang-format on
}

Symbol Lexer::GetSym() {
    if (peekDone) {
        peekDone = false;
        state = stateAfterPeek;
        return state.sym;
    }

    do {
        if (!hasMoreInput()) {
            state.sym = NONE;
            state.symc = 0;
            state.text = "";
            return state.sym;
        }
        skipWhiteSpace();
        skipComment();
    } while ((EOB || isspace(_BC) != 0 || _BC == '"'));

    state.startBufp = state.bufp;

    if (_BC == '\'') {
        lexString();
    }
    // clang-format off
    else _MATCH('[', NewBlock)
    else _MATCH(']', EndBlock)
        // clang-format on
        else if (_BC == ':') {
        if (buf[state.bufp + 1] == '=') {
            state.incPtr(2);
            state.sym = Assign;
            state.symc = 0;
            state.text = ":=";
        } else {
            state.incPtr();
            state.sym = Colon;
            state.symc = ':';
            state.text = ":";
        }
    }
    // clang-format off
    else _MATCH('(', NewTerm) else _MATCH(')', EndTerm)
    else _MATCH('#', Pound) else _MATCH('^', Exit)
    else _MATCH('.', Period)
        // clang-format on
        else if (_BC == '-') {
        if (buf.substr(state.bufp, SEPARATOR.length()).compare(SEPARATOR) ==
            0) {
            state.text.clear();
            while (_BC == '-') {
                state.text += buf[state.bufp];
                state.incPtr();
            }
            state.sym = Separator;
        } else {
            lexOperator();
        }
    }
    else if (_ISOP(_BC)) {
        lexOperator();
    }
    else if (nextWordInBufferIsPrimitive()) {
        state.incPtr(PRIMITIVE.length());
        state.sym = Primitive;
        state.symc = 0;
        state.text = "primitive";
    }
    else if (isalpha(_BC) != 0) {
        state.text.clear();
        state.symc = 0;
        while (isalpha(_BC) != 0 || isdigit(_BC) != 0 || _BC == '_') {
            state.text += buf[state.bufp];
            state.incPtr();
        }
        state.sym = Identifier;
        if (buf[state.bufp] == ':') {
            state.sym = Keyword;
            state.incPtr();
            state.text += ':';
            if (isalpha(_BC) != 0) {
                state.sym = KeywordSequence;
                while (isalpha(_BC) != 0 || _BC == ':') {
                    state.text += buf[state.bufp];
                    state.incPtr();
                }
            }
        }
    }
    else if (isdigit(_BC) != 0) {
        lexNumber();
    }
    else {
        state.sym = NONE;
        state.symc = _BC;
        state.text = _BC;
    }

    return state.sym;
}

bool Lexer::hasMoreInput() {
    while (EOB) {
        if (fillBuffer() == -1) {
            return false;
        }
    }
    return true;
}

bool Lexer::nextWordInBufferIsPrimitive() {
    if (0 != buf.substr(state.bufp, PRIMITIVE.length()).compare(PRIMITIVE)) {
        return false;
    }
    if (state.bufp + PRIMITIVE.length() >= buf.length()) {
        return true;
    }
    return isalnum(buf[state.bufp + PRIMITIVE.length()]) == 0;
}

Symbol Lexer::Peek() {
    const LexerState old = state;

    if (peekDone) {
        ErrorExit("Cannot Peek twice!\n");
    }
    GetSym();
    const Symbol nextSym = state.sym;

    stateAfterPeek = state;
    state = old;

    peekDone = true;
    return nextSym;
}
