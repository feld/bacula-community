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

import os
import logging
from baculak8s.plugins.k8sbackend.baculabackupimage import KUBERNETES_TAR_IMAGE


BACULABACKUPPODNAME = 'bacula-backup'
# BACULABACKUPIMAGE = "hub.baculasystems.com/bacula-backup:" + KUBERNETES_TAR_IMAGE
BACULABACKUPIMAGE = "bacula-backup:" + KUBERNETES_TAR_IMAGE
DEFAULTPODYAML = os.getenv('DEFAULTPODYAML', "/opt/bacula/scripts/bacula-backup.yaml")
PODTEMPLATE = """
apiVersion: v1
kind: Pod
metadata:
  name: {podname}
  namespace: {namespace}
  labels:
    app: baculabackup
spec:
  hostname: {podname}
  {nodenameparam}
  containers:
  - name: {podname}
    resources:
      limits:
        cpu: "1"
        memory: "64Mi"
      requests:
        cpu: "100m"
        memory: "16Mi"
    image: {image}
    env:
    - name: PLUGINMODE
      value: "{mode}"
    - name: PLUGINHOST
      value: "{host}"
    - name: PLUGINPORT
      value: "{port}"
    - name: PLUGINTOKEN
      value: "{token}"
    - name: PLUGINJOB
      value: "{job}"
    imagePullPolicy: {imagepullpolicy}
    volumeMounts:
      - name: {podname}-storage
        mountPath: /{mode}
  restartPolicy: Never
  volumes:
    - name: {podname}-storage
      persistentVolumeClaim:
        claimName: {pvcname}
"""


class ImagePullPolicy(object):
    IfNotPresent = 'IfNotPresent'
    Always = 'Always'
    Never = 'Never'
    params = (IfNotPresent, Always, Never)

    @staticmethod
    def process_param(imagepullpolicy):
        if imagepullpolicy is not None:
            for p in ImagePullPolicy.params:
                # logging.debug("imagepullpolicy test: {} {}".format(p, self.imagepullpolicy))
                if imagepullpolicy.lower() == p.lower():
                    return p
        return ImagePullPolicy.IfNotPresent


def prepare_backup_pod_yaml(mode='backup', nodename=None, host='localhost', port=9104, token='', namespace='default',
                            pvcname='', image=BACULABACKUPIMAGE, imagepullpolicy=ImagePullPolicy.IfNotPresent, job=''):
    podyaml = PODTEMPLATE
    if os.path.exists(DEFAULTPODYAML):
        with open(DEFAULTPODYAML, 'r') as file:
            podyaml = file.read()
    nodenameparam = ''
    if nodename is not None:
      nodenameparam = "nodeName: {nodename}".format(nodename=nodename)
    logging.debug('host:{} port:{} namespace:{} image:{} job:{}'.format(host, port, namespace, image, job))
    return podyaml.format(mode=mode, nodenameparam=nodenameparam, host=host, port=port, token=token, namespace=namespace,
                          image=image, pvcname=pvcname, podname=BACULABACKUPPODNAME, imagepullpolicy=imagepullpolicy, job=job)
