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
        self.check_arg_exists(self.hxcomm, "Connection")
        self.check_arg_exists(self.hxcomm, "ARQConnection")
        self.check_arg_exists(self.hxcomm, "SimConnection")
        self.check_arg_exists(self.hxcomm, "QuiggeldyClient")
        self.check_arg_exists(self.hxcomm, "get_connection_from_env")
        self.check_arg_exists(self.hxcomm, "TargetRestriction")
        self.check_arg_exists(self.hxcomm.ARQConnection, "supports")
        self.check_arg_exists(self.hxcomm.SimConnection, "supports")
        self.check_arg_exists(self.hxcomm.QuiggeldyClient, "supports")

    def check_arg_exists(self, parent, arg):
        self.assertTrue(getattr(parent, arg, None) is not None)


if __name__ == '__main__':
    unittest.main()
