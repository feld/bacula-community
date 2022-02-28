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
import datetime

from baculak8s.io.default_io import DefaultIO
from baculak8s.io.log import Log
from baculak8s.io.packet_definitions import EOD_PACKET

JOB_START_PACKET = "Job"
INVALID_JOB_START_PACKET = "Invalid Job Start Packet"
INVALID_JOB_PARAMETER_BLOCK = "Invalid Job Parameter Block"
INVALID_JOB_TYPE = "Invalid Job Type. The supported types are:" \
                   "B - Backup, R - Restore or E - Estimation"
INVALID_REPLACE_PARAM = "Invalid Replace Parameter. The supported values are:" \
                        "a - Replace always, w - Replace if newer, n - Never replace" \
                        "or o - Replace if older"
JOB_NAME_NOT_FOUND = "Parameter Job Name Not Found"
JOB_ID_NOT_FOUND = "Parameter Job ID Not Found"
JOB_TYPE_NOT_FOUND = "Parameter Job Type Not Found"


class JobInfoIO(DefaultIO):

    def read_job_info(self):
        """
            Reads blocks of parameters:

                "C000111\n"     (Command packet header)
                key1=value1\n   (Parameter)
                "C000222\n"
                key2=value2\n
                "C000333\n"
                key3=value3\n
                ...

            until an EOD packet (F000000) is found

            :return: A dictionary containing the parameters

        """

        block = {}
        while True:
            packet_header = self.read_line()
            if not packet_header:
                raise ValueError("Packet Header not found")

            if packet_header == EOD_PACKET:
                Log.save_received_eod(packet_header)
                return block
            else:
                packet_content = self.read_line().decode()
                param = packet_content.split("=", 1)
                # converts key to lowercase
                param[0] = param[0].lower()

                if param[0] == "since":
                    # converts the provided timestamp to a utc timestamp
                    parsed_param = int(param[1])
                    parsed_param = int(datetime.datetime \
                                       .utcfromtimestamp(parsed_param) \
                                       .replace(tzinfo=datetime.timezone.utc) \
                                       .timestamp())
                    block[param[0]] = parsed_param
                else:
                    block[param[0]] = param[1]

                Log.save_received_packet(packet_header, packet_content)
