# cython: freethreading_compatible = True
# distutils: language = c++

from cpython.bool cimport PyBool_FromLong
from cpython.exc cimport PyErr_SetObject
from cpython.list cimport PyList_Append, PyList_GET_SIZE, PyList_New
from cpython.long cimport PyLong_FromSsize_t
from cpython.object cimport PyObject_GetIter
from cpython.sequence cimport PySequence_Contains, PySequence_Count, PySequence_List
from libcpp.atomic cimport atomic

import copy
import types
from collections.abc import MutableSequence


cdef class FrozenList:
    __class_getitem__ = classmethod(types.GenericAlias)

    cdef atomic[bint] _frozen
    cdef list _items

    def __init__(self, object items=None):
        self._frozen.store(False)
        if items is not None:
            items = PySequence_List(items)
        else:
            items = PyList_New(0)
        self._items = items

    @property
    def frozen(self):
        return PyBool_FromLong(self._frozen.load())

    cdef int _check_frozen(self) except -1:
        if self._frozen.load():
            PyErr_SetObject(RuntimeError, "Cannot modify frozen list.")
            return -1
        return 0

    cdef inline Py_ssize_t _fast_len(self):
        return PyList_GET_SIZE(self._items)

    def freeze(self):
        self._frozen.store(True)

    def __getitem__(self, index):
        return self._items.__getitem__(index)

    def __setitem__(self, index, value):
        self._check_frozen()
        self._items[index] = value

    def __delitem__(self, index):
        self._check_frozen()
        del self._items[index]

    def __len__(self):
        return PyLong_FromSsize_t(self._fast_len())

    def __iter__(self):
        return PyObject_GetIter(self._items)

    def __reversed__(self):
        return self._items.__reversed__()

    def __richcmp__(self, other, op):
        if op == 0:  # <
            return list(self) < other
        if op == 1:  # <=
            return list(self) <= other
        if op == 2:  # ==
            return list(self) == other
        if op == 3:  # !=
            return list(self) != other
        if op == 4:  # >
            return list(self) > other
        if op == 5:  # =>
            return list(self) >= other

    def insert(self, pos, item):
        self._check_frozen()
        self._items.insert(pos, item)

    def __contains__(self, item):
        return PySequence_Contains(self._items, item)

    def __iadd__(self, items):
        self._check_frozen()
        self._items.extend(items)
        return self

    def sort(self, object key = None, bint reverse = False):
        self._check_frozen()
        self._items.sort(key, reverse)

    def index(self, item):
        return self._items.index(item)

    def remove(self, item):
        self._check_frozen()
        self._items.remove(item)

    def clear(self):
        self._check_frozen()
        self._items.clear()

    def extend(self, items):
        self._check_frozen()
        self._items.extend(items)

    def reverse(self):
        self._check_frozen()
        self._items.reverse()

    def pop(self, index=-1):
        self._check_frozen()
        return self._items.pop(index)

    def append(self, item):
        self._check_frozen()
        PyList_Append(self._items, item)

    def count(self, item):
        return PySequence_Count(self._items, item)

    def __repr__(self):
        return '<FrozenList(frozen={}, {!r})>'.format(self._frozen.load(),
                                                      self._items)

    def __hash__(self):
        if self._frozen.load():
            return hash(tuple(self._items))
        else:
            raise RuntimeError("Cannot hash unfrozen list.")

    def __deepcopy__(self, memo):
        cdef FrozenList new_list
        obj_id = id(self)

        # Return existing copy if already processed (circular reference)
        if PySequence_Contains(memo, obj_id):
            return memo[obj_id]

        # Create new instance and register immediately
        new_list = self.__class__([])
        memo[obj_id] = new_list

        # Deep copy items
        new_list._items[:] = [copy.deepcopy(item, memo) for item in self._items]

        # Preserve frozen state
        if self._frozen.load():
            new_list.freeze()

        return new_list


MutableSequence.register(FrozenList)
