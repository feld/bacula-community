/*
   Bacula(R) - The Network Backup Solution

   Copyright (C) 2000-2022 Kern Sibbald

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
 * @brief This is a simple smart array list (alist) resource guard conceptually based on C++11 - RAII.
 * @version 1.1.0
 * @date 2020-12-23
 *
 * @copyright Copyright (c) 2020 All rights reserved. IP transferred to Bacula Systems according to agreement.
 */

#ifndef _SMARTALIST_H_
#define _SMARTALIST_H_


/**
 * @brief This is a simple smart array list (alist) resource guard conceptually based on C++11 - RAII.
 *
 * @tparam T is a type for a pointer to allocate on smart_alist.
 */
template <typename T>
class smart_alist : public alist
{
public:
   smart_alist(int num = 10) : alist(num, not_owned_by_alist) {}
   ~smart_alist()
   {
      T * it;
      if (items) {
         for (int i = 0; i < max_items; i++) {
            if (items[i]) {
               it = (T*)items[i];
               items[i] = NULL;
               delete it;
            }
         }
      }
   }
};

#endif   /* _SMARTALIST_H_ */
