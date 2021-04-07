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
 * @file metaplugin.h
 * @author Radosław Korzeniewski (radoslaw@korzeniewski.net)
 * @brief This is a Bacula metaplugin interface.
 * @version 2.2.0
 * @date 2021-03-08
 *
 * @copyright Copyright (c) 2021 All rights reserved. IP transferred to Bacula Systems according to agreement.
 */

#include "pluginlib.h"
#include "ptcomm.h"
#include "lib/ini.h"
#include "pluginlib/commctx.h"
#include "pluginlib/smartalist.h"


#define USE_CMD_PARSER
#include "fd_common.h"

#ifndef PLUGINLIB_METAPLUGIN_H
#define PLUGINLIB_METAPLUGIN_H


// Plugin Info definitions
extern const char *PLUGIN_LICENSE;
extern const char *PLUGIN_AUTHOR;
extern const char *PLUGIN_DATE;
extern const char *PLUGIN_VERSION;
extern const char *PLUGIN_DESCRIPTION;

// Plugin linking time variables
extern const char *PLUGINPREFIX;
extern const char *PLUGINNAME;
extern const char *PLUGINNAMESPACE;
extern const bool CUSTOMNAMESPACE;
extern const char *PLUGINAPI;
extern const char *BACKEND_CMD;

// custom checkFile() callback
typedef bRC (*checkFile_t)(bpContext *ctx, char *fname);
extern checkFile_t checkFile;

// The list of restore options saved to the RestoreObject.
extern struct ini_items plugin_items_dump[];

// the list of valid plugin options
extern const char *valid_params[];

struct metadataTypeMap
{
   const char *command;
   metadata_type type;
};

extern const metadataTypeMap plugin_metadata_map[];

/*
 * This is a main plugin API class. It manages a plugin context.
 *  All the public methods correspond to a public Bacula API calls, even if
 *  a callback is not implemented.
 */
class METAPLUGIN: public SMARTALLOC
{
public:
   enum MODE
   {
      NONE = 0,
      BACKUP_FULL,
      BACKUP_INCR,
      BACKUP_DIFF,
      Estimate,
      Listing,
      QueryParams,
      RESTORE,
   };

   POOL_MEM backend_cmd;

   bRC getPluginValue(bpContext *ctx, pVariable var, void *value);
   bRC setPluginValue(bpContext *ctx, pVariable var, void *value);
   bRC handlePluginEvent(bpContext *ctx, bEvent *event, void *value);
   bRC startBackupFile(bpContext *ctx, struct save_pkt *sp);
   bRC endBackupFile(bpContext *ctx);
   bRC startRestoreFile(bpContext *ctx, const char *cmd);
   bRC endRestoreFile(bpContext *ctx);
   bRC pluginIO(bpContext *ctx, struct io_pkt *io);
   bRC createFile(bpContext *ctx, struct restore_pkt *rp);
   bRC setFileAttributes(bpContext *ctx, struct restore_pkt *rp);
   bRC checkFile(bpContext *ctx, char *fname);
   bRC handleXACLdata(bpContext *ctx, struct xacl_pkt *xacl);
   bRC queryParameter(bpContext *ctx, struct query_pkt *qp);
   bRC metadataRestore(bpContext *ctx, struct meta_pkt *mp);
   void setup_backend_command(bpContext *ctx, POOL_MEM &exepath);
   METAPLUGIN(bpContext *bpctx);
#if __cplusplus > 201103L
   METAPLUGIN() = delete;
   METAPLUGIN(METAPLUGIN&) = delete;
   METAPLUGIN(METAPLUGIN&&) = delete;
#endif
   ~METAPLUGIN();

private:
   enum LISTING
   {
      ListingNone,
      ListingMode,
      ListingQueryParams,
   };

   // TODO: define a variable which will signal job cancel
   bpContext *ctx;               // Bacula Plugin Context
   bool backend_available;       // When `False` then backend program is unuseable or unavailable
   POOL_MEM backend_error;       // Holds the error string when backend program is unavailable
   MODE mode;                    // Plugin mode of operation
   int JobId;                    // Job ID
   char *JobName;                // Job name
   time_t since;                 // Job since parameter
   char *where;                  // the Where variable for restore job if set by user
   char *regexwhere;             // the RegexWhere variable for restore job if set by user
   char replace;                 // the replace variable for restore job
   bool robjsent;                // set when RestoreObject was sent during Full backup
   bool estimate;                // used when mode is METAPLUGIN_BACKUP_* but we are doing estimate only
   LISTING listing;              // used for a Listing procedure for estimate
   bool nodata;                  // set when backend signaled no data for backup or no data for restore
   bool nextfile;                // set when IO_CLOSE got FNAME: command
   bool openerror;               // show if "openfile" was unsuccessful
   bool pluginobject;            // set when IO_CLOSE got FNAME: command
   bool pluginobjectsent;        // set when startBackupFile handled plugin object and endBackupFile has to check for nextfile
   bool readacl;                 // got ACL data from backend
   bool readxattr;               // got XATTR data from backend
   COMMCTX<PTCOMM> backend;      // the backend context list for multiple backend execution for a single job
   POOL_MEM fname;               // current file name to backup (grabbed from backend)
   POOL_MEM lname;               // current LSTAT data if any
   POOLMEM *robjbuf;             // the buffer for restore object data
   POOL_MEM plugin_obj_cat;      // Plugin object Category
   POOL_MEM plugin_obj_type;     // Plugin object Type
   POOL_MEM plugin_obj_name;     // Plugin object Name
   POOL_MEM plugin_obj_src;      // Plugin object Source
   POOL_MEM plugin_obj_uuid;     // Plugin object UUID
   uint64_t plugin_obj_size;     // Plugin object Size
   int acldatalen;               // the length of the data in acl buffer
   POOL_MEM acldata;             // the buffer for ACL data received from backend
   int xattrdatalen;             // the length of the data in xattr buffer
   POOL_MEM xattrdata;           // the buffer for XATTR data received from backend
   cmd_parser parser;            // Plugin command parser
   ConfigFile ini;               // Restore ini file handler
   alist metadatas_list;         //
   plugin_metadata metadatas;    //

   bRC parse_plugin_command(bpContext *ctx, const char *command, alist *params);
   bRC parse_plugin_restoreobj(bpContext *ctx, restore_object_pkt *rop);
   bRC run_backend(bpContext *ctx);
   bRC send_parameters(bpContext *ctx, char *command);
   bRC send_jobinfo(bpContext *ctx, char type);
   bRC send_startjob(bpContext *ctx, const char *command);
   bRC send_startbackup(bpContext *ctx);
   bRC send_startestimate(bpContext *ctx);
   bRC send_startlisting(bpContext *ctx);
   bRC send_startrestore(bpContext *ctx);
   bRC send_endjob(bpContext *ctx);
   bRC prepare_backend(bpContext *ctx, char type, char *command);
   bRC perform_backup_open(bpContext *ctx, struct io_pkt *io);
   bRC perform_restore_open(bpContext *ctx, struct io_pkt *io);
   bRC perform_read_data(bpContext *ctx, struct io_pkt *io);
   bRC perform_write_data(bpContext *ctx, struct io_pkt *io);
   bRC perform_write_end(bpContext *ctx, struct io_pkt *io);
   bRC perform_read_metadata(bpContext *ctx);
   bRC perform_read_fstatdata(bpContext *ctx, struct save_pkt *sp);
   bRC perform_read_pluginobject(bpContext *ctx, struct save_pkt *sp);
   bRC perform_read_acl(bpContext *ctx);
   bRC perform_write_acl(bpContext *ctx, struct xacl_pkt * xacl);
   bRC perform_read_xattr(bpContext *ctx);
   bRC perform_write_xattr(bpContext *ctx, struct xacl_pkt * xacl);
   bRC perform_read_metadata_info(bpContext *ctx, metadata_type type, struct save_pkt *sp);
   bRC perform_file_index_query(bpContext *ctx);
   // bRC perform_write_metadata_info(bpContext *ctx, struct meta_pkt *mp);
   metadata_type scan_metadata_type(const POOL_MEM &cmd);
   const char *prepare_metadata_type(metadata_type type);
   int check_ini_param(char *param);
   bool check_plugin_param(const char *param, alist *params);
   int get_ini_count();
   void new_backendlist(bpContext *ctx, char *command);
   bRC switch_or_run_backend(bpContext *ctx, char *command);
   bRC terminate_current_backend(bpContext *ctx);
   bRC terminate_all_backends(bpContext *ctx);
   bRC signal_finish_all_backends(bpContext *ctx);
   bRC render_param(bpContext *ctx, POOLMEM *param, INI_ITEM_HANDLER *handler, char *key, item_value val);
};

#endif   // PLUGINLIB_METAPLUGIN_H
