/*
 * stdvector.sip
 *
 *  Copyright 2022 Clement Savergne <csavergne@yahoo.com>

    This file is part of yasim-avr.

    yasim-avr is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    yasim-avr is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with yasim-avr.  If not, see <http://www.gnu.org/licenses/>.
 */

//=======================================================================================

template<_TYPE_>
%MappedType std::vector<_TYPE_>
    /TypeHintIn="Vector[_TYPE_]",
     TypeHintOut="List[_TYPE_]"/
{
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
    PyObject* l = PyList_New(sipCpp->size());
    if (!l) return 0;

    for (size_t i = 0; i < sipCpp->size(); i++) {
        _TYPE_* t = new _TYPE_((*sipCpp)[i]);
        PyObject* tobj = sipConvertFromNewType(t, sipType__TYPE_, sipTransferObj);

        if (!tobj) {
            delete t;
            Py_DECREF(l);
            return SIP_NULLPTR;
        }

        PyList_SET_ITEM(l, i, tobj);
    }
    return l;
%End

%ConvertToTypeCode
    PyObject *iter = PyObject_GetIter(sipPy);
    PyObject *item;
    int state;

    if (!sipIsErr) {
        Py_XDECREF(iter);
        return (iter && !PyUnicode_Check(sipPy));
    }

    if (!iter) {
        *sipIsErr = 1;
        return 0;
    }

    std::vector<_TYPE_> *v = new std::vector<_TYPE_>;
    while ((item = PyIter_Next(iter)) != NULL) {
        _TYPE_* t = reinterpret_cast<_TYPE_*>(sipForceConvertToType(item,
                                                                    sipType__TYPE_,
                                                                    sipTransferObj,
                                                                    SIP_NOT_NONE,
                                                                    &state,
                                                                    sipIsErr));
        if (*sipIsErr) {
            PyErr_Format(PyExc_TypeError,
                         "index %zd has type '%s' but '_TYPE_' is expected", v->size(),
                         Py_TYPE(item)->tp_name);
            Py_DECREF(item);
            break;
        }

        v->push_back(*t);
        sipReleaseType(t, sipType__TYPE_, state);
        Py_DECREF(item);
    }

    Py_DECREF(iter);

    if (PyErr_Occurred())
        *sipIsErr = 1;

    if (*sipIsErr) {
        delete v;
        return 0;
    } else {
        v->shrink_to_fit();
        *sipCppPtr = v;
        return sipGetState(sipTransferObj);
    }
%End
};


//=======================================================================================

template<_TYPE_>
%MappedType std::vector<_TYPE_*>
    /TypeHintIn="Vector[_TYPE_]",
     TypeHintOut="List[_TYPE_]"/
{
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
    PyObject* l = PyList_New(sipCpp->size());
    if (!l) return 0;

    for (size_t i = 0; i < sipCpp->size(); i++) {
        _TYPE_* t = (*sipCpp)[i];
        PyObject* tobj = sipConvertFromNewType((void *)t, sipType__TYPE_, sipTransferObj);

        if (!tobj) {
            Py_DECREF(l);
            return SIP_NULLPTR;
        }

        PyList_SET_ITEM(l, i, tobj);
    }
    return l;
%End

%ConvertToTypeCode
    PyObject* iter = PyObject_GetIter(sipPy);
    PyObject* item;
    Py_ssize_t index = 0;
    int state;

    if (!sipIsErr) {
        Py_XDECREF(iter);
        return (iter && !PyUnicode_Check(sipPy));
    }

    if (!iter) {
        *sipIsErr = 1;
        return 0;
    }

    std::vector<_TYPE_*>* v = new std::vector<_TYPE_*>;
    while ((item = PyIter_Next(iter)) != NULL) {
        _TYPE_* t = reinterpret_cast<_TYPE_*>(sipForceConvertToType(item,
                                                                    sipType__TYPE_,
                                                                    sipTransferObj,
                                                                    SIP_NOT_NONE,
                                                                    &state,
                                                                    sipIsErr));
        if (*sipIsErr) {
            PyErr_Format(PyExc_TypeError,
                         "index %zd has type '%s' but '_TYPE_' is expected", index,
                         Py_TYPE(item)->tp_name);
            Py_DECREF(item);
            break;
        }

        v->push_back(t);
        sipReleaseType(t, sipType__TYPE_, state);
        Py_DECREF(item);
        index++;
    }

    Py_DECREF(iter);

    if (PyErr_Occurred())
        *sipIsErr = 1;

    if (*sipIsErr) {
        delete v;
        return 0;
    } else {
        v->shrink_to_fit();
        *sipCppPtr = v;
        return sipGetState(sipTransferObj);
    }
%End
};


//=======================================================================================

%MappedType UnsignedInt8Vector
    /TypeHintIn="Vector[unsigned int]",
     TypeHintOut="List[unsigned int]"/
{
%TypeHeaderCode
#include <vector>
typedef std::vector<uint8_t> UnsignedInt8Vector;
#include "utils/buffer_utils.h"
%End

%ConvertFromTypeCode
    PyObject* py = export_to_pybuffer(sipAPI_core, sipCpp->data(), sipCpp->size());
    return py;
%End

%ConvertToTypeCode
    if (!sipIsErr) {
        return PyObject_CheckBuffer(sipPy);
    }

    uint8_t* buf = nullptr;
    size_t size = import_from_pybuffer(sipAPI_core, &buf, sipPy);

    std::vector<uint8_t>* v = new std::vector<uint8_t>(size);
    if (size) {
        memcpy(v->data(), buf, size);
        sipFree(buf);
    }

    *sipCppPtr = v;
    return sipGetState(sipTransferObj);
%End
};


//=======================================================================================

%MappedType UnsignedInt16Vector
    /TypeHintIn="Vector[unsigned int]",
     TypeHintOut="List[unsigned int]"/
{
%TypeHeaderCode
#include <vector>
typedef std::vector<uint16_t> UnsignedInt16Vector;
%End

%ConvertFromTypeCode
    PyObject* l = PyList_New(sipCpp->size());
    if (!l) return 0;

    for (size_t i = 0; i < sipCpp->size(); i++) {
        PyObject* tobj = PyLong_FromUnsignedLong((*sipCpp)[i]);

        if (!tobj) {
            Py_DECREF(l);
            return 0;
        }

        PyList_SET_ITEM(l, i, tobj);
    }
    return l;
%End

%ConvertToTypeCode
    PyObject *iter = PyObject_GetIter(sipPy);
    PyObject *item;

    if (!sipIsErr) {
        Py_XDECREF(iter);
        return (iter && !PyUnicode_Check(sipPy));
    }

    if (!iter) {
        *sipIsErr = 1;
        return 0;
    }

    std::vector<uint16_t>* v = new std::vector<uint16_t>;
    while ((item = PyIter_Next(iter)) != NULL) {
        uint16_t value = sipLong_AsUnsignedShort(item);
        if (PyErr_Occurred()) {
            Py_DECREF(item);
            break;
        }

        v->push_back(value);
        Py_DECREF(item);
    }

    Py_DECREF(iter);

    if (PyErr_Occurred())
        *sipIsErr = 1;

    if (*sipIsErr) {
        delete v;
        return 0;
    } else {
        v->shrink_to_fit();
        *sipCppPtr = v;
        return sipGetState(sipTransferObj);
    }
%End
};


//=======================================================================================

%MappedType UnsignedInt32Vector
    /TypeHintIn="Vector[unsigned int]",
     TypeHintOut="List[unsigned int]"/
{
%TypeHeaderCode
#include <vector>
typedef std::vector<uint32_t> UnsignedInt32Vector;
%End

%ConvertFromTypeCode
    PyObject* l = PyList_New(sipCpp->size());
    if (!l) return 0;

    for (size_t i = 0; i < sipCpp->size(); i++) {
        PyObject* tobj = PyLong_FromUnsignedLong((*sipCpp)[i]);

        if (!tobj) {
            Py_DECREF(l);
            return 0;
        }

        PyList_SET_ITEM(l, i, tobj);
    }
    return l;
%End

%ConvertToTypeCode
    PyObject *iter = PyObject_GetIter(sipPy);
    PyObject *item;

    if (!sipIsErr) {
        Py_XDECREF(iter);
        return (iter && !PyUnicode_Check(sipPy));
    }

    if (!iter) {
        *sipIsErr = 1;
        return 0;
    }

    std::vector<uint32_t>* v = new std::vector<uint32_t>;
    while ((item = PyIter_Next(iter)) != NULL) {
        uint32_t value = sipLong_AsUnsignedLong(item);
        if (PyErr_Occurred()) {
            Py_DECREF(item);
            break;
        }

        v->push_back(value);
        Py_DECREF(item);
    }

    Py_DECREF(iter);

    if (PyErr_Occurred())
        *sipIsErr = 1;

    if (*sipIsErr) {
        delete v;
        return 0;
    } else {
        v->shrink_to_fit();
        *sipCppPtr = v;
        return sipGetState(sipTransferObj);
    }
%End
};


//=======================================================================================

%MappedType UnsignedLongVector
    /TypeHintIn="Vector[unsigned long]",
     TypeHintOut="List[unsigned long]"/
{
%TypeHeaderCode
#include <vector>
typedef std::vector<unsigned long> UnsignedLongVector;
%End

%ConvertFromTypeCode
    PyObject* l = PyList_New(sipCpp->size());
    if (!l) return 0;

    for (size_t i = 0; i < sipCpp->size(); i++) {
        PyObject* tobj = PyLong_FromUnsignedLong((*sipCpp)[i]);

        if (!tobj) {
            Py_DECREF(l);
            return 0;
        }

        PyList_SET_ITEM(l, i, tobj);
    }
    return l;
%End

%ConvertToTypeCode
    PyObject *iter = PyObject_GetIter(sipPy);
    PyObject *item;

    if (!sipIsErr) {
        Py_XDECREF(iter);
        return (iter && !PyUnicode_Check(sipPy));
    }

    if (!iter) {
        *sipIsErr = 1;
        return 0;
    }

    std::vector<unsigned long>* v = new std::vector<unsigned long>;
    while ((item = PyIter_Next(iter)) != NULL) {
        unsigned long value = sipLong_AsUnsignedLong(item);
        if (PyErr_Occurred()) {
            Py_DECREF(item);
            break;
        }

        v->push_back(value);
        Py_DECREF(item);
    }

    Py_DECREF(iter);

    if (PyErr_Occurred())
        *sipIsErr = 1;

    if (*sipIsErr) {
        delete v;
        return 0;
    } else {
        v->shrink_to_fit();
        *sipCppPtr = v;
        return sipGetState(sipTransferObj);
    }
%End
};


//=======================================================================================

%MappedType StdStringVector
    /TypeHintIn="Vector[str]",
     TypeHintOut="List[str]"/
{
%TypeHeaderCode
#include <vector>
#include <string>
typedef std::vector<std::string> StdStringVector;
%End

%ConvertFromTypeCode
    PyObject* l = PyList_New(sipCpp->size());
    if (!l) return 0;

    for (size_t i = 0; i < sipCpp->size(); ++i) {
        PyObject* tobj = PyUnicode_FromString((*sipCpp)[i].c_str());

        if (!tobj) {
            Py_DECREF(l);
            return 0;
        }

        PyList_SET_ITEM(l, i, tobj);
    }
    return l;
%End

%ConvertToTypeCode
    PyObject *iter = PyObject_GetIter(sipPy);
    PyObject *item;

    if (!sipIsErr) {
        if (!iter)
            return 0;

        int ok = 1;
        while (ok && (item = PyIter_Next(iter)) != NULL) {
            ok &= (PyBytes_Check(item) || PyUnicode_Check(item));
            Py_DECREF(item);
        }

        Py_DECREF(iter);
        return ok;
    }

    if (!iter) {
        *sipIsErr = 1;
        return 0;
    }

    StdStringVector* v = new StdStringVector;
    while ((item = PyIter_Next(iter)) != NULL) {
        const char* py_str;
        if (PyUnicode_Check(item))
            py_str = PyUnicode_AsUTF8(item);
        else
            py_str = PyBytes_AsString(item);

        if (!PyErr_Occurred()) {
            std::string s = std::string(py_str);
            v->push_back(s);
        }

        Py_DECREF(item);
    }

    Py_DECREF(iter);

    if (PyErr_Occurred())
        *sipIsErr = 1;

    if (*sipIsErr) {
        //In case of error, do a heap tidy up
        delete v;
        //Nothing to return
        return 0;
    } else {
        //Optimize the size of the vector
        v->shrink_to_fit();
        //Return the instance
        *sipCppPtr = v;
        return sipGetState(sipTransferObj);
    }
%End
};
