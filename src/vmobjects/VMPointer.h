#pragma once

#ifndef PVMOBJECT_H_
#define PVMOBJECT_H_

#include <typeinfo>
#include "../misc/defs.h"
#include <iostream>
#include "IntegerBox.h"

template<class T>
class VMPointer {
public:
    //Constructors
    //Empty constructor creates NULL pointer
    VMPointer<T>() { 
        pointer = NULL; 
    };
    VMPointer<T>(T* o) { pointer = o; /*if (!IsTaggedInteger()) pointer->IncreaseGCCount(); */};
    VMPointer<T>(const VMPointer<T>& o) { pointer = o.GetPointer();/*(pointer = o.pointer)->IncreaseGCCount(); */};
//    ~VMPointer<T>() { if (pointer) pointer->DecreaseGCCount(); };
    
    //Type conversion constructor. Converts pointer to class U in pointer to class T
    template<class U>
    VMPointer<T>(VMPointer<U>& o) { 
        pointer = (T*)(o.GetPointer());
    };

    template<class U>
    VMPointer<T>(const VMPointer<U>& o) { 
        pointer = (T*)(o.GetPointer());
        /*if (!pointer) {
            cout << "Trying to convert unmatching types: " << typeid(U).name()
                 << " to " << typeid(T).name() << endl;
            throw std::bad_exception();
        }*/
    };

    //null checking
    inline bool IsNull() const { return pointer == NULL; };

    //operator VMPointer<VMObject>&();

    //operator VMObject*() { return pointer; };

    //type conversions

    //conversion to "dumb pointer" of the included type
    inline operator T*() { return pointer; };

    //conversion to reference of the included type
    inline T& operator*() {
        return *pointer;
    };

    //conversion to const reference of the included type
    inline T& operator*() const{  
        return *pointer;
    };

    //member access operator
    inline T* operator ->() {
        return pointer;
    };
    //member access operator for const methods
    inline T* operator ->() const{
        return pointer;
    };
    //assignments

    //allows to assign a "dumb pointer" to a smart pointer of the included type
    inline VMPointer<T>& operator =(T* ptr);
    //inline VMPointer<T>& operator =(const T* ptr);
    //inline VMPointer<T>& operator =(const T& ptr);
    inline VMPointer<T>& operator =( T& ptr);
    //allows to assign smart pointers of the same type
    inline VMPointer<T>& operator =(VMPointer<T> ptr);
    
    //allows to assign "dumb pointer" of a different (compatible) type
    template<class U>
    VMPointer<T>& operator =(U* ptr) {
        pointer = (T*)(ptr);
        return *this;
    }
    
    //allows to assign smart pointers of different (compatible) types
    template<class U>
    VMPointer<T>& operator =(VMPointer<U> o){
        //check for self assignment
        if ((void*)&o == (void*)this) return *this;
        pointer = (T*)(o.GetPointer());
        
        return *this;
    };

    //comparing
    inline bool operator==(VMPointer<T> o) const{
        return (void*)o.pointer == (void*)pointer;
    };
    inline bool operator==(void* ptr) const{
        return (void*)pointer == ptr;
    };
    
    template <class U>
    inline bool operator==(VMPointer<U> o) const{
        return (void*)pointer == (void*)o.GetPointer();
    };
    
    inline bool operator!=(VMPointer<T> o) const{
        return (void*)o.pointer != (void*)pointer;
    };
    inline bool operator!=(void* ptr) const{
        return (void*)pointer != ptr;
    };
    template <class U>
    inline bool operator!=(VMPointer<U> o) const{
        return (void*)pointer != (void*)o.GetPointer();
    };

    //neccessary to use VMPointers as keys in std::map
    inline int operator<(VMPointer<T> o) const {
        return (void*)pointer < (void*)o.GetPointer();
    }

    inline T* GetPointer() const{
        return pointer;
    }

protected:
    T* pointer;

};

class AbstractVMObject;

//we need to specialize operator-> if template argument is AbstractVMObject
template<> inline AbstractVMObject* VMPointer<AbstractVMObject>::operator ->() const {
        if (((intptr_t)pointer & 1) != 0) {
            return (AbstractVMObject*)GlobalBox::IntegerBox();
        }
        return pointer;
    };
template<> inline AbstractVMObject* VMPointer<AbstractVMObject>::operator ->(){
        if (((intptr_t)pointer & 1) != 0) {
            return (AbstractVMObject*)GlobalBox::IntegerBox();
        }
        return pointer;
    };

template < class T > std::ostream& operator <<(std::ostream& os, const VMPointer<T>& obj)
{
      os << obj.GetPointer();
      return os;
}

template < class T >
VMPointer<T>& VMPointer<T>::operator =( T& ptr )
{
    /*if (pointer) pointer->DecreaseGCCount();
    (pointer = &ptr)->IncreaseGCCount();*/
    pointer = &ptr;
    return *this;
};

template < class T >
VMPointer<T>& VMPointer<T>::operator =( T* ptr )
{
    /*if (pointer) pointer->DecreaseGCCount();
    (pointer = ptr)->IncreaseGCCount();
    */
    pointer = ptr;
    return *this;
};

template <class T>
VMPointer<T>& VMPointer<T>::operator =(VMPointer<T> ptr)
{
    if(&ptr == this) return *this;
    /*if (pointer) ((VMObject*)pointer)->DecreaseGCCount();
    (pointer = ptr.pointer)->IncreaseGCCount();*/
    pointer = ptr.pointer;
    return *this;
};





#endif


