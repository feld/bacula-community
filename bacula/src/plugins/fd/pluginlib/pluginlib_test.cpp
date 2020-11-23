/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2007-2017 Bacula Systems SA
   All rights reserved.

   The main author of Bacula is Kern Sibbald, with contributions from many
   others, a complete list can be found in the file AUTHORS.

   Licensees holding a valid Bacula Systems SA license may use this file
   and others of this release in accordance with the proprietary license
   agreement provided in the LICENSE file.  Redistribution of any part of
   this release is not permitted.

   Bacula® is a registered trademark of Kern Sibbald.
*/
/*
 *
 * All rights reserved. IP transferred to Bacula Systems according to agreement.
 *
 * Common definitions and utility functions for Inteos plugins.
 * Functions defines a common framework used in our utilities and plugins.
 * Author: Radosław Korzeniewski, radekk@inteos.pl, Inteos Sp. z o.o.
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

   return report();
}
