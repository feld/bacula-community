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
from io import RawIOBase

from swiftclient.service import SwiftUploadObject

from tests.base_tests import BaseTest


class ObjectTest(BaseTest):
    def tearDown(self):
        self.delete_all_containers()

    def test_should_upload_object(self):
        container = "cats"
        filename = "cats1.txt"
        file_like = FileLikeObject([self.gen_random_bytes(65553) for i in range(50)])
        obj = SwiftUploadObject(file_like, filename)
        self.swift_connection.put_container(container)
        r_gen = self.swift_service.upload(container, [obj])

        for r in r_gen:
            pass

        c_data = self.swift_connection.get_container(container)
        print(c_data)


class FileLikeObject(RawIOBase):
    def __init__(self, file_chunks, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.file_chunks = file_chunks

    def read(self, size=-1):
        if not self.file_chunks:
            return None

        return self.file_chunks.pop()

    def next_chunk(self, chunk_size):
        pass


if __name__ == '__main__':
    unittest.main()
