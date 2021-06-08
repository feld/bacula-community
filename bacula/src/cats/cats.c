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

static int dbglvl=100;

void append_filter(POOLMEM **buf, char *cond)
{
   if (*buf[0] != '\0') {
      pm_strcat(buf, " AND ");
   } else {
      pm_strcpy(buf, " WHERE ");
   }

   pm_strcat(buf, cond);
}

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

bool OBJECT_DBR::parse_plugin_object_string(char **obj_str)
{
   bool ret = false;
   int fnl, pnl;

   char *tmp = get_next_tag(obj_str);
   if (!tmp) {
      goto bail_out;
   }

   if (tmp[strlen(tmp) - 1] == '/') {
      pm_strcpy(Path, tmp);
      unbash_spaces(Path);
   } else {
      split_path_and_filename(tmp, &Path, &pnl, &Filename, &fnl);
      unbash_spaces(Path);
      unbash_spaces(Filename);
   }

   tmp = get_next_tag(obj_str);
   if (!tmp) {
      goto bail_out;
   }
   pm_strcpy(PluginName, tmp);
   unbash_spaces(PluginName);

   tmp = get_next_tag(obj_str);
   if (!tmp) {
      goto bail_out;
   }
   bstrncpy(ObjectCategory, tmp, sizeof(ObjectCategory));
   unbash_spaces(ObjectCategory);

   tmp = get_next_tag(obj_str);
   if (!tmp) {
      goto bail_out;
   }
   bstrncpy(ObjectType, tmp, sizeof(ObjectType));
   unbash_spaces(ObjectType);

   tmp = get_next_tag(obj_str);
   if (!tmp) {
      goto bail_out;
   }
   bstrncpy(ObjectName, tmp, sizeof(ObjectName));
   unbash_spaces(ObjectName);

   tmp = get_next_tag(obj_str);
   if (!tmp) {
      goto bail_out;
   }
   bstrncpy(ObjectSource, tmp, sizeof(ObjectSource));
   unbash_spaces(ObjectSource);

   tmp = get_next_tag(obj_str);
   if (!tmp) {
      goto bail_out;
   }
   bstrncpy(ObjectUUID, tmp, sizeof(ObjectUUID));
   unbash_spaces(ObjectUUID);

   tmp = get_next_tag(obj_str);
   if (tmp) {
      ObjectSize = str_to_uint64(tmp);
   } else if (*obj_str) {
      /* Object size is the last tag here, we are not expecting to have status in the stream */
      ObjectSize = str_to_uint64(*obj_str);
      ret = true;
      goto bail_out;
   } else {
      goto bail_out;
   }

   /* We should have status string in the end */
   if (*obj_str) {
      tmp = get_next_tag(obj_str);
      if (!tmp) {
         goto bail_out;
      }
      ObjectStatus = (int)*tmp;
      if (*obj_str) {
         ObjectCount = str_to_uint64(*obj_str);
      }
   } else {
      goto bail_out;
   }

   ret = true;

bail_out:
   if (!ret) {
      /* Reset parsed fields */
      reset();
   }

   return ret;
}

void OBJECT_DBR::create_db_filter(JCR *jcr, POOLMEM **where)
{
   POOL_MEM esc(PM_MESSAGE), tmp(PM_MESSAGE);

   if (ObjectId > 0) {
      Mmsg(tmp, " Object.ObjectId=%lu", ObjectId);
      append_filter(where, tmp.c_str());
   } else {
      if (JobId != 0) {
         Mmsg(tmp, " Object.JobId=%lu", JobId);
         append_filter(where, tmp.c_str());
      }

      if (Path[0] != 0) {
         db_escape_string(jcr, jcr->db, esc.c_str(), Path, strlen(Path));
         Mmsg(tmp, " Object.Path='%s'", esc.c_str());
         append_filter(where, tmp.c_str());
      }

      if (Filename[0] != 0) {
         db_escape_string(jcr, jcr->db, esc.c_str(), Filename, strlen(Filename));
         Mmsg(tmp, " Object.Filename='%s'", esc.c_str());
         append_filter(where, tmp.c_str());
      }

      if (PluginName[0] != 0) {
         db_escape_string(jcr, jcr->db, esc.c_str(), PluginName, strlen(PluginName));
         Mmsg(tmp, " Object.PluginName='%s'", esc.c_str());
         append_filter(where, tmp.c_str());
      }

      if (ObjectCategory[0] != 0) {
         db_escape_string(jcr, jcr->db, esc.c_str(), ObjectCategory, strlen(ObjectCategory));
         Mmsg(tmp, " Object.ObjectCategory='%s'", esc.c_str());
         append_filter(where, tmp.c_str());
      }

      if (ObjectType[0] != 0) {
         db_escape_string(jcr, jcr->db, esc.c_str(), ObjectType, strlen(ObjectType));
         Mmsg(tmp, " Object.ObjectType='%s'", esc.c_str());
         append_filter(where, tmp.c_str());
      }

      if (ObjectName[0] != 0) {
         db_escape_string(jcr, jcr->db, esc.c_str(), ObjectName, strlen(ObjectName));
         Mmsg(tmp, " Object.Objectname='%s'", esc.c_str());
         append_filter(where, tmp.c_str());
      }

      if (ObjectSource[0] != 0) {
         db_escape_string(jcr, jcr->db, esc.c_str(), ObjectSource, strlen(ObjectSource));
         Mmsg(tmp, " Object.ObjectSource='%s'", esc.c_str());
         append_filter(where, tmp.c_str());
      }

      if (ObjectUUID[0] != 0) {
         db_escape_string(jcr, jcr->db, esc.c_str(), ObjectUUID, strlen(ObjectUUID));
         Mmsg(tmp, " Object.ObjectUUID='%s'", esc.c_str());
         append_filter(where, tmp.c_str());
      }

      if (ObjectSize > 0) {
         Mmsg(tmp, " Object.ObjectSize=%llu", ObjectSize);
         append_filter(where, tmp.c_str());
      }

      if (ObjectStatus != 0) {
         Mmsg(tmp, " Object.ObjectStatus='%c'", ObjectStatus);
         append_filter(where, tmp.c_str());
      }
   }

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
   Dmsg7(dbglvl, "oname=%s stream=%d FT=%d FI=%d JobId=%ld, obj_len=%d\nobj=\"%s\"\n",
      robj_r->object_name, robj_r->Stream, robj_r->FileType, robj_r->FileIndex, robj_r->JobId,
      robj_r->object_len, robj_r->object);
}

#endif /* HAVE_SQLITE3 || HAVE_MYSQL || HAVE_POSTGRESQL */
