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
#     Copyright (c) 2019 by Inteos sp. z o.o.
#     All rights reserved. IP transfered to Bacula Systems according to agreement.
#     Author: Radosław Korzeniewski, radekk@inteos.pl, Inteos Sp. z o.o.
#

from baculak8s.entities.file_info import NOT_EMPTY_FILE
from baculak8s.plugins.k8sbackend.k8sfileinfo import *
from baculak8s.plugins.k8sbackend.k8sutils import *


def replication_controller_read_namespaced(corev1api, namespace, name):
    return corev1api.read_namespaced_replication_controller(name, namespace)


def replication_controller_list_namespaced(corev1api, namespace, estimate=False, labels=""):
    rclist = {}
    replicationcontroller = corev1api.list_namespaced_replication_controller(namespace=namespace, watch=False,
                                                                             label_selector=labels)
    for rc in replicationcontroller.items:
        rcdata = replication_controller_read_namespaced(corev1api, namespace, rc.metadata.name)
        spec = encoder_dump(rcdata)
        rclist['rc-' + rc.metadata.name] = {
            'spec': spec if not estimate else None,
            'fi': k8sfileinfo(objtype=K8SObjType.K8SOBJ_REPLICACONTR, nsname=namespace,
                              name=rc.metadata.name,
                              ftype=NOT_EMPTY_FILE,
                              size=len(spec),
                              creation_timestamp=rcdata.metadata.creation_timestamp),
        }
    return rclist


def replication_controller_restore_namespaced(corev1api, file_info, file_content):
    rc = encoder_load(file_content, file_info.name)
    metadata = prepare_metadata(rc.metadata)
    # Instantiate the replicationcontroller object
    replicationcontroller = client.V1ReplicationController(
        api_version=rc.api_version,
        kind="ReplicationController",
        spec=rc.spec,
        metadata=metadata
    )
    if file_info.objcache is not None:
        # object exist so we replace it
        response = corev1api.replace_namespaced_replication_controller(k8sfile2objname(file_info.name),
                                                                       file_info.namespace, replicationcontroller,
                                                                       pretty='true')
    else:
        # object does not exist, so create one as required
        response = corev1api.create_namespaced_replication_controller(file_info.namespace, replicationcontroller,
                                                                      pretty='true')
    return {'response': response}
