#include <Python.h>

#include "energi.h"

static PyObject *energi_getpowhash(PyObject *self, PyObject *args)
{
    char *output;
    PyObject *value;
#if PY_MAJOR_VERSION >= 3
    PyBytesObject *input;
#else
    PyStringObject *input;
#endif
    if (!PyArg_ParseTuple(args, "S", &input))
        return NULL;
    Py_INCREF(input);
    output = PyMem_Malloc(32);

#if PY_MAJOR_VERSION >= 3
    energi_hash((char *)PyBytes_AsString((PyObject*) input), (int)PyBytes_Size((PyObject*) input), output);
#else
    energi_hash((char *)PyString_AsString((PyObject*) input), (int)PyString_Size((PyObject*) input), output);
#endif
    Py_DECREF(input);
#if PY_MAJOR_VERSION >= 3
    value = Py_BuildValue("y#", output, 32);
#else
    value = Py_BuildValue("s#", output, 32);
#endif
    PyMem_Free(output);
    return value;
}

static PyMethodDef energiMethods[] = {
    { "getPoWHash", energi_getpowhash, METH_VARARGS, "Returns the proof of work hash using energi hash" },
    { NULL, NULL, 0, NULL }
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef energiModule = {
    PyModuleDef_HEAD_INIT,
    "energi_hash",
    "...",
    -1,
    energiMethods
};

PyMODINIT_FUNC PyInit_energi_hash(void) {
    return PyModule_Create(&energiModule);
}

#else

PyMODINIT_FUNC initenergi_hash(void) {
    (void) Py_InitModule("energi_hash", energiMethods);
}
#endif
