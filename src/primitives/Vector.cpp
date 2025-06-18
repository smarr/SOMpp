#include "Vector.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMVector.h"

static vm_oop_t vecNew(vm_oop_t clazz) {
    auto* classPtr = static_cast<VMClass*>(clazz);
    int64_t const size = 50;
    VMVector* vec = Universe::NewVector(size, classPtr);
    return vec;
}

static vm_oop_t vecNewSize(vm_oop_t clazz, vm_oop_t arg) {
    int64_t const size = INT_VAL(arg);
    return Universe::NewVector(size, static_cast<VMClass*>(clazz));
}

static vm_oop_t vecAt(vm_oop_t obj, vm_oop_t arg) {
    auto* self = static_cast<VMVector*>(obj);
    int64_t const index = INT_VAL(arg);
    return self->GetIndexableField(index);
}

static vm_oop_t vecAtPut(vm_oop_t obj, vm_oop_t at, vm_oop_t put) {
    auto* self = static_cast<VMVector*>(obj);  // Cast itself as a VMVector
    int64_t const index = INT_VAL(at);         // Set the index looking for
    // Call method to set the value at index. That deals with 1to0 indexing
    // conversion
    return self->SetIndexableField(index, put);
}

static vm_oop_t vecAppend(vm_oop_t obj, vm_oop_t arg) {
    auto* self = static_cast<VMVector*>(obj);
    self->Append(arg);
    return self;
}

static vm_oop_t vecFirst(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->GetFirst();
}

static vm_oop_t vecLast(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->GetLast();
}

static vm_oop_t removeLast(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->RemoveLast();
}

static vm_oop_t removeFirst(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->RemoveFirst();
}

static vm_oop_t removeObject(vm_oop_t obj, vm_oop_t other) {
    auto* self = static_cast<VMVector*>(obj);
    return self->RemoveObj(other);
}

static vm_oop_t removeAll(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    self->RemoveAll();
    return self;
}

static vm_oop_t vecSize(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->Size();
}

static vm_oop_t capacity(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->Capacity();
}

static vm_oop_t asArray(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->copyStorageArray();
}

/*
core-lib

at: 6078227970640332630
at:put: 6078227970640332630
first 130651218016064188
last 1947622275005066112
append: 2297668033614959926
remove 3806281540898964462
remove: 14432638981619695090
isEmpty 16935082123893034051
size 16935082123893034051
capacity 16935082123893034051
asArray 1836700529089583015
removeFirst 3806281540898964462

awfy-differences
at: 130651218016064188
at:put: 14777839075779364226
append: 2297668033614959926
isEmpty: 16935082123893034051
removeFirst: 17123654466142159564
removeAll: 4619310252099508733
remove: 14432638981619695090
size 16935082123893034051
capcity 16935082123893034051
*/

/* Handles initialization of primitives separately to object creation */
/* Object is created when statics are loaded and no .som file has been read */
void _Vector::LateInitialize(std::map<std::string, size_t>* hashes) {
#ifdef USE_VECTOR_PRIMITIVES

    // cout<<"at: hash " << (*hashes)["at:"] << endl;
    // cout<<"at:put: hash " << (*hashes)["at:put:"] << endl;
    // cout<<"first hash " << (*hashes)["first"] << endl;
    // cout<<"last hash " << (*hashes)["last"] << endl;
    // cout<<"append: hash " << (*hashes)["append:"] << endl;
    // cout<<"remove hash " << (*hashes)["remove"] << endl;
    // cout<<"remove: hash " << (*hashes)["remove:"] << endl;
    // cout<<"size hash " << (*hashes)["size"] << endl;
    // cout<<"capacity hash " << (*hashes)["capacity"] << endl;
    // cout<<"asArray hash " << (*hashes)["asArray"] << endl;
    // cout<<"removeFirst hash " << (*hashes)["removeFirst"] << endl;
    // cout<<"removeAll hash " << (*hashes)["removeAll"] << endl;

    /* A map of hashes with the key being method signature are added here */
    if ((*hashes)["at:"] == 15701225123643718540ULL ||
        (*hashes)["at:"] == 6812223825622950441ULL) {
        Add("at:", &vecAt, false);
    }

    if ((*hashes)["at:put:"] == 15701225123643718540ULL ||
        (*hashes)["at:put:"] == 1424373843580326011ULL) {
        Add("at:put:", &vecAtPut, false);
    }

    if ((*hashes)["first"] == 10026279788538546667ULL) {
        Add("first", &vecFirst, false);
    }

    if ((*hashes)["last"] == 12704268779516119438ULL) {
        Add("last", &vecLast, false);
    }

    if ((*hashes)["append:"] == 13118885541413071290ULL ||
        (*hashes)["append:"] == 13118885541413071290ULL) {
        Add("append:", &vecAppend, false);
    }

    if ((*hashes)["remove"] == 1325639674086910439ULL) {
        Add("remove", &removeLast, false);
    }

    if ((*hashes)["remove:"] == 18423337479191764055ULL ||
        (*hashes)["remove:"] == 18423337479191764055ULL) {
        Add("remove:", &removeObject, false);
    }

    if ((*hashes)["size"] == 275685630939944626ULL ||
        (*hashes)["size"] == 275685630939944626ULL) {
        Add("size", &vecSize, false);
    }

    if ((*hashes)["capacity"] == 12474412091446719372ULL ||
        (*hashes)["capacity"] == 12474412091446719372ULL) {
        Add("capacity", &capacity, false);
    }

    if ((*hashes)["asArray"] == 10354287759150596565ULL) {
        Add("asArray", &asArray, false);
    }

    if ((*hashes)["removeFirst"] == 10589921408041234885ULL ||
        (*hashes)["removeFirst"] == 14759376054960058883ULL) {
        Add("removeFirst", &removeFirst, false);
    }

    if ((*hashes)["removeAll"] == 13065396693477551925ULL) {
        Add("removeAll", &removeAll, false);
    }

#endif
}

_Vector::_Vector() = default;