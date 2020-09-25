import unittest
import epidemiology.secir as secir

class Test_ParameterDistribution(unittest.TestCase):
    def test_uniform(self):
        U = secir.ParameterDistributionUniform(1.0, 2.0)
        #properties
        self.assertEqual(U.lower_bound, 1.0)
        self.assertEqual(U.upper_bound, 2.0)
        #sample
        u = U.get_sample()
        self.assertGreaterEqual(u, 1.0)
        self.assertLessEqual(u, 2.0)

    def test_normal(self):
        N = secir.ParameterDistributionNormal(-1.0, 1.0, 0.0, 1.0)        
        #properties
        self.assertEqual(N.mean, 0.0)#std_dev automatically adapted
        self.assertEqual(N.lower_bound, -1.0)
        self.assertEqual(N.upper_bound, 1.0)        
        #sample
        n = N.get_sample()
        self.assertGreaterEqual(n, -1)
        self.assertLessEqual(n, 1)

    def test_polymorphic(self):
        uv = secir.UncertainValue()
        N = secir.ParameterDistributionNormal(0, 2, 1.0)
        uv.set_distribution(N)
        self.assertIsNot(uv.get_distribution(), N)
        self.assertIsInstance(uv.get_distribution(), secir.ParameterDistributionNormal)
        self.assertIsInstance(uv.get_distribution(), secir.ParameterDistribution)
        self.assertEqual(uv.get_distribution().mean, 1.0)
        
if __name__ == '__main__':
    unittest.main()