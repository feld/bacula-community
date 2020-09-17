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
 * Generic catalog class methods. 
 * 
 * Note: at one point, this file was assembled from parts of other files 
 *  by a programmer, and other than "wrapping" in a class, which is a trivial  
 *  change for a C++ programmer, nothing substantial was done, yet all the  
 *  code was recommitted under this programmer's name.  Consequently, we  
 *  undo those changes here.  
 */ 
 
#include "bacula.h" 
 
#if HAVE_SQLITE3 || HAVE_MYSQL || HAVE_POSTGRESQL 
 
#include "cats.h" 
 
bool BDB::bdb_match_database(const char *db_driver, const char *db_name, 
                             const char *db_address, int db_port) 
{ 
   BDB *mdb = this; 
   bool match; 
 
   if (db_driver) { 
      match = strcasecmp(mdb->m_db_driver, db_driver) == 0 && 
              bstrcmp(mdb->m_db_name, db_name) && 
              bstrcmp(mdb->m_db_address, db_address) && 
              mdb->m_db_port == db_port && 
              mdb->m_dedicated == false; 
   } else { 
      match = bstrcmp(mdb->m_db_name, db_name) && 
              bstrcmp(mdb->m_db_address, db_address) && 
              mdb->m_db_port == db_port && 
              mdb->m_dedicated == false; 
   } 
   return match; 
} 
 
BDB *BDB::bdb_clone_database_connection(JCR *jcr, bool mult_db_connections) 
{ 
   BDB *mdb = this; 
   /* 
    * See if its a simple clone e.g. with mult_db_connections set to false 
    * then we just return the calling class pointer. 
    */ 
   if (!mult_db_connections) { 
      mdb->m_ref_count++; 
      return mdb; 
   } 
 
   /* 
    * A bit more to do here just open a new session to the database. 
    */ 
   return db_init_database(jcr, mdb->m_db_driver, mdb->m_db_name, 
             mdb->m_db_user, mdb->m_db_password, mdb->m_db_address,
             mdb->m_db_port, mdb->m_db_socket,
             mdb->m_db_ssl_mode, mdb->m_db_ssl_key,
             mdb->m_db_ssl_cert, mdb->m_db_ssl_ca,
             mdb->m_db_ssl_capath, mdb->m_db_ssl_cipher,
             true, mdb->m_disabled_batch_insert);
} 
 
const char *BDB::bdb_get_engine_name(void) 
{ 
   BDB *mdb = this; 
   switch (mdb->m_db_driver_type) { 
   case SQL_DRIVER_TYPE_MYSQL: 
      return "MySQL"; 
   case SQL_DRIVER_TYPE_POSTGRESQL: 
      return "PostgreSQL"; 
   case SQL_DRIVER_TYPE_SQLITE3: 
      return "SQLite3"; 
   default: 
      return "Unknown"; 
   } 
} 
 
/* 
 * Lock database, this can be called multiple times by the same 
 * thread without blocking, but must be unlocked the number of 
 * times it was locked using db_unlock(). 
 */ 
void BDB::bdb_lock(const char *file, int line) 
{ 
   int errstat; 
   BDB *mdb = this; 
 
   if ((errstat = rwl_writelock_p(&mdb->m_lock, file, line)) != 0) { 
      berrno be; 
      e_msg(file, line, M_FATAL, 0, "rwl_writelock failure. stat=%d: ERR=%s\n", 
            errstat, be.bstrerror(errstat)); 
   } 
} 
 
/* 
 * Unlock the database. This can be called multiple times by the 
 * same thread up to the number of times that thread called 
 * db_lock()/ 
 */ 
void BDB::bdb_unlock(const char *file, int line) 
{ 
   int errstat; 
   BDB *mdb = this; 
 
   if ((errstat = rwl_writeunlock(&mdb->m_lock)) != 0) { 
      berrno be; 
      e_msg(file, line, M_FATAL, 0, "rwl_writeunlock failure. stat=%d: ERR=%s\n", 
            errstat, be.bstrerror(errstat)); 
   } 
} 
 
bool BDB::bdb_sql_query(const char *query, int flags) 
{ 
   bool retval; 
   BDB *mdb = this; 
 
   bdb_lock(); 
   retval = sql_query(query, flags); 
   if (!retval) { 
      Mmsg(mdb->errmsg, _("Query failed: %s: ERR=%s\n"), query, sql_strerror()); 
   } 
   bdb_unlock(); 
   return retval; 
} 
 
void BDB::print_lock_info(FILE *fp) 
{ 
   BDB *mdb = this; 
   if (mdb->m_lock.valid == RWLOCK_VALID) { 
      fprintf(fp, "\tRWLOCK=%p w_active=%i w_wait=%i\n",  
         &mdb->m_lock, mdb->m_lock.w_active, mdb->m_lock.w_wait); 
   } 
} 

void parse_plugin_object_string(char **obj_str, OBJECT_DBR *obj_r)
{
   char *p = *obj_str;

   obj_r->JobId = str_to_int32(p);
   skip_nonspaces(&p);                  /* skip JobId */
   skip_spaces(&p);

   obj_r->Path = p;
   skip_nonspaces(&p);                  /* skip Path */
   skip_spaces(&p);
   char *c = strpbrk(obj_r->Path, " ");
   if (c) {
      *c = '\0';
   }

   obj_r->Filename = p;
   skip_nonspaces(&p);                  /* skip FileName */
   skip_spaces(&p);
   c = strpbrk(obj_r->Filename, " ");
   if (c) {
      *c = '\0';
   }

   obj_r->PluginName = p;
   skip_nonspaces(&p);                  /* skip PluginName */
   skip_spaces(&p);
   c = strpbrk(obj_r->PluginName, " ");
   if (c) {
      *c = '\0';
   }

   char *obj_type = p;
   skip_nonspaces(&p);                  /* skip ObjectType */
   skip_spaces(&p);
   c = strpbrk(obj_type, " ");
   if (c) {
      *c = '\0';
   }
   bstrncpy(obj_r->ObjectType, obj_type, sizeof(obj_r->ObjectType));

   obj_r->ObjectName = p;
   skip_nonspaces(&p);                  /* skip ObjectName */
   skip_spaces(&p);
   c = strpbrk(obj_r->ObjectName, " ");
   if (c) {
      *c = '\0';
   }

   obj_r->ObjectSource = p;
   skip_nonspaces(&p);                  /* skip ObjectSource */
   skip_spaces(&p);
   c = strpbrk(obj_r->ObjectSource, " ");
   if (c) {
      *c = '\0';
   }

   obj_r->ObjectUUID = p;
   skip_nonspaces(&p);                  /* skip ObjectUuid */
   skip_spaces(&p);
   c = strpbrk(obj_r->ObjectUUID, " ");
   if (c) {
      *c = '\0';
   }

   obj_r->ObjectSize = str_to_uint64(p);
}


void parse_restore_object_string(char **r_obj_str, ROBJECT_DBR *robj_r)
{
   char *p = *r_obj_str;
   int len;

   robj_r->FileIndex = str_to_int32(p);        /* FileIndex */
   skip_nonspaces(&p);
   skip_spaces(&p);
   robj_r->FileType = str_to_int32(p);        /* FileType */
   skip_nonspaces(&p);
   skip_spaces(&p);
   robj_r->object_index = str_to_int32(p);    /* Object Index */
   skip_nonspaces(&p);
   skip_spaces(&p);
   robj_r->object_len = str_to_int32(p);      /* object length possibly compressed */
   skip_nonspaces(&p);
   skip_spaces(&p);
   robj_r->object_full_len = str_to_int32(p); /* uncompressed object length */
   skip_nonspaces(&p);
   skip_spaces(&p);
   robj_r->object_compression = str_to_int32(p); /* compression */
   skip_nonspaces(&p);
   skip_spaces(&p);

   robj_r->plugin_name = p;                      /* point to plugin name */
   len = strlen(robj_r->plugin_name);
   robj_r->object_name = &robj_r->plugin_name[len+1]; /* point to object name */
   len = strlen(robj_r->object_name);
   robj_r->object = &robj_r->object_name[len+1];      /* point to object */
   robj_r->object[robj_r->object_len] = 0;            /* add zero for those who attempt printing */
   Dmsg7(100, "oname=%s stream=%d FT=%d FI=%d JobId=%ld, obj_len=%d\nobj=\"%s\"\n",
      robj_r->object_name, robj_r->Stream, robj_r->FileType, robj_r->FileIndex, robj_r->JobId,
      robj_r->object_len, robj_r->object);
}

#endif /* HAVE_SQLITE3 || HAVE_MYSQL || HAVE_POSTGRESQL */ 
