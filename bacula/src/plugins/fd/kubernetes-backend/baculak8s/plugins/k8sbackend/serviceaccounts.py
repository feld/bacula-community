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


def service_accounts_read_namespaced(corev1api, namespace, name):
    return corev1api.read_namespaced_service_account(name, namespace)


def service_accounts_list_namespaced(corev1api, namespace, estimate=False, labels=""):
    salist = {}
    serviceaccounts = corev1api.list_namespaced_service_account(namespace=namespace, watch=False, label_selector=labels)
    for sa in serviceaccounts.items:
        sadata = service_accounts_read_namespaced(corev1api, namespace, sa.metadata.name)
        spec = encoder_dump(sadata)
        salist['sa-' + sa.metadata.name] = {
            'spec': spec if not estimate else None,
            'fi': k8sfileinfo(objtype=K8SObjType.K8SOBJ_SERVICEACCOUNT, nsname=namespace,
                              name=sa.metadata.name,
                              ftype=NOT_EMPTY_FILE,
                              size=len(spec),
                              creation_timestamp=sadata.metadata.creation_timestamp),
        }
    return salist


def service_accounts_restore_namespaced(corev1api, file_info, file_content):
    sa = encoder_load(file_content, file_info.name)
    metadata = prepare_metadata(sa.metadata)
    # Instantiate the serviceaccount object
    serviceaccount = client.V1ServiceAccount(
        api_version=sa.api_version,
        kind="ServiceAccount",
        automount_service_account_token=sa.automount_service_account_token,
        image_pull_secrets=sa.image_pull_secrets,
        secrets=sa.secrets,
        metadata=metadata
    )
    if file_info.objcache is not None:
        # object exist so we replace it
        response = corev1api.replace_namespaced_service_account(k8sfile2objname(file_info.name),
                                                                file_info.namespace, serviceaccount, pretty='true')
    else:
        # object does not exist, so create one as required
        response = corev1api.create_namespaced_service_account(file_info.namespace, serviceaccount, pretty='true')
    return {'response': response}
