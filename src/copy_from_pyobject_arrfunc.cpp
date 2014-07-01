//
// Copyright (C) 2011-14 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <Python.h>
#include <datetime.h>

#include "copy_from_pyobject_arrfunc.hpp"
#include "numpy_interop.hpp"
#include "utility_functions.hpp"
#include "type_functions.hpp"
#include "array_functions.hpp"
#include "array_from_py_typededuction.hpp"

#include <dynd/array.hpp>
#include <dynd/types/bytes_type.hpp>
#include <dynd/types/fixedbytes_type.hpp>
#include <dynd/types/string_type.hpp>
#include <dynd/types/date_type.hpp>
#include <dynd/types/time_type.hpp>
#include <dynd/types/datetime_type.hpp>
#include <dynd/types/option_type.hpp>
#include <dynd/types/var_dim_type.hpp>
#include <dynd/types/base_tuple_type.hpp>
#include <dynd/types/base_struct_type.hpp>
#include <dynd/kernels/assignment_kernels.hpp>
#include <dynd/func/copy_arrfunc.hpp>
#include <dynd/func/chain_arrfunc.hpp>
#include <dynd/parser_util.hpp>

using namespace std;
using namespace dynd;
using namespace pydynd;

namespace {
// Initialize the pydatetime API
struct init_pydatetime {
    init_pydatetime() {
        PyDateTime_IMPORT;
    }
};
init_pydatetime pdt;

inline void disallow_pyseq(PyObject *obj, const char *msg)
{
  PyObject *iter = PyObject_GetIter(obj);
  if (iter != NULL) {
    Py_DECREF(iter);
    throw broadcast_error(msg);
  } else {
    PyErr_Clear();
  }
}

struct bool_ck : public kernels::unary_ck<bool_ck> {
  inline void single(char *dst, const char *src)
  {
    PyObject *src_obj = *reinterpret_cast<PyObject *const *>(src);
    disallow_pyseq(src_obj, "Cannot assign a sequence to a dynd bool");
    int result = PyObject_IsTrue(src_obj);
    if (result < 0) {
      throw exception();
    }
    *dst = (result != 0);
  }
};

void pyint_to_int(int8_t *out, PyObject *obj) {
  long v = PyLong_AsLong(obj);
  if (v == -1 && PyErr_Occurred()) {
    throw exception();
  }
  if (parse::overflow_check<int8_t>::is_overflow(v)) {
    throw overflow_error("overflow assigning to dynd int8");
  }
  *out = static_cast<int8_t>(v);
}

void pyint_to_int(int16_t *out, PyObject *obj) {
  long v = PyLong_AsLong(obj);
  if (v == -1 && PyErr_Occurred()) {
    throw exception();
  }
  if (parse::overflow_check<int16_t>::is_overflow(v)) {
    throw overflow_error("overflow assigning to dynd int16");
  }
  *out = static_cast<int16_t>(v);
}

void pyint_to_int(int32_t *out, PyObject *obj) {
  long v = PyLong_AsLong(obj);
  if (v == -1 && PyErr_Occurred()) {
    throw exception();
  }
cout << "v is " << v << endl;
  if (parse::overflow_check<int32_t>::is_overflow(v)) {
    throw overflow_error("overflow assigning to dynd int32");
  }
  *out = static_cast<int32_t>(v);
}

void pyint_to_int(int64_t *out, PyObject *obj) {
  int64_t v = PyLong_AsLongLong(obj);
  if (v == -1 && PyErr_Occurred()) {
    throw exception();
  }
  *out = static_cast<int64_t>(v);
}

void pyint_to_int(dynd_int128 *out, PyObject *obj) {
  uint64_t lo = PyLong_AsUnsignedLongLongMask(obj);
  pyobject_ownref sixtyfour(PyLong_FromLong(64));
  pyobject_ownref value_shr1(PyNumber_Rshift(obj, sixtyfour.get()));
  uint64_t hi = PyLong_AsUnsignedLongLongMask(value_shr1.get());
  dynd_int128 result(hi, lo);

  // Shift right another 64 bits, and check that nothing is remaining
  pyobject_ownref value_shr2(
      PyNumber_Rshift(value_shr1.get(), sixtyfour.get()));
  long remaining = PyLong_AsLong(value_shr2.get());
  if ((remaining != 0 || (remaining == 0 && result.is_negative())) &&
      (remaining != -1 || PyErr_Occurred() ||
       (remaining == -1 && !result.is_negative()))) {
    throw overflow_error("int is too big to fit in an int128");
  }

  *out = result;
}

void pyint_to_int(uint8_t *out, PyObject *obj) {
  unsigned long v = PyLong_AsUnsignedLong(obj);
  if (v == -1 && PyErr_Occurred()) {
    throw exception();
  }
  if (parse::overflow_check<uint8_t>::is_overflow(v)) {
    throw overflow_error("overflow assigning to dynd uint8");
  }
  *out = static_cast<uint8_t>(v);
}

void pyint_to_int(uint16_t *out, PyObject *obj) {
  unsigned long v = PyLong_AsUnsignedLong(obj);
  if (v == -1 && PyErr_Occurred()) {
    throw exception();
  }
  if (parse::overflow_check<uint16_t>::is_overflow(v)) {
    throw overflow_error("overflow assigning to dynd uint16");
  }
  *out = static_cast<uint16_t>(v);
}

void pyint_to_int(uint32_t *out, PyObject *obj) {
  unsigned long v = PyLong_AsUnsignedLong(obj);
  if (v == -1 && PyErr_Occurred()) {
    throw exception();
  }
  if (parse::overflow_check<uint32_t>::is_overflow(v)) {
    throw overflow_error("overflow assigning to dynd uint32");
  }
  *out = static_cast<uint32_t>(v);
}

void pyint_to_int(uint64_t *out, PyObject *obj) {
  uint64_t v = PyLong_AsUnsignedLongLong(obj);
  if (v == -1 && PyErr_Occurred()) {
    throw exception();
  }
  *out = v;
}

void pyint_to_int(dynd_uint128 *out, PyObject *obj) {
  uint64_t lo = PyLong_AsUnsignedLongLongMask(obj);
  pyobject_ownref sixtyfour(PyLong_FromLong(64));
  pyobject_ownref value_shr1(PyNumber_Rshift(obj, sixtyfour.get()));
  uint64_t hi = PyLong_AsUnsignedLongLongMask(value_shr1.get());
  dynd_uint128 result(hi, lo);

  // Shift right another 64 bits, and check that nothing is remaining
  pyobject_ownref value_shr2(
      PyNumber_Rshift(value_shr1.get(), sixtyfour.get()));
  long remaining = PyLong_AsLong(value_shr2.get());
  if (remaining != 0) {
    throw overflow_error("int is too big to fit in an uint128");
  }

  *out = result;
}

template <class T>
struct int_ck : public kernels::unary_ck<int_ck<T> > {
  inline void single(char *dst, const char *src)
  {
    PyObject *src_obj = *reinterpret_cast<PyObject *const *>(src);
    pyint_to_int(reinterpret_cast<T *>(dst), src_obj);
  }
};

template<class T>
struct float_ck : public kernels::unary_ck<float_ck<T> > {
  inline void single(char *dst, const char *src)
  {
    PyObject *src_obj = *reinterpret_cast<PyObject *const *>(src);
    double v = PyFloat_AsDouble(src_obj);
    if (v == -1 && PyErr_Occurred()) {
      throw exception();
    }
    *reinterpret_cast<T *>(dst) = static_cast<T>(v);
  }
};

template <class T>
struct complex_float_ck : public kernels::unary_ck<complex_float_ck<T> > {
  inline void single(char *dst, const char *src)
  {
    PyObject *src_obj = *reinterpret_cast<PyObject *const *>(src);
    Py_complex v = PyComplex_AsCComplex(src_obj);
    if (v.real == -1 && PyErr_Occurred()) {
      throw exception();
    }
    reinterpret_cast<T *>(dst)[0] = static_cast<T>(v.real);
    reinterpret_cast<T *>(dst)[1] = static_cast<T>(v.imag);
  }
};

// TODO: This is not very efficient, could be made better
//       with specialized bytes/fixedbytes versions
struct any_bytes_ck : public kernels::unary_ck<any_bytes_ck> {
  ndt::type m_dst_tp;
  const char *m_dst_arrmeta;
  inline void single(char *dst, const char *src)
  {
    PyObject *src_obj = *reinterpret_cast<PyObject *const *>(src);
    char *pybytes_data = NULL;
    intptr_t pybytes_len = 0;
    if (PyBytes_Check(src_obj)) {
      if (PyBytes_AsStringAndSize(src_obj, &pybytes_data, &pybytes_len) < 0) {
        throw runtime_error("Error getting byte string data");
      }
    } else if (WArray_Check(src_obj)) {
      typed_data_assign(m_dst_tp, m_dst_arrmeta, dst, ((WArray *)src_obj)->v);
      return;
    } else {
      stringstream ss;
      ss << "Cannot assign object " << pyobject_repr(src_obj)
         << " to a dynd bytes value";
      throw invalid_argument(ss.str());
    }

    ndt::type bytes_tp = ndt::make_bytes(1);
    string_type_data bytes_d;
    string_type_arrmeta bytes_md;
    bytes_d.begin = pybytes_data;
    bytes_d.end = pybytes_data + pybytes_len;
    bytes_md.blockref = NULL;

    typed_data_assign(m_dst_tp, m_dst_arrmeta, dst, bytes_tp,
                      reinterpret_cast<const char *>(&bytes_md),
                      reinterpret_cast<const char *>(&bytes_d));
  }
};

// TODO: This is not very efficient, could be made better
//       with specialized versions
struct any_string_ck : public kernels::unary_ck<any_string_ck> {
  ndt::type m_dst_tp;
  const char *m_dst_arrmeta;
  inline void single(char *dst, const char *src)
  {
cout << "processing string " << m_dst_tp << endl;
    PyObject *src_obj = *reinterpret_cast<PyObject *const *>(src);
    char *pybytes_data = NULL;
    intptr_t pybytes_len = 0;
    if (PyUnicode_Check(src_obj)) {
      // Go through UTF8 (was accessing the cpython unicode values directly
      // before, but on Python 3.3 OS X it didn't work correctly.)
      pyobject_ownref utf8(PyUnicode_AsUTF8String(src_obj));
      char *s = NULL;
      Py_ssize_t len = 0;
      if (PyBytes_AsStringAndSize(utf8.get(), &s, &len) < 0) {
        throw exception();
      }

      ndt::type str_dt = ndt::make_string();
      string_type_data str_d;
      string_type_arrmeta str_md;
      str_d.begin = s;
      str_d.end = s + len;
      str_md.blockref = NULL;

      typed_data_assign(m_dst_tp, m_dst_arrmeta, dst, str_dt,
                        reinterpret_cast<const char *>(&str_md),
                        reinterpret_cast<const char *>(&str_d));
#if PY_VERSION_HEX < 0x03000000
    } else if {
      char *pystr_data = NULL;
      intptr_t pystr_len = 0;
      if (PyString_AsStringAndSize(value, &pystr_data, &pystr_len) < 0) {
        throw runtime_error("Error getting string data");
      }

      ndt::type str_dt = ndt::make_string(string_encoding_ascii);
      string_type_data str_d;
      string_type_arrmeta str_md;
      str_d.begin = pystr_data;
      str_d.end = pystr_data + pystr_len;
      str_md.blockref = NULL;

      typed_data_assign(m_dst_tp, m_dst_arrmeta, dst, str_dt,
                        reinterpret_cast<const char *>(&str_md),
                        reinterpret_cast<const char *>(&str_d));
#endif
    } else if (WArray_Check(src_obj)) {
      typed_data_assign(m_dst_tp, m_dst_arrmeta, dst, ((WArray *)src_obj)->v);
      return;
    } else {
      stringstream ss;
      ss << "Cannot assign object " << pyobject_repr(src_obj)
         << " to a dynd bytes value";
      throw invalid_argument(ss.str());
    }

    ndt::type bytes_tp = ndt::make_bytes(1);
    string_type_data bytes_d;
    string_type_arrmeta bytes_md;
    bytes_d.begin = pybytes_data;
    bytes_d.end = pybytes_data + pybytes_len;
    bytes_md.blockref = NULL;

    typed_data_assign(m_dst_tp, m_dst_arrmeta, dst, bytes_tp,
                      reinterpret_cast<const char *>(&bytes_md),
                      reinterpret_cast<const char *>(&bytes_d));
  }
};

struct date_ck : public kernels::unary_ck<date_ck> {
  ndt::type m_dst_tp;
  const char *m_dst_arrmeta;
  inline void single(char *dst, const char *src)
  {
    PyObject *src_obj = *reinterpret_cast<PyObject *const *>(src);
    if (PyDate_Check(src_obj)) {
      const date_type *dd = m_dst_tp.tcast<date_type>();
      dd->set_ymd(m_dst_arrmeta, dst, assign_error_fractional,
                  PyDateTime_GET_YEAR(src_obj), PyDateTime_GET_MONTH(src_obj),
                  PyDateTime_GET_DAY(src_obj));
    } else if (PyDateTime_Check(src_obj)) {
      PyDateTime_DateTime *src_dt = (PyDateTime_DateTime *)src_obj;
      if (src_dt->hastzinfo && src_dt->tzinfo != NULL) {
        throw runtime_error("Converting datetimes with a timezone to dynd "
                            "arrays is not yet supported");
      }
      if (PyDateTime_DATE_GET_HOUR(src_obj) != 0 ||
          PyDateTime_DATE_GET_MINUTE(src_obj) != 0 ||
          PyDateTime_DATE_GET_SECOND(src_obj) != 0 ||
          PyDateTime_DATE_GET_MICROSECOND(src_obj) != 0) {
        stringstream ss;
        ss << "Cannot convert a datetime with non-zero time "
           << pyobject_repr(src_obj) << " to a datetime date";
        throw invalid_argument(ss.str());
      }
      const date_type *dd = m_dst_tp.tcast<date_type>();
      dd->set_ymd(m_dst_arrmeta, dst, assign_error_fractional,
                  PyDateTime_GET_YEAR(src_obj), PyDateTime_GET_MONTH(src_obj),
                  PyDateTime_GET_DAY(src_obj));
    } else if (WArray_Check(src_obj)) {
      typed_data_assign(m_dst_tp, m_dst_arrmeta, dst, ((WArray *)src_obj)->v);
    } else {
      stringstream ss;
      ss << "Cannot assign object " << pyobject_repr(src_obj)
         << " to a dynd date value";
      throw invalid_argument(ss.str());
    }
  }
};

struct time_ck : public kernels::unary_ck<time_ck> {
  ndt::type m_dst_tp;
  const char *m_dst_arrmeta;
  inline void single(char *dst, const char *src)
  {
    PyObject *src_obj = *reinterpret_cast<PyObject *const *>(src);
    if (PyTime_Check(src_obj)) {
      const time_type *tt = m_dst_tp.tcast<time_type>();
      tt->set_time(m_dst_arrmeta, dst, assign_error_fractional,
                   PyDateTime_TIME_GET_HOUR(src_obj),
                   PyDateTime_TIME_GET_MINUTE(src_obj),
                   PyDateTime_TIME_GET_SECOND(src_obj),
                   PyDateTime_TIME_GET_MICROSECOND(src_obj) *
                       DYND_TICKS_PER_MICROSECOND);
    } else if (WArray_Check(src_obj)) {
      typed_data_assign(m_dst_tp, m_dst_arrmeta, dst, ((WArray *)src_obj)->v);
    } else {
      stringstream ss;
      ss << "Cannot assign object " << pyobject_repr(src_obj)
         << " to a dynd time value";
      throw invalid_argument(ss.str());
    }
  }
};

struct datetime_ck : public kernels::unary_ck<datetime_ck> {
  ndt::type m_dst_tp;
  const char *m_dst_arrmeta;
  inline void single(char *dst, const char *src)
  {
    PyObject *src_obj = *reinterpret_cast<PyObject *const *>(src);
    if (PyDateTime_Check(src_obj)) {
      PyDateTime_DateTime *src_dt = (PyDateTime_DateTime *)src_obj;
      if (src_dt->hastzinfo && src_dt->tzinfo != NULL) {
        throw runtime_error("Converting datetimes with a timezone to dynd "
                            "arrays is not yet supported");
      }
      const datetime_type *dd = m_dst_tp.tcast<datetime_type>();
      dd->set_cal(m_dst_arrmeta, dst, assign_error_fractional,
                  PyDateTime_GET_YEAR(src_obj), PyDateTime_GET_MONTH(src_obj),
                  PyDateTime_GET_DAY(src_obj),
                  PyDateTime_DATE_GET_HOUR(src_obj),
                  PyDateTime_DATE_GET_MINUTE(src_obj),
                  PyDateTime_DATE_GET_SECOND(src_obj),
                  PyDateTime_DATE_GET_MICROSECOND(src_obj) * 10);
    } else if (WArray_Check(src_obj)) {
      typed_data_assign(m_dst_tp, m_dst_arrmeta, dst, ((WArray *)src_obj)->v);
    } else {
      stringstream ss;
      ss << "Cannot assign object " << pyobject_repr(src_obj)
         << " to a dynd date value";
      throw invalid_argument(ss.str());
    }
  }
};

struct type_ck : public kernels::unary_ck<type_ck> {
  inline void single(char *dst, const char *src)
  {
    PyObject *src_obj = *reinterpret_cast<PyObject *const *>(src);
    *reinterpret_cast<ndt::type *>(dst) = make_ndt_type_from_pyobject(src_obj);
  }
};

// TODO: Should make a more efficient strided kernel function
struct option_ck : public kernels::unary_ck<option_ck> {
  intptr_t m_copy_value_offset;
  ndt::type m_dst_tp;
  const char *m_dst_arrmeta;

  inline void single(char *dst, const char *src)
  {
    PyObject *src_obj = *reinterpret_cast<PyObject *const *>(src);
    ckernel_prefix *assign_na = get_child_ckernel();
    expr_single_t assign_na_fn = assign_na->get_function<expr_single_t>();
    ckernel_prefix *copy_value = get_child_ckernel(m_copy_value_offset);
    expr_single_t copy_value_fn = copy_value->get_function<expr_single_t>();
    if (src_obj == Py_None) {
      assign_na_fn(dst, NULL, assign_na);
    } else if (WArray_Check(src_obj)) {
      typed_data_assign(m_dst_tp, m_dst_arrmeta, dst, ((WArray *)src_obj)->v);
    } else {
      copy_value_fn(dst, &src, copy_value);
    }
  }

  inline void destruct_children()
  {
    get_child_ckernel()->destroy();
    base.destroy_child_ckernel(m_copy_value_offset);
  }
};

// TODO: Could instantiate the dst_tp -> dst_tp assignment
//       as part of the ckernel instead of dynamically
struct strided_ck : public kernels::unary_ck<strided_ck> {
  intptr_t m_dim_size, m_stride;
  ndt::type m_dst_tp;
  const char *m_dst_arrmeta;
  inline void single(char *dst, const char *src)
  {
    PyObject *src_obj = *reinterpret_cast<PyObject *const *>(src);

    if (WArray_Check(src_obj)) {
      typed_data_assign(m_dst_tp, m_dst_arrmeta, dst, ((WArray *)src_obj)->v);
      return;
    }
#ifdef DYND_NUMPY_INTEROP
    if (PyArray_Check(src_obj)) {
      throw runtime_error("TODO: implement copy_from_pyobject call to numpy assignment");
    }
#endif
    // TODO: PEP 3118 support here

    ckernel_prefix *copy_el = get_child_ckernel();
    expr_strided_t copy_el_fn = copy_el->get_function<expr_strided_t>();
cout << "copy_el function is " << (void *)copy_el_fn << endl;

    // Get the input as an array of PyObject *
    pyobject_ownref src_fast;
    const char *child_src;
    intptr_t child_stride = sizeof(PyObject *);
    intptr_t src_dim_size;
    if (broadcast_as_scalar(m_dst_tp, src_obj)) {
      child_src = src;
      src_dim_size = 1;
    } else {
      src_fast.reset(PySequence_Fast(
          src_obj, "Require a sequence to copy to a dynd dimension"));
      child_src =
          reinterpret_cast<const char *>(PySequence_Fast_ITEMS(src_fast.get()));
      src_dim_size = PySequence_Fast_GET_SIZE(src_fast.get());
    }

    if (src_dim_size != 1 && m_dim_size != src_dim_size) {
        throw broadcast_error(m_dst_tp, m_dst_arrmeta, "python sequence object");
    }
    if (src_dim_size == 1 && m_dim_size > 1) {
      // Copy once from Python, then duplicate that element
cout << "copy el function dst type is " << m_dst_tp << endl;
      intptr_t src_stride = 0;
      copy_el_fn(dst, 0, &src, &src_stride, 1, copy_el);
cout << "copy to all the rest" << endl;
      assignment_strided_ckernel_builder ckb;
      make_assignment_kernel(&ckb, 0, m_dst_tp, m_dst_arrmeta, m_dst_tp,
                             m_dst_arrmeta, kernel_request_strided,
                             &eval::default_eval_context);
      ckb(dst + m_stride, m_stride, dst, 0, m_dim_size - 1);
    } else {
cout << "copy el function dst type is " << m_dst_tp << endl;
      copy_el_fn(dst, m_stride, &child_src, &child_stride, m_dim_size, copy_el);
cout << "copied all" << endl;
    }
    if (PyErr_Occurred()) {
      throw std::exception();
    }
  }

  inline void destruct_children()
  {
    get_child_ckernel()->destroy();
  }
};

struct var_dim_ck : public kernels::unary_ck<var_dim_ck> {
  intptr_t m_offset, m_stride;
  ndt::type m_dst_tp;
  const char *m_dst_arrmeta;
  inline void single(char *dst, const char *src)
  {
    PyObject *src_obj = *reinterpret_cast<PyObject *const *>(src);

    if (WArray_Check(src_obj)) {
      typed_data_assign(m_dst_tp, m_dst_arrmeta, dst, ((WArray *)src_obj)->v);
      return;
    }
#ifdef DYND_NUMPY_INTEROP
    if (PyArray_Check(src_obj)) {
      throw runtime_error("TODO: implement copy_from_pyobject call to numpy assignment");
    }
#endif
    // TODO: PEP 3118 support here

    ckernel_prefix *copy_el = get_child_ckernel();
    expr_strided_t copy_el_fn = copy_el->get_function<expr_strided_t>();

    // Get the input as an array of PyObject *
    pyobject_ownref src_fast;
    const char *child_src;
    intptr_t child_stride = sizeof(PyObject *);
    intptr_t src_dim_size;
    if (broadcast_as_scalar(m_dst_tp, src_obj)) {
      child_src = src;
      src_dim_size = 1;
    } else {
      src_fast.reset(PySequence_Fast(
          src_obj, "Require a sequence to copy to a dynd dimension"));
      child_src =
          reinterpret_cast<const char *>(PySequence_Fast_ITEMS(src_fast.get()));
      src_dim_size = PySequence_Fast_GET_SIZE(src_fast.get());
    }

    // If the var dim element hasn't been allocated, initialize it
    var_dim_type_data *vdd = reinterpret_cast<var_dim_type_data *>(dst);
    if (vdd->begin == NULL) {
      if (m_offset != 0) {
        throw runtime_error("Cannot assign to an uninitialized dynd var_dim "
                            "which has a non-zero offset");
      }
      ndt::var_dim_element_initialize(m_dst_tp, m_dst_arrmeta, dst,
                                      src_dim_size);
    }

    if (src_dim_size != 1 && vdd->size != src_dim_size) {
        throw broadcast_error(m_dst_tp, m_dst_arrmeta, "python sequence object");
    }
    if (src_dim_size == 1 && vdd->size > 1) {
      // Copy once from Python, then duplicate that element
      intptr_t src_stride = 0;
      copy_el_fn(vdd->begin + m_offset, 0, &src, &src_stride, 1, copy_el);
      assignment_strided_ckernel_builder ckb;
      make_assignment_kernel(&ckb, 0, m_dst_tp, m_dst_arrmeta, m_dst_tp,
                             m_dst_arrmeta, kernel_request_strided,
                             &eval::default_eval_context);
      ckb(vdd->begin + m_offset + m_stride, m_stride, vdd->begin + m_offset, 0,
          vdd->size - 1);
    } else {
      copy_el_fn(vdd->begin + m_offset, m_stride, &child_src, &child_stride,
                 vdd->size, copy_el);
    }
    if (PyErr_Occurred()) {
      throw std::exception();
    }
  }

  inline void destruct_children()
  {
    get_child_ckernel()->destroy();
  }
};

// TODO: Should make a more efficient strided kernel function
struct tuple_ck : public kernels::unary_ck<tuple_ck> {
  ndt::type m_dst_tp;
  const char *m_dst_arrmeta;
  vector<intptr_t> m_copy_el_offsets;

  inline void single(char *dst, const char *src)
  {
    PyObject *src_obj = *reinterpret_cast<PyObject *const *>(src);

    if (WArray_Check(src_obj)) {
      typed_data_assign(m_dst_tp, m_dst_arrmeta, dst, ((WArray *)src_obj)->v);
      return;
    }
#ifdef DYND_NUMPY_INTEROP
    if (PyArray_Check(src_obj)) {
      throw runtime_error("TODO: implement copy_from_pyobject call to numpy assignment");
    }
#endif
    // TODO: PEP 3118 support here

    intptr_t field_count = m_dst_tp.tcast<base_tuple_type>()->get_field_count();
    const uintptr_t *field_offsets =
        m_dst_tp.tcast<base_tuple_type>()->get_data_offsets(m_dst_arrmeta);

    // Get the input as an array of PyObject *
    pyobject_ownref src_fast;
    const char *child_src;
    intptr_t child_stride = sizeof(PyObject *);
    intptr_t src_dim_size;
    if (broadcast_as_scalar(m_dst_tp, src_obj)) {
      child_src = src;
      src_dim_size = 1;
    } else {
      src_fast.reset(PySequence_Fast(
          src_obj, "Require a sequence to copy to a dynd struct"));
      child_src =
          reinterpret_cast<const char *>(PySequence_Fast_ITEMS(src_fast.get()));
      src_dim_size = PySequence_Fast_GET_SIZE(src_fast.get());
    }

    if (src_dim_size != 1 && field_count != src_dim_size) {
        throw broadcast_error(m_dst_tp, m_dst_arrmeta, "python sequence object");
    }
    if (src_dim_size == 1) {
      child_stride = 0;
    }
    for (intptr_t i = 0; i < field_count; ++i) {
      ckernel_prefix *copy_el = get_child_ckernel(m_copy_el_offsets[i]);
      expr_single_t copy_el_fn = copy_el->get_function<expr_single_t>();
      char *el_dst = dst + field_offsets[i];
      const char *el_src = child_src + i * child_stride;
      copy_el_fn(el_dst, &el_src, copy_el);
    }
    if (PyErr_Occurred()) {
      throw std::exception();
    }
  }

  inline void destruct_children()
  {
    for (size_t i = 0; i < m_copy_el_offsets.size(); ++i) {
      base.destroy_child_ckernel(m_copy_el_offsets[i]);
    }
  }
};

// TODO: Should make a more efficient strided kernel function
struct struct_ck : public kernels::unary_ck<struct_ck> {
  ndt::type m_dst_tp;
  const char *m_dst_arrmeta;
  vector<intptr_t> m_copy_el_offsets;

  inline void single(char *dst, const char *src)
  {
    PyObject *src_obj = *reinterpret_cast<PyObject *const *>(src);

    if (WArray_Check(src_obj)) {
      typed_data_assign(m_dst_tp, m_dst_arrmeta, dst, ((WArray *)src_obj)->v);
      return;
    }
#ifdef DYND_NUMPY_INTEROP
    if (PyArray_Check(src_obj)) {
      throw runtime_error("TODO: implement copy_from_pyobject call to numpy assignment");
    }
#endif
    // TODO: PEP 3118 support here

    intptr_t field_count = m_dst_tp.tcast<base_tuple_type>()->get_field_count();
    const uintptr_t *field_offsets =
        m_dst_tp.tcast<base_tuple_type>()->get_data_offsets(m_dst_arrmeta);

    if (PyDict_Check(src_obj)) {
      // Keep track of which fields we've seen
      shortvector<bool> populated_fields(field_count);
      memset(populated_fields.get(), 0, sizeof(bool) * field_count);

      PyObject *dict_key = NULL, *dict_value = NULL;
      Py_ssize_t dict_pos = 0;

      while (PyDict_Next(src_obj, &dict_pos, &dict_key, &dict_value)) {
        string name = pystring_as_string(dict_key);
        intptr_t i = m_dst_tp.tcast<base_struct_type>()->get_field_index(name);
        // TODO: Add an error policy of whether to throw an error
        //       or not. For now, just raise an error
        if (i >= 0) {
          ckernel_prefix *copy_el = get_child_ckernel(m_copy_el_offsets[i]);
          expr_single_t copy_el_fn = copy_el->get_function<expr_single_t>();
          char *el_dst = dst + field_offsets[i];
          const char *el_src = reinterpret_cast<const char *>(&dict_value);
          copy_el_fn(el_dst, &el_src, copy_el);
          populated_fields[i] = true;
        } else {
          stringstream ss;
          ss << "Input python dict has key ";
          print_escaped_utf8_string(ss, name);
          ss << ", but no such field is in destination dynd type " << m_dst_tp;
          throw runtime_error(ss.str());
        }
      }

      for (intptr_t i = 0; i < field_count; ++i) {
        if (!populated_fields[i]) {
          stringstream ss;
          ss << "python dict does not contain the field ";
          print_escaped_utf8_string(
              ss, m_dst_tp.tcast<base_struct_type>()->get_field_name(i));
          ss << " as required by the data type " << m_dst_tp;
          throw runtime_error(ss.str());
        }
      }
    } else {
      // Get the input as an array of PyObject *
      pyobject_ownref src_fast;
      const char *child_src;
      intptr_t child_stride = sizeof(PyObject *);
      intptr_t src_dim_size;
      if (broadcast_as_scalar(m_dst_tp, src_obj)) {
        child_src = src;
        src_dim_size = 1;
      } else {
        src_fast.reset(PySequence_Fast(
            src_obj, "Require a sequence to copy to a dynd struct"));
        child_src =
            reinterpret_cast<const char *>(PySequence_Fast_ITEMS(src_fast.get()));
        src_dim_size = PySequence_Fast_GET_SIZE(src_fast.get());
      }

      if (src_dim_size != 1 && field_count != src_dim_size) {
          throw broadcast_error(m_dst_tp, m_dst_arrmeta, "python sequence object");
      }
      if (src_dim_size == 1) {
        child_stride = 0;
      }
      for (intptr_t i = 0; i < field_count; ++i) {
        ckernel_prefix *copy_el = get_child_ckernel(m_copy_el_offsets[i]);
        expr_single_t copy_el_fn = copy_el->get_function<expr_single_t>();
        char *el_dst = dst + field_offsets[i];
        const char *el_src = child_src + i * child_stride;
        copy_el_fn(el_dst, &el_src, copy_el);
      }
    }
    if (PyErr_Occurred()) {
      throw std::exception();
    }
  }

  inline void destruct_children()
  {
    for (size_t i = 0; i < m_copy_el_offsets.size(); ++i) {
      base.destroy_child_ckernel(m_copy_el_offsets[i]);
    }
  }
};

} // anonymous namespace

static intptr_t instantiate_copy_from_pyobject(
    const arrfunc_type_data *self_af, dynd::ckernel_builder *ckb,
    intptr_t ckb_offset, const ndt::type &dst_tp, const char *dst_arrmeta,
    const ndt::type *src_tp, const char *const *src_arrmeta,
    kernel_request_t kernreq, const eval::eval_context *ectx)
{
  if (src_tp[0].get_type_id() != void_type_id) {
    stringstream ss;
    ss << "Cannot instantiate arrfunc with signature ";
    ss << self_af->func_proto << " with types (";
    ss << src_tp[0] << ") -> " << dst_tp;
    throw type_error(ss.str());
  }
cout << "instantiate_copy_from_pyobject with type " << dst_tp << endl;

  switch (dst_tp.get_type_id()) {
  case bool_type_id:
    bool_ck::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case int8_type_id:
    int_ck<int8_t>::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case int16_type_id:
    int_ck<int16_t>::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case int32_type_id:
    int_ck<int32_t>::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case int64_type_id:
    int_ck<int64_t>::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case int128_type_id:
    int_ck<dynd_int128>::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case uint8_type_id:
    int_ck<uint8_t>::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case uint16_type_id:
    int_ck<uint16_t>::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case uint32_type_id:
    int_ck<uint32_t>::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case uint64_type_id:
    int_ck<uint64_t>::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case uint128_type_id:
    int_ck<dynd_uint128>::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case float16_type_id:
    float_ck<dynd_float16>::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case float32_type_id:
    float_ck<float>::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case float64_type_id:
    float_ck<double>::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case complex_float32_type_id:
    complex_float_ck<float>::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case complex_float64_type_id:
    complex_float_ck<double>::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case bytes_type_id:
  case fixedbytes_type_id: {
    any_bytes_ck *self = any_bytes_ck::create_leaf(ckb, kernreq, ckb_offset);
    self->m_dst_tp = dst_tp;
    self->m_dst_arrmeta = dst_arrmeta;
    return ckb_offset;
  }
  case string_type_id:
  case fixedstring_type_id: {
cout << "instantiate string at offset " << ckb_offset << endl;
    any_string_ck *self = any_string_ck::create_leaf(ckb, kernreq, ckb_offset);
    self->m_dst_tp = dst_tp;
    self->m_dst_arrmeta = dst_arrmeta;
cout << "function is " << self->base.function << endl;
    return ckb_offset;
  }
  case date_type_id: {
    date_ck *self = date_ck::create_leaf(ckb, kernreq, ckb_offset);
    self->m_dst_tp = dst_tp;
    self->m_dst_arrmeta = dst_arrmeta;
    return ckb_offset;
  }
  case time_type_id: {
    time_ck *self = time_ck::create_leaf(ckb, kernreq, ckb_offset);
    self->m_dst_tp = dst_tp;
    self->m_dst_arrmeta = dst_arrmeta;
    return ckb_offset;
  }
  case datetime_type_id: {
    datetime_ck *self = datetime_ck::create_leaf(ckb, kernreq, ckb_offset);
    self->m_dst_tp = dst_tp;
    self->m_dst_arrmeta = dst_arrmeta;
    return ckb_offset;
  }
  case type_type_id:
    type_ck::create_leaf(ckb, kernreq, ckb_offset);
    return ckb_offset;
  case option_type_id: {
    intptr_t root_ckb_offset = ckb_offset;
    option_ck *self = option_ck::create(ckb, kernreq, ckb_offset);
    self->m_dst_tp = dst_tp;
    self->m_dst_arrmeta = dst_arrmeta;
    const arrfunc_type_data *assign_na_af =
        dst_tp.tcast<option_type>()->get_assign_na_arrfunc();
    ckb_offset = assign_na_af->instantiate(assign_na_af, ckb, ckb_offset,
                                           dst_tp, dst_arrmeta, NULL, NULL,
                                           kernel_request_single, ectx);
    ckb->ensure_capacity(ckb_offset);
    self = ckb->get_at<option_ck>(root_ckb_offset);
    self->m_copy_value_offset = ckb_offset - root_ckb_offset;
    ckb_offset = self_af->instantiate(
        self_af, ckb, ckb_offset, dst_tp.tcast<option_type>()->get_value_type(),
        dst_arrmeta, src_tp, src_arrmeta, kernel_request_single, ectx);
    return ckb_offset;
  }
  case strided_dim_type_id:
  case fixed_dim_type_id:
  case cfixed_dim_type_id: {
    intptr_t dim_size, stride;
    ndt::type el_tp;
    const char *el_arrmeta;
    if (dst_tp.get_as_strided_dim(dst_arrmeta, dim_size, stride, el_tp,
                                  el_arrmeta)) {
cout << "instantiating strided for type " << dst_tp << endl;
      strided_ck *self = strided_ck::create(ckb, kernreq, ckb_offset);
      self->m_dim_size = dim_size;
      self->m_stride = stride;
      self->m_dst_tp = dst_tp;
      self->m_dst_arrmeta = dst_arrmeta;
cout << "recursive call with type " << el_tp << " ckb_offset is " << ckb_offset << endl;
      return self_af->instantiate(self_af, ckb, ckb_offset, el_tp, el_arrmeta,
                                  src_tp, src_arrmeta, kernel_request_strided,
                                  ectx);
    }
    break;
  }
  case var_dim_type_id: {
    var_dim_ck *self = var_dim_ck::create(ckb, kernreq, ckb_offset);
    self->m_offset =
        reinterpret_cast<const var_dim_type_arrmeta *>(dst_arrmeta)->offset;
    self->m_stride =
        reinterpret_cast<const var_dim_type_arrmeta *>(dst_arrmeta)->stride;
    self->m_dst_tp = dst_tp;
    self->m_dst_arrmeta = dst_arrmeta;
    ndt::type el_tp = dst_tp.tcast<var_dim_type>()->get_element_type();
    const char *el_arrmeta = dst_arrmeta + sizeof(var_dim_type_arrmeta);
    return self_af->instantiate(self_af, ckb, ckb_offset, dst_tp, dst_arrmeta,
                                &el_tp, &el_arrmeta, kernel_request_strided,
                                ectx);
  }
  case tuple_type_id:
  case ctuple_type_id: {
    intptr_t root_ckb_offset = ckb_offset;
    tuple_ck *self = tuple_ck::create(ckb, kernreq, ckb_offset);
    self->m_dst_tp = dst_tp;
    self->m_dst_arrmeta = dst_arrmeta;
    intptr_t field_count = dst_tp.tcast<base_tuple_type>()->get_field_count();
    const ndt::type *field_types =
        dst_tp.tcast<base_tuple_type>()->get_field_types_raw();
    const uintptr_t *arrmeta_offsets =
        dst_tp.tcast<base_tuple_type>()->get_arrmeta_offsets_raw();
    self->m_copy_el_offsets.resize(field_count);
    for (intptr_t i = 0; i < field_count; ++i) {
      ckb->ensure_capacity(ckb_offset);
      self = ckb->get_at<tuple_ck>(root_ckb_offset);
      self->m_copy_el_offsets[i] = ckb_offset - root_ckb_offset;
      const char *field_arrmeta = dst_arrmeta + arrmeta_offsets[i];
      ckb_offset = self_af->instantiate(
          self_af, ckb, ckb_offset, field_types[i], field_arrmeta, src_tp,
          src_arrmeta, kernel_request_single, ectx);
    }
    return ckb_offset;
  }
  case struct_type_id:
  case cstruct_type_id: {
    intptr_t root_ckb_offset = ckb_offset;
    struct_ck *self = struct_ck::create(ckb, kernreq, ckb_offset);
    self->m_dst_tp = dst_tp;
    self->m_dst_arrmeta = dst_arrmeta;
    intptr_t field_count = dst_tp.tcast<base_struct_type>()->get_field_count();
    const ndt::type *field_types =
        dst_tp.tcast<base_struct_type>()->get_field_types_raw();
    const uintptr_t *arrmeta_offsets =
        dst_tp.tcast<base_struct_type>()->get_arrmeta_offsets_raw();
    self->m_copy_el_offsets.resize(field_count);
    for (intptr_t i = 0; i < field_count; ++i) {
      ckb->ensure_capacity(ckb_offset);
      self = ckb->get_at<struct_ck>(root_ckb_offset);
      self->m_copy_el_offsets[i] = ckb_offset - root_ckb_offset;
      const char *field_arrmeta = dst_arrmeta + arrmeta_offsets[i];
      ckb_offset = self_af->instantiate(
          self_af, ckb, ckb_offset, field_types[i], field_arrmeta, src_tp,
          src_arrmeta, kernel_request_single, ectx);
    }
    return ckb_offset;
  }
  }

  stringstream ss;
  ss << "Unable to copy a Python object to dynd value with type " << dst_tp;
  throw invalid_argument(ss.str());
}

static nd::arrfunc make_copy_from_pyobject_arrfunc()
{
  nd::array out_af = nd::empty(ndt::make_arrfunc());
  arrfunc_type_data *af =
      reinterpret_cast<arrfunc_type_data *>(out_af.get_readwrite_originptr());
  af->func_proto = ndt::type("(void) -> A... * T");
  af->instantiate = &instantiate_copy_from_pyobject;
  out_af.flag_as_immutable();
  return out_af;
}

dynd::nd::arrfunc pydynd::copy_from_pyobject =
    make_copy_from_pyobject_arrfunc();
