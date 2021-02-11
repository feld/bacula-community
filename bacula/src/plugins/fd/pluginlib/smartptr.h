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
 * @file smartptr.h
 * @author Radosław Korzeniewski (radoslaw@korzeniewski.net)
 * @brief This is a simple smart pointer guard conceptually based on C++11 smart pointers - unique_ptr.
 * @version 1.1.0
 * @date 2020-12-23
 *
 * @copyright Copyright (c) 2020 All rights reserved. IP transferred to Bacula Systems according to agreement.
 */

#ifndef _SMARTPTR_H_
#define _SMARTPTR_H_

/**
 * @brief This is a simple smart pointer based conceptually on C++11 smart unique pointer.
 *
 * @tparam T is a type for a pointer to guard
 */
template <typename T>
class smart_ptr
{
   T * _ptr;
public:
   smart_ptr(T *ptr) : _ptr(ptr) {};
   ~smart_ptr()
   {
      if (_ptr != NULL){
         delete _ptr;
         _ptr = NULL;
      }
   };

   T * get() { return _ptr; };
};

#endif   /* _SMARTPTR_H_ */
