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
/*
 * Storage manager classes.
 * All of this code is intented to make managing
 * (accessing, setting, incrementing counters, applying storage policy...) storage easier
 * from the code perspective.
 */

#ifndef STORE_MNGR_H
#define STORE_MNGR_H 1

/* Forward delcaration */
class STORE;


/*
 * Helper class to make managing each storage type (write/read) easier.
 * It contains storage resource ('store' member) which is currently used as well as list of all
 * possible storage resource choices.
 */
class storage {
    private:
      bool write;            /* Write or read storage */
      STORE *store;          /* Selected storage to be used */
      alist *list;           /* Storage possibilities */
      POOLMEM *source;       /* Where the storage came from */
      POOLMEM *list_str;     /* List of storage names in the list */
      pthread_mutex_t mutex; /* Mutex for accessing items */
      bool unused_stores_decremented; /* Set if only currently used storage has NumConcurrentJobs incremented */

      /* Only when we are a read storage - increment concurrent read counters for all storages on the list */
      bool inc_rstores(JCR *jcr);

      /* Only when we are a write storage - increment concurrent write counters for all storages on the list */
      bool inc_wstores(JCR *jcr);
   public:
      storage();

      ~storage();

      /* Determine if we are write or read storage */
      void set_rw(bool write);

      /* Get storage which will be used next */
      STORE *get_store() {
         return store;
      }

      /* Get list of all possible storages */
      alist *get_list();

      /* Get source of the storage (pool, job, commandline, unknown, ...) */
      const char *get_source() const;

      /* Get media type of current storage */
      const char *get_media_type() const;

      /* Set storage override. Remove all previous storage.
       * Can be called for single storage - list consists only one, specified storage then.
       * Can be called for setting a list - internal list consists of same elemets as the list passed
       * as an arg. First item from the list becames storage currently used.
       */
      void set(STORE *storage, const char *source);
      void set(alist *storage, const char *source);

      /* Reset class, remove all items from list, unset storage currently used, clean source */
      void reset();

      /* Set custom storage for next usage (it needs to be an item from the current store list) */
      bool set_current_storage(STORE *storage);

      /* Increment concurrent read/write counters for all storages on the list */
      bool inc_stores(JCR *jcr);

      /* Decrement concurrent read/write counters for all storages on the list */
      void dec_stores();

      void dec_unused_stores();

      void dec_curr_store();

      /* Print all elements of the list (sample result of print_list() -> "File1, File2, File3" */
      const char *print_list();
};


/*
 * Storage Manager class responsible for managing all of the storage used by the JCR.
 * It's holds read as well as write storage resources assigned to the JCR.
 * It is a base class for Storage Policy (hence virtual 'apply_policy' method).
 * Most of member functions are just wrappers around the storage class to make accessing
 * and managin read/write storage in a bit more friendly way.
 *
 */
class StorageManager : public SMARTALLOC {

   protected:
      storage rstore;               /* Read storage */
      storage wstore;               /* Write storage */
      const char *policy;           /* Storage Group Policy used */

   public:
      virtual void apply_policy(bool write_store) = 0;

      virtual ~StorageManager() {
         reset_rwstorage();
         free(policy);
      };

      StorageManager(const char *policy);


      /************ READ STORAGE HELPERS ************/
      STORE *get_rstore();

      alist *get_rstore_list();

      const char *get_rsource() const;

      const char *get_rmedia_type() const;

      void set_rstore(STORE *storage, const char *source);

      void set_rstore(alist *storage, const char *source);

      void reset_rstorage();

      bool inc_read_stores(JCR *jcr);

      void dec_read_stores();

      const char *print_rlist();

      /************ WRITE STORAGE HELPERS ************/
      STORE *get_wstore() {
         return wstore.get_store();
      }

      alist *get_wstore_list();

      const char *get_wsource() const;

      const char *get_wmedia_type() const;

      bool set_current_wstorage(STORE *storage);

      void set_wstorage(STORE *storage, const char *source);

      void set_wstorage(alist *storage, const char *source);

      void reset_wstorage();

      const char *print_wlist();

      bool inc_write_stores(JCR *jcr);

      void dec_write_stores();

      void dec_curr_wstore();

      void dec_unused_wstores();

      /************ GENERIC STORAGE HELPERS ************/
      void reset_rwstorage();

      const char *get_policy_name() {
         return policy;
      }
};

/*
 * Least used policy chooses storage from the list which has the least concurrent jobs number.
 */
class LeastUsedStore : public StorageManager {
   public:
      void apply_policy(bool write_store);

   LeastUsedStore() : StorageManager("LeastUsed") {
   }

   ~LeastUsedStore() {
   }
};

/*
 * Default policy for the storage group. It uses first available storage from the list.
 */
class ListedOrderStore : public StorageManager {
   private:

   public:
      void apply_policy(bool write_store) {
         /* Do nothing for now */
      }

   ListedOrderStore(): StorageManager("ListedOrder")  {
   }

   ~ListedOrderStore() {
   }
};

#endif // STORE_MNGR_H
