#!/usr/bin/env python
# encoding: utf-8

import unittest
import collections as c
import functools as ft
import itertools as it


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

    @property
    def connections(self):
        return ["Connection", "ARQConnection", "SimConnection"]

    @property
    def all_objects(self):
        return it.chain(self.handles, self.managed_connections)

    @property
    def handles(self):
        return map(lambda c: f"{c}Handle", self.connections)

    @property
    def managed_connections(self):
        return map(lambda c: f"Managed{c}", self.connections)

    @property
    def get_from_toplevel(self):
        return ft.partial(self.check_arg_exists, self.hxcomm)

    def test_context_import(self):
        c.deque(map(self.get_from_toplevel, self.all_objects), maxlen=0)

    def test_handle_not_constructable(self):
        for Handle in map(self.get_from_toplevel, self.handles):
            with self.assertRaises(TypeError):
                Handle()

    def check_arg_exists(self, parent, arg):
        retrieved = getattr(parent, arg, None)
        self.assertTrue(retrieved is not None,
                        msg=f"{str(parent)}.{arg} does not exist!")
        return retrieved


if __name__ == '__main__':
    unittest.main()
