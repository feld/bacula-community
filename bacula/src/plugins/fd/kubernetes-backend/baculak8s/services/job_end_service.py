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
import sys
from baculak8s.io.default_io import DefaultIO
from baculak8s.io.log import Log
from baculak8s.io.packet_definitions import TERMINATION_PACKET
from baculak8s.services.service import Service

END_JOB_START_PACKET = "END"
INVALID_END_JOB_START_PACKET = "Invalid End Job Start Packet"
INVALID_TERMINATION_PACKET = "Invalid Termination Packet." \
                             " This indicates a possible error " \
                             "with the reading of the input sent" \
                             "to the Backend."


class JobEndService(Service):
    """
       Service that contains the business logic
       related to ending the Backend Job
    """

    def __init__(self, job_info, plugin):
        # The job_info parameter will probably be used in the future
        self.plugin = plugin
        self.job_info = job_info
        self.io = DefaultIO()

    def execute(self, params=None):
        if self.job_info.get("query", None) is not None:
            return
        self.__read_start()
        self.plugin.disconnect()
        self.__read_termination()

    def __read_start(self):
        _, packet = self.io.read_packet()
        if packet != END_JOB_START_PACKET:
            self.io.send_abort(INVALID_END_JOB_START_PACKET)
            self.__abort()
            return
        self.io.send_eod()

    def __read_termination(self):
        packet_header = self.io.read_line()

        if packet_header != TERMINATION_PACKET:
            self.__abort()

        Log.save_received_termination(packet_header)

    def __abort(self):
        self.plugin.disconnect()
        sys.exit(0)
