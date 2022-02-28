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

from baculak8s.jobs.backup_job import BackupJob
from baculak8s.jobs.estimation_job import EstimationJob
from baculak8s.jobs.listing_job import ListingJob
from baculak8s.jobs.query_job import QueryJob
from baculak8s.jobs.restore_job import RestoreJob
from baculak8s.services.job_info_service import (TYPE_BACKUP, TYPE_ESTIMATION,
                                                 TYPE_RESTORE)


class JobFactory(object):
    """
        Creates a Job that will be executed by the Backend

        :param job_type: The type of the created Job (Backup, Restore, Estimation)
        :param plugin: The plugin that this Job should use

        :raise: ValueError, if an invalid job_type was provided

    """

    @staticmethod
    def create(params, plugin):
        if params["type"] == TYPE_BACKUP:
            return BackupJob(plugin, params)

        elif params["type"] == TYPE_RESTORE:
            return RestoreJob(plugin, params)

        elif params["type"] == TYPE_ESTIMATION:
            if params.get("listing", None) is not None:
                return ListingJob(plugin, params)
            if params.get("query", None) is not None:
                return QueryJob(plugin, params)

            return EstimationJob(plugin, params)

        else:
            raise ValueError("Invalid Job Type")
