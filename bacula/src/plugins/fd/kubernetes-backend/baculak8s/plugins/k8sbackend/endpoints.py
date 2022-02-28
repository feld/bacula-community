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


def endpoints_read_namespaced(corev1api, namespace, name):
    return corev1api.read_namespaced_endpoints(name, namespace)


def endpoints_list_namespaced(corev1api, namespace, estimate=False, labels=""):
    eplist = {}
    endpoints = corev1api.list_namespaced_endpoints(namespace=namespace, watch=False, label_selector=labels)
    for ep in endpoints.items:
        epdata = endpoints_read_namespaced(corev1api, namespace, ep.metadata.name)
        spec = encoder_dump(epdata)
        eplist['ep-' + ep.metadata.name] = {
            'spec': spec if not estimate else None,
            'fi': k8sfileinfo(objtype=K8SObjType.K8SOBJ_ENDPOINT, nsname=namespace,
                              name=ep.metadata.name,
                              ftype=NOT_EMPTY_FILE,
                              size=len(spec),
                              creation_timestamp=epdata.metadata.creation_timestamp),
        }
    return eplist


def endpoints_restore_namespaced(corev1api, file_info, file_content):
    ep = encoder_load(file_content, file_info.name)
    metadata = prepare_metadata(ep.metadata)
    # Instantiate the endpoint object
    endpoint = client.V1Endpoints(
        api_version=ep.api_version,
        kind="Endpoint",
        subsets=ep.subsets,
        metadata=metadata
    )
    if file_info.objcache is not None:
        # object exist so we replace it
        response = corev1api.replace_namespaced_endpoints(k8sfile2objname(file_info.name),
                                                          file_info.namespace, endpoint, pretty='true')
    else:
        # object does not exist, so create one as required
        response = corev1api.create_namespaced_endpoints(file_info.namespace, endpoint, pretty='true')
    return {'response': response}
