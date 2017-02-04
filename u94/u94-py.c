#include <Python.h>
#include <u94.h>

static PyObject*
u94_decode(PyObject* self, PyObject* args)
{
    const char* input;
    int input_len;
    char* output;
    Py_ssize_t output_len;
    PyObject* result;
    size_t final_len;

    if (!PyArg_ParseTuple(args, "ns#", &output_len, &input, &input_len))
        return NULL;

    result = PyBytes_FromStringAndSize(NULL, output_len);
    if (!result)
        return NULL;

    output = PyBytes_AS_STRING(result);

    Py_BEGIN_ALLOW_THREADS;

    final_len = u94dec((unsigned char*)output, output_len,
                       (const unsigned char*)input, input_len);

    Py_END_ALLOW_THREADS;

    if (!final_len) {
        PyErr_SetString(PyExc_ValueError, "Invalid input");
        Py_DECREF(output);
        return NULL;
    }

    return result;
}

static PyObject*
u94_encode(PyObject* self, PyObject* args)
{
    const char* input;
    int input_len;
    char* output;
    Py_ssize_t output_len;
    size_t final_len;
    PyObject* result;

    if (!PyArg_ParseTuple(args, "y#", &input, &input_len))
        return NULL;

    /* worst case: */
    output_len = input_len * 16 / 13 + 3;

    result = PyBytes_FromStringAndSize(NULL, output_len);
    if (!result)
        return NULL;

    output = PyBytes_AS_STRING(result);

    Py_BEGIN_ALLOW_THREADS;

    final_len = u94enc((unsigned char*)output, output_len,
                       (const unsigned char*)input, input_len);

    Py_END_ALLOW_THREADS;

    if (!final_len) {
        PyErr_SetString(PyExc_RuntimeError, "Encoding failed");
        Py_DECREF(output);
        return NULL;
    }

    if (_PyBytes_Resize(&result, final_len) != 0)
        return NULL;

    return result;
}

static struct PyMethodDef U94_METHODS[] = {
    {"encode",  u94_encode, METH_VARARGS,
     "u94-encode binary data."},
    {"decode",  u94_decode, METH_VARARGS,
     "Decode u94-encoded data."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static PyModuleDef U94_MODULE = {
    PyModuleDef_HEAD_INIT,
    "u94",
    "u94 encoder and decoder",
    -1,
    U94_METHODS
};

PyMODINIT_FUNC
PyInit_u94(void)
{
    return PyModule_Create(&U94_MODULE);
}
