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

#include "VMSymbol.h"
#include "VMInteger.h"


VMSymbol::VMSymbol(const char* str) : VMString(str) {
}


VMSymbol::VMSymbol( const StdString& s ): VMString(s) {
}


StdString VMSymbol::GetPlainString() const {
    ostringstream str;
    char* chars = this->GetChars();
    for(size_t i=0; i <= (size_t)this->GetStringLength(); i++) {
        char c = chars[i];
        switch (c) {
            case '~':
                str << "tilde";
                break;
            case '&':
                str << "and";
                break;
            case '|':
                str << "bar";
                break;
            case '*':
                str << "star";
                break;
            case '/':
                str << "slash";
                break;
            case '@':
                str << "at";
                break;
            case '+':
                str << "plus";
                break;
            case '-':
                str << "minus";
                break;
            case '=':
                str << "equal";
                break;     
            case '>':
                str << "greaterthan" ;
                break;
            case '<':
                str << "lowerthan";
                break;
            case ',':
                str << "comma";
                break;
            case '%':
                str << "percent";
                break;
            case '\\':
                str << "backslash";
                break;
            case ':':
                str << '_';
                break;
            default:
                if (c != 0) {
                    str << c;
                }
                break;
        }
    }
    StdString st = str.str();
    
    return st;
}

