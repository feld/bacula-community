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

import datetime

from baculak8s.util.iso8601 import parse_date


def iso8601_to_unix_timestamp(iso_string):
    dt = parse_date(iso_string) \
        .replace(tzinfo=datetime.timezone.utc)
    return int(dt.timestamp())


def gmt_to_unix_timestamp(gmt_string):
    dt = datetime.datetime.strptime(gmt_string, "%a, %d %b %Y %H:%M:%S GMT")
    return int(dt.timestamp())


def get_time_now():
    tstamp = datetime.datetime.now(tz=datetime.timezone.utc).timestamp()
    return int(tstamp)


def datetime_to_unix_timestamp(dt):
    return int(datetime.datetime.timestamp(dt))


def k8stimestamp_to_unix_timestamp(ts):
    if isinstance(ts, datetime.datetime):
        return datetime_to_unix_timestamp(ts)
    return iso8601_to_unix_timestamp(ts)
