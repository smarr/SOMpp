#pragma once
#ifndef GARBAGECOLLECTOR_H_
#define GARBAGECOLLECTOR_H_

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


#include "../vmobjects/ObjectFormats.h"
#include "../misc/defs.h"

class VMObject;
class Heap;

class GarbageCollector {
public:
	GarbageCollector(Heap* h);
	~GarbageCollector();
	void Collect();
    void PrintGCStat() const;
    void PrintCollectStat() const;
	

private:
	void markReachableObjects();
	void mergeFreeSpaces();
	Heap* heap;

    //
    // values for GC statistics
    //
    uint32_t numCollections;
	uint32_t numLive;
	uint32_t spcLive;
	uint32_t numFreed;
	uint32_t spcFreed;

	

};

#endif
