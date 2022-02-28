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
import logging
import baculak8s
from baculak8s.io.services.job_info_io import JobInfoIO, INVALID_JOB_PARAMETER_BLOCK, INVALID_JOB_TYPE, \
    JOB_NAME_NOT_FOUND, JOB_ID_NOT_FOUND, JOB_TYPE_NOT_FOUND, INVALID_REPLACE_PARAM, JOB_START_PACKET, \
    INVALID_JOB_START_PACKET
from baculak8s.services.service import Service

TYPE_BACKUP = "b"
TYPE_RESTORE = "r"
TYPE_ESTIMATION = "e"

REPLACE_ALWAYS = 'a'
REPLACE_IFNEWER = 'w'
REPLACE_NEVER = 'n'
REPLACE_IFOLDER = 'o'


class JobInfoService(Service):
    """
       Service that contains the business logic
       related to reading and parsing the information about the Job
       that should be created and executed by the Backend.
    """

    def __init__(self):
        self.io = JobInfoIO()

    def execute(self, params=None):
        self.__read_start()
        params_block = self.__read_params_block()
        self.io.send_eod()
        return params_block

    def __read_start(self):
        _, packet = self.io.read_packet()

        if packet != JOB_START_PACKET:
            self.io.send_abort(INVALID_JOB_START_PACKET)
            sys.exit(0)

    def __read_params_block(self):
        job_info = self.io.read_job_info()

        # Parameters validation
        if not job_info:
            self.io.send_abort(INVALID_JOB_PARAMETER_BLOCK)
            sys.exit(0)
        if "name" not in job_info:
            self.io.send_abort(JOB_NAME_NOT_FOUND)
            sys.exit(0)
        if "jobid" not in job_info:
            self.io.send_abort(JOB_ID_NOT_FOUND)
            sys.exit(0)
        if "replace" in job_info:
            job_info["replace"] = job_info["replace"].lower()
            if job_info["replace"] not in [REPLACE_ALWAYS, REPLACE_IFNEWER, REPLACE_NEVER, REPLACE_IFOLDER]:
                self.io.send_abort(INVALID_REPLACE_PARAM)
                sys.exit(0)
        if "namespace" in job_info:
            logging.debug("FILE NAMESPACE: {}".format(job_info.get("namespace")))
            baculak8s.plugins.k8sbackend.k8sfileinfo.defaultk8spath = job_info.get("namespace")
            job_info.pop("namespace")
        if "type" not in job_info:
            self.io.send_abort(JOB_TYPE_NOT_FOUND)
            sys.exit(0)
        else:
            job_info["type"] = job_info["type"].lower()
            if job_info["type"] not in [TYPE_BACKUP, TYPE_RESTORE, TYPE_ESTIMATION]:
                self.io.send_abort(INVALID_JOB_TYPE)
                sys.exit(0)

        return job_info
