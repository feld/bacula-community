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
import json
import logging
import os
from json import JSONEncoder

import yaml

from baculak8s.entities.file_info import FileInfo
from baculak8s.entities.k8sobjtype import K8SObjType
from baculak8s.plugins.k8sbackend.k8sfileinfo import encoder_load
from baculak8s.plugins.plugin import Plugin


class K8SEncoder(JSONEncoder):
    # handles a JSON encoder for kubernetes objects
    def default(self, o):
        if isinstance(o, datetime.datetime):
            return o.strftime("%Y-%m-%dT%H:%M:%S%Z")
        else:
            todict = getattr(o, 'to_dict', None)
            if todict is not None:
                odict = o.to_dict()
                # try to remap dictionary to attributes map
                amap = getattr(o, 'attribute_map', None)
                if amap is not None:
                    for att in amap:
                        attval = amap.get(att)
                        if attval != att and att in odict:
                            odict[attval] = odict[att]
                            del odict[att]
                return odict
            else:
                return json.JSONEncoder.default(self, o)


class FileSystemPlugin(Plugin):
    """
        Plugin that communicates with the Local Filesystem
    """

    def __init__(self, confdata):
        logging.debug("params:" + str(confdata))
        self._params = confdata

    def list_in_path(self, path):
        raise NotImplementedError

    def list_all_namespaces(self):
        raise NotImplementedError

    def list_all_persistentvolumes(self):
        raise NotImplementedError

    def list_namespaced_objects(self, namespace):
        raise NotImplementedError

    def query_parameter(self, parameter):
        raise NotImplementedError

    def check_file(self, file_info):
        file_path = os.path.join(self._params.get('where', '/'), file_info.name)
        try:
            file_stats = os.stat(file_path)
        except FileNotFoundError:
            return None
        return file_stats

    def connect(self):
        """
            Implementation of Plugin.connect(self)
            No need to connect to the Local FS
        """
        return {}

    def disconnect(self):
        """
            Implementation of Plugin.disconnect(self)
            No need to disconnect from the Local FS
        """
        return {}

    def apply_where_parameter(self, file_info, where_param):
        """
            Implementation of Plugin.apply_where_parameter(self, file_info, where_param)
        """
        file_info.name = "{}{}{}{}".format(os.path.sep, where_param, os.path.sep, os.path.sep.join(file_info.fullfname))
        return file_info

    def upload_bucket_acl(self, bucket, acl):
        return {}

    def upload_bucket_xattrs(self, file_info: FileInfo):
        return {}

    def upload_file_xattrs(self, file_info: FileInfo):
        return {}

    def upload_file_acl(self, bucket, filename, acl):
        return {}

    def upload_bucket(self, bucket_info):
        """
            Implementation of Plugin.upload_bucket(self, bucket_info)
            Creates the bucket as a directory on the Local Filesystem
        """

        dir_path = os.path.join(self._params["restore_local_path"],
                                bucket_info.bucket)

        # We change the path to the local OS path format
        dir_path = os.path.abspath(dir_path)

        # If the files directory doesn't exist yet, we create it
        os.makedirs(dir_path, exist_ok=True)

        os.chmod(dir_path, int(bucket_info.mode, 8))
        return {"success": 'True'}

    def restore_file(self, file_info, file_content_source=None):
        """
            Implementation of Plugin.restore_file(self, file_info, file_content_source=None)
            Creates the file on the Local Filesystem
        """
        mode = None
        file_ext = file_info.name[-4:]
        if file_info.objtype != K8SObjType.K8SOBJ_PVCDATA:
            # if change output format then mode is not None
            mode = self._params.get('outputformat', None)
            logging.debug("outputformat: {}".format(mode))
            if mode is not None:
                mode = mode.lower()
                if mode not in ('json', 'yaml'):
                    mode = None
                else:
                    file_ext = mode

        # We change the path to the local OS path format
        file_path = os.path.abspath(file_info.name[:-4]+file_ext)

        # If the files path doesn't exist yet, we create it
        os.makedirs(os.path.dirname(file_path), exist_ok=True)
        logging.debug("file_path: {}".format(file_path))
        if not file_content_source:
            # We create an empty file
            with open(file_path, "w") as f:
                f.close()
        else:
            logging.debug("save mode: {}".format(mode))
            if mode is not None:
                strdata = b''
                while True:
                    chunk = file_content_source.read()
                    if chunk is None:
                        break
                    strdata = strdata + chunk
                # here we have an api object translated to simple dict
                # logging.debug("STRDATA:" + str(strdata))
                el = encoder_load(strdata, file_info.name)
                jd = json.dumps(el, cls=K8SEncoder, sort_keys=True)
                data = json.loads(jd)
                # logging.debug("EL:" + str(el))
                # logging.debug("JD:" + str(jd))
                # logging.debug("DATA:" + str(data))
                if 'json' == mode:
                    with open(file_path, "w") as out_file:
                        json.dump(data, out_file, indent=3, sort_keys=True)
                if 'yaml' == mode:
                    with open(file_path, "w") as out_file:
                        yaml.dump(data, out_file, Dumper=yaml.SafeDumper, default_flow_style=False)
                    out_file.close()
            else:
                # We create an file with the contents from file_content_source
                with open(file_path, "wb") as f:
                    while True:
                        chunk = file_content_source.read()
                        if chunk is None:
                            break
                        f.write(chunk)
                    f.close()

        os.chmod(file_path, int(file_info.mode, 8))
        return {"success": 'True'}
