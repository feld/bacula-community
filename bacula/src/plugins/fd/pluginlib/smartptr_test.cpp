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
/**
 * @file smartptr_test.cpp
 * @author Radosław Korzeniewski (radoslaw@korzeniewski.net)
 * @brief Common definitions and utility functions for Inteos plugins - unittest.
 * @version 1.1.0
 * @date 2020-12-23
 *
 * @copyright Copyright (c) 2020 All rights reserved. IP transferred to Bacula Systems according to agreement.
 */

#include "pluginlib.h"
#include "smartptr.h"
#include "unittests.h"

bFuncs *bfuncs;
bInfo *binfo;

static int referencenumber = 0;

struct testclass
{
   testclass() { referencenumber++; };
   ~testclass() { referencenumber--; };
};

int main()
{
   Unittests pluglib_test("smartalist_test");

   // Pmsg0(0, "Initialize tests ...\n");

   {
      smart_ptr<testclass> ptr(new testclass);
      ok(referencenumber == 1, "check smart allocation");
   }

   ok(referencenumber == 0, "check smart free");

   return report();
}
