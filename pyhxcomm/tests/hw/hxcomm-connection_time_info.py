#!/usr/bin/env python

import unittest
import pyhxcomm_vx as hxcomm


class HwTestPyhxcomm(unittest.TestCase):
    def test_get_time_info(self):
        with hxcomm.ManagedConnection() as connection:
            time_info = connection.time_info
            reference = hxcomm.ConnectionTimeInfo()
            self.assertEqual(time_info, reference)


if __name__ == "__main__":
    unittest.main()
