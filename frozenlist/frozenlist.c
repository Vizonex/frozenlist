#ifndef PY_SSIZE_T_CLEAN 
#define PY_SSIZE_T_CLEAN
#endif /* PY_SSIZE_T_CLEAN */


#include <Python.h>
#include "_libfrozenlist/compiler.h"
#include "_libfrozenlist/list.h"
#include "_libfrozenlist/state.h"

#include "_libfrozenlist/pythoncapi_compat.h"


/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

static int
frozenlist_check_frozen(FrozenListObject *self)
{
    if (self->frozen) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot modify frozen list.");
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Construction & Destruction                                         */
/* ------------------------------------------------------------------ */

static int
frozenlist_tp_init(FrozenListObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"items", NULL};
    PyObject *items = NULL;
    PyObject *list;

    PyObject* mod = PyType_GetModuleByDef(Py_TYPE(self), &frozenlist_module);
    if (mod == NULL){
        return -1;
    }
    mod_state* state = get_mod_state(mod);


    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O:FrozenList", kwlist, &items))
        return -1;

    if (items == NULL || items == Py_None) {
        list = PyList_New(0);
    } else {
        list = PySequence_List(items);          /* forces a real list */
        if (list == NULL)
            return -1;
    }

    Py_XDECREF(self->items);
    self->items = list;
    self->frozen = 0;
    /* Need access to copy.deepcopy */
    self->state = state;
    return 0;
}

/* reference to frozenlist_tp_dealloc */
static inline int 
frozenlist_tp_clear(FrozenListObject* self);

static void
frozenlist_tp_dealloc(FrozenListObject *self)
{
    PyObject_GC_UnTrack(self);
    Py_TRASHCAN_BEGIN(self, frozenlist_tp_dealloc)
        frozenlist_tp_clear(self);
        Py_TYPE(self)->tp_free((PyObject *)self);
    Py_TRASHCAN_END
}

/* ------------------------------------------------------------------ */
/* Properties & Methods                                               */
/* ------------------------------------------------------------------ */

static PyObject *
frozenlist_get_frozen(FrozenListObject *self, void *closure)
{
    return PyBool_FromLong(self->frozen);
}

static PyObject *
frozenlist_freeze(FrozenListObject *self, PyObject *Py_UNUSED(ignored))
{
    self->frozen = 1;
    Py_RETURN_NONE;
}

/* ------------------------------------------------------------------ */
/* Sequence protocol                                                  */
/* ------------------------------------------------------------------ */

static Py_ssize_t
frozenlist_length(FrozenListObject *self)
{
    return PyList_GET_SIZE(self->items);
}

static PyObject *
frozenlist_item(FrozenListObject *self, Py_ssize_t i)
{
    return PySequence_GetItem(self->items, i);
}

static int
frozenlist_ass_item(FrozenListObject *self, Py_ssize_t i, PyObject *v)
{
    if (frozenlist_check_frozen(self) < 0)
        return -1;
    if (v == NULL)
        return PySequence_DelItem(self->items, i);
    return PySequence_SetItem(self->items, i, v);
}

static PyObject *
frozenlist_subscript(FrozenListObject *self, PyObject *key)
{
    return PyObject_GetItem(self->items, key);
}

static int
frozenlist_ass_subscript(FrozenListObject *self, PyObject *key, PyObject *value)
{
    if (frozenlist_check_frozen(self) < 0)
        return -1;
    if (value == NULL)
        return PyObject_DelItem(self->items, key);
    return PyObject_SetItem(self->items, key, value);
}

static int
frozenlist_contains(FrozenListObject *self, PyObject *value)
{
    return PySequence_Contains(self->items, value);
}


/* ------------------------------------------------------------------ */
/* MutableSequence Methods                                            */
/* ------------------------------------------------------------------ */

static PyObject *
frozenlist_append(FrozenListObject *self, PyObject *item)
{
    if (frozenlist_check_frozen(self) < 0)
        return NULL;
    if (PyList_Append(self->items, item) < 0)
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *
frozenlist_extend(FrozenListObject *self, PyObject *iterable)
{
    if (frozenlist_check_frozen(self) < 0)
        return NULL;
    if (PyList_Extend(self->items, iterable) < 0){
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
frozenlist_insert(FrozenListObject *self, PyObject *args)
{
    Py_ssize_t index;
    PyObject *item;
    if (!PyArg_ParseTuple(args, "nO:insert", &index, &item))
        return NULL;
    if (frozenlist_check_frozen(self) < 0)
        return NULL;
    if (PyList_Insert(self->items, index, item) < 0)
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *
frozenlist_pop(FrozenListObject *self, PyObject *args)
{
    Py_ssize_t index = -1;
    if (!PyArg_ParseTuple(args, "|n:pop", &index))
        return NULL;
    if (frozenlist_check_frozen(self) < 0)
        return NULL;

    Py_ssize_t size = PyList_GET_SIZE(self->items);
    if (index < 0)
        index += size;
    if (index < 0 || index >= size) {
        PyErr_SetString(PyExc_IndexError, "pop index out of range");
        return NULL;
    }  
    PyObject *item = Py_NewRef(PyList_GET_ITEM(self->items, index));
    if (PyList_SetSlice(self->items, index, index + 1, NULL) < 0) {
        Py_DECREF(item);
        return NULL;
    }
    return item;
}

static PyObject *
frozenlist_remove(FrozenListObject *self, PyObject *item)
{
    if (frozenlist_check_frozen(self) < 0)
        return NULL;
    Py_ssize_t i = PySequence_Index(self->items, item);
    if (i < 0)
        return NULL;
    if (PyList_SetSlice(self->items, i, i + 1, NULL) < 0)
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *
frozenlist_clear(FrozenListObject *self, PyObject *Py_UNUSED(ignored))
{
    if (frozenlist_check_frozen(self) < 0)
        return NULL;
    if (PyList_Clear(self->items) < 0)
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *
frozenlist_reverse(FrozenListObject *self, PyObject *Py_UNUSED(ignored))
{
    if (frozenlist_check_frozen(self) < 0)
        return NULL;
    if (PyList_Reverse(self->items) < 0)
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *
frozenlist_index(FrozenListObject *self, PyObject *args)
{
    PyObject *value;
    Py_ssize_t start = 0, stop = PY_SSIZE_T_MAX;
    if (!PyArg_ParseTuple(args, "O|nn:index", &value, &start, &stop))
        return NULL;
    return PyLong_FromSsize_t(PySequence_Index(self->items, value));
}

static PyObject *
frozenlist_count(FrozenListObject *self, PyObject *value)
{
    return PyLong_FromSsize_t(PySequence_Count(self->items, value));
}



/* ------------------------------------------------------------------ */
/* Other special methods                                              */
/* ------------------------------------------------------------------ */

static PyObject *
frozenlist_repr(FrozenListObject *self)
{
    return PyUnicode_FromFormat("<%s(frozen=%s, %R)>",
                                Py_TYPE(self)->tp_name,
                                self->frozen ? "True" : "False",
                                self->items);
}

static PyObject *
frozenlist_richcompare(FrozenListObject *self, PyObject *other, int op)
{
    PyObject *list_self = PySequence_List((PyObject *)self);
    if (list_self == NULL)
        return NULL;
    PyObject *result = PyObject_RichCompare(list_self, other, op);
    Py_DECREF(list_self);
    return result;
}

static Py_hash_t
frozenlist_hash(FrozenListObject *self)
{
    if (!self->frozen) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot hash unfrozen list.");
        return -1;
    }
    PyObject *tup = PyList_AsTuple(self->items);
    if (tup == NULL)
        return -1;
    Py_hash_t h = PyObject_Hash(tup);
    Py_DECREF(tup);
    return h;
}

static PyObject *
frozenlist_iter(FrozenListObject *self)
{
    return PyObject_GetIter(self->items);
}

static PyObject *
frozenlist_reversed(FrozenListObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyObject_CallMethod(self->items, "__reversed__", NULL);
}

static PyObject *
frozenlist_copy(FrozenListObject *self, PyObject *Py_UNUSED(ignored))
{
    /* Ensure that the new type is doing self.__class__(self._items) */
    FrozenListObject *newobj = (FrozenListObject *)
        PyObject_CallOneArg((PyObject*)Py_TYPE(self), self->items);
    if (newobj == NULL)
        return NULL;
    if (self->frozen)
        newobj->frozen = 1;
    return (PyObject *)newobj;
}


/* Next part is a hack but it allows us to mimic __builtin__.id() excactly 
Will call ours Object_id() (incase python ever implements one for us in C) 
there is no dunder `__id__` method in python so that will always be 
skipped even if existant. Will still audit this function so that nothing
ever feels out of place. */
static PyObject *
Object_id(PyObject *v)
{
    PyObject *id = PyLong_FromVoidPtr(v);

    if ((id != NULL) && (PySys_Audit("builtins.id", "O", id) < 0)) {
        Py_DECREF(id);
        return NULL;
    }
    return id;
}


/* __deepcopy__ longest function in the code. */
static PyObject* 
frozenlist_deepcopy(FrozenListObject* self, PyObject* memo){
    PyObject* obj_id;
    PyObject* has_deepcopy = NULL;
    int in_memo;
    FrozenListObject* new_list;

    obj_id = Object_id((PyObject*)self);
    if (obj_id == NULL){
        Py_DECREF(new_list);
        return NULL;
    }
    if (PyDict_GetItemRef(memo, obj_id, &has_deepcopy) < 0){
        Py_DECREF(obj_id);
        Py_DECREF(new_list);
        return NULL;
    }

    if (has_deepcopy != NULL){
        /* already existant so don't repeat */
        return has_deepcopy;
    }


    /* TODO: Optimization would be to use PyList_New on the number of new items needed */
    new_list = (FrozenListObject *)
        PyObject_CallOneArg((PyObject*)Py_TYPE(self), Py_None);
    
    if (PyDict_SetItem(memo, obj_id, new_list) < 0){
        Py_DECREF(obj_id);
        Py_DECREF(new_list);
        return NULL;
    }


    /* no use in using this now... */
    Py_DECREF(obj_id);

    if (new_list == NULL)
        return NULL;
    
    Py_ssize_t size = PyList_GET_SIZE(self->items); 

    /* This is why mod_state* was needed in the class */
    PyObject* copy_deepcopy = self->state->copy_deepcopy;

    for (Py_ssize_t i = 0; i < size; i++){
        PyObject* result;
        PyObject* item = PyList_GET_ITEM(self->items, i);
        /* ensure item remains with us. */
        Py_INCREF(item);
        result = PyObject_CallFunctionObjArgs(copy_deepcopy, item, memo);
        Py_DECREF(item);

        if (result == NULL){
            Py_DECREF(new_list);
            return NULL;
        }
        
        if (PyList_Append(new_list->items, result) < 0){
            Py_DECREF(result);
            Py_DECREF(new_list);
            return NULL;
        }
    }
    new_list->frozen = self->frozen;
    return new_list;
}


static PyObject *
frozenlist_iadd(FrozenListObject *self, PyObject *other)
{
    if (frozenlist_check_frozen(self) < 0)
        return NULL;
    if (PyList_Extend(self->items, other) < 0)
        return NULL;
    return Py_NewRef(self);
}

static int 
frozenlist_tp_traverse(FrozenListObject* self, visitproc visit, void* arg){
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->items);
    return 0;
}

static inline int 
frozenlist_tp_clear(FrozenListObject* self){
    Py_CLEAR(self->items);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Method table                                                       */
/* ------------------------------------------------------------------ */

static PyMethodDef frozenlist_methods[] = {
    {"freeze",   (PyCFunction)frozenlist_freeze,   METH_NOARGS,  "Freeze the list"},
    {"append",   (PyCFunction)frozenlist_append,   METH_O,       "Append an item"},
    {"extend",   (PyCFunction)frozenlist_extend,   METH_O,       "Extend with iterable"},
    {"insert",   (PyCFunction)frozenlist_insert,   METH_VARARGS, "Insert item at position"},
    {"pop",      (PyCFunction)frozenlist_pop,      METH_VARARGS, "Pop item"},
    {"remove",   (PyCFunction)frozenlist_remove,   METH_O,       "Remove first occurrence"},
    {"clear",    (PyCFunction)frozenlist_clear,    METH_NOARGS,  "Clear the list"},
    {"reverse",  (PyCFunction)frozenlist_reverse,  METH_NOARGS,  "Reverse in place"},
    {"index",    (PyCFunction)frozenlist_index,    METH_VARARGS, "Return index of value"},
    {"count",    (PyCFunction)frozenlist_count,    METH_O,       "Count occurrences"},
    {"__reversed__", (PyCFunction)frozenlist_reversed, METH_NOARGS, NULL},
    {"__copy__", (PyCFunction)frozenlist_copy,     METH_NOARGS,  NULL},
    {"__deepcopy__", (PyCFunction)frozenlist_deepcopy, METH_O, NULL},
    {NULL, NULL, 0, NULL}
};

static PyGetSetDef frozenlist_getset[] = {
    {"frozen", (getter)frozenlist_get_frozen, NULL, "True if the list is frozen", NULL},
    {NULL}
};

static PySequenceMethods frozenlist_as_sequence = {
    .sq_length   = (lenfunc)frozenlist_length,
    .sq_item     = (ssizeargfunc)frozenlist_item,
    .sq_ass_item = (ssizeobjargproc)frozenlist_ass_item,
    .sq_contains = (objobjproc)frozenlist_contains,
};

static PyMappingMethods frozenlist_as_mapping = {
    .mp_length        = (lenfunc)frozenlist_length,
    .mp_subscript     = (binaryfunc)frozenlist_subscript,
    .mp_ass_subscript = (objobjargproc)frozenlist_ass_subscript,
};

static PyNumberMethods frozenlist_as_number = {
    .nb_inplace_add = (binaryfunc)frozenlist_iadd,
};


#ifndef MANAGED_WEAKREFS
static PyMemberDef frozenlist_members[] = {
    {"__weaklistoffset__",
     Py_T_PYSSIZET,
     offsetof(FrozenListObject, weaklist),
     Py_READONLY},
    {NULL} /* Sentinel */
};
#endif

static PyType_Slot frozenlist_slots[] = {
    {Py_tp_dealloc, frozenlist_tp_dealloc},
    {Py_tp_doc, "List-like structure that can be frozen (made immutable)"},
    {Py_tp_traverse, frozenlist_tp_traverse},
    {Py_tp_clear, frozenlist_tp_clear},
    {Py_tp_methods, frozenlist_methods},
    {Py_tp_init, frozenlist_tp_init},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_new, PyType_GenericNew},
    {Py_tp_free, PyObject_GC_Del},
    {Py_tp_getset, frozenlist_getset},
    /* PySequenceMethods */
    {Py_sq_length, (lenfunc)frozenlist_length},
    {Py_sq_item, (ssizeargfunc)frozenlist_item},
    {Py_sq_ass_item, (ssizeobjargproc)frozenlist_ass_item},
    {Py_sq_contains, (objobjproc)frozenlist_contains},
    
    /* PyMappingMethods (Shortcut incase needed) */
    {Py_mp_length, (lenfunc)frozenlist_length},
    {Py_mp_subscript, (binaryfunc)frozenlist_subscript},
    {Py_mp_ass_subscript, (objobjargproc)frozenlist_ass_subscript},

    /* PyNumberMethods */
    {Py_nb_inplace_add, (binaryfunc)frozenlist_iadd},

    {0, NULL},

}

// static PyTypeObject FrozenList_Type = {
//     PyVarObject_HEAD_INIT(NULL, 0)
//     .tp_name      = "frozenlist.FrozenList",
//     .tp_basicsize = sizeof(FrozenListObject),
//     .tp_dealloc   = (destructor)frozenlist_tp_dealloc,
//     .tp_repr      = (reprfunc)frozenlist_repr,
//     .tp_as_number = &frozenlist_as_number,
//     .tp_as_sequence = &frozenlist_as_sequence,
//     .tp_as_mapping  = &frozenlist_as_mapping,
//     .tp_hash      = (hashfunc)frozenlist_hash,
//     .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
//     .tp_doc       = "List-like structure that can be frozen (made immutable)",
//     .tp_richcompare = (richcmpfunc)frozenlist_richcompare,
//     .tp_iter      = (getiterfunc)frozenlist_iter,
//     .tp_methods   = frozenlist_methods,
//     .tp_getset    = frozenlist_getset,
//     .tp_init      = (initproc)frozenlist_tp_init,
//     .tp_new       = PyType_GenericNew,
// };


static int
module_exec(PyObject* mod)
{
    mod_state* state = get_mod_state(mod);
    state->mod = mod;
    state->FrozenListType = &FrozenList_Type;


}


static struct PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, module_exec},
#if PY_VERSION_HEX >= 0x030c00f0
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#endif
#if PY_VERSION_HEX >= 0x030d00f0
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
    {0, NULL},
};


