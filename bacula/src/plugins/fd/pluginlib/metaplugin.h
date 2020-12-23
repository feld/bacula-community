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
 * @file metaplugin.h
 * @author Radosław Korzeniewski (radoslaw@korzeniewski.net)
 * @brief This is a Bacula metaplugin interface.
 * @version 2.1.0
 * @date 2020-12-23
 *
 * @copyright Copyright (c) 2020 All rights reserved. IP transferred to Bacula Systems according to agreement.
 */

#include "pluginlib.h"
#include "ptcomm.h"
#include "lib/ini.h"


#define USE_CMD_PARSER
#include "fd_common.h"

#ifndef _METAPLUGIN_H_
#define _METAPLUGIN_H_


// Plugin Info definitions
extern const char *PLUGIN_LICENSE;
extern const char *PLUGIN_AUTHOR;
extern const char *PLUGIN_DATE;
extern const char *PLUGIN_VERSION;
extern const char *PLUGIN_DESCRIPTION;

// Plugin linking time variables
extern const char *PLUGINPREFIX;
extern const char *PLUGINNAME;
extern const char *PLUGINAPI;
extern const char *BACKEND_CMD;

// The list of restore options saved to the RestoreObject.
extern struct ini_items plugin_items_dump[];

// the list of valid plugin options
extern const char *valid_params[];

// types used by Plugin


/*
 * This is a single backend context for backup and restore.
 *  The BackendCTX is used especially during multiple Plugin=... parameters
 *  defined in FileSet. Each Plugin=... parameter will have a dedicated
 *  backend handling job data. All of these backends will be executed
 *  concurrently, especially for restore where restored streams could achieve
 *  in different order. The allocated contests will be stored in a simple list.
 */
class BackendCTX : public SMARTALLOC
{
public:
   char *cmd;
   PTCOMM *comm;
   cmd_parser *parser;
   ConfigFile *ini;

   BackendCTX(char *command);
   ~BackendCTX();
};

/*
 * This is a main plugin API class. It manages a plugin context.
 *  All the public methods correspond to a public Bacula API calls, even if
 *  a callback is not implemented.
 */
class METAPLUGIN: public SMARTALLOC
{
public:
   enum class MODE
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
   bRC handleXACLdata(bpContext *ctx, struct xacl_pkt *xacl);
   bRC queryParameter(bpContext *ctx, struct query_pkt *qp);
   bRC setup_backend_command(bpContext *ctx, const char *exepath);
   METAPLUGIN(bpContext *bpctx);
#if __cplusplus > 201103L
   METAPLUGIN() = delete;
   METAPLUGIN(METAPLUGIN&) = delete;
   METAPLUGIN(METAPLUGIN&&) = delete;
#endif
   ~METAPLUGIN();

private:
   enum ListingMode
   {
      ListingNone,
      Listing,
      QueryParams,
   };

   bpContext *ctx;               // Bacula Plugin Context
   MODE mode;                    // Plugin mode of operation
   int JobId;                    // Job ID
   char *JobName;                // Job name
   time_t since;                 // Job since parameter
   char *where;                  // the Where variable for restore job if set by user
   char *regexwhere;             // the RegexWhere variable for restore job if set by user
   char replace;                 // the replace variable for restore job
   bool robjsent;                // set when RestoreObject was sent during Full backup
   bool estimate;                // used when mode is METAPLUGIN_BACKUP_* but we are doing estimate only
   ListingMode listing;          // used for a Listing procedure for estimate
   bool nodata;                  // set when backend signaled no data for backup or no data for restore
   bool nextfile;                // set when IO_CLOSE got FNAME: command
   bool openerror;               // show if "openfile" was unsuccessful
   bool pluginobject;            // set when IO_CLOSE got FNAME: command
   bool readacl;                 // got ACL data from backend
   bool readxattr;               // got XATTR data from backend
   bool accurate_warning;        // for sending accurate mode warning once */
   // TODO: define a variable which will signal job cancel
   BackendCTX *backendctx;       // the current backend context
   alist *backendlist;           // the backend context list for multiple backend execution for a single job
   POOL_MEM fname;               // current file name to backup (grabbed from backend)
   POOLMEM *lname;               // current LSTAT data if any
   POOLMEM *robjbuf;             // the buffer for restore object data
   POOL_MEM plugin_obj_cat;      //
   POOL_MEM plugin_obj_type;     //
   POOL_MEM plugin_obj_name;     //
   POOL_MEM plugin_obj_src;      //
   POOL_MEM plugin_obj_uuid;     //
   uint64_t plugin_obj_size;     //
   int acldatalen;               // the length of the data in acl buffer
   POOL_MEM acldata;             // the buffer for ACL data received from backend
   int xattrdatalen;             // the length of the data in xattr buffer
   POOL_MEM xattrdata;           // the buffer for XATTR data received from backend

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
   int check_ini_param(ConfigFile *ini, char *param);
   bool check_plugin_param(const char *param, alist *params);
   int get_ini_count(ConfigFile *ini);
   void new_backendlist(bpContext *ctx, char *command);
   bRC switch_or_run_backend(bpContext *ctx, char *command);
   bRC terminate_current_backend(bpContext *ctx);
   bRC terminate_all_backends(bpContext *ctx);
   bRC signal_finish_all_backends(bpContext *ctx);
   bRC render_param(bpContext *ctx, POOLMEM *param, INI_ITEM_HANDLER *handler, char *key, item_value val);
   void dump_backendlist(bpContext *ctx);
};

#endif   /* _METAPLUGIN_H_ */
