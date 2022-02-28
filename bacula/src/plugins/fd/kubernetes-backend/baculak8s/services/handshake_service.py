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
from baculak8s.services.service import Service

INVALID_HANDSHAKE_PACKET = "Invalid Handshake Packet"
INVALID_PLUGIN_NAME = "Invalid Plugin Name"
INVALID_PLUGIN_API = "Invalid Plugin API Version"
HANDSHAKE_OK = "Hello Bacula"


class HandshakeService(Service):
    """
       Service that contains the business logic
       related to the Backend Handshake.
    """

    def __init__(self):
        self.io = DefaultIO()

    def execute(self, params=None):
        """
           Reads and parses the Handshake Packet sent to the Backend.
           The Handshake Packet should have the format:
           "Hello $pluginname$ $pluginAPI$".

           :raise SystemExit, in case of a invalid HandshakePacket or in case
                  of unsupported $pluginname$, $pluginAPI$
                  or if the Backend received an invalid Handshake Packet.

           :returns The $pluginname$.
        """

        handshake_data = self.__read_handshake_packet()
        self.__verify_packet_data(handshake_data)
        self.io.send_command(HANDSHAKE_OK)
        return handshake_data[1]

    def __read_handshake_packet(self):
        _, packet = self.io.read_packet()
        if not packet:
            self.io.send_abort(INVALID_HANDSHAKE_PACKET)
            sys.exit(0)

        packet = packet.lower()
        packet_data = packet.split(" ")

        if len(packet_data) != 3 or packet_data[0] != "hello":
            self.io.send_abort(INVALID_HANDSHAKE_PACKET)
            sys.exit(0)
        return packet_data

    def __verify_packet_data(self, packet_data):
        valid_plugins = {
            "kubernetes": "3",
            "openshift": "3",
        }

        plugin_name = packet_data[1]
        plugin_api = packet_data[2]

        if plugin_name not in valid_plugins:
            self.io.send_abort(INVALID_PLUGIN_NAME)
            sys.exit(0)

        if plugin_api is not valid_plugins[plugin_name]:
            self.io.send_abort(INVALID_PLUGIN_API)
            sys.exit(0)
