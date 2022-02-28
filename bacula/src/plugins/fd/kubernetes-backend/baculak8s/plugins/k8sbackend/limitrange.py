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


def limit_range_read_namespaced(corev1api, namespace, name):
    return corev1api.read_namespaced_limit_range(name, namespace)


def limit_range_list_namespaced(corev1api, namespace, estimate=False, labels=""):
    lrlist = {}
    limitranges = corev1api.list_namespaced_limit_range(namespace=namespace, watch=False, label_selector=labels)
    for lr in limitranges.items:
        lrdata = limit_range_read_namespaced(corev1api, namespace, lr.metadata.name)
        spec = encoder_dump(lrdata)
        lrlist['lr-' + lr.metadata.name] = {
            'spec': spec if not estimate else None,
            'fi': k8sfileinfo(objtype=K8SObjType.K8SOBJ_LIMITRANGE, nsname=namespace,
                              name=lr.metadata.name,
                              ftype=NOT_EMPTY_FILE,
                              size=len(spec),
                              creation_timestamp=lrdata.metadata.creation_timestamp),
        }
    return lrlist


def limit_range_restore_namespaced(corev1api, file_info, file_content):
    lr = encoder_load(file_content, file_info.name)
    metadata = prepare_metadata(lr.metadata)
    # Instantiate the limitrange object
    limitrange = client.V1LimitRange(
        api_version=lr.api_version,
        kind="Limitrange",
        spec=lr.spec,
        metadata=metadata
    )
    if file_info.objcache is not None:
        # object exist so we replace it
        response = corev1api.replace_namespaced_config_map(k8sfile2objname(file_info.name),
                                                           file_info.namespace, limitrange, pretty='true')
    else:
        # object does not exist, so create one as required
        response = corev1api.create_namespaced_config_map(file_info.namespace, limitrange, pretty='true')
    return {'response': response}
