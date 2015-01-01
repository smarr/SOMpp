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

#include "Signature.h"
#include "VMSymbol.h"

int Signature::GetNumberOfArguments(VMSymbol* sig) {
    return sig->numberOfArgumentsOfSignature;
}

bool Signature::IsBinary(VMSymbol* sig) {
    return sig->numberOfArgumentsOfSignature == 2;
}

int Signature::DetermineNumberOfArguments(const char* sig) {
    // check default binaries
    if (Signature::IsBinary(sig))
        return 2;

    // colons in str
    int numColons = 0;
    int i = 0;
    while (sig[i] != '\0')
        if (sig[i++] == ':')
            numColons++;
    return numColons + 1;
}

bool Signature::IsBinary(const char* sig) {
    switch(sig[0]) {
        case '~' :
        case '&' :
        case '|' :
        case '*' :
        case '/' :
        case '@' :
        case '+' :
        case '-' :
        case '=' :
        case '>' :
        case '<' :
        case ',' :
        case '%' :
        case '\\':
        return true;
        default: break;
    }
    return false;
}
