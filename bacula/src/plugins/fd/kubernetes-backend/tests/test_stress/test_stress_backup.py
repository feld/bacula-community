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


from tests.test_baculak8s.test_system.system_test import SystemTest


class StressBackupTest(SystemTest):
    # TODO Refactor
    pass
    # def test_backup_5M_file(self):
    #     container = "cats"
    #     filename = "cats1.txt"
    #     file_content = self.gen_random_bytes(5 * 1024 * 1024)
    #     self.create_test_data(container, file_content, filename)
    #     command = BackupFileCommandBuilder.build(container, filename)
    #     output = self.execute_plugin(command)
    #     self.verify_param_existence(output, "file", filename)
    #     self.verify_param_existence(output, "content-length", len(file_content))
    #
    # def test_backup_50M_file(self):
    #     container = "cats"
    #     filename = "cats1.txt"
    #     file_content = self.gen_random_bytes(50 * 1024 * 1024)
    #     self.create_test_data(container, file_content, filename)
    #     command = BackupFileCommandBuilder.build(container, filename)
    #     output = self.execute_plugin(command)
    #     self.verify_param_existence(output, "file", filename)
    #     self.verify_param_existence(output, "content-length", len(file_content))
    #
    # def test_backup_100M_file(self):
    #     container = "cats"
    #     filename = "cats1.txt"
    #     file_content = self.gen_random_bytes(100 * 1024 * 1024)
    #     self.create_test_data(container, file_content, filename)
    #     command = BackupFileCommandBuilder.build(container, filename)
    #     output = self.execute_plugin(command)
    #     self.verify_param_existence(output, "file", filename)
    #     self.verify_param_existence(output, "content-length", len(file_content))
    #
    # def test_backup_500M_file(self):
    #     container = "cats"
    #     filename = "cats1.txt"
    #     file_content = self.gen_random_bytes(500 * 1024 * 1024)
    #     self.create_test_data(container, file_content, filename)
    #     command = BackupFileCommandBuilder.build(container, filename)
    #     output = self.execute_plugin(command)
    #     self.verify_param_existence(output, "file", filename)
    #     self.verify_param_existence(output, "content-length", len(file_content))
