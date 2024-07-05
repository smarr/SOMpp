#include <misc/ParseInteger.h>

#include <cerrno>
#include <string>

#include <vmobjects/VMInteger.h>
#include <vm/Universe.h>


vm_oop_t ParseInteger(const char* str, int base, bool negateValue) {
  errno = 0;

  char* pEnd {};

  const int64_t i = std::strtoll(str, &pEnd, base);

  if (str == pEnd) {
    // did not parse anything
    return NEW_INT(0);
  }

  const bool rangeError = errno == ERANGE;
  if (rangeError) {
    // TODO: try a big int library
    return NEW_INT(0);
  }

  // the normal case
  if (negateValue) {
    return NEW_INT(-i);
  }
  return NEW_INT(i);
}

vm_oop_t ParseInteger(StdString& str, int base, bool negateValue) {
  return ParseInteger(str.c_str(), base, negateValue);
}
