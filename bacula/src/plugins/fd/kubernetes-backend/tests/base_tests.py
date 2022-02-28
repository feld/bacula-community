# -*- coding: UTF-8 -*-
# Bacula(R) - The Network Backup Solution
#
#   Copyright (C) 2000-2022 Kern Sibbald
#
#   The original author of Bacula is Kern Sibbald, with contributions
#   from many others, a complete list can be found in the file AUTHORS.
#
#   You may use this file and others of this release according to the
#   license defined in the LICENSE file, which includes the Affero General
#   Public License, v3.0 ("AGPLv3") and some additional permissions and
#   terms pursuant to its AGPLv3 Section 7.
#
#   This notice must be preserved when any source code is
#   conveyed and/or propagated.
#
#   Bacula(R) is a registered trademark of Kern Sibbald.


import unittest

from tests import TMP_TEST_FILES_ROOT
from tests.util.io_test_util import IOTestUtil
from tests.util.os_test_util import OsTestUtil


class BaseTest(unittest.TestCase, OsTestUtil, IOTestUtil):
    """
        Base Class for all tests
    """

    def setUp(self):
        self._create_test_folders()

    def _create_test_folders(self):
        self.test_files_root = self.create_test_dir(TMP_TEST_FILES_ROOT)

    def tearDown(self):
        self.delete_if_dir(self.test_files_root)

    def assertItemsContainsValueType(self, items, key, val_type):
        for item in items:
            self.assertDictContainsKey(item, key, val_type)

    def assertDictContainsKey(self, item, key, val_type):
        self.assertIn(key, item)
        self.assertIsInstance(item[key], val_type)

    def assertItemsContainsValue(self, items, key, val):
        for item in items:
            self.assertEqual(item[key], val)

    def assertIsSubsetOf(self, subset, dictionary):
        isSubset = set(subset.items()).issubset(set(dictionary.items()))
        self.assertTrue(isSubset, "Wasn't a subset!")
