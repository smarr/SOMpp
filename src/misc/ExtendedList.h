#pragma once

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

#include <list>
#include <memory>
#include <iterator>
#include <iostream>

template<class T>
class ExtendedList {
public:
    explicit ExtendedList();

    void Add(const T& ptr);
    void AddIfAbsent(const T& ptr);
    void AddAll(const ExtendedList<T>* list);
    void PushBack(const T& ptr);
    void Clear();
    size_t Size() const;
    T Get(long index);
    int32_t IndexOf(const T& needle);

    typedef typename std::list<T>::iterator iterator_t;
    typedef typename std::list<T>::const_iterator const_iterator_t;

    iterator_t Begin() {
        return theList.begin();
    }
    // implement for const objects...
    const_iterator_t Begin() const {
        return theList.begin();
    }

private:
    std::list<T> theList;
};

template<class T>
ExtendedList<T>::ExtendedList() {
    theList.clear();
}

template<class T>
void ExtendedList<T>::Add(const T& ptr) {
    theList.push_back(ptr);
}

template<class T>
void ExtendedList<T>::AddAll(const ExtendedList<T>* list) {
    theList.merge(list->theList);
}

template<class T>
void ExtendedList<T>::AddIfAbsent(const T& ptr) {
    if (IndexOf(ptr) == -1)
        Add(ptr);
}

template<class T>
void ExtendedList<T>::Clear() {
    theList.clear();
}

template<class T>
T ExtendedList<T>::Get(long index) {
    for (iterator_t it = theList.begin(); it != theList.end(); ++it) {
        if (index == 0)
            return *it;
        --index;
    }
    return nullptr;
}

template<class T>
size_t ExtendedList<T>::Size() const {
    return theList.size();
}

template<class T>
int32_t ExtendedList<T>::IndexOf(const T& needle) {
    for (iterator_t it = theList.begin(); it != theList.end(); ++it) {
        if (*it == needle)
            return (int32_t) distance(theList.begin(), it);

    }
    return -1;
}

template<class T>
void ExtendedList<T>::PushBack(const T& ptr) {
    theList.push_back(ptr);
}
