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
#     Copyright (c) 2019 by Inteos sp. z o.o.
#     All rights reserved. IP transfered to Bacula Systems according to agreement.
#     Author: Radosław Korzeniewski, radekk@inteos.pl, Inteos Sp. z o.o.
#

import time

from kubernetes import client


def prepare_metadata(data, annotations=False):
    metadata = client.V1ObjectMeta(
        name=data.name,
        namespace=data.namespace,
        annotations=data.annotations if annotations else {},
        deletion_grace_period_seconds=data.deletion_grace_period_seconds,
        deletion_timestamp=data.deletion_timestamp,
        # finalizers=data.finalizers,
        # generate_name=data.generate_name,
        # initializers=data.initializers,
        labels=data.labels
    )
    return metadata


def wait_for_pod_ready(corev1api, namespace, name, waits=600):
    a = 0
    for a in range(waits):
        status = corev1api.read_namespaced_pod_status(name=name, namespace=namespace)
        isready = status.status.container_statuses[0].ready
        if isready:
            break
        time.sleep(1)
    return a < waits - 1


def wait_for_pod_terminated(corev1api, namespace, name, waits=600):
    a = 0
    for a in range(waits):
        status = corev1api.read_namespaced_pod_status(name=name, namespace=namespace)
        container = status.status.container_statuses[0]
        isterminated = not container.ready and container.state.terminated is not None
        if isterminated:
            break
        time.sleep(1)
    return a < waits - 1
