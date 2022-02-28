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
import os
from os import environ

TESTS_ROOT = os.path.abspath(os.path.dirname(__file__))
TMP_TEST_FILES_ROOT = os.path.join(TESTS_ROOT, "tmp")

BACKEND_PLUGIN_TYPE = environ.get('BE_PLUGIN_TYPE')
BACKEND_PLUGIN_VERSION = environ.get('BE_PLUGIN_VERSION')
BACKEND_PLUGIN_URL = environ.get('BE_PLUGIN_URL')
BACKEND_PLUGIN_USER = environ.get('BE_PLUGIN_USER')
BACKEND_PLUGIN_PWD = environ.get('BE_PLUGIN_PWD')
