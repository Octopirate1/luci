#define PY_SSIZE_T_CLEAN

#include <python3.5m/Python.h> // Sometimes just Python.h

#include "luci.h"

static PyObject *method_luci(PyObject *self, PyObject *args)
{
	char *filename = NULL;

	/* Parse arguments */
	if(!PyArg_ParseTuple(args, "s", &filename)) {
		return NULL;
	}

	long long ret = (long long)map_and_process(filename);

	return PyLong_FromLongLong(ret);
}


static PyMethodDef luciMethods[] = {
	{"luci", method_luci, METH_VARARGS, "Python interface for Slippi parser module"},
	{NULL, NULL, 0, NULL}
};


static struct PyModuleDef lucimodule = {
	PyModuleDef_HEAD_INIT,
	"luci",
	"Python interface for the LuCi C library",
	-1,
	luciMethods
};


PyMODINIT_FUNC PyInit_slippi(void)
{
    return PyModule_Create(&lucimodule);
}
