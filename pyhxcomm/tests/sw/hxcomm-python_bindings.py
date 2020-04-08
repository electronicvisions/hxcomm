#!/usr/bin/env python
# encoding: utf-8

import unittest


class PyBindings(unittest.TestCase):
    def setUp(self):
        import pyhxcomm_vx
        self.hxcomm = pyhxcomm_vx

    def test_simple_import(self):
        """
        Since most connection types are not default-constructable and acquire
        their connection handle upon construction, we can only check for their
        existence here.
        """
        self.check_arg_exists("Connection")
        self.check_arg_exists("ARQConnection")
        self.check_arg_exists("SimConnection")
        self.check_arg_exists("get_connection_from_env")

    def check_arg_exists(self, arg):
        self.assertTrue(getattr(self.hxcomm, arg, None) is not None)


if __name__ == '__main__':
    unittest.main()
