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

#ifndef __PROGRESS_H
#define __PROGRESS_H

#include <time.h>

/*
 * ProgressCounter is useful to remotely monitor the progress, or cancel a "process"
 *
 * - The progress object can be created an passed to function/objects to monitor
 *   the progress from another function.
 * - For exemple a progress object is passed to any function involved in the
 *   dedup vacuum and used by "dedup usage" to report the vacuum progress or
 *   "dedup vacuum cancel" to cancel the vacuum.
 * - You must initialize the "goal" at the beginning of every part and maintain
 *   the "current" using  inc_progress() or set_progress()
 * - use get_progress() to retrieve the "current" and "goal" values
 * - A "state" can also be used when the process requires different sub-process
 * - use set_canceled() and is_canceled() to change and query the "cancel" status
 * - every time the "current" is updated, the time is recorded in "last_t"
 *   it can be compared with "start_t" (when the object was created or reset)
 *   or with "intermediate_t" when using "state"
 */
class ProgressCounter {
public:
   int state;
   int64_t goal, current;
   bool cancel;
   int64_t start_t, end_t, intermediate_t, last_t;

   ProgressCounter() { reset(); };
   ~ProgressCounter() {};
   void reset(int new_state = 0, int64_t new_goal = 0, int64_t start = 0) { state = new_state; goal = new_goal; current = start; cancel=false; start_t=intermediate_t=last_t=time(NULL); end_t = 0; };
   void change_state(int new_state, int64_t new_goal, int64_t start = 0)
     { state = new_state; goal = new_goal; current = start; intermediate_t=time(NULL); };
//   void set_state(int new_state) { state = new_state; intermediate_t=last_t=time(NULL); };
   void set_goal(int64_t new_goal) { goal = new_goal; };
   void set_progress(int64_t val) { current = val; last_t = time(NULL); };
   void inc_progress(int64_t inc) { current += inc; last_t = time(NULL); };
   void get_progress(int64_t &g, int64_t &c) { g = goal; c = current; };
   void get_progress(int &s, int64_t &g, int64_t &c, int64_t &delta) { s = state; g = goal; c = current; delta = last_t - intermediate_t; };
   bool is_canceled() { return cancel; };
   void set_cancel(bool new_cancel = true) { cancel = new_cancel; }
   void done(int new_state = 0) { state = new_state; end_t = time(NULL); };
};

#endif /* !__PROGRESS_H */
