#
# Copyright (C) 2011-13 Mark Wiebe, DyND Developers
# BSD 2-Clause License, see LICENSE.txt
#

# cython: c_string_type=str, c_string_encoding=ascii

# Initialize Numpy
import_numpy()

# Exceptions to convert from C++
class BroadcastError(Exception):
    pass

# Initialize ctypes C level interop data
pydynd_init()

# Register all the exception objects with the exception translator
set_broadcast_exception(BroadcastError)

# Initialize C++ access to the Cython type objects
init_w_array_typeobject(w_array)
init_w_arrfunc_typeobject(w_arrfunc)
init_w_type_typeobject(w_type)
init_w_array_callable_typeobject(w_array_callable)
init_w__type_callable_typeobject(w_type_callable)
init_w_eval_context_typeobject(w_eval_context)

from dynd cimport *
from ndt.type cimport *

from nd.array cimport *
from nd.gfunc_callable cimport *

from eval_context cimport *

# Issue a performance warning if any of the diagnostics macros are enabled
cdef extern from "<dynd/diagnostics.hpp>" namespace "dynd":
    bint any_diagnostics_enabled()
    string which_diagnostics_enabled()
if any_diagnostics_enabled():
    import warnings
    class PerformanceWarning(Warning):
        pass
    warnings.warn("Performance is reduced because of enabled diagnostics:\n" +
                str(<char *>which_diagnostics_enabled().c_str()), PerformanceWarning)

from cython.operator import dereference
# Save the built-in type operator, so we can have parameters called 'type'
builtin_type = type

# Get a boolean indicating whether CUDA support was built in or not
cuda_support = built_with_cuda()

# Expose the git hashes and version numbers of this build
# NOTE: Cython generates code which is not const-correct, so
#       have to cast it away.
_dynd_version_string = str(<char *>dynd_version_string)
_dynd_git_sha1 = str(<char *>dynd_git_sha1)
_dynd_python_version_string = str(<char *>dynd_python_version_string)
_dynd_python_git_sha1 = str(<char *>dynd_python_git_sha1)

def _get_lowlevel_api():
    return <size_t>dynd_get_lowlevel_api()

def _get_py_lowlevel_api():
    return <size_t>dynd_get_py_lowlevel_api()

from dynd.ndt.type import Unsupplied


cdef class w_type:
    """
    ndt.type(obj=None)

    Create a dynd type object.

    A dynd type object describes the dimensional
    structure and element type of a dynd array.

    Parameters
    ----------
    obj : string or other data type, optional
        A Blaze datashape string or a data type from another
        system such as NumPy or ctypes.

    Examples
    --------
    >>> from dynd import nd, ndt

    >>> ndt.type('int16')
    ndt.int16
    >>> ndt.type('5 * var * float32')
    ndt.type("5 * var * float32")
    >>> ndt.type('{x: float32, y: float32, z: float32}')
    ndt.type("{x : float32, y : float32, z : float32}")
    """
    def __cinit__(self, rep=None):
        if rep is not None:
            self.v = make__type_from_pyobject(rep)

    def __dir__(self):
        # Customize dir() so that additional properties of various types
        # will show up in IPython tab-complete, for example.
        result = dict(w_type.__dict__)
        result.update(object.__dict__)
        add__type_names_to_dir_dict(self.v, result)
        return result.keys()

    def __call__(self, *args, **kwargs):
        return call__type_constructor_function(self.v, args, kwargs)

    def __getattr__(self, name):
        return get__type_dynamic_property(self.v, name)

    def match(self, rhs):
        """
        tp.match(candidate)

        Returns True if the candidate type ``candidate`` matches the possibly
        symbolic pattern type ``tp``, False otherwise.

        Examples
        --------
        >>> from dynd import nd, ndt

        >>> ndt.type("T").match(ndt.int32)
        True
        >>> ndt.type("Dim * T").match(ndt.int32)
        False
        >>> ndt.type("M * {x: ?T, y: T}").match("10 * {x : ?int32, y : int32}")
        True
        >>> ndt.type("M * {x: ?T, y: T}").match("10 * {x : ?int32, y : ?int32}")
        False
        """
        return self.v.match(w_type(rhs).v)

    property shape:
        """
        tp.shape

        The shape of the array dimensions of the type. For
        dimensions whose size is unknown without additional
        array arrmeta or array data, a -1 is returned.
        """
        def __get__(self):
            return _type_get_shape(self.v)

    property dshape:
        """
        tp.dshape

        The blaze datashape of the dynd type, as a string.
        """
        def __get__(self):
            return str(<char *>dynd_format_datashape(self.v).c_str())

    property data_size:
        """
        tp.data_size

        The size, in bytes, of the data for an instance
        of this dynd type.

        None is returned if array arrmeta is required to
        fully specify it. For example, both the fixed and
        struct types require such arrmeta.
        """
        def __get__(self):
            cdef ssize_t result = self.v.get_data_size()
            if result > 0:
                return result
            else:
                return None

    property default_data_size:
        """
        tp.default_data_size

        The size, in bytes, of the data for a default-constructed
        instance of this dynd type.
        """
        def __get__(self):
            return self.v.get_default_data_size()

    property data_alignment:
        """
        tp.data_alignment

        The alignment, in bytes, of the data for an
        instance of this dynd type.

        Data for this dynd type must always be aligned
        according to this alignment, unaligned data
        requires an adapter transformation applied.
        """
        def __get__(self):
            return self.v.get_data_alignment()

    property arrmeta_size:
        """
        tp.arrmeta_size

        The size, in bytes, of the arrmeta for
        this dynd type.
        """
        def __get__(self):
            return self.v.get_arrmeta_size()

    property kind:
        """
        tp.kind

        The kind of this dynd type, as a string.

        Example kinds are 'bool', 'int', 'uint',
        'real', 'complex', 'string', 'uniform_array',
        'expression'.
        """
        def __get__(self):
            return _type_get_kind(self.v)

    property type_id:
        """
        tp.type_id

        The type id of this dynd type, as a string.

        Example type ids are 'bool', 'int8', 'uint32',
        'float64', 'complex_float32', 'string', 'byteswap'.
        """
        def __get__(self):
            return _type_get_type_id(self.v)

    property ndim:
        """
        tp.ndim

        The number of array dimensions in this dynd type.

        This property is like NumPy
        _array's 'ndim'. Indexing with [] can in many cases
        go deeper than just the array dimensions, for
        example structs can be indexed this way.
        """
        def __get__(self):
            return self.v.get_ndim()

    property dtype:
        """
        tp.dtype

        The dynd type of the element after the 'ndim'
        array dimensions are indexed away.

        This property is roughly equivalent to NumPy
        _array's 'dtype'.
        """
        def __get__(self):
            cdef w_type result = w_type()
            result.v = self.v.get_dtype()
            return result;

    property value_type:
        """
        tp.value_type

        If this is an expression dynd type, returns the
        dynd type that values after evaluation have. Otherwise,
        returns this dynd type unchanged.
        """
        def __get__(self):
            cdef w_type result = w_type()
            result.v = self.v.value_type()
            return result

    property operand_type:
        """
        tp.operand_type

        If this is an expression dynd type, returns the
        dynd type that inputs to its expression evaluation
        have. Otherwise, returns this dynd type unchanged.
        """
        def __get__(self):
            cdef w_type result = w_type()
            result.v = self.v.operand_type()
            return result

    property canonical_type:
        """
        tp.canonical_type

        Returns a version of this type that is canonical,
        where any intermediate pointers are removed and expressions
        are stripped away.
        """
        def __get__(self):
            cdef w_type result = w_type()
            result.v = self.v.get_canonical_type()
            return result

    property property_names:
        """
        tp.property_names

        Returns the names of properties exposed by dynd arrays
        of this type.
        """
        def __get__(self):
            return _type_array_property_names(self.v)

    def as_numpy(self):
        """
        tp.as_numpy()

        If possible, converts the ndt.type object into an
        equivalent numpy dtype.

        Examples
        --------
        >>> from dynd import nd, ndt

        >>> ndt.int32.as_numpy()
        dtype('int32')
        """
        return numpy_dtype_obj_from__type(self.v)

    def __getitem__(self, x):
        cdef w_type result = w_type()
        result.v = _type_getitem(self.v, x)
        return result

    def __str__(self):
        return str(<char *>_type_str(self.v).c_str())

    def __repr__(self):
        return str(<char *>_type_repr(self.v).c_str())

    def __richcmp__(lhs, rhs, int op):
        if op == Py_EQ:
            if type(lhs) == w_type and type(rhs) == w_type:
                return (<w_type>lhs).v == (<w_type>rhs).v
            else:
                return False
        elif op == Py_NE:
            if type(lhs) == w_type and type(rhs) == w_type:
                return (<w_type>lhs).v != (<w_type>rhs).v
            else:
                return False
        return NotImplemented

##############################################################################

# NOTE: This is a possible alternative to the init_w_array_typeobject() call
#       above, but it generates a 1300 line header file and still requires calling
#       import__dnd from the C++ code, so directly using C++ primitives seems simpler.
#cdef public api class w_array [object WNDArrayObject, type WNDArrayObject_Type]:

cdef class w_array:
    """
    nd.array(obj=None, dtype=None, type=None, access=None)

    Create a dynd array out of the provided object.

    The dynd array is the dynamically typed multi-dimensional
    object provided by the dynd library. It is similar to
    NumPy's _array, but has its dimensional structure encoded
    in the dynd type, along with the element type.

    When given a NumPy array, the resulting dynd array is a view
    into the NumPy array data. When given lists of Python object,
    an attempt is made to deduce an appropriate dynd type for
    the array, and a conversion is made if possible, or an
    exception is raised.

    Parameters
    ----------
    value : multi-dimensional object, optional
        Any object which dynd knows how to interpret as a dynd array.
    dtype: dynd type
        If provided, the type is used as the data type for the
        input, and the shape of the leading dimensions is deduced.
        This parameter cannot be used together with 'type'.
    type: dynd type
        If provided, the type is used as the full type for the input.
        If needed by the type, the shape is deduced from the input.
        This parameter cannot be used together with 'dtype'.
    access:  'readwrite'/'rw', 'readonly'/'r', or 'immutable', optional
        If provided, this specifies the access control for the
        created array. If the array is being allocated, as in
        construction from Python objects, this is the access control
        set.

        If the array is a view into another array data source,
        such as NumPy arrays or objects which support the buffer
        protocol, the access control must be compatible with that
        of the input object's.

        The default is immutable, or to inherit the access control
        of the object being viewed if it is an object supporting
        the buffer protocol.

    Examples
    --------
    >>> from dynd import nd, ndt

    >>> nd.array([1, 2, 3, 4, 5])
    nd.array([1, 2, 3, 4, 5],
             type="5 * int32")
    >>> nd.array([[1, 2], [3, 4, 5.0]])
    nd.array([   [1, 2], [3, 4, 5]],
             type="2 * var * float64")
    >>> from datetime import date
    >>> nd.array([date(2000,2,14), date(2012,1,1)])
    nd.array([2000-02-14, 2012-01-01],
             type="2 * date")
    """
    def __init__(self, value=Unsupplied, dtype=None, type=None, access=None):
        if value is not Unsupplied:
            # Get the array data
            if dtype is not None:
                if type is not None:
                    raise ValueError('Must provide only one of ' +
                                    'dtype or type, not both')
                array_init_from_pyobject(self.v, value, dtype, False, access)
            elif type is not None:
                array_init_from_pyobject(self.v, value, type, True, access)
            else:
                array_init_from_pyobject(self.v, value, access)
        elif dtype is not None or type is not None or access is not None:
            raise ValueError('a value for the array construction must ' +
                            'be provided when another keyword parameter is used')

    def __dir__(self):
        # Customize dir() so that additional properties of various types
        # will show up in IPython tab-complete, for example.
        result = dict(w_array.__dict__)
        result.update(object.__dict__)
        add_array_names_to_dir_dict(self.v, result)
        return result.keys()

    def __getattr__(self, name):
        return get_array_dynamic_property(self.v, name)

    def __setattr__(self, name, value):
        set_array_dynamic_property(self.v, name, value)

    def __contains__(self, x):
        return array_contains(self.v, x)

    def eval(self, ectx=None):
        """
        a.eval(ectx=<default eval_context>)

        Returns a version of the dynd array with plain values,
        all expressions evaluated. This returns the original
        array back if it has no expression type.

        Parameters
        ----------
        ectx : nd.eval_context
            The evaluation context to use.

        Examples
        --------
        >>> from dynd import nd, ndt

        >>> a = nd.array([1.5, 2, 3])
        >>> a
        nd.array([1.5,   2,   3],
                 type="3 * float64")
        >>> b = a.ucast(ndt.complex_float32)
        >>> b
        nd.array([(1.5 + 0j),   (2 + 0j),   (3 + 0j)],
                 type="3 * convert[to=complex[float32], from=float64]")
        >>> b.eval()
        nd.array([(1.5 + 0j),   (2 + 0j),   (3 + 0j)],
                 type="3 * complex[float32]")
        """
        cdef w_array result = w_array()
        result.v = array_eval(self.v, ectx)
        return result

    def eval_immutable(self):
        """
        a.eval_immutable()

        Evaluates into an immutable dynd array. If the array is
        already immutable and not an expression type, returns it
        as is.
        """
        cdef w_array result = w_array()
        result.v = self.v.eval_immutable()
        return result

    def eval_copy(self, access=None, ectx=None):
        """
        a.eval_copy(access='readwrite', ectx=<default eval_context>)

        Evaluates into a new dynd array, guaranteeing a copy is made.

        Parameters
        ----------
        access : 'readwrite' or 'immutable'
            Specifies the access control of the resulting copy.
        ectx : nd.eval_context
            The evaluation context to use.
        """
        cdef w_array result = w_array()
        result.v = array_eval_copy(self.v, access, ectx)
        return result

    def storage(self):
        """
        a.storage()

        Returns a version of the dynd array with its storage type,
        all expressions discarded. For data types that are plain
        old data, views them as a bytes type.

        Examples
        --------
        >>> from dynd import nd, ndt

        >>> a = nd.array([1, 2, 3], dtype=ndt.int16)
        >>> a
        nd.array([1, 2, 3],
                 type="3 * int16")
        >>> a.storage()
        nd.array([0100, 0200, 0300],
                 type="3 * bytes[2, align=2]")
        """
        cdef w_array result = w_array()
        result.v = self.v.storage()
        return result

    def cast(self, type):
        """
        a.cast(type)

        Casts the dynd array's type to the requested type,
        producing a conversion type. If the data for the
        new type is identical, it is used directly to avoid
        the conversion.

        Parameters
        ----------
        type : dynd type
            The type is cast into this type.
        """
        cdef w_array result = w_array()
        result.v = array_cast(self.v, w_type(type).v)
        return result

    def ucast(self, dtype, int replace_ndim=0):
        """
        a.ucast(dtype, replace_ndim=0)

        Casts the dynd array's dtype to the requested type,
        producing a conversion type. The dtype is the type
        after the nd.ndim_of(a) array dimensions.

        Parameters
        ----------
        dtype : dynd type
            The dtype of the array is cast into this type.
            If `replace_ndim` is not zero, then that many
            dimensions are included in what is cast as well.
        replace_ndim : integer, optional
            The number of array dimensions to replace in doing
            the cast.

        Examples
        --------
        >>> from dynd import nd, ndt

        >>> from datetime import date
        >>> a = nd.array([date(1929,3,13), date(1979,3,22)]).ucast('{month: int32, year: int32, day: float32}')
        >>> a
        nd.array([[3, 1929, 13], [3, 1979, 22]], type="Fixed * convert[to={month : int32, year : int32, day : float32}, from=date]")
        >>> a.eval()
        nd.array([[3, 1929, 13], [3, 1979, 22]], type="2 * {month : int32, year : int32, day : float32}")
        """
        cdef w_array result = w_array()
        result.v = array_ucast(self.v, w_type(dtype).v, replace_ndim)
        return result

    def view_scalars(self, dtype):
        """
        a.view_scalars(dtype)

        Views the data of the dynd array as the requested dtype,
        where it makes sense.

        If the array is a one-dimensional contiguous
        array of plain old data, the new dtype may have a different
        element size than original one.

        When the array has an expression type, the
        view is created by layering another view dtype
        onto the array's existing expression.

        Parameters
        ----------
        dtype : dynd type
            The scalars are viewed as this dtype.
        """
        cdef w_array result = w_array()
        result.v = self.v.view_scalars(w_type(dtype).v)
        return result

    def flag_as_immutable(self):
        """
        a.flag_as_immutable()

        When there's still only one reference to a
        dynd array, can be used to flag it as immutable.
        """
        self.v.flag_as_immutable()

    property access_flags:
        """
        a.access_flags

        The access flags of the dynd array, as a string.
        Returns 'immutable', 'readonly', or 'readwrite'
        """
        def __get__(self):
            return str(<char *>array_access_flags_string(self.v))

    property is_scalar:
        """
        a.is_scalar

        True if the dynd array is a scalar.
        """
        def __get__(self):
            return array_is_scalar(self.v)

    property ndim:
        def __get__(self):
            return self.v.get_type().get_ndim()

    property shape:
        def __get__(self):
            return array_get_shape(self.v)

    property strides:
        def __get__(self):
            return array_get_strides(self.v)

    property type:
        def __get__(self):
            cdef w_type result = w_type()
            result.v = self.v.get_type()
            return result

    property dtype:
        def __get__(self):
            cdef w_type result = w_type()
            result.v = self.v.get_dtype()
            return result

    def __repr__(self):
        return str(<char *>array_repr(self.v).c_str())

    def __str__(self):
        return array_str(self.v)

    def __unicode__(self):
        return array_unicode(self.v)

    def __index__(self):
        return array_index(self.v)

    def __nonzero__(self):
        return array_nonzero(self.v)

    def __int__(self):
        return array_int(self.v)

    def __float__(self):
        return array_float(self.v)

    def __complex__(self):
        return array_complex(self.v)

    def __len__(self):
        return self.v.get_dim_size()

    def __getitem__(self, x):
        cdef w_array result = w_array()
        result.v = array_getitem(self.v, x)
        return result

    def __setitem__(self, x, y):
        array_setitem(self.v, x, y)

    def __getbuffer__(w_array self, Py_buffer* buffer, int flags):
        # Docstring triggered Cython bug (fixed in master), so it's commented out
        #"""PEP 3118 buffer protocol"""
        array_getbuffer_pep3118(self, buffer, flags)

    def __releasebuffer__(w_array self, Py_buffer* buffer):
        # Docstring triggered Cython bug (fixed in master), so it's commented out
        #"""PEP 3118 buffer protocol"""
        array_releasebuffer_pep3118(self, buffer)

    def __add__(lhs, rhs):
        cdef w_array res = w_array()
        res.v = array_add(asarray(lhs).v, asarray(rhs).v)
        return res

    def __sub__(lhs, rhs):
        cdef w_array res = w_array()
        res.v = array_subtract(asarray(lhs).v, asarray(rhs).v)
        return res

    def __mul__(lhs, rhs):
        cdef w_array res = w_array()
        res.v = array_multiply(asarray(lhs).v, asarray(rhs).v)
        return res

    def __div__(lhs, rhs):
        cdef w_array res = w_array()
        res.v = array_divide(asarray(lhs).v, asarray(rhs).v)
        return res

    def __truediv__(lhs, rhs):
        cdef w_array res = w_array()
        res.v = array_divide(asarray(lhs).v, asarray(rhs).v)
        return res

cdef class w_arrfunc(w_array):
    """
    nd.arrfunc(func, proto)

    This holds a dynd nd.arrfunc object, which represents a single typed
    function. The particular abstraction this represents is still being
    sorted out.

    The constructor creates an arrfunc out of a Python function.

    Parameters
    ----------
    func : callable
        A Python function or object that implements __call__.
    proto : ndt.type
        A funcproto describing the types for the resulting arrfunc.

    Examples
    --------
    >>> from dynd import nd, ndt

    >>> af = nd.arrfunc(lambda x, y: [x, y], '(int, int) -> {x:int, y:int}')
    >>>
    >>> af(1, 10)
    nd.array([1, 10], type="{x : int32, y : int32}")
    >>> af(1, "test")
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
      File "_pydynd.pyx", line 1340, in _pydynd.w_arrfunc.__call__ (_pydynd.cxx:9774)
    ValueError: parameter 2 to arrfunc does not match, expected int32, received string
    """

    def __call__(self, *args, **kwds):
        # Handle the keyword-only arguments
        ectx = kwds.pop('ectx', None)
#        if kwds:
#            msg = "nd.arrfunc call got an unexpected keyword argument '%s'"
#            raise TypeError(msg % (kwds.keys()[0]))
        return arrfunc_call(self, args, kwds, ectx)


def view(obj, type=None, access=None):
    """
    nd.view(obj, type=None, access=None)

    Constructs a dynd array which is a view of the data from
    `obj`. The `access` parameter can be used to require writable
    access for an output parameter, or to produce a read-only
    view of writable data.

    Parameters
    ----------
    obj : object
        A Python object which backs some array data, such as
        a dynd array, a numpy array, or an object supporting
        the Python buffer protocol.
    type : ndt.type, optional
        If provided, requests that the memory of ``obj`` be viewed
        as this type.
    access : 'readwrite'/'rw' or 'readonly'/'r', optional
        The access flags for the constructed array. Use 'readwrite'
        to require that the view be writable, and 'readonly' to
        provide a view of data to someone else without allowing
        writing.
    """
    cdef w_array result = w_array()
    result.v = array_view(obj, type, access)
    return result

def adapt(arr, tp, op):
    """
    nd.adapt(arr, tp, op)

    Adapt an array to the given type, using the adaptation operation
    provided. The resulting array is a view of ``arr``, and has an
    ``adapt`` type.

    Parameters
    ----------
    arr : nd.array
        The array whose dtype is to be adapted.
    tp : ndt.type
        The type the adaptation results in.
    op : string
        A string describing the operation. For example
        an ``int`` to ``date`` adaptation can be done with
        an ``op`` value like "days since 2000-01-01".

    Examples
    --------
    >>> from dynd import nd, ndt

    >>> a = nd.array([1, 3, 10, 31, 365])
    >>> nd.adapt(a, ndt.date, 'days since 2012')
    nd.array([2012-01-02, 2012-01-04, 2012-01-11, 2012-02-01, 2012-12-31],
             type="5 * adapt[(int32) -> date, 'days since 2012']")
    """
    return array_adapt(arr, tp, op)

cpdef w_array asarray(obj, access=None):
    """
    nd.asarray(obj, access=None)

    Constructs a dynd array from the object, taking a view
    if possible, otherwise making a copy.

    Parameters
    ----------
    obj : object
        The object which is to be converted into a dynd array,
        as a view if possible, otherwise a copy.
    access : 'readwrite'/'rw', 'readonly'/'r', 'immutable', optional
        If provided, the access flags the resulting array should
        satisfy. When a view can be taken, but these access flags
        cannot, a copy is made.
    """

    cdef w_array result = w_array()
    result.v = array_asarray(obj, access)
    return result

def type_of(w_array a):
    """
    nd.type_of(a)

    The dynd type of the array. This is the full
    data type, including its multi-dimensional structure.

    Parameters
    ----------
    a : dynd array
        The array whose type is requested.

    Examples
    --------
    >>> from dynd import nd, ndt

    >>> nd.type_of(nd.array([1,2,3,4]))
    ndt.type("4 * int32")
    >>> nd.type_of(nd.array([[1,2],[3.0]]))
    ndt.type("2 * var * float64")
    """
    cdef w_type result = w_type()
    result.v = a.v.get_type()
    return result

def dshape_of(w_array a):
    """
    nd.dshape_of(a)

    The blaze datashape of the dynd array, as a string.

    Parameters
    ----------
    a : dynd array
        The array whose type is requested.
    """
    return str(<char *>dynd_format_datashape(a.v).c_str())

def dtype_of(w_array a, size_t include_ndim=0):
    """
    nd.dtype_of(a)

    The data type of the dynd array. This is
    the type after removing all the array
    dimensions from the dynd type of `a`.
    If `include_ndim` is provided, that many
    array dimensions are included in the
    data type returned.

    Parameters
    ----------
    a : dynd array
        The array whose type is requested.
    include_ndim : int, optional
        The number of array dimensions to include
        in the data type, default zero.
    """
    cdef w_type result = w_type()
    result.v = a.v.get_dtype(include_ndim)
    return result

def ndim_of(w_array a):
    """
    nd.ndim_of(a)

    The number of array dimensions in the dynd array `a`.
    This corresponds to the number of dimensions
    in a NumPy array.
    """
    return a.v.get_ndim()

def is_c_contiguous(w_array a):
    """
    nd.is_c_contiguous(a)

    Returns True if the array is C-contiguous, False
    otherwise. An array is C-contiguous if all its array
    dimensions are ``fixed``, the strides are in decreasing
    order, and the data is tightly packed.
    """
    return array_is_c_contiguous(a.v)

def is_f_contiguous(w_array a):
    """
    nd.is_f_contiguous(a)

    Returns True if the array is F-contiguous, False
    otherwise. An array is F-contiguous if all its array
    dimensions are ``fixed``, the strides are in increasing
    order, and the data is tightly packed.
    """
    return array_is_f_contiguous(a.v)

def as_py(w_array n, tuple=False):
    """
    nd.as_py(n, tuple=False)

    Evaluates the dynd array, converting it into native Python types.

    Uniform dimensions convert into Python lists, struct types convert
    into Python dicts, scalars convert into the most appropriate Python
    scalar for their type.

    Parameters
    ----------
    n : dynd array
        The dynd array to convert into native Python types.
    tuple : bool
        If true, produce tuples instead of dicts when converting
        dynd struct arrays.

    Examples
    --------
    >>> from dynd import nd, ndt

    >>> a = nd.array([1, 2, 3, 4.0])
    >>> a
    nd.array([1, 2, 3, 4],
             type="4 * float64")
    >>> nd.as_py(a)
    [1.0, 2.0, 3.0, 4.0]
    """
    cdef bint tup = tuple
    return array_as_py(n.v, tup != 0)

def as_numpy(w_array n, allow_copy=False):
    """
    nd.as_numpy(n, allow_copy=False)

    Evaluates the dynd array, and converts it into a NumPy object.

    Parameters
    ----------
    n : dynd array
        The array to convert into native Python types.
    allow_copy : bool, optional
        If true, allows a copy to be made when the array types
        can't be directly viewed as a NumPy array, but with a
        data-preserving copy they can be.

    Examples
    --------
    >>> from dynd import nd, ndt

    >>> import numpy as np
    >>> a = nd.array([[1, 2, 3], [4, 5, 6]])
    >>> a
    nd.array([[1, 2, 3], [4, 5, 6]],
             type="2 * 3 * int32")
    >>> nd.as_numpy(a)
    array([[1, 2, 3],
           [4, 5, 6]])
    """
    # TODO: Could also convert dynd types into numpy dtypes
    return array_as_numpy(n, bool(allow_copy))

def zeros(*args, **kwargs):
    """
    nd.zeros(dtype, *, access=None)
    nd.zeros(shape, dtype, *, access=None)
    nd.zeros(shape_0, shape_1, ..., shape_(n-1), dtype, *, access=None)

    Creates an array of zeros of the specified
    type. Dimensions may be provided as integer
    positional parameters, a tuple of integers,
    or within the dtype itself.

    TODO: In the immutable case it should use zero-strides to optimize storage.

    TODO: Add order= keyword-only argument. This would accept
    'C', 'F', or a permutation tuple.

    Parameters
    ----------
    shape : list of int, optional
        If provided, specifies the shape for dimensions which
        are prepended to the following dtype.
    dtype : dynd type
        The dtype of the uninitialized array to create.
    access : 'readwrite' or 'immutable', optional
        Specifies the access control of the resulting copy. Defaults
        to readwrite.
    """
    # Handle the keyword-only arguments
    access = kwargs.pop('access', None)
    if kwargs:
        msg = "nd.zeros() got an unexpected keyword argument '%s'"
        raise TypeError(msg % (kwargs.keys()[0]))

    cdef w_array result = w_array()
    largs = len(args)
    if largs  == 1:
        # Only the full type is provided
        result.v = array_zeros(w_type(args[0]).v, access)
    elif largs == 2:
        # The shape is a provided as a tuple (or single integer)
        result.v = array_zeros(args[0], w_type(args[1]).v, access)
    elif largs > 2:
        # The shape is expanded out in the arguments
        result.v = array_zeros(args[:-1], w_type(args[-1]).v, access)
    else:
        raise TypeError('nd.zeros() expected at least 1 positional argument, got 0')
    return result

def ones(*args, **kwargs):
    """
    nd.ones(type, *, access=None)
    nd.ones(shape, dtype, *, access=None)
    nd.ones(shape_0, shape_1, ..., shape_(n-1), dtype, *, access=None)

    Creates an array of ones of the specified
    type. Dimensions may be provided as integer
    positional parameters, a tuple of integers,
    or within the dtype itself.

    TODO: In the immutable case it should use zero-strides to optimize storage.

    TODO: Add order= keyword-only argument. This would accept
    'C', 'F', or a permutation tuple.

    Parameters
    ----------
    shape : list of int, optional
        If provided, specifies the shape for dimensions which
        are prepended to the following dtype.
    dtype : dynd type
        The dtype of the uninitialized array to create.
    access : 'readwrite' or 'immutable', optional
        Specifies the access control of the resulting copy. Defaults
        to readwrite.
    """
    # Handle the keyword-only arguments
    access = kwargs.pop('access', None)
    if kwargs:
        msg = "nd.ones() got an unexpected keyword argument '%s'"
        raise TypeError(msg % (kwargs.keys()[0]))

    cdef w_array result = w_array()
    largs = len(args)
    if largs  == 1:
        # Only the full type is provided
        result.v = array_ones(w_type(args[0]).v, access)
    elif largs == 2:
        # The shape is a provided as a tuple (or single integer)
        result.v = array_ones(args[0], w_type(args[1]).v, access)
    elif largs > 2:
        # The shape is expanded out in the arguments
        result.v = array_ones(args[:-1], w_type(args[-1]).v, access)
    else:
        raise TypeError('nd.ones() expected at least 1 positional argument, got 0')
    return result

def full(*args, **kwargs):
    """
    nd.full(type, *, value, access=None)
    nd.ones(shape, dtype, *, value, access=None)
    nd.ones(shape_0, shape_1, ..., shape_(n-1), dtype, *, value, access=None)

    Creates an array filled with the given value and
    of the specified type. Dimensions may be provided as
    integer positional parameters, a tuple of integers,
    or within the dtype itself.

    TODO: In the immutable case it should use zero-strides to optimize storage.

    TODO: Add order= keyword-only argument. This would accept
    'C', 'F', or a permutation tuple.

    Parameters
    ----------
    shape : list of int, optional
        If provided, specifies the shape for dimensions which
        are prepended to the following dtype.
    dtype : dynd type
        The dtype of the uninitialized array to create.
    access : 'readwrite' or 'immutable', optional
        Specifies the access control of the resulting copy. Defaults
        to readwrite.

    Examples
    --------
    >>> from dynd import nd, ndt

    >>> nd.full(2, 3, ndt.int32, value=123)
    nd.array([[123, 123, 123], [123, 123, 123]],
             type="2 * 3 * int32")
    >>> nd.full(2, 2, ndt.int32, value=[1, 5])
    nd.array([[1, 5], [1, 5]],
             type="2 * 2 * int32")
    >>> nd.full('3 * {x : int32, y : 2 * int16}', value=[1, [2, 3]], access='rw')
    nd.array([[1, [2, 3]], [1, [2, 3]], [1, [2, 3]]],
             type="3 * {x : int32, y : 2 * int16}")
    """
    # Handle the keyword-only arguments
    if 'value' not in kwargs:
        raise TypeError("nd.full() missing required " +
                    "keyword-only argument: 'value'")
    access = kwargs.pop('access', None)
    value = kwargs.pop('value', None)
    if kwargs:
        msg = "nd.full() got an unexpected keyword argument '%s'"
        raise TypeError(msg % (kwargs.keys()[0]))

    cdef w_array result = w_array()
    largs = len(args)
    if largs  == 1:
        # Only the full type is provided
        result.v = array_full(w_type(args[0]).v, value, access)
    elif largs == 2:
        # The shape is a provided as a tuple (or single integer)
        result.v = array_full(args[0], w_type(args[1]).v, value, access)
    elif largs > 2:
        # The shape is expanded out in the arguments
        result.v = array_full(args[:-1], w_type(args[-1]).v, value, access)
    else:
        raise TypeError('nd.full() expected at least 1 positional argument, got 0')
    return result

def empty(*args, **kwargs):
    """
    nd.empty(type, access=None)
    nd.empty(shape, dtype, access=None)
    nd.empty(shape_0, shape_1, ..., shape_(n-1), dtype, access=None)

    Creates an uninitialized array of the specified type.
    Dimensions may be provided as integer positional
    parameters, a tuple of integers, or within the dtype itself.

    TODO: Add order= keyword-only argument. This would accept
    'C', 'F', or a permutation tuple.

    Parameters
    ----------
    shape : list of int, optional
        If provided, specifies the shape for dimensions which
        are prepended to the following dtype.
    dtype : dynd type
        The dtype of the uninitialized array to create.
    access : 'readwrite' or 'immutable', optional
        Specifies the access control of the resulting copy. Defaults
        to readwrite.

    Examples
    --------
    >>> from dynd import nd, ndt

    >>> nd.empty('2 * 2 * int8')
    nd.array([[0, -14], [0, 0]],
             type="2 * 2 * int8")
    >>> nd.empty((2, 2), ndt.int16)
    nd.array([[0, 27], [159, 0]],
             type="2 * 2 * int16")
    """
    # Handle the keyword-only arguments
    access = kwargs.pop('access', None)
    if kwargs:
        msg = "nd.empty() got an unexpected keyword argument '%s'"
        raise TypeError(msg % (kwargs.keys()[0]))

    cdef w_array result = w_array()
    largs = len(args)
    if largs  == 1:
        # Only the full type is provided
        result.v = array_empty(w_type(args[0]).v, access)
    elif largs == 2:
        # The shape is a provided as a tuple (or single integer)
        result.v = array_empty(args[0], w_type(args[1]).v, access)
    elif largs > 2:
        # The shape is expanded out in the arguments
        result.v = array_empty(args[:-1], w_type(args[-1]).v, access)
    else:
        raise TypeError('nd.empty() expected at least 1 positional argument, got 0')
    return result

def empty_like(w_array prototype, dtype=None):
    """
    nd.empty_like(prototype, dtype=None)

    Creates an uninitialized array whose array dimensions match
    the structure of the prototype's array dimensions.

    Parameters
    ----------
    prototype : dynd array
        The array whose structure is to be matched.
    dtype : dynd type, optional
        If provided, replaces the prototype's dtype in
        the result.

    Examples
    --------
    >>> from dynd import nd, ndt

    >>> a = nd.array([[1, 2], [3, 4]])
    >>> a
    nd.array([[1, 2], [3, 4]],
             type="2 * 2 * int32")
    >>> nd.empty_like(a)
    nd.array([[1852383276, 1577728116],       [2695690,       0]],
             type="2 * 2 * int32")
    >>> nd.empty_like(a, dtype=ndt.float32)
    nd.array([[1.50878e-019, 1.80372e+028], [2.41507e-018, 7.25506e-039]],
             type="2 * 2 * float32")
    """
    cdef w_array result = w_array()
    if dtype is None:
        result.v = array_empty_like(prototype.v)
    else:
        result.v = array_empty_like(prototype.v, w_type(dtype).v)
    return result

def memmap(filename, begin=None, end=None, access=None):
    """
    nd.memmap(filename, begin=None, end=None, access=None)

    Memory maps a file as a dynd array with type 'bytes'.

    Parameters
    ----------
    filename : string
        The name of the file to memory map
    begin : integer, optional
        If provided, provides the offset into the file where memory
        mapping should start. Follows Python slicing convention for
        negative and out of bounds value. (Default 0.)
    end : integer, optional
        If provided, provides the offset into the file where memory
        mapping should stop. Follows Python slicing convention for
        negative and out of bounds value. (Default end of file.)
    access : 'readwrite'/'rw', 'readonly'/'r', or 'immutable', optional
        If provided, this specifies the access control for the
        created array. If the array is being allocated, as in
        construction from Python objects, this is the access control
        set.

    Examples
    --------
    >>> from dynd import nd, ndt

    >>> with open("test.txt", "w") as f:
    ...     f.write("Testing 1 2 3")
    >>> a = nd.memmap("test.txt").view_scalars(ndt.string)
    >>> a
    nd.array("Testing 1 2 3", string)
    """
    cdef w_array result = w_array()
    result.v = array_memmap(filename, begin, end, access)
    return result

def groupby(data, by, groups = None):
    """
    nd.groupby(data, by, groups=None)

    Produces an array containing the elements of `data`, grouped
    according to `by` which has corresponding shape.

    Parameters
    ----------
    data : dynd array
        A one-dimensional array of the data to be copied into
        the resulting grouped array.
    by : dynd array
        A one-dimensional array, of the same size as 'data',
        with the category values for the grouping.
    groups : categorical dynd type, optional
        If provided, the categories of this type are used
        as the groups for the grouping operation.

    Examples
    --------
    >>> from dynd import nd, ndt

    >>> a = nd.groupby([1, 2, 3, 4, 5, 6], ['M', 'F', 'M', 'M', 'F', 'F'])
    >>> a.groups
    nd.array(["F", "M"],
             type="2 * string")
    >>> a.eval()
    nd.array([[2, 5, 6], [1, 3, 4]],
             type="fixed[2] * var * int32")
    >>> a = nd.groupby([1, 2, 3, 4, 5, 6], ['M', 'F', 'M', 'M', 'F', 'F'], ['M', 'N', 'F'])
    >>> a.groups
    nd.array(["M", "N", "F"],
             type="3 * string")
    >>> a.eval()
    nd.array([[1, 3, 4],        [], [2, 5, 6]],
             type="fixed[3] * var * int32")
    """
    cdef w_array result = w_array()
    if groups is None:
        result.v = dynd_groupby(w_array(data).v, w_array(by).v)
    else:
        if type(groups) in [list, w_array]:
            # If groups is a list or dynd array, assume it's a list
            # of groups for a categorical type
            result.v = dynd_groupby(w_array(data).v, w_array(by).v,
                            dynd_make_categorical_type(w_array(groups).v))
        else:
            result.v = dynd_groupby(w_array(data).v, w_array(by).v, w_type(groups).v)
    return result

def range(start=None, stop=None, step=None, dtype=None):
    """
    nd.range(stop, dtype=None)
    nd.range(start, stop, step=None, dtype=None)

    Constructs a dynd array representing a stepped range of values.

    This function assumes that (stop-start)/step is approximately
    an integer, so as to be able to produce reasonable values with
    fractional steps which can't be exactly represented, such as 0.1.

    Parameters
    ----------
    start : int, optional
        If provided, this is the first value in the resulting dynd array.
    stop : int
        This provides the stopping criteria for the range, and is
        not included in the resulting dynd array.
    step : int
        This is the increment.
    dtype : dynd type, optional
        If provided, it must be a scalar type, and the result
        is of this type.
    """
    cdef w_array result = w_array()
    # Move the first argument to 'stop' if stop isn't specified
    if stop is None:
        if start is not None:
            result.v = array_range(None, start, step, dtype)
        else:
            raise ValueError("No value provided for 'stop'")
    else:
        result.v = array_range(start, stop, step, dtype)
    return result

def linspace(start, stop, count=50, dtype=None):
    """
    nd.linspace(start, stop, count=50, dtype=None)

    Constructs a specified count of values interpolating a range.

    Parameters
    ----------
    start : floating point scalar
        The value of the first element of the resulting dynd array.
    stop : floating point scalar
        The value of the last element of the resulting dynd array.
    count : int, optional
        The number of elements in the resulting dynd array.
    dtype : dynd type, optional
        If provided, it must be a scalar type, and the result
        is of this type.
    """
    cdef w_array result = w_array()
    result.v = array_linspace(start, stop, count, dtype)
    return result

def fields(w_array struct_array, *fields_list):
    """
    nd.fields(struct_array, *fields_list)

    Selects fields from an array of structs.

    Parameters
    ----------
    struct_array : dynd array with struct dtype
        A dynd array whose dtype has kind 'struct'. This
        could be a single struct instance, or an array of structs.
    *fields_list : string
        The remaining parameters must all be strings, and are the field
        names to select.
    """
    cdef w_array result = w_array()
    result.v = nd_fields(struct_array.v, fields_list)
    return result

def parse_json(type, json, ectx=None):
    """
    nd.parse_json(type, json, ectx)

    Parses an input JSON string as a particular dynd type.

    Parameters
    ----------
    type : dynd type
        The type to interpret the input JSON as. If the data
        does not match this type, an error is raised during parsing.
    json : string or bytes
        String that contains the JSON to parse.
    ectx : eval_context, optional
        If provided an evaluation context to use when processing the JSON.

    Examples
    --------
    >>> from dynd import nd, ndt

    >>> nd.parse_json('var * int8', '[1, 2, 3, 4, 5]')
    nd.array([1, 2, 3, 4, 5],
             type="var * int8")
    >>> nd.parse_json('4 * int8', '[1, 2, 3, 4]')
    nd.array([1, 2, 3, 4],
             type="4 * int8")
    >>> nd.parse_json('2 * {x: int8, y: int8}', '[{"x":0, "y":1}, {"y":2, "x":3}]')
    nd.array([[0, 1], [3, 2]],
             type="2 * {x : int8, y : int8}")
    """
    cdef w_array result = w_array()
    if builtin_type(type) is w_array:
        dynd_parse_json_array((<w_array>type).v, w_array(json).v, ectx)
    else:
        result.v = dynd_parse_json_type(w_type(type).v, w_array(json).v, ectx)
        return result

def format_json(w_array a, bint tuple=False):
    """
    nd.format_json(a, tuple=False)

    Formats a dynd array as JSON.

    Parameters
    ----------
    n : dynd array
        The object to format as JSON.
    tuple : bool
        If set to true, outputs lists instead of objects/dicts for
        structured types.

    Examples
    --------
    >>> from dynd import nd, ndt

    >>> a = nd.array([[1, 2, 3], [1, 2]])
    >>> a
    nd.array([[1, 2, 3],    [1, 2]],
             type="2 * var * int32")
    >>> nd.format_json(a)
    nd.array("[[1,2,3],[1,2]]",
             type="string")
    """
    cdef w_array result = w_array()
    result.v = dynd_format_json(a.v, tuple != 0)
    return result

def rolling_apply(af, arr, window_size, ectx=None):
    """
    nd.rolling_apply(af, arr, window_size, ectx=None)

    Applies a function to successive overlapping windows
    into the provided array.

    Parameters
    ----------
    af : nd.arrfunc or callable
        The function to apply to each window into ``arr``.
    arr : nd.array
        The array to operate on.
    window_size : int
        How big the window should be.
    ectx : nd.eval_context, optional
        If provided, provides the evaluation context for the operation.
    """
    return arrfunc_rolling_apply(af, arr, window_size, ectx)

def get_published_arrfuncs():
    return dynd_get_published_arrfuncs()

class DebugReprObj(object):
    def __init__(self, repr_str):
        self.repr_str = repr_str

    def __str__(self):
        return self.repr_str

    def __repr__(self):
        return self.repr_str

def debug_repr(obj):
    """
    nd.debug_repr(a)

    Returns a raw representation of dynd array data.

    This can be useful for diagnosing bugs in the dynd array
    or type/arrmeta/data abstraction arrays are based on.

    Parameters
    ----------
    a : dynd array
        The object whose debug repr is desired
    """
    if isinstance(obj, w_array):
        return DebugReprObj(str(<char *>array_debug_print((<w_array>obj).v).c_str()))

cdef class w_array_callable:
    cdef array_callable_wrapper v

    def __call__(self, *args, **kwargs):
        return array_callable_call(self.v, args, kwargs)

cdef class w_type_callable:
    cdef _type_callable_wrapper v

    def __call__(self, *args, **kwargs):
        return _type_callable_call(self.v, args, kwargs)

cdef class w_eval_context:
    """
    nd.eval_context(reset=False,
                    errmode=None,
                    cuda_device_errmode=None,
                    date_parse_order=None,
                    century_window=None)

    Create a dynd evaluation context, overriding the defaults via
    the chosen parameters. Evaluation contexts can be used to
    adjust default settings

    Parameters
    ----------
    reset : bool, optional
        If set to true, first resets the evaluation context to
        factory settings instead of starting with the default
        evaluation context.
    errmode : 'inexact', 'fractional', 'overflow', 'nocheck', optional
        The default error mode used in computations when none is specified.
    cuda_device_errmode : 'inexact', 'fractional', 'overflow', 'nocheck', optional
        The default error mode used in cuda computations when none is
        specified.
    date_parse_order : 'NoAmbig', 'YMD', 'MDY', 'DMY', optional
        How to interpret dates being parsed when the order of year, month and
        day is ambiguous from the format.
    century_window : int, optional
        Whether and how to interpret two digit years. If 0, disallow them.
        If 1-99, use a sliding window beginning that number of years ago.
        If greater than 1000, use a fixed window starting at that year.
    """
    # NOTE: This layout is also accessed from C++
    cdef eval_context *ectx
    cdef bint own_ectx

    def __cinit__(self, *args, **kwargs):
        self.own_ectx = False
        if len(args) > 0:
            raise TypeError('nd.eval_context() accepts no positional args')

        # Start with a copy of the default eval context
        self.ectx = new_eval_context(kwargs)
        self.own_ectx = True

    def __dealloc__(self):
        if self.own_ectx:
            del self.ectx

    property errmode:
        def __get__(self):
            return get_eval_context_errmode(self)

    property cuda_device_errmode:
        def __get__(self):
            return get_eval_context_cuda_device_errmode(self)

    property date_parse_order:
        def __get__(self):
            return get_eval_context_date_parse_order(self)

    property century_window:
        def __get__(self):
            return get_eval_context_century_window(self)

    property _ectx_ptr:
        def __get__(self):
            return <uintptr_t>self.ectx

    def __str__(self):
        return get_eval_context_repr(self)

    def __repr__(self):
        return get_eval_context_repr(self)

def modify_default_eval_context(**kwargs):
    """
    nd.modify_default_eval_context(reset=False,
                    errmode=None,
                    cuda_device_errmode=None,
                    date_parse_order=None,
                    century_window=None)

    Modify the default dynd evaluation context, overriding the defaults via
    the chosen parameters. This is not recommended for typical use
    except in interactive sessions. Using the ``ectx=`` parameter to
    evaluation methods is preferred in library code.

    Parameters
    ----------
    reset : bool, optional
        If set to true, first resets the default evaluation context to
        factory settings.
    errmode : 'inexact', 'fractional', 'overflow', 'nocheck', optional
        The default error mode used in computations when none is specified.
    cuda_device_errmode : 'inexact', 'fractional', 'overflow', 'nocheck', optional
        The default error mode used in cuda computations when none is
        specified.
    date_parse_order : 'NoAmbig', 'YMD', 'MDY', 'DMY', optional
        How to interpret dates being parsed when the order of year, month and
        day is ambiguous from the format.
    century_window : int, optional
        Whether and how to interpret two digit years. If 0, disallow them.
        If 1-99, use a sliding window beginning that number of years ago.
        If greater than 1000, use a fixed window starting at that year.
    """
    dynd_modify_default_eval_context(kwargs)
