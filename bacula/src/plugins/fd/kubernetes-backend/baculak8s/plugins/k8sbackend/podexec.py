# -*- coding: UTF-8 -*-
#
#  Bacula(R) - The Network Backup Solution
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
#
#     All rights reserved. IP transfered to Bacula Systems according to agreement.
#     Author: Radosław Korzeniewski, radekk@inteos.pl, Inteos Sp. z o.o.
#

import logging

import kubernetes
import yaml
from kubernetes.stream import stream
from kubernetes.stream.ws_client import (ERROR_CHANNEL, STDERR_CHANNEL,
                                         STDOUT_CHANNEL)

DEFAULTTIMEOUT = 600


class ExecStatus(object):
    Success = 'Success'
    Failure = 'Failure'

    @staticmethod
    def check_status(info_channel):
        if info_channel is not None:
            if info_channel.get('status', ExecStatus.Failure) == ExecStatus.Success:
                return True
        return False


def exec_commands(corev1api, namespace, podname, container, command):
    exec_command = [
        '/bin/sh',
        '-c',
        command
    ]
    client = stream(corev1api.connect_get_namespaced_pod_exec,
                    podname,
                    namespace,
                    command=exec_command,
                    container=container,
                    stderr=True, stdin=False,
                    stdout=True, tty=False,
                    _preload_content=False)
    client.run_forever(timeout=DEFAULTTIMEOUT)
    out_channel = client.read_channel(STDOUT_CHANNEL)
    err_channel = client.read_channel(STDERR_CHANNEL)
    info_channel = yaml.load(client.read_channel(ERROR_CHANNEL), Loader=yaml.FullLoader)
    return out_channel, err_channel, info_channel
