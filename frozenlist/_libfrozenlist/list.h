#ifndef __LIST_H__
#define __LIST_H__

#ifdef __cplusplus
extern "C" {
#endif 

#include "Python.h"
#include "state.h"
#include <stdbool.h>

#if PY_VERSION_HEX >= 0x030c00f0
#define MANAGED_WEAKREFS
#endif


typedef struct {
    PyObject_HEAD
    PyObject* items;
    bool frozen;
    mod_state* state;
#ifndef MANAGED_WEAKREFS
    PyObject* weaklist;
#endif
} FrozenListObject;

static PyTypeObject FrozenList_Type;



#ifdef __cplusplus
}
#endif 


#endif // __LIST_H__