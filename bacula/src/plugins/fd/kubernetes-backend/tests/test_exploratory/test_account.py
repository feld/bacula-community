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

from tests.base_tests import BaseTest


class AccountTest(BaseTest):

    def test_should_retrieve_account_stats(self):
        stats = self.swift_service.stat()
        self.assertEqual(True, stats['success'])

    def test_should_list_account_containers_metadata(self):
        containers_meta = self.swift_service.list()
        for metadata in containers_meta:
            self.assertEqual(True, metadata['success'])


if __name__ == '__main__':
    unittest.main()