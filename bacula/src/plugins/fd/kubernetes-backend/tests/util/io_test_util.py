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


import io
import sys

from baculaswift.io.packet_definitions import EOD_PACKET, STATUS_COMMAND, STATUS_DATA, STATUS_ABORT, STATUS_ERROR, \
    STATUS_WARNING


class IOTestUtil(object):
    """
       Class with utility methods for dealing with IO
    """

    def stub_bytestream_stdin(testcase_inst, input):
        stdin = sys.stdin

        def cleanup():
            sys.stdin = stdin

        wrapper = io.TextIOWrapper(
            io.BytesIO(input),
            newline='\n'
        )

        testcase_inst.addCleanup(cleanup)
        sys.stdin = wrapper

    def stub_stdout(testcase_inst):
        wrapper = io.TextIOWrapper(
            io.BytesIO(),
            newline='\n'
        )

        saved_stdout = sys.stdout
        stub_stdout = wrapper
        sys.stdout = stub_stdout

        def cleanup():
            stub_stdout.close()
            sys.stdout = saved_stdout

        testcase_inst.addCleanup(cleanup)
        return stub_stdout

    def verify_command_packet(self, output, packet):
        self.verify_packet_existence(output, STATUS_COMMAND, packet.encode())

    def verify_not_command_packet(self, output, packet):
        self.verify_not_packet_existence(output, STATUS_COMMAND, packet.encode())

    def verify_data_packet(self, output, packet):
        packet_length = str(len(packet)).zfill(6).encode()
        packet_header = STATUS_DATA.encode() + packet_length + b'\n'
        packet = packet
        self.assertIn(packet_header + packet, output)

    def verify_abort_packet(self, output, packet):
        self.verify_packet_existence(output, STATUS_ABORT, packet.encode())

    def verify_error_packet(self, output, packet):
        self.verify_packet_existence(output, STATUS_ERROR, packet.encode())

    def verify_warning_packet(self, output, packet):
        self.verify_packet_existence(output, STATUS_WARNING, packet.encode())

    def verify_no_aborts(self, output):
        self.assertNotIn(b'A00000', output)
        self.assertNotIn(b'A0000', output)
        self.assertNotIn(b'A000', output)

    def verify_no_errors(self, output):
        self.assertNotIn(b'E00000', output)
        self.assertNotIn(b'E0000', output)
        self.assertNotIn(b'E000', output)

    def verify_no_warnings(self, output):
        self.assertNotIn(b'W00000', output)
        self.assertNotIn(b'W0000', output)
        self.assertNotIn(b'W000', output)

    def verify_eod_packet(self, output):
        self.assertIn(EOD_PACKET, output)

    def verify_eod_packet_count(self, output, expected_count):
        self.verify_packet_count(output, EOD_PACKET, expected_count)

    def verify_str_count(self, output, packet, expected_count):
        real_count = output.count(packet.encode())
        self.assertEquals(expected_count, real_count)

    def verify_packet_count(self, output, packet, expected_count):
        real_count = output.count(packet)
        self.assertEquals(expected_count, real_count)

    def verify_packet_existence(self, output, status, packet):
        packet_length = str(len(packet) + 1).zfill(6).encode()
        packet_header = status.encode() + packet_length + b'\n'
        packet = packet
        self.assertIn(packet_header + packet, output)

    def verify_not_packet_existence(self, output, status, packet):
        packet_length = str(len(packet) + 1).zfill(6).encode()
        packet_header = status.encode() + packet_length + b'\n'
        packet = packet
        self.assertNotIn(packet_header + packet, output)
