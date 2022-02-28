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
from baculak8s.plugins.fs_plugin import FileSystemPlugin
from baculak8s.plugins.kubernetes_plugin import KubernetesPlugin


class PluginFactory(object):
    @staticmethod
    def create(plugin_name, config):
        """
                Creates a plugin to be used for Backup and / or Restore
                operations.

                :param plugin_name: The name of the plugin to be Created
                :param config: The plugin configuration

                :return: The created plugin

                :raise: ValueError, if an invalid plugin_name was provided

        """
        if "where" in config and len(config["where"]) > 1:
            return FileSystemPlugin(config)

        if plugin_name in ("kubernetes", "openshift"):
            return KubernetesPlugin(config)
        else:
            raise ValueError("Invalid Plugin Type")
