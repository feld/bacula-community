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


import os
import shutil


class OsTestUtil:
    """
       Class with utility methods for dealing with System Calls
    """

    def create_test_dir(self, dir_name):
        self.delete_if_dir(dir_name)
        os.mkdir(dir_name)
        return dir_name

    def create_test_file(self, dir, filename, bytes_content):
        path = os.path.join(dir, filename)
        file = open(path, 'w+b')
        file.write(bytes_content)
        file.close()
        return path

    def delete_if_dir(self, path):
        if os.path.isdir(path):
            shutil.rmtree(path)

    def gen_random_bytes(self, bytes_size):
        return os.urandom(bytes_size)


def gen_random_bytes(bytes_size):
    return os.urandom(bytes_size)


def create_byte_chunks(chunk_count, chunk_size):
    chunk = gen_random_bytes(chunk_size)
    return [chunk] * chunk_count
