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
/*
 *
 * All rights reserved. IP transferred to Bacula Systems according to agreement.
 *
 * Common definitions and utility functions for Inteos plugins.
 * Functions defines a common framework used in our utilities and plugins.
 * Author: Radosław Korzeniewski, radekk@inteos.pl, Inteos Sp. z o.o.
 */

#ifndef _PLUGINLIB_H_
#define _PLUGINLIB_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

#include "bacula.h"
#include "fd_plugins.h"

/* Pointers to Bacula functions used in plugins */
extern bFuncs *bfuncs;
extern bInfo *binfo;

/* module definition */
#ifndef PLUGMODULE
#define PLUGMODULE   "PluginLib::"
#endif

/* size of different string or query buffers */
#define BUFLEN       4096
#define BIGBUFLEN    65536

/* debug and messages functions */
#define JMSG0(ctx,type,msg) \
   if (ctx) bfuncs->JobMessage ( ctx, __FILE__, __LINE__, type, 0, PLUGINPREFIX " " msg );
#define JMSG1 JMSG
#define JMSG(ctx,type,msg,var) \
   if (ctx) bfuncs->JobMessage ( ctx, __FILE__, __LINE__, type, 0, PLUGINPREFIX " " msg, var );
#define JMSG2(ctx,type,msg,var1,var2) \
   if (ctx) bfuncs->JobMessage ( ctx, __FILE__, __LINE__, type, 0, PLUGINPREFIX " " msg, var1, var2 );
#define JMSG3(ctx,type,msg,var1,var2,var3) \
   if (ctx) bfuncs->JobMessage ( ctx, __FILE__, __LINE__, type, 0, PLUGINPREFIX " " msg, var1, var2, var3 );
#define JMSG4(ctx,type,msg,var1,var2,var3,var4) \
   if (ctx) bfuncs->JobMessage ( ctx, __FILE__, __LINE__, type, 0, PLUGINPREFIX " " msg, var1, var2, var3, var4 );

#define DMSG0(ctx,level,msg) \
   if (ctx) bfuncs->DebugMessage ( ctx, __FILE__, __LINE__, level, PLUGINPREFIX " " msg );
#define DMSG1 DMSG
#define DMSG(ctx,level,msg,var) \
   if (ctx) bfuncs->DebugMessage ( ctx, __FILE__, __LINE__, level, PLUGINPREFIX " " msg, var );
#define DMSG2(ctx,level,msg,var1,var2) \
   if (ctx) bfuncs->DebugMessage ( ctx, __FILE__, __LINE__, level, PLUGINPREFIX " " msg, var1, var2 );
#define DMSG3(ctx,level,msg,var1,var2,var3) \
   if (ctx) bfuncs->DebugMessage ( ctx, __FILE__, __LINE__, level, PLUGINPREFIX " " msg, var1, var2, var3 );
#define DMSG4(ctx,level,msg,var1,var2,var3,var4) \
   if (ctx) bfuncs->DebugMessage ( ctx, __FILE__, __LINE__, level, PLUGINPREFIX " " msg, var1, var2, var3, var4 );
#define DMSG6(ctx,level,msg,var1,var2,var3,var4,var5,var6) \
   if (ctx) bfuncs->DebugMessage ( ctx, __FILE__, __LINE__, level, PLUGINPREFIX " " msg, var1, var2, var3, var4, var5, var6 );

/* fixed debug level definitions */
#define D1  1                    /* debug for every error */
#define DERROR D1
#define D2  10                   /* debug only important stuff */
#define DINFO  D2
#define D3  200                  /* debug for information only */
#define DDEBUG D3
#define D4  800                  /* debug for detailed information only */
#define DVDEBUG D4

#define getBaculaVar(bvar,val)  bfuncs->getBaculaValue(ctx, bvar, val);

/* used for sanity check in plugin functions */
#define ASSERT_CTX \
   if (!ctx || !ctx->pContext || !bfuncs) \
   { \
      return bRC_Error; \
   }

/* defines for handleEvent */
#define DMSG_EVENT_STR(event,value)       DMSG2(ctx, DINFO, "%s value=%s\n", eventtype2str(event), NPRT((char *)value));
#define DMSG_EVENT_CHAR(event,value)      DMSG2(ctx, DINFO, "%s value='%c'\n", eventtype2str(event), (char)value);
#define DMSG_EVENT_LONG(event,value)      DMSG2(ctx, DINFO, "%s value=%ld\n", eventtype2str(event), (intptr_t)value);
#define DMSG_EVENT_PTR(event,value)       DMSG2(ctx, DINFO, "%s value=%p\n", eventtype2str(event), value);

/* pure debug macros */
#define DMsg0(level,msg)                  Dmsg1(level,PLUGMODULE "%s: " msg,__func__)
#define DMsg1(level,msg,a1)               Dmsg2(level,PLUGMODULE "%s: " msg,__func__,a1)
#define DMsg2(level,msg,a1,a2)            Dmsg3(level,PLUGMODULE "%s: " msg,__func__,a1,a2)
#define DMsg3(level,msg,a1,a2,a3)         Dmsg4(level,PLUGMODULE "%s: " msg,__func__,a1,a2,a3)
#define DMsg4(level,msg,a1,a2,a3,a4)      Dmsg5(level,PLUGMODULE "%s: " msg,__func__,a1,a2,a3,a4)

#define BOOLSTR(b)                        (b?"True":"False")

/*
 * Common structure for key/pair values
 */
class key_pair : public SMARTALLOC
{
public:
   POOL_MEM key;
   POOL_MEM value;

   key_pair() : key(PM_NAME), value(PM_MESSAGE) {};
   key_pair(const char *k, const char *v)
   {
      pm_strcpy(key, k);
      pm_strcpy(value, v);
   };
   ~key_pair() {};
};

const char *eventtype2str(bEvent *event);
uint64_t pluglib_size_suffix(int disksize, char suff);
uint64_t pluglib_size_suffix(double disksize, char suff);
bRC pluglib_mkpath(bpContext* ctx, char* path, bool isfatal);

/*
 * Checks if plugin command points to our Plugin
 *
 * in:
 *    command - the plugin command used for backup/restore
 * out:
 *    True - if it is our plugin command
 *    False - the other plugin command
 */
inline bool isourplugincommand(const char *pluginprefix, const char *command)
{
   /* check if it is our Plugin command */
   if (strncmp(pluginprefix, command, strlen(pluginprefix)) == 0){
      /* it is not our plugin prefix */
      return true;
   }
   return false;
}

alist * plugutil_str_split_to_alist(const char * str, const char sep = '.');

/* plugin parameters manipulation */
bool render_param(POOLMEM **param, const char *pname, const char *fmt, const char *name, char *value);
bool render_param(POOLMEM **param, const char *pname, const char *fmt, const char *name, int value);
bool render_param(bool &param, const char *pname, const char *name, bool value);
bool parse_param(bool &param, const char *pname, const char *name, char *value);
bool parse_param(int &param, const char *pname, const char *name, char *value, bool *err = NULL);
bool parse_param(POOL_MEM &param, const char *pname, const char *name, char *value);
bool add_param_str(alist **list, const char *pname, const char *name, char *value);

bool scan_parameter_str(const char * cmd, const char *prefix, POOL_MEM &param);
inline bool scan_parameter_str(const POOL_MEM &cmd, const char *prefix, POOL_MEM &param) { return scan_parameter_str(cmd.c_str(), prefix, param); }

#endif   /* _PLUGINLIB_H_ */