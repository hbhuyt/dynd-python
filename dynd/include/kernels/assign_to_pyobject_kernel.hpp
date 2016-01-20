//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/types/fixed_bytes_type.hpp>

using namespace dynd;

namespace detail {

template <type_id_t Arg0ID, type_id_t Arg0BaseID>
struct assign_to_pyobject_kernel;

template <>
struct assign_to_pyobject_kernel<bool_type_id, bool_kind_type_id>
    : nd::base_kernel<
          assign_to_pyobject_kernel<bool_type_id, bool_kind_type_id>, 1> {
  void single(char *dst, char *const *src)
  {
    Py_XDECREF(*reinterpret_cast<PyObject **>(dst));
    *reinterpret_cast<PyObject **>(dst) =
        *reinterpret_cast<bool *>(src[0]) ? Py_True : Py_False;
    Py_INCREF(*reinterpret_cast<PyObject **>(dst));
  }
};

PyObject *pyint_from_int(int8_t v)
{
#if PY_VERSION_HEX >= 0x03000000
  return PyLong_FromLong(v);
#else
  return PyInt_FromLong(v);
#endif
}

PyObject *pyint_from_int(uint8_t v)
{
#if PY_VERSION_HEX >= 0x03000000
  return PyLong_FromLong(v);
#else
  return PyInt_FromLong(v);
#endif
}

PyObject *pyint_from_int(int16_t v)
{
#if PY_VERSION_HEX >= 0x03000000
  return PyLong_FromLong(v);
#else
  return PyInt_FromLong(v);
#endif
}

PyObject *pyint_from_int(uint16_t v)
{
#if PY_VERSION_HEX >= 0x03000000
  return PyLong_FromLong(v);
#else
  return PyInt_FromLong(v);
#endif
}

PyObject *pyint_from_int(int32_t v)
{
#if PY_VERSION_HEX >= 0x03000000
  return PyLong_FromLong(v);
#else
  return PyInt_FromLong(v);
#endif
}

PyObject *pyint_from_int(uint32_t v) { return PyLong_FromUnsignedLong(v); }

#if SIZEOF_LONG == 8
PyObject *pyint_from_int(int64_t v)
{
#if PY_VERSION_HEX >= 0x03000000
  return PyLong_FromLong(v);
#else
  return PyInt_FromLong(v);
#endif
}

PyObject *pyint_from_int(uint64_t v) { return PyLong_FromUnsignedLong(v); }
#else
PyObject *pyint_from_int(int64_t v) { return PyLong_FromLongLong(v); }

PyObject *pyint_from_int(uint64_t v) { return PyLong_FromUnsignedLongLong(v); }
#endif

PyObject *pyint_from_int(const dynd::uint128 &val)
{
  if (val.m_hi == 0ULL) {
    return PyLong_FromUnsignedLongLong(val.m_lo);
  }
  // Use the pynumber methods to shift and or together the 64 bit parts
  pydynd::pyobject_ownref hi(PyLong_FromUnsignedLongLong(val.m_hi));
  pydynd::pyobject_ownref sixtyfour(PyLong_FromLong(64));
  pydynd::pyobject_ownref hi_shifted(PyNumber_Lshift(hi.get(), sixtyfour));
  pydynd::pyobject_ownref lo(PyLong_FromUnsignedLongLong(val.m_lo));
  return PyNumber_Or(hi_shifted.get(), lo.get());
}

PyObject *pyint_from_int(const dynd::int128 &val)
{
  if (val.is_negative()) {
    if (val.m_hi == 0xffffffffffffffffULL &&
        (val.m_hi & 0x8000000000000000ULL) != 0) {
      return PyLong_FromLongLong(static_cast<int64_t>(val.m_lo));
    }
    pydynd::pyobject_ownref absval(
        pyint_from_int(static_cast<dynd::uint128>(-val)));
    return PyNumber_Negative(absval.get());
  }
  else {
    return pyint_from_int(static_cast<dynd::uint128>(val));
  }
}

template <typename T>
struct assign_int_kernel : dynd::nd::base_kernel<assign_int_kernel<T>, 1> {
  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    *dst_obj = pyint_from_int(*reinterpret_cast<const T *>(src[0]));
  }
};

template <>
struct assign_to_pyobject_kernel<dynd::int8_type_id, int_kind_type_id>
    : assign_int_kernel<int8_t> {
};

template <>
struct assign_to_pyobject_kernel<dynd::int16_type_id, int_kind_type_id>
    : assign_int_kernel<int16_t> {
};

template <>
struct assign_to_pyobject_kernel<dynd::int32_type_id, int_kind_type_id>
    : assign_int_kernel<int32_t> {
};

template <>
struct assign_to_pyobject_kernel<dynd::int64_type_id, int_kind_type_id>
    : assign_int_kernel<int64_t> {
};

template <>
struct assign_to_pyobject_kernel<dynd::int128_type_id, int_kind_type_id>
    : assign_int_kernel<dynd::int128> {
};

template <>
struct assign_to_pyobject_kernel<dynd::uint8_type_id, uint_kind_type_id>
    : assign_int_kernel<uint8_t> {
};

template <>
struct assign_to_pyobject_kernel<dynd::uint16_type_id, uint_kind_type_id>
    : assign_int_kernel<uint16_t> {
};

template <>
struct assign_to_pyobject_kernel<dynd::uint32_type_id, uint_kind_type_id>
    : assign_int_kernel<uint32_t> {
};

template <>
struct assign_to_pyobject_kernel<dynd::uint64_type_id, uint_kind_type_id>
    : assign_int_kernel<uint64_t> {
};

template <>
struct assign_to_pyobject_kernel<dynd::uint128_type_id, uint_kind_type_id>
    : assign_int_kernel<dynd::uint128> {
};

template <typename T>
struct float_assign_kernel : dynd::nd::base_kernel<float_assign_kernel<T>, 1> {
  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    *dst_obj = PyFloat_FromDouble(*reinterpret_cast<const T *>(src[0]));
  }
};

template <>
struct assign_to_pyobject_kernel<dynd::float16_type_id, float_kind_type_id>
    : float_assign_kernel<dynd::float16> {
};

template <>
struct assign_to_pyobject_kernel<dynd::float32_type_id, float_kind_type_id>
    : float_assign_kernel<float> {
};

template <>
struct assign_to_pyobject_kernel<dynd::float64_type_id, float_kind_type_id>
    : float_assign_kernel<double> {
};

template <class T>
struct complex_float_assign_kernel
    : dynd::nd::base_kernel<complex_float_assign_kernel<T>, 1> {
  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    const dynd::complex<T> *val =
        reinterpret_cast<const dynd::complex<T> *>(src[0]);
    *dst_obj = PyComplex_FromDoubles(val->real(), val->imag());
  }
};

template <>
struct assign_to_pyobject_kernel<dynd::complex_float32_type_id,
                                 complex_kind_type_id>
    : complex_float_assign_kernel<float> {
};

template <>
struct assign_to_pyobject_kernel<dynd::complex_float64_type_id,
                                 complex_kind_type_id>
    : complex_float_assign_kernel<double> {
};

template <>
struct assign_to_pyobject_kernel<dynd::bytes_type_id, scalar_kind_type_id>
    : dynd::nd::base_kernel<
          assign_to_pyobject_kernel<dynd::bytes_type_id, scalar_kind_type_id>,
          1> {
  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    const dynd::bytes *bd = reinterpret_cast<const dynd::bytes *>(src[0]);
    *dst_obj = PyBytes_FromStringAndSize(bd->begin(), bd->end() - bd->begin());
  }
};

template <>
struct assign_to_pyobject_kernel<dynd::fixed_bytes_type_id, scalar_kind_type_id>
    : dynd::nd::base_kernel<assign_to_pyobject_kernel<dynd::fixed_bytes_type_id,
                                                      scalar_kind_type_id>,
                            1> {
  intptr_t data_size;

  assign_to_pyobject_kernel(intptr_t data_size) : data_size(data_size) {}

  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    *dst_obj = PyBytes_FromStringAndSize(src[0], data_size);
  }

  static intptr_t instantiate(
      char *DYND_UNUSED(static_data), char *DYND_UNUSED(data), void *ckb,
      intptr_t ckb_offset, const dynd::ndt::type &DYND_UNUSED(dst_tp),
      const char *DYND_UNUSED(dst_arrmeta), intptr_t DYND_UNUSED(nsrc),
      const dynd::ndt::type *src_tp,
      const char *const *DYND_UNUSED(src_arrmeta),
      dynd::kernel_request_t kernreq, intptr_t nkwd,
      const dynd::nd::array *DYND_UNUSED(kwds),
      const std::map<std::string, dynd::ndt::type> &DYND_UNUSED(tp_vars))
  {
    assign_to_pyobject_kernel::make(
        ckb, kernreq, ckb_offset,
        src_tp[0].extended<dynd::ndt::fixed_bytes_type>()->get_data_size());
    return ckb_offset;
  }
};

template <>
struct assign_to_pyobject_kernel<dynd::char_type_id, scalar_kind_type_id>
    : dynd::nd::base_kernel<
          assign_to_pyobject_kernel<dynd::char_type_id, scalar_kind_type_id>,
          1> {
  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    *dst_obj = PyUnicode_DecodeUTF32(src[0], 4, NULL, NULL);
  }
};

struct string_ascii_assign_kernel
    : dynd::nd::base_kernel<string_ascii_assign_kernel, 1> {
  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    const dynd::string *sd = reinterpret_cast<const dynd::string *>(src[0]);
    *dst_obj =
        PyUnicode_DecodeASCII(sd->begin(), sd->end() - sd->begin(), NULL);
  }
};

struct string_utf8_assign_kernel
    : dynd::nd::base_kernel<string_utf8_assign_kernel, 1> {
  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    const dynd::string *sd = reinterpret_cast<const dynd::string *>(src[0]);
    *dst_obj = PyUnicode_DecodeUTF8(sd->begin(), sd->end() - sd->begin(), NULL);
  }
};

struct string_utf16_assign_kernel
    : dynd::nd::base_kernel<string_utf16_assign_kernel, 1> {
  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    const dynd::string *sd = reinterpret_cast<const dynd::string *>(src[0]);
    *dst_obj =
        PyUnicode_DecodeUTF16(sd->begin(), sd->end() - sd->begin(), NULL, NULL);
  }
};

struct string_utf32_assign_kernel
    : dynd::nd::base_kernel<string_utf32_assign_kernel, 1> {
  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    const dynd::string *sd = reinterpret_cast<const dynd::string *>(src[0]);
    *dst_obj =
        PyUnicode_DecodeUTF32(sd->begin(), sd->end() - sd->begin(), NULL, NULL);
  }
};

template <>
struct assign_to_pyobject_kernel<dynd::string_type_id, scalar_kind_type_id>
    : dynd::nd::base_kernel<assign_to_pyobject_kernel<dynd::string_type_id,
                                                      scalar_kind_type_id>> {
  static intptr_t
  instantiate(char *static_data, char *data, void *ckb, intptr_t ckb_offset,
              const dynd::ndt::type &dst_tp, const char *dst_arrmeta,
              intptr_t nsrc, const dynd::ndt::type *src_tp,
              const char *const *src_arrmeta, dynd::kernel_request_t kernreq,
              intptr_t nkwd, const dynd::nd::array *kwds,
              const std::map<std::string, dynd::ndt::type> &tp_vars)
  {
    switch (src_tp[0].extended<dynd::ndt::base_string_type>()->get_encoding()) {
    case dynd::string_encoding_ascii:
      return string_ascii_assign_kernel::instantiate(
          static_data, data, ckb, ckb_offset, dst_tp, dst_arrmeta, nsrc, src_tp,
          src_arrmeta, kernreq, nkwd, kwds, tp_vars);
    case dynd::string_encoding_utf_8:
      return string_utf8_assign_kernel::instantiate(
          static_data, data, ckb, ckb_offset, dst_tp, dst_arrmeta, nsrc, src_tp,
          src_arrmeta, kernreq, nkwd, kwds, tp_vars);
    case dynd::string_encoding_ucs_2:
    case dynd::string_encoding_utf_16:
      return string_utf16_assign_kernel::instantiate(
          static_data, data, ckb, ckb_offset, dst_tp, dst_arrmeta, nsrc, src_tp,
          src_arrmeta, kernreq, nkwd, kwds, tp_vars);
    case dynd::string_encoding_utf_32:
      return string_utf32_assign_kernel::instantiate(
          static_data, data, ckb, ckb_offset, dst_tp, dst_arrmeta, nsrc, src_tp,
          src_arrmeta, kernreq, nkwd, kwds, tp_vars);
    default:
      throw std::runtime_error("no string_assign_kernel");
    }
  }
};

struct fixed_string_ascii_assign_kernel
    : dynd::nd::base_kernel<fixed_string_ascii_assign_kernel, 1> {
  intptr_t data_size;

  fixed_string_ascii_assign_kernel(intptr_t data_size) : data_size(data_size) {}

  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    intptr_t size = std::find(src[0], src[0] + data_size, 0) - src[0];
    *dst_obj = PyUnicode_DecodeASCII(src[0], size, NULL);
  }

  static intptr_t instantiate(
      char *DYND_UNUSED(static_data), char *DYND_UNUSED(data), void *ckb,
      intptr_t ckb_offset, const dynd::ndt::type &DYND_UNUSED(dst_tp),
      const char *DYND_UNUSED(dst_arrmeta), intptr_t DYND_UNUSED(nsrc),
      const dynd::ndt::type *src_tp,
      const char *const *DYND_UNUSED(src_arrmeta),
      dynd::kernel_request_t kernreq, intptr_t DYND_UNUSED(nkwd),
      const dynd::nd::array *DYND_UNUSED(kwds),
      const std::map<std::string, dynd::ndt::type> &DYND_UNUSED(tp_vars))
  {
    fixed_string_ascii_assign_kernel::make(ckb, kernreq, ckb_offset,
                                           src_tp[0].get_data_size());
    return ckb_offset;
  }
};

struct fixed_string_utf8_assign_kernel
    : dynd::nd::base_kernel<fixed_string_utf8_assign_kernel, 1> {
  intptr_t data_size;

  fixed_string_utf8_assign_kernel(intptr_t data_size) : data_size(data_size) {}

  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    intptr_t size = std::find(src[0], src[0] + data_size, 0) - src[0];
    *dst_obj = PyUnicode_DecodeUTF8(src[0], size, NULL);
  }

  static intptr_t instantiate(
      char *DYND_UNUSED(static_data), char *DYND_UNUSED(data), void *ckb,
      intptr_t ckb_offset, const dynd::ndt::type &DYND_UNUSED(dst_tp),
      const char *DYND_UNUSED(dst_arrmeta), intptr_t DYND_UNUSED(nsrc),
      const dynd::ndt::type *src_tp,
      const char *const *DYND_UNUSED(src_arrmeta),
      dynd::kernel_request_t kernreq, intptr_t DYND_UNUSED(nkwd),
      const dynd::nd::array *DYND_UNUSED(kwds),
      const std::map<std::string, dynd::ndt::type> &DYND_UNUSED(tp_vars))
  {
    fixed_string_utf8_assign_kernel::make(ckb, kernreq, ckb_offset,
                                          src_tp[0].get_data_size());
    return ckb_offset;
  }
};

struct fixed_string_utf16_assign_kernel
    : dynd::nd::base_kernel<fixed_string_utf16_assign_kernel, 1> {
  intptr_t data_size;

  fixed_string_utf16_assign_kernel(intptr_t data_size) : data_size(data_size) {}

  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    const uint16_t *char16_src = reinterpret_cast<const uint16_t *>(src[0]);
    intptr_t size =
        std::find(char16_src, char16_src + (data_size >> 1), 0) - char16_src;
    *dst_obj = PyUnicode_DecodeUTF16(src[0], size * 2, NULL, NULL);
  }

  static intptr_t instantiate(
      char *DYND_UNUSED(static_data), char *DYND_UNUSED(data), void *ckb,
      intptr_t ckb_offset, const dynd::ndt::type &DYND_UNUSED(dst_tp),
      const char *DYND_UNUSED(dst_arrmeta), intptr_t DYND_UNUSED(nsrc),
      const dynd::ndt::type *src_tp,
      const char *const *DYND_UNUSED(src_arrmeta),
      dynd::kernel_request_t kernreq, intptr_t nkwd,
      const dynd::nd::array *DYND_UNUSED(kwds),
      const std::map<std::string, dynd::ndt::type> &DYND_UNUSED(tp_vars))
  {
    fixed_string_utf16_assign_kernel::make(ckb, kernreq, ckb_offset,
                                           src_tp[0].get_data_size());
    return ckb_offset;
  }
};

struct fixed_string_utf32_assign_kernel
    : dynd::nd::base_kernel<fixed_string_utf32_assign_kernel, 1> {
  intptr_t data_size;

  fixed_string_utf32_assign_kernel(intptr_t data_size) : data_size(data_size) {}

  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    const uint32_t *char32_src = reinterpret_cast<const uint32_t *>(src[0]);
    intptr_t size =
        std::find(char32_src, char32_src + (data_size >> 2), 0) - char32_src;
    *dst_obj = PyUnicode_DecodeUTF32(src[0], size * 4, NULL, NULL);
  }

  static intptr_t instantiate(
      char *DYND_UNUSED(static_data), char *DYND_UNUSED(data), void *ckb,
      intptr_t ckb_offset, const dynd::ndt::type &DYND_UNUSED(dst_tp),
      const char *DYND_UNUSED(dst_arrmeta), intptr_t DYND_UNUSED(nsrc),
      const dynd::ndt::type *src_tp,
      const char *const *DYND_UNUSED(src_arrmeta),
      dynd::kernel_request_t kernreq, intptr_t nkwd,
      const dynd::nd::array *DYND_UNUSED(kwds),
      const std::map<std::string, dynd::ndt::type> &DYND_UNUSED(tp_vars))
  {
    fixed_string_utf32_assign_kernel::make(ckb, kernreq, ckb_offset,
                                           src_tp[0].get_data_size());
    return ckb_offset;
  }
};

template <>
struct assign_to_pyobject_kernel<dynd::fixed_string_type_id,
                                 scalar_kind_type_id>
    : dynd::nd::base_kernel<assign_to_pyobject_kernel<
          dynd::fixed_string_type_id, scalar_kind_type_id>> {
  static intptr_t
  instantiate(char *static_data, char *data, void *ckb, intptr_t ckb_offset,
              const dynd::ndt::type &dst_tp, const char *dst_arrmeta,
              intptr_t nsrc, const dynd::ndt::type *src_tp,
              const char *const *src_arrmeta, dynd::kernel_request_t kernreq,
              intptr_t nkwd, const dynd::nd::array *kwds,
              const std::map<std::string, dynd::ndt::type> &tp_vars)
  {
    switch (src_tp[0].extended<dynd::ndt::base_string_type>()->get_encoding()) {
    case dynd::string_encoding_ascii:
      return fixed_string_ascii_assign_kernel::instantiate(
          static_data, data, ckb, ckb_offset, dst_tp, dst_arrmeta, nsrc, src_tp,
          src_arrmeta, kernreq, nkwd, kwds, tp_vars);
    case dynd::string_encoding_utf_8:
      return fixed_string_utf8_assign_kernel::instantiate(
          static_data, data, ckb, ckb_offset, dst_tp, dst_arrmeta, nsrc, src_tp,
          src_arrmeta, kernreq, nkwd, kwds, tp_vars);
    case dynd::string_encoding_ucs_2:
    case dynd::string_encoding_utf_16:
      return fixed_string_utf16_assign_kernel::instantiate(
          static_data, data, ckb, ckb_offset, dst_tp, dst_arrmeta, nsrc, src_tp,
          src_arrmeta, kernreq, nkwd, kwds, tp_vars);
    case dynd::string_encoding_utf_32:
      return fixed_string_utf32_assign_kernel::instantiate(
          static_data, data, ckb, ckb_offset, dst_tp, dst_arrmeta, nsrc, src_tp,
          src_arrmeta, kernreq, nkwd, kwds, tp_vars);
    default:
      throw std::runtime_error("no fixed_string_assign_kernel");
    }
  }
};

template <>
struct assign_to_pyobject_kernel<dynd::date_type_id, scalar_kind_type_id>
    : dynd::nd::base_kernel<
          assign_to_pyobject_kernel<dynd::date_type_id, scalar_kind_type_id>,
          1> {
  dynd::ndt::type src_tp;
  const char *src_arrmeta;

  assign_to_pyobject_kernel(dynd::ndt::type src_tp, const char *src_arrmeta)
      : src_tp(src_tp), src_arrmeta(src_arrmeta)
  {
  }

  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    const dynd::ndt::date_type *dd = src_tp.extended<dynd::ndt::date_type>();
    dynd::date_ymd ymd = dd->get_ymd(src_arrmeta, src[0]);
    *dst_obj = PyDate_FromDate(ymd.year, ymd.month, ymd.day);
  }

  static intptr_t instantiate(
      char *DYND_UNUSED(static_data), char *DYND_UNUSED(data), void *ckb,
      intptr_t ckb_offset, const dynd::ndt::type &DYND_UNUSED(dst_tp),
      const char *DYND_UNUSED(dst_arrmeta), intptr_t DYND_UNUSED(nsrc),
      const dynd::ndt::type *src_tp, const char *const *src_arrmeta,
      dynd::kernel_request_t kernreq, intptr_t nkwd,
      const dynd::nd::array *DYND_UNUSED(kwds),
      const std::map<std::string, dynd::ndt::type> &DYND_UNUSED(tp_vars))
  {
    assign_to_pyobject_kernel::make(ckb, kernreq, ckb_offset, src_tp[0],
                                    src_arrmeta[0]);
    return ckb_offset;
  }
};

template <>
struct assign_to_pyobject_kernel<dynd::time_type_id, scalar_kind_type_id>
    : dynd::nd::base_kernel<
          assign_to_pyobject_kernel<dynd::time_type_id, scalar_kind_type_id>,
          1> {
  dynd::ndt::type src_tp;
  const char *src_arrmeta;

  assign_to_pyobject_kernel(dynd::ndt::type src_tp, const char *src_arrmeta)
      : src_tp(src_tp), src_arrmeta(src_arrmeta)
  {
  }

  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    const dynd::ndt::time_type *tt = src_tp.extended<dynd::ndt::time_type>();
    dynd::time_hmst hmst = tt->get_time(src_arrmeta, src[0]);
    *dst_obj = PyTime_FromTime(hmst.hour, hmst.minute, hmst.second,
                               hmst.tick / DYND_TICKS_PER_MICROSECOND);
  }

  static intptr_t instantiate(
      char *DYND_UNUSED(static_data), char *DYND_UNUSED(data), void *ckb,
      intptr_t ckb_offset, const dynd::ndt::type &DYND_UNUSED(dst_tp),
      const char *DYND_UNUSED(dst_arrmeta), intptr_t DYND_UNUSED(nsrc),
      const dynd::ndt::type *src_tp, const char *const *src_arrmeta,
      dynd::kernel_request_t kernreq, intptr_t nkwd,
      const dynd::nd::array *DYND_UNUSED(kwds),
      const std::map<std::string, dynd::ndt::type> &DYND_UNUSED(tp_vars))
  {
    assign_to_pyobject_kernel::make(ckb, kernreq, ckb_offset, src_tp[0],
                                    src_arrmeta[0]);
    return ckb_offset;
  }
};

template <>
struct assign_to_pyobject_kernel<dynd::datetime_type_id, scalar_kind_type_id>
    : dynd::nd::base_kernel<assign_to_pyobject_kernel<dynd::datetime_type_id,
                                                      scalar_kind_type_id>,
                            1> {
  dynd::ndt::type src_tp;
  const char *src_arrmeta;

  assign_to_pyobject_kernel(dynd::ndt::type src_tp, const char *src_arrmeta)
      : src_tp(src_tp), src_arrmeta(src_arrmeta)
  {
  }

  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    const dynd::ndt::datetime_type *dd =
        src_tp.extended<dynd::ndt::datetime_type>();
    int32_t year, month, day, hour, minute, second, tick;
    dd->get_cal(src_arrmeta, src[0], year, month, day, hour, minute, second,
                tick);
    int32_t usecond = tick / 10;
    *dst_obj = PyDateTime_FromDateAndTime(year, month, day, hour, minute,
                                          second, usecond);
  }

  static intptr_t instantiate(
      char *DYND_UNUSED(static_data), char *DYND_UNUSED(data), void *ckb,
      intptr_t ckb_offset, const dynd::ndt::type &DYND_UNUSED(dst_tp),
      const char *DYND_UNUSED(dst_arrmeta), intptr_t DYND_UNUSED(nsrc),
      const dynd::ndt::type *src_tp, const char *const *src_arrmeta,
      dynd::kernel_request_t kernreq, intptr_t nkwd,
      const dynd::nd::array *DYND_UNUSED(kwds),
      const std::map<std::string, dynd::ndt::type> &DYND_UNUSED(tp_vars))
  {
    assign_to_pyobject_kernel::make(ckb, kernreq, ckb_offset, src_tp[0],
                                    src_arrmeta[0]);
    return ckb_offset;
  }
};

// TODO: Should make a more efficient strided kernel function
template <>
struct assign_to_pyobject_kernel<dynd::option_type_id, any_kind_type_id>
    : dynd::nd::base_kernel<
          assign_to_pyobject_kernel<dynd::option_type_id, any_kind_type_id>,
          1> {
  intptr_t m_assign_value_offset;

  ~assign_to_pyobject_kernel()
  {
    get_child()->destroy();
    get_child(m_assign_value_offset)->destroy();
  }

  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    dynd::ckernel_prefix *is_missing = get_child();
    dynd::kernel_single_t is_missing_fn =
        is_missing->get_function<dynd::kernel_single_t>();
    dynd::ckernel_prefix *assign_value = get_child(m_assign_value_offset);
    dynd::kernel_single_t assign_value_fn =
        assign_value->get_function<dynd::kernel_single_t>();
    char value_is_missing = 1;
    is_missing_fn(is_missing, &value_is_missing, src);
    if (value_is_missing == 0) {
      assign_value_fn(assign_value, dst, src);
    }
    else {
      *dst_obj = Py_None;
      Py_INCREF(*dst_obj);
    }
  }

  static intptr_t
  instantiate(char *static_data, char *data, void *ckb, intptr_t ckb_offset,
              const dynd::ndt::type &dst_tp, const char *dst_arrmeta,
              intptr_t nsrc, const dynd::ndt::type *src_tp,
              const char *const *src_arrmeta, dynd::kernel_request_t kernreq,
              intptr_t nkwd, const dynd::nd::array *kwds,
              const std::map<std::string, dynd::ndt::type> &tp_vars)
  {
    intptr_t root_ckb_offset = ckb_offset;
    assign_to_pyobject_kernel *self_ck =
        assign_to_pyobject_kernel::make(ckb, kernreq, ckb_offset);
    dynd::nd::callable &is_missing = dynd::nd::is_missing::get();
    ckb_offset = is_missing.get()->instantiate(
        is_missing.get()->static_data(), NULL, ckb, ckb_offset,
        dynd::ndt::make_type<dynd::bool1>(), NULL, nsrc, src_tp, src_arrmeta,
        dynd::kernel_request_single, 0, NULL, tp_vars);
    reinterpret_cast<dynd::ckernel_builder<dynd::kernel_request_host> *>(ckb)
        ->reserve(ckb_offset);
    self_ck =
        reinterpret_cast<dynd::ckernel_builder<dynd::kernel_request_host> *>(
            ckb)
            ->get_at<assign_to_pyobject_kernel>(root_ckb_offset);
    self_ck->m_assign_value_offset = ckb_offset - root_ckb_offset;
    dynd::ndt::type src_value_tp =
        src_tp[0].extended<dynd::ndt::option_type>()->get_value_type();
    ckb_offset = nd::assign::get()->instantiate(
        nd::assign::get()->static_data(), NULL, ckb, ckb_offset, dst_tp,
        dst_arrmeta, nsrc, &src_value_tp, src_arrmeta,
        dynd::kernel_request_single, 0, NULL, tp_vars);
    return ckb_offset;
  }
};

template <>
struct assign_to_pyobject_kernel<dynd::type_type_id, any_kind_type_id>
    : dynd::nd::base_kernel<
          assign_to_pyobject_kernel<dynd::type_type_id, any_kind_type_id>, 1> {
  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    dynd::ndt::type tp = *reinterpret_cast<const dynd::ndt::type *>(src[0]);
    *dst_obj = DyND_PyWrapper_New(std::move(tp));
  }
};

// TODO: Should make a more efficient strided kernel function
template <>
struct assign_to_pyobject_kernel<dynd::tuple_type_id, scalar_kind_type_id>
    : dynd::nd::base_kernel<
          assign_to_pyobject_kernel<dynd::tuple_type_id, scalar_kind_type_id>,
          1> {
  dynd::ndt::type src_tp;
  const char *src_arrmeta;
  std::vector<intptr_t> m_copy_el_offsets;

  assign_to_pyobject_kernel(dynd::ndt::type src_tp, const char *src_arrmeta)
      : src_tp(src_tp), src_arrmeta(src_arrmeta)
  {
  }

  ~assign_to_pyobject_kernel()
  {
    for (size_t i = 0; i < m_copy_el_offsets.size(); ++i) {
      get_child(m_copy_el_offsets[i])->destroy();
    }
  }

  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    intptr_t field_count =
        src_tp.extended<dynd::ndt::tuple_type>()->get_field_count();
    const uintptr_t *field_offsets =
        src_tp.extended<dynd::ndt::tuple_type>()->get_data_offsets(src_arrmeta);
    pydynd::pyobject_ownref tup(PyTuple_New(field_count));
    for (intptr_t i = 0; i < field_count; ++i) {
      ckernel_prefix *copy_el = get_child(m_copy_el_offsets[i]);
      dynd::kernel_single_t copy_el_fn =
          copy_el->get_function<dynd::kernel_single_t>();
      char *el_src = src[0] + field_offsets[i];
      char *el_dst =
          reinterpret_cast<char *>(((PyTupleObject *)tup.get())->ob_item + i);
      copy_el_fn(copy_el, el_dst, &el_src);
    }
    if (PyErr_Occurred()) {
      throw std::exception();
    }
    *dst_obj = tup.release();
  }

  static intptr_t
  instantiate(char *static_data, char *data, void *ckb, intptr_t ckb_offset,
              const dynd::ndt::type &dst_tp, const char *dst_arrmeta,
              intptr_t nsrc, const dynd::ndt::type *src_tp,
              const char *const *src_arrmeta, dynd::kernel_request_t kernreq,
              intptr_t nkwd, const dynd::nd::array *kwds,
              const std::map<std::string, dynd::ndt::type> &tp_vars)
  {
    intptr_t root_ckb_offset = ckb_offset;
    assign_to_pyobject_kernel *self_ck = assign_to_pyobject_kernel::make(
        ckb, kernreq, ckb_offset, src_tp[0], src_arrmeta[0]);
    intptr_t field_count =
        src_tp[0].extended<dynd::ndt::tuple_type>()->get_field_count();
    const dynd::ndt::type *field_types =
        src_tp[0].extended<dynd::ndt::tuple_type>()->get_field_types_raw();
    const uintptr_t *arrmeta_offsets =
        src_tp[0].extended<dynd::ndt::tuple_type>()->get_arrmeta_offsets_raw();
    self_ck->m_copy_el_offsets.resize(field_count);
    for (intptr_t i = 0; i < field_count; ++i) {
      reinterpret_cast<dynd::ckernel_builder<dynd::kernel_request_host> *>(ckb)
          ->reserve(ckb_offset);
      self_ck =
          reinterpret_cast<dynd::ckernel_builder<dynd::kernel_request_host> *>(
              ckb)
              ->get_at<assign_to_pyobject_kernel>(root_ckb_offset);
      self_ck->m_copy_el_offsets[i] = ckb_offset - root_ckb_offset;
      const char *field_arrmeta = src_arrmeta[0] + arrmeta_offsets[i];
      ckb_offset = nd::assign::get()->instantiate(
          nd::assign::get()->static_data(), data, ckb, ckb_offset, dst_tp,
          dst_arrmeta, nsrc, &field_types[i], &field_arrmeta,
          dynd::kernel_request_single, nkwd, kwds, tp_vars);
    }
    return ckb_offset;
  }
};

// TODO: Should make a more efficient strided kernel function
template <>
struct assign_to_pyobject_kernel<dynd::struct_type_id, tuple_type_id>
    : dynd::nd::base_kernel<
          assign_to_pyobject_kernel<dynd::struct_type_id, tuple_type_id>, 1> {
  dynd::ndt::type m_src_tp;
  const char *m_src_arrmeta;
  std::vector<intptr_t> m_copy_el_offsets;
  pydynd::pyobject_ownref m_field_names;

  ~assign_to_pyobject_kernel()
  {
    for (size_t i = 0; i < m_copy_el_offsets.size(); ++i) {
      get_child(m_copy_el_offsets[i])->destroy();
    }
  }

  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    intptr_t field_count =
        m_src_tp.extended<dynd::ndt::tuple_type>()->get_field_count();
    const uintptr_t *field_offsets =
        m_src_tp.extended<dynd::ndt::tuple_type>()->get_data_offsets(
            m_src_arrmeta);
    pydynd::pyobject_ownref dct(PyDict_New());
    for (intptr_t i = 0; i < field_count; ++i) {
      dynd::ckernel_prefix *copy_el = get_child(m_copy_el_offsets[i]);
      dynd::kernel_single_t copy_el_fn =
          copy_el->get_function<dynd::kernel_single_t>();
      char *el_src = src[0] + field_offsets[i];
      pydynd::pyobject_ownref el;
      copy_el_fn(copy_el, reinterpret_cast<char *>(el.obj_addr()), &el_src);
      PyDict_SetItem(dct.get(), PyTuple_GET_ITEM(m_field_names.get(), i),
                     el.get());
    }
    if (PyErr_Occurred()) {
      throw std::exception();
    }
    *dst_obj = dct.release();
  }

  static intptr_t
  instantiate(char *static_data, char *data, void *ckb, intptr_t ckb_offset,
              const dynd::ndt::type &dst_tp, const char *dst_arrmeta,
              intptr_t nsrc, const dynd::ndt::type *src_tp,
              const char *const *src_arrmeta, dynd::kernel_request_t kernreq,
              intptr_t nkwd, const dynd::nd::array *kwds,
              const std::map<std::string, dynd::ndt::type> &tp_vars)
  {
    intptr_t root_ckb_offset = ckb_offset;
    assign_to_pyobject_kernel *self_ck =
        assign_to_pyobject_kernel::make(ckb, kernreq, ckb_offset);
    self_ck->m_src_tp = src_tp[0];
    self_ck->m_src_arrmeta = src_arrmeta[0];
    intptr_t field_count =
        src_tp[0].extended<dynd::ndt::struct_type>()->get_field_count();
    const dynd::ndt::type *field_types =
        src_tp[0].extended<dynd::ndt::struct_type>()->get_field_types_raw();
    const uintptr_t *arrmeta_offsets =
        src_tp[0].extended<dynd::ndt::struct_type>()->get_arrmeta_offsets_raw();
    self_ck->m_field_names.reset(PyTuple_New(field_count));
    for (intptr_t i = 0; i < field_count; ++i) {
      const dynd::string &rawname =
          src_tp[0].extended<dynd::ndt::struct_type>()->get_field_name_raw(i);
      pydynd::pyobject_ownref name(PyUnicode_DecodeUTF8(
          rawname.begin(), rawname.end() - rawname.begin(), NULL));
      PyTuple_SET_ITEM(self_ck->m_field_names.get(), i, name.release());
    }
    self_ck->m_copy_el_offsets.resize(field_count);
    for (intptr_t i = 0; i < field_count; ++i) {
      reinterpret_cast<dynd::ckernel_builder<dynd::kernel_request_host> *>(ckb)
          ->reserve(ckb_offset);
      self_ck =
          reinterpret_cast<dynd::ckernel_builder<dynd::kernel_request_host> *>(
              ckb)
              ->get_at<assign_to_pyobject_kernel>(root_ckb_offset);
      self_ck->m_copy_el_offsets[i] = ckb_offset - root_ckb_offset;
      const char *field_arrmeta = src_arrmeta[0] + arrmeta_offsets[i];
      ckb_offset = nd::assign::get()->instantiate(
          nd::assign::get()->static_data(), NULL, ckb, ckb_offset, dst_tp,
          dst_arrmeta, nsrc, &field_types[i], &field_arrmeta,
          dynd::kernel_request_single, 0, NULL, tp_vars);
    }
    return ckb_offset;
  }
};

template <>
struct assign_to_pyobject_kernel<dynd::fixed_dim_type_id, dim_kind_type_id>
    : dynd::nd::base_kernel<
          assign_to_pyobject_kernel<dynd::fixed_dim_type_id, dim_kind_type_id>,
          1> {
  intptr_t dim_size, stride;

  assign_to_pyobject_kernel(intptr_t dim_size, intptr_t stride)
      : dim_size(dim_size), stride(stride)
  {
  }

  ~assign_to_pyobject_kernel() { get_child()->destroy(); }

  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    pydynd::pyobject_ownref lst(PyList_New(dim_size));
    ckernel_prefix *copy_el = get_child();
    dynd::kernel_strided_t copy_el_fn =
        copy_el->get_function<dynd::kernel_strided_t>();
    copy_el_fn(copy_el,
               reinterpret_cast<char *>(((PyListObject *)lst.get())->ob_item),
               sizeof(PyObject *), src, &stride, dim_size);
    if (PyErr_Occurred()) {
      throw std::exception();
    }
    *dst_obj = lst.release();
  }

  static intptr_t
  instantiate(char *static_data, char *data, void *ckb, intptr_t ckb_offset,
              const dynd::ndt::type &dst_tp, const char *dst_arrmeta,
              intptr_t nsrc, const dynd::ndt::type *src_tp,
              const char *const *src_arrmeta, dynd::kernel_request_t kernreq,
              intptr_t nkwd, const dynd::nd::array *kwds,
              const std::map<std::string, dynd::ndt::type> &tp_vars)
  {
    intptr_t dim_size, stride;
    dynd::ndt::type el_tp;
    const char *el_arrmeta;
    if (src_tp[0].get_as_strided(src_arrmeta[0], &dim_size, &stride, &el_tp,
                                 &el_arrmeta)) {
      assign_to_pyobject_kernel::make(ckb, kernreq, ckb_offset, dim_size,
                                      stride);
      return nd::assign::get()->instantiate(
          nd::assign::get()->static_data(), data, ckb, ckb_offset, dst_tp,
          dst_arrmeta, nsrc, &el_tp, &el_arrmeta, dynd::kernel_request_strided,
          nkwd, kwds, tp_vars);
    }

    throw std::runtime_error("cannot process as strided");
  }
};

template <>
struct assign_to_pyobject_kernel<dynd::var_dim_type_id, dim_kind_type_id>
    : dynd::nd::base_kernel<
          assign_to_pyobject_kernel<dynd::var_dim_type_id, dim_kind_type_id>,
          1> {
  intptr_t offset, stride;

  assign_to_pyobject_kernel(intptr_t offset, intptr_t stride)
      : offset(offset), stride(stride)
  {
  }

  ~assign_to_pyobject_kernel() { get_child()->destroy(); }

  void single(char *dst, char *const *src)
  {
    PyObject **dst_obj = reinterpret_cast<PyObject **>(dst);
    Py_XDECREF(*dst_obj);
    *dst_obj = NULL;
    const dynd::ndt::var_dim_type::data_type *vd =
        reinterpret_cast<const dynd::ndt::var_dim_type::data_type *>(src[0]);
    pydynd::pyobject_ownref lst(PyList_New(vd->size));
    dynd::ckernel_prefix *copy_el = get_child();
    dynd::kernel_strided_t copy_el_fn =
        copy_el->get_function<dynd::kernel_strided_t>();
    char *el_src = vd->begin + offset;
    copy_el_fn(copy_el,
               reinterpret_cast<char *>(((PyListObject *)lst.get())->ob_item),
               sizeof(PyObject *), &el_src, &stride, vd->size);
    if (PyErr_Occurred()) {
      throw std::exception();
    }
    *dst_obj = lst.release();
  }

  static intptr_t
  instantiate(char *static_data, char *data, void *ckb, intptr_t ckb_offset,
              const dynd::ndt::type &dst_tp, const char *dst_arrmeta,
              intptr_t nsrc, const dynd::ndt::type *src_tp,
              const char *const *src_arrmeta, dynd::kernel_request_t kernreq,
              intptr_t nkwd, const dynd::nd::array *kwds,
              const std::map<std::string, dynd::ndt::type> &tp_vars)
  {
    assign_to_pyobject_kernel::make(
        ckb, kernreq, ckb_offset,
        reinterpret_cast<const dynd::ndt::var_dim_type::metadata_type *>(
            src_arrmeta[0])
            ->offset,
        reinterpret_cast<const dynd::ndt::var_dim_type::metadata_type *>(
            src_arrmeta[0])
            ->stride);
    dynd::ndt::type el_tp =
        src_tp[0].extended<dynd::ndt::var_dim_type>()->get_element_type();
    const char *el_arrmeta =
        src_arrmeta[0] + sizeof(dynd::ndt::var_dim_type::metadata_type);
    return nd::assign::get()->instantiate(
        nd::assign::get()->static_data(), data, ckb, ckb_offset, dst_tp,
        dst_arrmeta, nsrc, &el_tp, &el_arrmeta, dynd::kernel_request_strided,
        nkwd, kwds, tp_vars);
  }
};

} // namespace detail

template <type_id_t Arg0ID>
using assign_to_pyobject_kernel =
    ::detail::assign_to_pyobject_kernel<Arg0ID, base_type_id_of<Arg0ID>::value>;

namespace dynd {
namespace ndt {

  template <type_id_t Arg0ID>
  struct traits<assign_to_pyobject_kernel<Arg0ID>> {
    static type equivalent()
    {
      return callable_type::make(ndt::make_type<pyobject_type>(),
                                 {type(Arg0ID)});
    }
  };

} // namespace dynd::ndt
} // namespace dynd