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

import os
import string

from baculak8s.util.token import generate_token

DEFAULTCLONEYAML = os.getenv('DEFAULTCLONEYAML', "/opt/bacula/scripts/bacula-backup-clone.yaml")
CLONETEMPLATE = """
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: {clonename}
  namespace: {namespace}
  labels:
    app: baculabackup
spec:
  storageClassName: {storageclassname}
  dataSource:
    name: {pvcname}
    kind: PersistentVolumeClaim
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: {pvcsize}
"""


def prepare_backup_clone_yaml(namespace, pvcname, pvcsize, scname, clonename=None):
    """ Handles PVC clone yaml preparation based on available templates

    Args:
        namespace (str): k8s namespace for pvc clone
        pvcname (str): source pvc name to clone from
        pvcsize (str): k8s capacity of the original pvc
        scname (str): storage class of the original pvc
        clonename (str, optional): the cloned - destination - pvcname; if `None` then name will be assigned automatically. Defaults to None.

    Returns:
        tuple(2): return a prepared pvc clone yaml string and assigned pvc clone name, especially useful when this name was created automatically.
    """
    cloneyaml = CLONETEMPLATE
    if os.path.exists(DEFAULTCLONEYAML):
        with open(DEFAULTCLONEYAML, 'r') as file:
            cloneyaml = file.read()
    if clonename is None:
        validchars = tuple(string.ascii_lowercase) + tuple(string.digits)
        clonename = "{pvcname}-baculaclone-{id}".format(pvcname=pvcname, id=generate_token(size=6, chars=validchars))

    return cloneyaml.format(namespace=namespace, pvcname=pvcname, pvcsize=pvcsize, clonename=clonename, storageclassname=scname), clonename
