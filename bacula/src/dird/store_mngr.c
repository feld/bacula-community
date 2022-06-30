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

#include "bacula.h"
#include "dird.h"
#include "lib/lockmgr.h"

static const int dbglvl = 200;

storage::storage() {
   list = New(alist(10, not_owned_by_alist));
   origin_list = New(alist(10, not_owned_by_alist));
   source = get_pool_memory(PM_MESSAGE);
   list_str = get_pool_memory(PM_MESSAGE);
   *source = 0;
   store = NULL;
   pthread_mutex_init(&mutex, NULL);
}

storage::~storage() {
   store = NULL;
   if (list) {
      delete list;
      list = NULL;
   }
   if (origin_list) {
      delete origin_list;
      origin_list = NULL;
   }
   if (source) {
      free_and_null_pool_memory(source);
   }
   if (list_str) {
      free_and_null_pool_memory(list_str);
   }

   pthread_mutex_destroy(&mutex);
}

void storage::set_rw(bool write) {
   P(mutex);
   this->write = write;
   V(mutex);
}

alist *storage::get_list() {
   return list;
}

alist *storage::get_origin_list() {
   return origin_list;
}

const char *storage::get_source() const {
   return source;
}

const char *storage::get_media_type() const {
   return store->media_type;
}

void storage::set(STORE *storage, const char *source) {
   if (!storage) {
      return;
   }

   lock_guard lg(mutex);

   reset();

   list->append(storage);
   origin_list->append(storage);

   store = storage;
   if (!source) {
      pm_strcpy(this->source, _("Not specified"));
   } else {
      pm_strcpy(this->source, source);
   }
}

/* Set storage override. Remove all previous storage */
void storage::set(alist *storage, const char *source) {
   if (!storage) {
      return;
   }

   lock_guard lg(mutex);
   reset();

   STORE *s;
   foreach_alist(s, storage) {
      list->append(s);
      origin_list->append(s);
   }

   store = (STORE *)list->first();
   if (!source) {
      pm_strcpy(this->source, _("Not specified"));
   } else {
      pm_strcpy(this->source, source);
   }
}

void storage::reset() {
   store = NULL;

   while (list->size()) {
      list->remove(0);
   }
   while (origin_list->size()) {
      origin_list->remove(0);
   }
   *source = 0;
   *list_str = 0;
}

/* Set custom storage for next usage (it needs to be an item from the current store list) */
bool storage::set_current_storage(STORE *storage) {
   if (!storage) {
      return false;
   }

   lock_guard lg(mutex);

   STORE *s;
   foreach_alist(s, list) {
      if (s == storage) {
         store = storage;
         return true;
      }
   }

   return false;
}

/* Returns true if it was possible to increment Storage's write counter,
 * returns false otherwise */
static bool inc_wstore(STORE *wstore) {
   Dmsg1(dbglvl, "Wstore=%s\n", wstore->name());
   int num = wstore->getNumConcurrentJobs();
   if (num < wstore->MaxConcurrentJobs) {
      num = wstore->incNumConcurrentJobs(1);
      Dmsg2(dbglvl, "Store: %s Inc wncj=%d\n", wstore->name(), num);
      return true;
   }

   return false;
}

/* Returns true if it was possible to increment Storage's read counter,
 * returns false otherwise */
static bool inc_rstore(JCR *jcr, STORE *rstore) {
   int num = rstore->getNumConcurrentJobs();
   int numread = rstore->getNumConcurrentReadJobs();
   int maxread = rstore->MaxConcurrentReadJobs;
   if (num < rstore->MaxConcurrentJobs &&
         (jcr->getJobType() == JT_RESTORE ||
          numread == 0     ||
          maxread == 0     ||     /* No limit set */
          numread < maxread))     /* Below the limit */
   {
      numread = rstore->incNumConcurrentReadJobs(1);
      num = rstore->incNumConcurrentJobs(1);
      Dmsg3(dbglvl, "Store: %s Inc ncj= %d rncj=%d\n", rstore->name(), num, numread);
      return true;
   }

   return false;
}

bool storage::inc_stores(JCR *jcr) {
   lock_guard lg(mutex);
   STORE *tmp_store;
   bool ret = false; /* Set if any of the storages in the list was incremented */

   if (list->empty()) {
      return true;
   }

   /* Create a temp copy of the store list */
   alist *tmp_list = New(alist(10, not_owned_by_alist));
   if (!tmp_list) {
      Dmsg1(dbglvl, "Failed to allocate tmp list for jobid: %d\n", jcr->JobId);
      return false;
   }

   foreach_alist(tmp_store, list) {
      tmp_list->append(tmp_store);
   }

   /* Reset list */
   list->destroy();
   list->init(10, not_owned_by_alist);

   foreach_alist(tmp_store, tmp_list) {
      if (write) {
         if (inc_wstore(tmp_store)) {
            /* Counter incremented, can be added to list */
            list->append(tmp_store);
         } else {
            Dmsg1(dbglvl, "Storage %s cannot be included\n", tmp_store->name());
         }

      } else if (inc_rstore(jcr, tmp_store)) {
         /* Counter incremented, can be added to list */
         list->append(tmp_store);
      }
   }

   if (!list->empty()) {
      /* We were able to increment at least 1 storage from the list */
      ret = true;
   } else {
      /* Failed to increment counter for at least one storage */
      ret = false;
   }

   if (!ret) {
      /* We don't want to return empty list in case of fail, it should not be changed at this point */
      delete list;
      list = tmp_list;
   } else {
      /* tmp list not needed anymore since only the devices that were reserved are returned in the list */
      delete tmp_list;
   }

   return ret;
}

void storage::dec_stores() {
   lock_guard lg(mutex);

   if (list->empty()) {
      return;
   }

   if (unused_stores_decremented) {
      /* Only currently used storage needs to be decrased, rest of it was decremented before */
      if(!write) {
         store->incNumConcurrentReadJobs(-1);
      }
      int num = store->incNumConcurrentJobs(-1);
      Dmsg2(dbglvl, "Store: %s Dec ncj=%d\n", store->name(), num);
      unused_stores_decremented = false;
   } else {
      /* We need to decrement all storages in the list */
      STORE *tmp_store;
      foreach_alist(tmp_store, list) {
         if(!write) {
            tmp_store->incNumConcurrentReadJobs(-1);
         }
         int num = tmp_store->incNumConcurrentJobs(-1);
         Dmsg2(dbglvl, "Store: %s Dec ncj=%d\n", tmp_store->name(), num);
      }
   }
}

const char *storage::print_list(alist *list) {
   lock_guard lg(mutex);

   *list_str = 0;
   STORE *store;
   POOL_MEM tmp;
   bool first = true;

   foreach_alist(store, list) {
      if (first) {
         first = false;
      } else {
         pm_strcat(tmp.addr(), ", ");
      }
      pm_strcat(tmp.addr(), store->name());
   }

   return quote_string(list_str, tmp.addr());
}

const char *storage::print_origin_list() {
   return print_list(origin_list);
}

const char *storage::print_possible_list() {
   return print_list(list);
}

void storage::dec_unused_stores() {
   lock_guard lg(mutex);
   STORE *tmp_store;

   foreach_alist(tmp_store, list) {
      if (store == tmp_store) {
         /* We don't want to decrement this one since it's the one that will be used */
         continue;
      } else {
         int num = tmp_store->incNumConcurrentJobs(-1);
         Dmsg2(dbglvl, "Store: %s Dec ncj=%d\n", store->name(), num);
      }
   }

   unused_stores_decremented = true;
}

void storage::dec_curr_store() {
   lock_guard lg(mutex);

   int num = store->incNumConcurrentJobs(-1);
   Dmsg2(dbglvl, "Store: %s Dec ncj=%d\n", store->name(), num);
}

static void swapit(uint32_t *v1, uint32_t *v2)
{
   uint32_t temp = *v1;
   *v1 = *v2;
   *v2 = temp;
}

void LeastUsedStore::apply_policy(bool write_store) {
   alist *store = write_store ? wstore.get_list() : rstore.get_list();
   alist tmp_list(10, not_owned_by_alist);
   uint32_t store_count = store->size();
   uint32_t i, j;

   uint32_t *conc_arr = (uint32_t*) malloc((store_count+1) * sizeof(uint32_t));
   uint32_t *idx_arr = (uint32_t*) malloc((store_count+1) * sizeof(uint32_t));


   for (uint32_t i=0; i<store_count; i++) {
      tmp_list.append(store->get(i));
   }

   /* Reset list */
   store->destroy();
   store->init(10, not_owned_by_alist);

   STORE *storage;
   foreach_alist_index(i, storage, &tmp_list) {
      idx_arr[i] = i;
      conc_arr[i] = storage->getNumConcurrentJobs();
   }

   /* Simple sort */
   for (i = 0; i<store_count - 1; i++) {
      for (j =0; j<store_count - i -1; j++) {
         if (conc_arr[j] > conc_arr[j+1]) {
            swapit(&conc_arr[j], &conc_arr[j+1]);
            swapit(&idx_arr[j], &idx_arr[j+1]);
         }
      }
   }
   for (i=0; i<store_count; i++) {
      storage = (STORE *)tmp_list.get(idx_arr[i]);
      store->append(storage);
   }
   free(conc_arr);
   free(idx_arr);
}

void LeastUsedStore::apply_write_policy() {
   return apply_policy(true);
}

void LeastUsedStore::apply_read_policy() {
   return apply_policy(false);
}

StorageManager::StorageManager(const char *policy) {
   this->policy = bstrdup(policy);
   rstore.set_rw(false);
   wstore.set_rw(true);
};

STORE *StorageManager::get_rstore() {
   return rstore.get_store();
}

alist *StorageManager::get_rstore_list() {
   return rstore.get_list();
}

alist *StorageManager::get_origin_rstore_list() {
   return rstore.get_origin_list();
}

const char *StorageManager::get_rsource() const {
   return rstore.get_source();
}

const char *StorageManager::get_rmedia_type() const {
   return rstore.get_media_type();
}

alist *StorageManager::get_wstore_list() {
   return wstore.get_list();
}

alist *StorageManager::get_origin_wstore_list() {
   return wstore.get_origin_list();
}

const char *StorageManager::get_wsource() const {
   return wstore.get_source();
}

const char *StorageManager::get_wmedia_type() const {
   return wstore.get_media_type();
}

void StorageManager::set_rstore(STORE *storage, const char *source) {
   rstore.set(storage, source);
}

void StorageManager::set_rstore(alist *storage, const char *source) {
   rstore.set(storage, source);
}

void StorageManager::reset_rstorage() {
   rstore.reset();
}

const char *StorageManager::print_possible_rlist() {
   return rstore.print_possible_list();
}

const char *StorageManager::print_origin_rlist() {
   return rstore.print_origin_list();
}

bool StorageManager::set_current_wstorage(STORE *storage) {
   return wstore.set_current_storage(storage);
}

void StorageManager::set_wstorage(STORE *storage, const char *source) {
   wstore.set(storage, source);
}

void StorageManager::set_wstorage(alist *storage, const char *source) {
   wstore.set(storage, source);
}

void StorageManager::reset_wstorage() {
   wstore.reset();
}

const char *StorageManager::print_possible_wlist() {
   return wstore.print_possible_list();
}

const char *StorageManager::print_origin_wlist() {
   return wstore.print_origin_list();
}

void StorageManager::reset_rwstorage() {
   rstore.reset();
   wstore.reset();
}

bool StorageManager::inc_read_stores(JCR *jcr) {
   return rstore.inc_stores(jcr);
}

/* Decrement job counter for all of the storages in the list */
void StorageManager::dec_read_stores() {
   return rstore.dec_stores();
}

/* Increment job counter for all of the storages in the list */
bool StorageManager::inc_write_stores(JCR *jcr) {
   return wstore.inc_stores(jcr);
}

void StorageManager::dec_write_stores() {
   wstore.dec_stores();
}

/* Decrement job counter for currently used write storage */
void StorageManager::dec_curr_wstore() {
   wstore.dec_curr_store();
}

/* Decrement job counters for write storages which won't be used */
void StorageManager::dec_unused_wstores() {
   wstore.dec_unused_stores();
}
