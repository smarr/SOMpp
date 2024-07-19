#include "../vmobjects/ObjectFormats.h"
#include "Globals.h"

GCObject* nilObject;
GCObject* trueObject;
GCObject* falseObject;

GCClass* objectClass;
GCClass* classClass;
GCClass* metaClassClass;

GCClass* nilClass;
GCClass* integerClass;
GCClass* arrayClass;
GCClass* methodClass;
GCClass* symbolClass;
GCClass* primitiveClass;
GCClass* stringClass;
GCClass* systemClass;
GCClass* blockClass;
GCClass* doubleClass;

GCClass* trueClass;
GCClass* falseClass;

GCSymbol* symbolIfTrue;
GCSymbol* symbolIfFalse;
