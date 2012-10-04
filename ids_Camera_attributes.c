#include <Python.h>
#include <structmember.h>
#include <ueye.h>

#include "ids.h"
#include "intobject.h"

static PyObject *ids_Camera_getwidth(ids_Camera *self, void *closure);
static int ids_Camera_setwidth(ids_Camera *self, PyObject *value, void *closure);

static PyObject *ids_Camera_getheight(ids_Camera *self, void *closure);
static int ids_Camera_setheight(ids_Camera *self, PyObject *value, void *closure);

static PyObject *ids_Camera_getpixelclock(ids_Camera *self, void *closure);
static int ids_Camera_setpixelclock(ids_Camera *self, PyObject *value, void *closure);

static PyObject *ids_Camera_getcolor_mode(ids_Camera *self, void *closure);
static int ids_Camera_setcolor_mode(ids_Camera *self, PyObject *value, void *closure);

static PyObject *ids_Camera_getgain(ids_Camera *self, void *closure);
static int ids_Camera_setgain(ids_Camera *self, PyObject *value, void *closure);

static PyObject *ids_Camera_getexposure(ids_Camera *self, void *closure);
static int ids_Camera_setexposure(ids_Camera *self, PyObject *value, void *closure);

PyGetSetDef ids_Camera_getseters[] = {
    {"width", (getter) ids_Camera_getwidth, (setter) ids_Camera_setwidth, "Image width", NULL},
    {"height", (getter) ids_Camera_getheight, (setter) ids_Camera_setheight, "Image height", NULL},
    {"pixelclock", (getter) ids_Camera_getpixelclock, (setter) ids_Camera_setpixelclock, "Pixel Clock of camera", NULL},
    {"color_mode", (getter) ids_Camera_getcolor_mode, (setter) ids_Camera_setcolor_mode, "Color mode of images", NULL},
    {"gain", (getter) ids_Camera_getgain, (setter) ids_Camera_setgain, "Hardware gain (individual RGB gains not yet supported)", NULL},
    {"exposure", (getter) ids_Camera_getexposure, (setter) ids_Camera_setexposure, "Exposure time", NULL},
    {NULL}
};

static PyObject *ids_Camera_getexposure(ids_Camera *self, void *closure) {
    double exposure;
    int ret;
    ret = is_Exposure(self->handle, IS_EXPOSURE_CMD_GET_EXPOSURE, &exposure, sizeof(exposure));
    switch (ret) {
    case IS_SUCCESS:
        return PyFloat_FromDouble(exposure);
        break;
    default:
        PyErr_Format(PyExc_IOError, "Failed to retrieve exposure time from camera. Returned: %d", ret);
    }

    return NULL;
}

static int ids_Camera_setexposure(ids_Camera *self, PyObject *value, void *closure) {
    double exposure;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete attribute 'exposure' (that would be silly)");
        return -1;
    }

    PyObject *flt = PyNumber_Float(value);
    if (flt == NULL) {
        PyErr_SetString(PyExc_TypeError, "Could not convert your crappy arg to a float.");
        Py_DECREF(value);
        return -1;
    }

    exposure = PyFloat_AsDouble(flt); 

    Py_DECREF(flt);

    int ret;
    ret = is_Exposure(self->handle, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*) &exposure, sizeof(exposure));
    switch (ret) {
    case IS_SUCCESS:
        return 0;
        break;
    case IS_INVALID_PARAMETER:
        PyErr_SetString(PyExc_ValueError, "Exposure out of range");
        break;
    default:
        PyErr_SetString(PyExc_IOError, "Failed to set exposure time");
    }

    return -1;
}


static PyObject *ids_Camera_getwidth(ids_Camera *self, void *closure) {
    return PyInt_FromLong(self->width);
}

static int ids_Camera_setwidth(ids_Camera *self, PyObject *value, void *closure) {
    PyErr_SetString(PyExc_NotImplementedError, "Changing image width not yet supported.");
    return -1;
}

static PyObject *ids_Camera_getheight(ids_Camera *self, void *closure) {
    return PyInt_FromLong(self->height);
}

static int ids_Camera_setheight(ids_Camera *self, PyObject *value, void *closure) {
    PyErr_SetString(PyExc_NotImplementedError, "Changing image height not yet supported.");
    return -1;
}

static PyObject *ids_Camera_getpixelclock(ids_Camera *self, void *closure) {
    UINT clock;
    int ret;

    ret = is_PixelClock(self->handle, IS_PIXELCLOCK_CMD_GET, &clock, sizeof(clock));
    switch (ret) {
    case IS_SUCCESS:
        return PyInt_FromLong(clock);
        break;
    default:
        PyErr_SetString(PyExc_IOError, "Failed to retreive pixel clock from camera");
    }

    return NULL;
}

static int ids_Camera_setpixelclock(ids_Camera *self, PyObject *value, void *closure) {
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete attribute 'pixelclock'");
        return -1;
    }

    int clock;

    if (PyInt_Check(value)) {
        clock = (int) PyInt_AsLong(value);
    }
    else if (PyLong_Check(value)) {
        clock = (int) PyLong_AsLong(value);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "Pixel clock must be an int or long.");
        return -1;
    }

    if (clock < 0) {
        PyErr_SetString(PyExc_ValueError, "Pixel clock must be positive.");
        return -1;
    }

    int ret;
    ret = is_PixelClock(self->handle, IS_PIXELCLOCK_CMD_SET, (void*) &clock, sizeof(clock));
    switch (ret) {
    case IS_SUCCESS:
        return 0;
        break;
    case IS_INVALID_PARAMETER:
        PyErr_SetString(PyExc_ValueError, "Pixel clock value out of range");
        break;
    default:
        PyErr_SetString(PyExc_IOError, "Failed to set pixel clock.");
    }

    return -1;
}

static PyObject *ids_Camera_getcolor_mode(ids_Camera *self, void *closure) {
    int color = is_SetColorMode(self->handle, IS_GET_COLOR_MODE);

    return PyInt_FromLong(color);
}

static int ids_Camera_setcolor_mode(ids_Camera *self, PyObject *value, void *closure) {
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete attribute 'color'");
        return -1;
    }

    if (!PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "Color mode must be an int.");
        return -1;
    }

    int color = (int) PyInt_AsLong(value);
    Py_DECREF(value);

    if (self->bitdepth != color_to_bitdepth(color)) {
        PyErr_SetString(PyExc_NotImplementedError, "Changing color mode to different bitdepth not yet supported.");
        return -1;
    }

    if (!set_color_mode(self, color)) {
        return -1;
    }

    return 0;
}

static PyObject *ids_Camera_getgain(ids_Camera *self, void *closure) {
    int gain = is_SetHardwareGain(self->handle, IS_GET_MASTER_GAIN, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);

    return PyInt_FromLong(gain);
}

static int ids_Camera_setgain(ids_Camera *self, PyObject *value, void *closure) {
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete attribute 'gain'");
        return -1;
    }

    if (!PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "Gain must be an int.");
        return -1;
    }

    int gain = (int) PyInt_AsLong(value);
    Py_DECREF(value);

    int ret = is_SetHardwareGain(self->handle, gain, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
    switch (ret) {
    case IS_SUCCESS:
        return 0;
    case IS_INVALID_PARAMETER:
        PyErr_SetString(PyExc_ValueError, "Gain out of range.");
        return -1;
    default:
        PyErr_SetString(PyExc_IOError, "Unable to set gain.");
        return -1;
    }

    return -1;
}
