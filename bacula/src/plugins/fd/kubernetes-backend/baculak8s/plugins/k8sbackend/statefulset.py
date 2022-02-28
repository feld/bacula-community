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


def stateful_sets_read_namespaced(appsv1api, namespace, name):
    return appsv1api.read_namespaced_stateful_set(name, namespace)


def stateful_sets_list_namespaced(appsv1api, namespace, estimate=False, labels=""):
    sslist = {}
    statefulsets = appsv1api.list_namespaced_stateful_set(namespace=namespace, watch=False, label_selector=labels)
    for ss in statefulsets.items:
        ssdata = stateful_sets_read_namespaced(appsv1api, namespace, ss.metadata.name)
        spec = encoder_dump(ssdata)
        sslist['ss-' + ss.metadata.name] = {
            'spec': spec if not estimate else None,
            'fi': k8sfileinfo(objtype=K8SObjType.K8SOBJ_STATEFULSET, nsname=namespace,
                              name=ss.metadata.name,
                              ftype=NOT_EMPTY_FILE,
                              size=len(spec),
                              creation_timestamp=ssdata.metadata.creation_timestamp),
        }
    return sslist


def stateful_sets_restore_namespaced(appsv1api, file_info, file_content):
    ss = encoder_load(file_content, file_info.name)
    metadata = prepare_metadata(ss.metadata)
    # Instantiate the daemon_set object
    statefulset = client.V1StatefulSet(
        api_version=ss.api_version,
        kind="StatefulSet",
        spec=ss.spec,
        metadata=metadata
    )
    if file_info.objcache is not None:
        # object exist so we replace it
        response = appsv1api.replace_namespaced_stateful_set(k8sfile2objname(file_info.name),
                                                             file_info.namespace, statefulset, pretty='true')
    else:
        # object does not exist, so create one as required
        response = appsv1api.create_namespaced_stateful_set(file_info.namespace, statefulset, pretty='true')
    return {'response': response}
