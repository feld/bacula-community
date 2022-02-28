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

from abc import ABCMeta, abstractmethod

UNRECOGNIZED_CONNECTION_ERROR = -1
ERROR_SSL_FAILED = 1200
ERROR_HOST_NOT_FOUND = 1400
ERROR_HOST_TIMEOUT = 1500
ERROR_AUTH_FAILED = 401
ERROR_CONNECTION_REFUSED = 111


class Plugin(metaclass=ABCMeta):
    """
        Abstract Base Class for all the Plugins
    """

    @abstractmethod
    def connect(self):
        """
            Connects to the Plugin Data Source
        """
        raise NotImplementedError

    @abstractmethod
    def disconnect(self):
        """
            Disconnects from the Plugin Data Source
        """
        raise NotImplementedError

    @abstractmethod
    def list_in_path(self, path):
        """
            Lists all FileInfo objects belonging to the provided $path$

        :return:
        """
        raise NotImplementedError

    @abstractmethod
    def query_parameter(self, parameter):
        """
            Response to client parameter query

        :return:
        """
        raise NotImplementedError

    @abstractmethod
    def check_file(self, file_info):
        """

        :return:
        """
        raise NotImplementedError

    @abstractmethod
    def restore_file(self, file_info, file_content_source=None):
        """

                :return:
                """
        raise NotImplementedError

    @abstractmethod
    def list_all_namespaces(self):
        """


        :return:
        """
        raise NotImplementedError

    @abstractmethod
    def list_namespaced_objects(self, namespace):
        """


        :return:
        """
        raise NotImplementedError

