#ifndef __STATE_H__
#define __STATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <Python.h>
#include "compiler.h"

typedef struct {
    PyTypeObject* FrozenListType;
    PyObject* copy_deepcopy; /* copy.deepcopy */
    PyObject* mod;
} mod_state;

static mod_state*
get_mod_state(PyObject* mod)
{
    mod_state* state = (mod_state*)PyModule_GetState(mod);
    assert(state != NULL);
    return state;
}

static PyModuleDef frozenlist_module;

static int
get_mod_state_by_def_checked(PyObject* self, mod_state** ret)
{
    PyTypeObject* tp = Py_TYPE(self);
    PyObject* mod = PyType_GetModuleByDef(tp, &frozenlist_module);
    if (UNLIKELY(mod == NULL)) {
        *ret = NULL;
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_Clear();
            return 0;
        }
        return -1;
    }
    *ret = get_mod_state(mod);
    return 1;
}

static mod_state*
get_mod_state_by_def(PyObject* self)
{
    PyTypeObject* tp = Py_TYPE(self);
    PyObject* mod = PyType_GetModuleByDef(tp, &frozenlist_module);
    assert(mod != NULL);
    return get_mod_state(mod);
}




#ifdef __cplusplus
}
#endif


#endif // __STATE_H__