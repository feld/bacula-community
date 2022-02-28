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
#     Copyright (c) 2020 by Inteos sp. z o.o.
#     All rights reserved. IP transfered to Bacula Systems according to agreement.
#     Author: Radosław Korzeniewski, radekk@inteos.pl, Inteos Sp. z o.o.
#

class BoolParam(object):

   @staticmethod
   def handleParam(param, _default = False):
      """ Function handles configuration bool parameter expressed in any known value.
          It handles string values like '1', 'Yes', etc. as True and '0', 'No', etc as False.

      Args:
          param (any): parameter value to process
          _default (bool, optional): use this value when parameter handling fail. Defaults to False.

      Returns:
          bool: return processed value
      """
      if not isinstance(_default, bool):
         _default = False
      if param is not None:
         if isinstance(param, str):
            if param.lower() in ('1', 'yes', 'true'):
               return True
            if param.lower() in ('0', 'no', 'false'):
               return False
         if isinstance(param, int) or isinstance(param, bool) or isinstance(param, float):
            if param:
               return True
            else:
               return False
      # finally return default
      return _default
