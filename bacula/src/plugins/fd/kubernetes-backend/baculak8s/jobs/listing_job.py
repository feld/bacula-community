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

LISTING_START = "ListingStart"
LISTING_EMPTY_RESULT = "Listing returned an empty result"
LISTING_ERROR_RESPONSE = "Listing returned error response: {}"


class ListingJob(Job):
    """
        Job that contains the business logic
        related to the listing mode of the Backend.
        It depends upon a Plugin Class implementation
        that retrieves listing data from the Plugins Data Source
    """

    def __init__(self, plugin, params):
        super().__init__(plugin, DefaultIO(), params)

    def execute(self):
        self._start(LISTING_START)
        self.execution_loop()
        self._io.send_eod()

    def execution_loop(self):
        self.__listing_loop()

    def __listing_loop(self):
        found_any = False
        file_info_list = self._plugin.list_in_path(self._params["listing"])
        if isinstance(file_info_list, dict) and 'error' in file_info_list:
            logging.warning(file_info_list)
            self._io.send_warning(LISTING_ERROR_RESPONSE.format(parse_json_descr(file_info_list)))
        else:
            for file_info in file_info_list:
                found_any = True
                self._io.send_file_info(file_info_list.get(file_info).get('fi'))

            if not found_any:
                self._io.send_warning(LISTING_EMPTY_RESULT)
