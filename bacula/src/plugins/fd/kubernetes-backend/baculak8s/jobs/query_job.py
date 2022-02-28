# -*- coding: UTF-8 -*-
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

import logging

from baculak8s.io.default_io import DefaultIO
from baculak8s.jobs.job import Job
from baculak8s.util.respbody import parse_json_descr

QUERY_START = "QueryStart"
QUERY_ERROR_RESPONSE = "QueryParam returned error response: {}"


class QueryJob(Job):
    """
        Job that contains the business logic
        related to the queryParams mode of the Backend.
        It depends upon a Plugin Class implementation
        that retrieves listing data from the Plugins Data Source
    """

    def __init__(self, plugin, params):
        super().__init__(plugin, DefaultIO(), params)

    def execute(self):
        self._start(QUERY_START)
        self.execution_loop()
        self._io.send_eod()

    def execution_loop(self):
        self.__query_loop()

    def __query_loop(self):
        found_any = False
        query_param_list = self._plugin.query_parameter(self._params["query"])
        if isinstance(query_param_list, dict) and 'error' in query_param_list:
            logging.warning(query_param_list)
            self._io.send_warning(QUERY_ERROR_RESPONSE.format(parse_json_descr(query_param_list)))
        else:
            for param_data in query_param_list:
                found_any = True
                self._io.send_query_response(param_data)
