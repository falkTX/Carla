#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from ctypes import *

PythonSideFn = CFUNCTYPE(None, c_int)

lib = CDLL("./ctypes-test.so", RTLD_LOCAL)

lib.set_python_side_fn.argtypes = [PythonSideFn]
lib.set_python_side_fn.restype = None
lib.call_python_side_fn.argtypes = None
lib.call_python_side_fn.restype = None

def pyFn(checker):
    print("Python function called from C code, checker:", checker)

_pyFn = PythonSideFn(pyFn)
lib.set_python_side_fn(_pyFn)

print("Python side ready, calling C function now")
lib.call_python_side_fn()
