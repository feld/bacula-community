#   The main author of Bacula(R) - The Network Backup Solution
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
import logging
import traceback

from baculak8s.io.default_io import DefaultIO
from baculak8s.io.packet_definitions import UNEXPECTED_ERROR_PACKET
from baculak8s.services.service import Service


class UnexpectedErrorService(Service):
    """
       Service that is executed whenever an unexpected exception happens
       during the Backend execution.
    """

    def __init__(self):
        self.io = DefaultIO()

    def execute(self, params=None):
        # We log the Exception Stack Trace
        logging.error(UNEXPECTED_ERROR_PACKET)
        logging.error(traceback.format_exc())
        self.io.send_abort(UNEXPECTED_ERROR_PACKET)
