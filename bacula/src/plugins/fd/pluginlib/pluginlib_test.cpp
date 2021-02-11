/*
   Bacula(R) - The Network Backup Solution

   Copyright (C) 2000-2020 Kern Sibbald

   The original author of Bacula is Kern Sibbald, with contributions
   from many others, a complete list can be found in the file AUTHORS.

   You may use this file and others of this release according to the
   license defined in the LICENSE file, which includes the Affero General
   Public License, v3.0 ("AGPLv3") and some additional permissions and
   terms pursuant to its AGPLv3 Section 7.

   This notice must be preserved when any source code is
   conveyed and/or propagated.

   Bacula(R) is a registered trademark of Kern Sibbald.
 */
/**
 * @file commctx.h
 * @author Radosław Korzeniewski (radoslaw@korzeniewski.net)
 * @brief Common definitions and utility functions for Inteos plugins - unittest.
 * @version 1.1.0
 * @date 2020-12-23
 *
 * @copyright Copyright (c) 2020 All rights reserved. IP transferred to Bacula Systems according to agreement.
 */

#include "pluginlib.h"
#include "unittests.h"

bFuncs *bfuncs;
bInfo *binfo;

int main()
{
   Unittests pluglib_test("pluglib_test");
   alist * list;
   char * s;

   // Pmsg0(0, "Initialize tests ...\n");

   list = plugutil_str_split_to_alist("123456789");
   ok(list != NULL, "default split");
   ok(list->size() == 1, "expect single strings");
   foreach_alist(s, list){
      ok(strlen(s) == 9, "check element length");
   }
   delete list;

   list = plugutil_str_split_to_alist("123.456");
   ok(list != NULL, "split: 123.456");
   ok(list->size() == 2, "expect two strings");
   foreach_alist(s, list){
      ok(strlen(s) == 3, "check element length");
   }
   delete list;

   list = plugutil_str_split_to_alist("12345.56789.abcde");
   ok(list != NULL, "split: 12345.56789.abcde");
   ok(list->size() == 3, "expect three strings");
   foreach_alist(s, list){
      ok(strlen(s) == 5, "check element length");
   }
   delete list;

   list = plugutil_str_split_to_alist("1.bacula..Eric.Kern");
   ok(list != NULL, "split: 1.bacula..Eric.Kern");
   ok(list->size() == 5, "expect three strings");
   ok(strcmp((char*)list->first(), "1") == 0, "check element 1");
   ok(strcmp((char*)list->next(), "bacula") == 0, "check element bacula");
   ok(strlen((char*)list->next()) == 0, "check empty element");
   ok(strcmp((char*)list->next(), "Eric") == 0, "check element Eric");
   ok(strcmp((char*)list->next(), "Kern") == 0, "check element Kern");
   delete list;

   POOL_MEM cmd1(PM_NAME);
   POOL_MEM param(PM_NAME);
   const char *prefix = "FNAME:";
   const char *fname1 = "/etc/passwd";
   pm_strcpy(cmd1, prefix);
   pm_strcat(cmd1, fname1);
   pm_strcat(cmd1, "\n");
   ok(scan_parameter_str(cmd1, prefix, param), "check scan parameter str match");
   ok(bstrcmp(param.c_str(), fname1) , "check scan parameter str param");
   nok(scan_parameter_str(cmd1, "prefix", param), "check scan parameter str not match");

   const char *fname2 = "/home/this is a filename with spaces which /are hard to/ manage.com";
   pm_strcpy(cmd1, prefix);
   pm_strcat(cmd1, fname2);
   pm_strcat(cmd1, "\n");
   ok(scan_parameter_str(cmd1, prefix, param), "check scan parameter str with spaces match");
   ok(bstrcmp(param.c_str(), fname2) , "check scan parameter str with spaces param");

   char cmd2[256];
   snprintf(cmd2, 256, "%s%s\n", prefix, fname1);
   ok(scan_parameter_str(cmd2, prefix, param), "check scan parameter for char* str match");
   ok(bstrcmp(param.c_str(), fname1) , "check scan parameter for char* str param");
   nok(scan_parameter_str(cmd2, "prefix", param), "check scan parameter for char* str not match");

   return report();
}
