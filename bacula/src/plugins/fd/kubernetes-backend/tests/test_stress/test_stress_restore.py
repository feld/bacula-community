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
import time

from baculaswift.plugins.plugin import MEGABYTE
from baculaswift.services.job_info_service import TYPE_RESTORE
from tests.test_baculak8s.test_system.system_test import SystemTest
from tests.util.os_test_util import create_byte_chunks
from tests.util.packet_builders import BackendCommandBuilder


class StressRestoreTest(SystemTest):
    def test_restore_100M_file_regular_upload_65K_data_packets(self):
        total_size = 1600 * 65553
        buckets = [{
            "name": "bucket_1",
            "files": [
                {
                    "name": "file1.txt",
                    "size": total_size,
                    "content": create_byte_chunks(1600, 65553)
                },
            ]
        }]
        packet = BackendCommandBuilder().build(TYPE_RESTORE, buckets, segment_size=8000 * MEGABYTE)
        start = time.clock()
        output = self.execute_plugin(packet)
        end = time.clock()
        elapsed = (end - start)
        self.verify_no_aborts(output)
        resp = self.swift_connection.head_object("bucket_1", "file1.txt")
        self.assertEqual(total_size, int(resp["content-length"]))

    def test_restore_100M_file_regular_upload_999K_data_packets(self):
        total_size = 105 * 999999
        buckets = [{
            "name": "bucket_1",
            "files": [
                {
                    "name": "file1.txt",
                    "size": total_size,
                    "content": create_byte_chunks(105, 999999)
                },
            ]
        }]
        packet = BackendCommandBuilder().build(TYPE_RESTORE, buckets, segment_size=8000 * MEGABYTE)
        start = time.clock()
        output = self.execute_plugin(packet)
        end = time.clock()
        elapsed = (end - start)
        self.verify_no_aborts(output)
        resp = self.swift_connection.head_object("bucket_1", "file1.txt")
        self.assertEqual(total_size, int(resp["content-length"]))
