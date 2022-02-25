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

%MappedType StringVector
	/TypeHintIn="Vector[str]",
	 TypeHintOut="List[str]"/
{
%TypeHeaderCode
#include <vector>
typedef std::vector<const char*> StringVector;
%End

%ConvertFromTypeCode
	PyObject* l = PyList_New(sipCpp->size());
	if (!l) return 0;
	
	for (size_t i = 0; i < sipCpp->size(); i++) {
		PyObject* tobj = PyUnicode_FromString((*sipCpp)[i]);
		
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

    std::vector<const char*>* v = new std::vector<const char*>;
	while ((item = PyIter_Next(iter)) != NULL) {
        const char* s = PyUnicode_AsUTF8(item);
		if (PyErr_Occurred()) {
			Py_DECREF(item);
			break;
		}
		
		v->push_back(s);
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

%MappedType C_STR
{
%TypeHeaderCode
    typedef const char* C_STR;
%End

%ConvertFromTypeCode
    return PyUnicode_FromString(*sipCpp);
%End

%ConvertToTypeCode
    if (!sipIsErr) {
        return PyUnicode_Check(sipPy);
    } else {
        (**sipCppPtr) = PyUnicode_AsUTF8(sipPy);
        return 0;
    }
%End
};
