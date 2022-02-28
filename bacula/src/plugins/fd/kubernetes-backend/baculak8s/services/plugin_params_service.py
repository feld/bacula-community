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
import os

from baculak8s.io.services.plugin_params_io import PluginParamsIO, INVALID_PLUGIN_PARAMETERS_BLOCK, URL_NOT_FOUND, \
    USER_NOT_FOUND, PWD_NOT_FOUND, PASSFILE_NOT_FOUND, PWD_INSIDE_PASSFILE_NOT_FOUND, RESTORE_LOCAL_WITHOUT_WHERE, \
    PLUGIN_PARAMETERS_START, INVALID_PLUGIN_PARAMETERS_START
from baculak8s.services.service import Service


class PluginParamsService(Service):
    """
       Service that contains the business logic
       related to reading and parsing the information about the Plugin
       that will be used by the Job to fulfill it's purpose
    """

    def __init__(self, job_info):
        self.io = PluginParamsIO()
        self.job_info = job_info

    def execute(self, params=None):
        self.__read_start()
        params_block = self.__read_params_block()
        self.io.send_eod()
        return params_block

    def __read_start(self):
        _, packet = self.io.read_packet()

        if packet != PLUGIN_PARAMETERS_START:
            self.io.send_abort(INVALID_PLUGIN_PARAMETERS_START)
            sys.exit(0)

    def __read_params_block(self):
        params_block = self.io.read_plugin_params()

        # Parameters validation
        if not params_block:
            self.io.send_abort(INVALID_PLUGIN_PARAMETERS_BLOCK)
            sys.exit(0)

        if "restore_local" in params_block and ("where" not in self.job_info or self.job_info["where"] == ""):
            self.io.send_abort(RESTORE_LOCAL_WITHOUT_WHERE)
            sys.exit(0)

        if 'pv' in params_block:
            # 'pv' is an alias for 'persistentvolume'
            params_block.get('persistentvolume').append(params_block.get('pv'))
            del params_block['pv']

        if 'ns' in params_block:
            # 'ns' is an alias for 'namespace'
            params_block.get('namespace').append(params_block.get('ns'))
            del params_block['ns']

        return params_block

    def __get_password(self, params_block):
        """
           Reads the password from the passfile and puts it into the Plugin Params
        """
        if not os.path.isfile(params_block["passfile"]):
            self.io.send_abort(PASSFILE_NOT_FOUND)
            sys.exit(0)

        with open(params_block["passfile"], "r") as f:
            params_block["password"] = f.readline()
            f.close()

        if not params_block["password"]:
            self.io.send_abort(PWD_INSIDE_PASSFILE_NOT_FOUND)
            sys.exit(0)

        return params_block
