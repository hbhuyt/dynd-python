import sys
import unittest
from datetime import date
from dynd import nd, ndt

class TestStringConstruct(unittest.TestCase):
    def test_empty_array(self):
        # Empty arrays default to float64
        # TODO: Is there a better default, e.g. "void"?
        a = nd.array([])
        self.assertEqual(a.dtype, nd.dtype('M, float64'))
        self.assertEqual(a.shape, (0,))
        a = nd.array([[], [], []])
        self.assertEqual(a.dtype, nd.dtype('M, N, float64'))
        self.assertEqual(a.shape, (3, 0))

    def test_string(self):
        a = nd.array('abc', dtype=ndt.string)
        self.assertEqual(a.dtype, ndt.string)
        a = nd.array('abc', udtype=ndt.string)
        self.assertEqual(a.dtype, ndt.string)

    def test_string_array(self):
        a = nd.array(['this', 'is', 'a', 'test'],
                        udtype=ndt.string)
        self.assertEqual(a.dtype, nd.dtype('N, string'))
        self.assertEqual(nd.as_py(a), ['this', 'is', 'a', 'test'])

        a = nd.array(['this', 'is', 'a', 'test'],
                        udtype='string("U16")')
        self.assertEqual(a.dtype, nd.dtype('N, string("U16")'))
        self.assertEqual(nd.as_py(a), ['this', 'is', 'a', 'test'])

    def test_fixedstring_array(self):
        a = nd.array(['a', 'b', 'c'],
                        udtype='string(1,"A")')
        self.assertEqual(a[0].dtype.type_id, 'fixedstring')
        self.assertEqual(a[0].dtype.data_size, 1)
        self.assertEqual(nd.as_py(a), ['a', 'b', 'c'])

class TestStructConstruct(unittest.TestCase):
    def test_single_struct(self):
        a = nd.array([12, 'test', True], dtype='{x:int32; y:string; z:bool}')
        self.assertEqual(a.dtype, nd.dtype('{x:int32; y:string; z:bool}'))
        self.assertEqual(nd.as_py(a[0]), 12)
        self.assertEqual(nd.as_py(a[1]), 'test')
        self.assertEqual(nd.as_py(a[2]), True)

        a = nd.array({'x':12, 'y':'test', 'z':True}, dtype='{x:int32; y:string; z:bool}')
        self.assertEqual(a.dtype, nd.dtype('{x:int32; y:string; z:bool}'))
        self.assertEqual(nd.as_py(a[0]), 12)
        self.assertEqual(nd.as_py(a[1]), 'test')
        self.assertEqual(nd.as_py(a[2]), True)

    def test_nested_struct(self):
        a = nd.array([[1,2], ['test', 3.5], [3j]],
                    dtype='{x: 2, int16; y: {a: string; b: float64}; z: 1, cfloat32}')
        self.assertEqual(nd.as_py(a.x), [1, 2])
        self.assertEqual(nd.as_py(a.y.a), 'test')
        self.assertEqual(nd.as_py(a.y.b), 3.5)
        self.assertEqual(nd.as_py(a.z), [3j])

        a = nd.array({'x':[1,2], 'y':{'a':'test', 'b':3.5}, 'z':[3j]},
                    dtype='{x: 2, int16; y: {a: string; b: float64}; z: 1, cfloat32}')
        self.assertEqual(nd.as_py(a.x), [1, 2])
        self.assertEqual(nd.as_py(a.y.a), 'test')
        self.assertEqual(nd.as_py(a.y.b), 3.5)
        self.assertEqual(nd.as_py(a.z), [3j])

    def test_single_struct_array(self):
        a = nd.array([(0,0), (3,5), (12,10)], udtype='{x:int32; y:int32}')
        self.assertEqual(a.dtype, nd.dtype('N, {x:int32; y:int32}'))
        self.assertEqual(nd.as_py(a.x), [0, 3, 12])
        self.assertEqual(nd.as_py(a.y), [0, 5, 10])

        a = nd.array([{'x':0,'y':0}, {'x':3,'y':5}, {'x':12,'y':10}],
                    udtype='{x:int32; y:int32}')
        self.assertEqual(a.dtype, nd.dtype('N, {x:int32; y:int32}'))
        self.assertEqual(nd.as_py(a.x), [0, 3, 12])
        self.assertEqual(nd.as_py(a.y), [0, 5, 10])

        a = nd.array([[(3, 'X')], [(10, 'L'), (12, 'M')]],
                        udtype='{count:int32; size:string(1,"A")}')
        self.assertEqual(a.dtype, nd.dtype('N, var, {count:int32; size:string(1,"A")}'))
        self.assertEqual(nd.as_py(a.count), [[3], [10, 12]])
        self.assertEqual(nd.as_py(a.size), [['X'], ['L', 'M']])

        a = nd.array([[{'count':3, 'size':'X'}],
                        [{'count':10, 'size':'L'}, {'count':12, 'size':'M'}]],
                        udtype='{count:int32; size:string(1,"A")}')
        self.assertEqual(a.dtype, nd.dtype('N, var, {count:int32; size:string(1,"A")}'))
        self.assertEqual(nd.as_py(a.count), [[3], [10, 12]])
        self.assertEqual(nd.as_py(a.size), [['X'], ['L', 'M']])

    def test_nested_struct_array(self):
        a = nd.array([((0,1),0), ((2,2),5), ((100,10),10)],
                    udtype='{x:{a:int16; b:int16}; y:int32}')
        self.assertEqual(a.dtype, nd.dtype('N, {x:{a:int16; b:int16}; y:int32}'))
        self.assertEqual(nd.as_py(a.x.a), [0, 2, 100])
        self.assertEqual(nd.as_py(a.x.b), [1, 2, 10])
        self.assertEqual(nd.as_py(a.y), [0, 5, 10])

        a = nd.array([{'x':{'a':0,'b':1},'y':0},
                        {'x':{'a':2,'b':2},'y':5},
                        {'x':{'a':100,'b':10},'y':10}],
                    udtype='{x:{a:int16; b:int16}; y:int32}')
        self.assertEqual(a.dtype, nd.dtype('N, {x:{a:int16; b:int16}; y:int32}'))
        self.assertEqual(nd.as_py(a.x.a), [0, 2, 100])
        self.assertEqual(nd.as_py(a.x.b), [1, 2, 10])
        self.assertEqual(nd.as_py(a.y), [0, 5, 10])

        a = nd.array([[(3, ('X', 10))], [(10, ('L', 7)), (12, ('M', 5))]],
                        udtype='{count:int32; size:{name:string(1,"A"); id: int8}}')
        self.assertEqual(a.dtype,
                    nd.dtype('N, var, {count:int32; size:{name:string(1,"A"); id: int8}}'))
        self.assertEqual(nd.as_py(a.count), [[3], [10, 12]])
        self.assertEqual(nd.as_py(a.size.name), [['X'], ['L', 'M']])
        self.assertEqual(nd.as_py(a.size.id), [[10], [7, 5]])

    def test_missing_field(self):
        self.assertRaises(RuntimeError, nd.array,
                        [0, 1], udtype='{x:int32; y:int32; z:int32}')
        self.assertRaises(RuntimeError, nd.array,
                        {'x':0, 'z':1}, udtype='{x:int32; y:int32; z:int32}')

    def test_extra_field(self):
        self.assertRaises(RuntimeError, nd.array,
                        [0, 1, 2, 3], udtype='{x:int32; y:int32; z:int32}')
        self.assertRaises(RuntimeError, nd.array,
                        {'x':0,'y':1,'z':2,'w':3}, udtype='{x:int32; y:int32; z:int32}')

class TestIteratorConstruct(unittest.TestCase):
    # Test dynd construction from iterators
    # NumPy's np.fromiter(x, dtype) becomes nd.array(x, dtype='var, <dtype>')

    def test_simple_fromiter(self):
        # Var dimension construction from a generator
        a = nd.array((2*x + 5 for x in range(10)), dtype='var, int32')
        self.assertEqual(a.dtype, nd.dtype('var, int32'))
        self.assertEqual(len(a), 10)
        self.assertEqual(nd.as_py(a), [2*x + 5 for x in range(10)])
        # Fixed dimension construction from a generator
        a = nd.array((2*x + 5 for x in range(10)), dtype='10, int32')
        self.assertEqual(a.dtype, nd.dtype('10, int32'))
        self.assertEqual(len(a), 10)
        self.assertEqual(nd.as_py(a), [2*x + 5 for x in range(10)])
        # Produce an error if it's a fixed dimension with too few elements
        self.assertRaises(nd.BroadcastError, nd.array,
                        (2*x + 5 for x in range(10)), dtype='11, int32')
        # Produce an error if it's a fixed dimension with too many elements
        self.assertRaises(nd.BroadcastError, nd.array,
                        (2*x + 5 for x in range(10)), dtype='9, int32')
        # Produce an error if it's a strided dimension
        self.assertRaises(RuntimeError, nd.array,
                        (2*x + 5 for x in range(10)), dtype='M, int32')

    def test_simple_fromiter_medsize(self):
        # A bigger input to exercise the dynamic resizing a bit
        a = nd.array((2*x + 5 for x in range(100000)), dtype='var, int32')
        self.assertEqual(a.dtype, nd.dtype('var, int32'))
        self.assertEqual(len(a), 100000)
        self.assertEqual(nd.as_py(a), [2*x + 5 for x in range(100000)])

    def test_ragged_fromiter(self):
        # Strided array of var from list of iterators
        a = nd.array([(1+x for x in range(3)), (5*x - 10 for x in range(5)),
                        [2, 10]], dtype='M, var, int32')
        self.assertEqual(a.dtype, nd.dtype('M, var, int32'))
        self.assertEqual(nd.as_py(a),
                        [[1,2,3], [-10, -5, 0, 5, 10], [2, 10]])
        # Var array of var from iterator of iterators
        a = nd.array(((2*x for x in range(y)) for y in range(4)),
                        dtype='var, var, int32')
        self.assertEqual(a.dtype, nd.dtype('var, var, int32'))
        self.assertEqual(nd.as_py(a), [[], [0], [0, 2], [0, 2, 4]])

    def test_uniform_fromiter(self):
        # Specify uniform type instead of full type
        a = nd.array((2*x + 1 for x in range(7)), udtype=ndt.int32)
        self.assertEqual(a.dtype, nd.dtype('var, int32'))
        self.assertEqual(nd.as_py(a), [2*x + 1 for x in range(7)])

if __name__ == '__main__':
    unittest.main()
