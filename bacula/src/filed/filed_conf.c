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
/*
 *   Main configuration file parser for Bacula File Daemon (Client)
 *    some parts may be split into separate files such as
 *    the schedule configuration (sch_config.c).
 *
 *   Note, the configuration file parser consists of three parts
 *
 *   1. The generic lexical scanner in lib/lex.c and lib/lex.h
 *
 *   2. The generic config  scanner in lib/parse_config.c and
 *      lib/parse_config.h.
 *      These files contain the parser code, some utility
 *      routines, and the common store routines (name, int,
 *      string).
 *
 *   3. The daemon specific file, which contains the Resource
 *      definitions as well as any specific store routines
 *      for the resource records.
 *
 *     Kern Sibbald, September MM
 *
 */

#include "bacula.h"
#include "filed.h"
#include "lib/status-pkt.h"

/* Define the first and last resource ID record
 * types. Note, these should be unique for each
 * daemon though not a requirement.
 */
int32_t r_first = R_FIRST;
int32_t r_last  = R_LAST;
RES_HEAD **res_head;


/* Forward referenced subroutines */


/* We build the current resource here as we are
 * scanning the resource configuration definition,
 * then move it to allocated memory when the resource
 * scan is complete.
 */
#if defined(_MSC_VER)
extern "C" { // work around visual compiler mangling variables
   URES res_all;
}
#else
URES res_all;
#endif
int32_t res_all_size = sizeof(res_all);

/* Definition of records permitted within each
 * resource with the routine to process the record
 * information.
 */

/* Client or File daemon "Global" resources */
static RES_ITEM cli_items[] = {
   {"Name",        store_name,  ITEM(res_client.hdr.name), 0, ITEM_REQUIRED, 0},
   {"Description", store_str,   ITEM(res_client.hdr.desc), 0, 0, 0},
   {"FdPort",      store_addresses_port,    ITEM(res_client.FDaddrs),  0, ITEM_DEFAULT, 9102},
   {"FdAddress",   store_addresses_address, ITEM(res_client.FDaddrs),  0, ITEM_DEFAULT, 9102},
   {"FdAddresses", store_addresses,         ITEM(res_client.FDaddrs),  0, ITEM_DEFAULT, 9102},
   {"FdSourceAddress", store_addresses_address, ITEM(res_client.FDsrc_addr),  0, ITEM_DEFAULT, 0},

   {"WorkingDirectory",  store_dir, ITEM(res_client.working_directory), 0, ITEM_REQUIRED, 0},
   {"PidDirectory",  store_dir,     ITEM(res_client.pid_directory),     0, ITEM_REQUIRED, 0},
   {"SubsysDirectory",  store_dir,  ITEM(res_client.subsys_directory),  0, 0, 0},
   {"PluginDirectory",  store_dir,  ITEM(res_client.plugin_directory),  0, 0, 0},
   {"SnapshotCommand",  store_str,  ITEM(res_client.snapshot_command), 0, 0, 0},
   {"ScriptsDirectory", store_dir,  ITEM(res_client.scripts_directory),  0, 0, 0},
   {"MaximumConcurrentJobs", store_pint32,  ITEM(res_client.MaxConcurrentJobs), 0, ITEM_DEFAULT, 20},
   {"Messages",      store_res, ITEM(res_client.messages), R_MSGS, 0, 0},
   {"SdConnectTimeout", store_time,ITEM(res_client.SDConnectTimeout), 0, ITEM_DEFAULT, 60 * 30},
   {"HeartbeatInterval", store_time, ITEM(res_client.heartbeat_interval), 0, ITEM_DEFAULT, 5 * 60},
   {"MaximumNetworkBufferSize", store_pint32, ITEM(res_client.max_network_buffer_size), 0, 0, 0},
#if BEEF
   {"FipsRequire", store_bool, ITEM(res_client.require_fips), 0, 0, 0},
#endif
#ifdef DATA_ENCRYPTION
   {"PkiSignatures",         store_bool,    ITEM(res_client.pki_sign), 0, ITEM_DEFAULT, 0},
   {"PkiEncryption",         store_bool,    ITEM(res_client.pki_encrypt), 0, ITEM_DEFAULT, 0},
   {"PkiKeyPair",            store_dir,         ITEM(res_client.pki_keypair_file), 0, 0, 0},
   {"PkiSigner",             store_alist_str,   ITEM(res_client.pki_signing_key_files), 0, 0, 0},
   {"PkiMasterKey",          store_alist_str,   ITEM(res_client.pki_master_key_files), 0, 0, 0},
   {"PkiCipher",             store_cipher_type, ITEM(res_client.pki_cipher), 0, 0, 0},
   {"PkiDigest",             store_digest_type, ITEM(res_client.pki_digest), 0, 0, 0},
#endif
   {"TlsAuthenticate",       store_bool,    ITEM(res_client.tls_authenticate),  0, 0, 0},
   {"TlsEnable",             store_bool,    ITEM(res_client.tls_enable),  0, 0, 0},
   {"TlsPskEnable",          store_bool,    ITEM(res_client.tls_psk_enable),  0, ITEM_DEFAULT, tls_psk_default},
   {"TlsRequire",            store_bool,    ITEM(res_client.tls_require), 0, 0, 0},
   {"TlsCaCertificateFile",  store_dir,     ITEM(res_client.tls_ca_certfile), 0, 0, 0},
   {"TlsCaCertificateDir",   store_dir,     ITEM(res_client.tls_ca_certdir), 0, 0, 0},
   {"TlsCertificate",        store_dir,     ITEM(res_client.tls_certfile), 0, 0, 0},
   {"TlsKey",                store_dir,     ITEM(res_client.tls_keyfile), 0, 0, 0},
   {"VerId",                 store_str,     ITEM(res_client.verid), 0, 0, 0},
   {"MaximumBandwidthPerJob",store_speed,   ITEM(res_client.max_bandwidth_per_job), 0, 0, 0},
   {"CommCompression",       store_bool,    ITEM(res_client.comm_compression), 0, ITEM_DEFAULT, true},
   {"DisableCommand",        store_alist_str, ITEM(res_client.disable_cmds), 0, 0, 0},
   {"MaximumJobErrorCount",  store_pint32,    ITEM(res_client.max_job_errors),  0, ITEM_DEFAULT, 1000},
   {"SdPacketCheck",         store_pint32,    ITEM(res_client.sd_packet_check),  0, ITEM_DEFAULT, 0},
#if BEEF
   {"DedupIndexDirectory",   store_dir,    ITEM(res_client.dedup_index_dir), 0, 0, 0}, /* deprecated */
   {"EnableClientRehydration", store_bool,    ITEM(res_client.allow_dedup_cache), 0, ITEM_DEFAULT, false},
#endif
   {NULL, NULL, {0}, 0, 0, 0}
};

/* Directors that can use our services */
static RES_ITEM dir_items[] = {
   {"Name",        store_name,     ITEM(res_dir.hdr.name),  0, ITEM_REQUIRED, 0},
   {"Description", store_str,      ITEM(res_dir.hdr.desc),  0, 0, 0},
   {"Password",    store_password, ITEM(res_dir.dirinfo.password),  0, ITEM_REQUIRED, 0},
   /* ***BEE*** In BWeb/BConfig, the store_str is replaced by
   store_addresses_address to check more carefully user inputs */
   {"Address",     store_str,      ITEM(res_dir.dirinfo.address),   0, 0, 0},
   {"DirPort",     store_pint32,    ITEM(res_dir.dirinfo.DIRport),  0, ITEM_DEFAULT, 9101},
   {"Monitor",     store_bool,   ITEM(res_dir.monitor),   0, ITEM_DEFAULT, 0},
   {"Remote",      store_bool,   ITEM(res_dir.remote),   0, ITEM_DEFAULT, 0},
   {"TlsAuthenticate",      store_bool,    ITEM(res_dir.dirinfo.tls_authenticate), 0, 0, 0},
   {"TlsEnable",            store_bool,    ITEM(res_dir.dirinfo.tls_enable), 0, 0, 0},
   {"TlsPskEnable",         store_bool,    ITEM(res_dir.dirinfo.tls_psk_enable), 0, ITEM_DEFAULT, tls_psk_default},
   {"TlsRequire",           store_bool,    ITEM(res_dir.dirinfo.tls_require), 0, 0, 0},
   {"TlsVerifyPeer",        store_bool,    ITEM(res_dir.dirinfo.tls_verify_peer), 0, ITEM_DEFAULT, 1},
   {"TlsCaCertificateFile", store_dir,       ITEM(res_dir.dirinfo.tls_ca_certfile), 0, 0, 0},
   {"TlsCaCertificateDir",  store_dir,       ITEM(res_dir.dirinfo.tls_ca_certdir), 0, 0, 0},
   {"TlsCertificate",       store_dir,       ITEM(res_dir.dirinfo.tls_certfile), 0, 0, 0},
   {"TlsKey",               store_dir,       ITEM(res_dir.dirinfo.tls_keyfile), 0, 0, 0},
   {"TlsDhFile",            store_dir,       ITEM(res_dir.dirinfo.tls_dhfile), 0, 0, 0},
   {"TlsAllowedCn",         store_alist_str, ITEM(res_dir.dirinfo.tls_allowed_cns), 0, 0, 0},
   {"MaximumBandwidthPerJob", store_speed,     ITEM(res_dir.max_bandwidth_per_job), 0, 0, 0},
   {"DisableCommand",        store_alist_str, ITEM(res_dir.disable_cmds), 0, 0, 0},
   {"Console",              store_res, ITEM(res_dir.console),  R_CONSOLE, 0, 0},
   {"ConnectToDirector",    store_bool,     ITEM(res_dir.connect_to_dir), 0, 0, 0},
   {"Schedule", store_res, ITEM(res_dir.schedule), R_SCHEDULE, 0, 0},
   {"ReconnectionTime", store_time,ITEM(res_dir.reconnection_time), 0, ITEM_DEFAULT, 60 * 45},
   {NULL, NULL, {0}, 0, 0, 0}
};

/* Consoles that we can use to connect a Director */
static RES_ITEM cons_items[] = {
   {"Name",        store_name,     ITEM(res_cons.hdr.name),  0, ITEM_REQUIRED, 0},
   {"Description", store_str,      ITEM(res_cons.hdr.desc),  0, 0, 0},
   {"Password",    store_password, ITEM(res_cons.dirinfo.password),  0, ITEM_REQUIRED, 0},
   /* ***BEE*** In BWeb/BConfig, the store_str is replaced by
   store_addresses_address to check more carefully user inputs */
   {"Address",     store_str,      ITEM(res_cons.dirinfo.address),   0, 0, 0},
   {"DirPort",        store_pint32,    ITEM(res_cons.dirinfo.DIRport),  0, ITEM_DEFAULT, 9101},
   {"TlsAuthenticate",      store_bool,    ITEM(res_cons.dirinfo.tls_authenticate), 0, 0, 0},
   {"TlsEnable",            store_bool,    ITEM(res_cons.dirinfo.tls_enable), 0, 0, 0},
   {"TlsPskEnable",         store_bool,    ITEM(res_cons.dirinfo.tls_psk_enable), 0, ITEM_DEFAULT, tls_psk_default},
   {"TlsRequire",           store_bool,    ITEM(res_cons.dirinfo.tls_require), 0, 0, 0},
   {"TlsVerifyPeer",        store_bool,    ITEM(res_cons.dirinfo.tls_verify_peer), 0, ITEM_DEFAULT, 1},
   {"TlsCaCertificateFile", store_dir,       ITEM(res_cons.dirinfo.tls_ca_certfile), 0, 0, 0},
   {"TlsCaCertificateDir",  store_dir,       ITEM(res_cons.dirinfo.tls_ca_certdir), 0, 0, 0},
   {"TlsCertificate",       store_dir,       ITEM(res_cons.dirinfo.tls_certfile), 0, 0, 0},
   {"TlsKey",               store_dir,       ITEM(res_cons.dirinfo.tls_keyfile), 0, 0, 0},
   {"TlsDhFile",            store_dir,       ITEM(res_cons.dirinfo.tls_dhfile), 0, 0, 0},
   {"TlsAllowedCn",         store_alist_str, ITEM(res_cons.dirinfo.tls_allowed_cns), 0, 0, 0},
   {NULL, NULL, {0}, 0, 0, 0}
};

/* Message resource */
extern RES_ITEM msgs_items[];

/* Statistics resource */
extern RES_ITEM collector_items[];

/*
 * Statistics resource directives
 *
 *  name         handler      value       code   flags  default_value
 */
RES_ITEM sched_items[] = {
   {"Name",        store_name,  ITEM(res_sched.hdr.name),          0, ITEM_REQUIRED, 0},
   {"Description", store_str,   ITEM(res_sched.hdr.desc),          0, 0, 0},
   {"Enabled",     store_bool,  ITEM(res_sched.Enabled),  0, ITEM_DEFAULT, true},
   {"Connect",     store_runres, ITEM(res_sched.run),      0, 0, 0},
   {NULL,               NULL,             {0},                                   0, 0, 0}
};

/*
 * This is the master resource definition.
 * It must have one item for each of the resources.
 */
RES_TABLE resources[] = {
   {"Director",      dir_items,        R_DIRECTOR},
   {"FileDaemon",    cli_items,        R_CLIENT},
   {"Messages",      msgs_items,       R_MSGS},
   {"Console",       cons_items,       R_CONSOLE},
   {"Statistics",    collector_items,  R_COLLECTOR},
   {"Schedule",      sched_items,      R_SCHEDULE},
   {"Client",        cli_items,        R_CLIENT},     /* keep last, alias for filedaemon */
   {NULL,            NULL,             0}
};

struct s_ct ciphertypes[] = {
   {"aes128",        CRYPTO_CIPHER_AES_128_CBC},
   {"aes192",        CRYPTO_CIPHER_AES_192_CBC},
   {"aes256",        CRYPTO_CIPHER_AES_256_CBC},
   {"blowfish",      CRYPTO_CIPHER_BLOWFISH_CBC},
   {NULL,            0}
};

struct s_ct digesttypes[] = {
   {"md5",         CRYPTO_DIGEST_MD5},
   {"sha1",        CRYPTO_DIGEST_SHA1},
   {"sha256",      CRYPTO_DIGEST_SHA256},
//   {"sha512",      CRYPTO_DIGEST_SHA512}, /* Not working yet */
   {NULL,                             0}
};

/*
 * Store cipher type
 *
 */
void store_cipher_type(LEX *lc, RES_ITEM *item, int index, int pass)
{
   int i;

   lex_get_token(lc, T_NAME);
   /* Store the type both pass 1 and pass 2 */
   for (i=0; ciphertypes[i].type_name; i++) {
      if (strcasecmp(lc->str, ciphertypes[i].type_name) == 0) {
         *(uint32_t *)(item->value) = ciphertypes[i].type_value;
         i = 0;
         break;
      }
   }
   if (i != 0) {
      scan_err1(lc, _("Expected a Cipher Type keyword, got: %s"), lc->str);
   }
   scan_to_eol(lc);
   set_bit(index, res_all.hdr.item_present);
}

/*
 * Store digest type
 *
 */
void store_digest_type(LEX *lc, RES_ITEM *item, int index, int pass)
{
   int i;

   lex_get_token(lc, T_NAME);
   /* Store the type both pass 1 and pass 2 */
   for (i=0; digesttypes[i].type_name; i++) {
      if (strcasecmp(lc->str, digesttypes[i].type_name) == 0) {
         *(uint32_t *)(item->value) = digesttypes[i].type_value;
         i = 0;
         break;
      }
   }
   if (i != 0) {
      scan_err1(lc, _("Expected a Cipher Type keyword, got: %s"), lc->str);
   }
   scan_to_eol(lc);
   set_bit(index, res_all.hdr.item_present);
}

/* Dump Client/FileDaemon resource */
static void dump_client_resource(CLIENT *client, void sendit(const char *msg, int len, STATUS_PKT *sp), STATUS_PKT *sp)
{
   OutputWriter ow(sp->api_opts);
   POOL_MEM tmp;
   char *p;

   ow.start_group("resource");

   ow.get_output(OT_START_OBJ,
                 OT_STRING,   "Name", client->hdr.name,
                 OT_STRING,   "WorkingDirectory", client->working_directory,
                 OT_STRING,   "PidDirectory", client->pid_directory,
                 OT_STRING,   "SubsysDirectory", client->subsys_directory,
                 OT_STRING,   "ScriptsDirectory", client->scripts_directory,
                 OT_STRING,   "PluginDirectory", client->plugin_directory,
#if BEEF
                 OT_BOOL,     "FIPSRequire", client->require_fips,
#endif
                 OT_INT64,    "HeartbeatInterval", client->heartbeat_interval,
                 OT_INT32,    "MaximumConcurrentJobs", client->MaxConcurrentJobs,
                 OT_INT,      "FDPort", get_first_port_host_order(client->FDaddrs),
                 OT_STRING,   "FDAddress", get_first_address(client->FDaddrs, tmp.c_str(), PM_MESSAGE),
                 OT_INT64,    "SDConnectTimeout", client->SDConnectTimeout,
                 OT_INT32,    "MaximumNetworkBufferSize", client->max_network_buffer_size,
                 OT_INT64,    "MaximumBandwidthPerJob", client->max_bandwidth_per_job,
                 OT_BOOL,     "CommCommpression", client->comm_compression,
                 OT_ALIST_STR, "DisableCommand", client->disable_cmds,
                 OT_BOOL,      "TLSEnable", client->tls_enable,
                 OT_BOOL,      "TLSPSKEnable", client->tls_psk_enable,
                 OT_BOOL,      "TLSRequire", client->tls_require,
                 OT_BOOL,      "TLSAuthenticate", client->tls_authenticate,
#ifdef DATA_ENCRYTION
                 OT_BOOL, "PKISignatures", client->pki_sign,
                 OT_BOOL, "PKIEncrytption", client->pki_encrypt,
#endif
                 OT_END_OBJ,
                 OT_END);

   p = ow.end_group("resource");
   sendit(p, strlen(p), sp);
}

/* In order to link with libbaccfg
 * Empty method is here just be able to link with libbaccfg (see overload in @parse_conf.h).
 * FileDaemon should use overloaded one, see methodb below.
 * TODO this should be removed as soon as Director's dump_resource() is updated to work
 * with STATUS_PKT.
 * */
void dump_resource(int type, RES *ares, void sendit(void *sock, const char *fmt, ...), void *sock)
{
   /**** NOT USED ****/
}

/* Dump contents of a resource. */
void dump_resource(int type, RES *ares, void sendit(const char *msg, int len, STATUS_PKT *sp), STATUS_PKT *sp)
{
   URES *res = (URES *)ares;
   int recurse = 1, len;
   POOL_MEM msg(PM_MESSAGE);

   if (res == NULL) {
      len = Mmsg (msg, "No record for %d %s\n", type, res_to_str(type));
      sendit(msg.c_str(), len, sp);
      return;
   }
   if (type < 0) {                    /* no recursion */
      type = - type;
      recurse = 0;
   }
   switch (type) {
   case R_CONSOLE:
      len = Mmsg(msg, "Console: name=%s password=%s\n", ares->name,
             res->res_cons.dirinfo.password);
      sendit(msg.c_str(), len, sp);
      break;
   case R_DIRECTOR:
      len = Mmsg(msg, "Director: name=%s password=%s\n", ares->name,
              res->res_dir.dirinfo.password);
      sendit(msg.c_str(), len, sp);
      break;
   case R_CLIENT:
      {
         CLIENT *client = (CLIENT *)ares;
         dump_client_resource(client, sendit, sp);
         break;
      }
   case R_MSGS:
      len = Mmsg(msg, "Messages: name=%s\n", res->res_msgs.hdr.name);
      sendit(msg.c_str(), len, sp);
      if (res->res_msgs.mail_cmd) {
         len = Mmsg(msg, "      mailcmd=%s\n", res->res_msgs.mail_cmd);
         sendit(msg.c_str(), len, sp);
      }
      if (res->res_msgs.operator_cmd) {
         len = Mmsg(msg, "      opcmd=%s\n", res->res_msgs.operator_cmd);
         sendit(msg.c_str(), len, sp);
      }
      break;
   case R_COLLECTOR:
      dump_collector_resource(res->res_collector, sendit, sp);
      break;
   case R_SCHEDULE:
      if (res->res_sched.run) {
         int i;
         RUNRES *run = res->res_sched.run;
         char buf[1000], num[30];
         len = Mmsg(msg, "Schedule: Name=%s Enabled=%d\n", 
                    res->res_sched.hdr.name, res->res_sched.is_enabled());
         sendit(msg.c_str(), len, sp);
         if (!run) {
            break;
         }
next_run:
         if (run->MaxConnectTime) {
            len = Mmsg(msg, "      MaxConnectTime=%u\n", run->MaxConnectTime);
            sendit(msg.c_str(), len, sp);
         }
         bstrncpy(buf, _("      hour="), sizeof(buf));
         for (i=0; i<24; i++) {
            if (bit_is_set(i, run->hour)) {
               bsnprintf(num, sizeof(num), "%d ", i);
               bstrncat(buf, num, sizeof(buf));
            }
         }
         bstrncat(buf, "\n", sizeof(buf));
         sendit(buf, strlen(buf), sp);
         bstrncpy(buf, _("      mday="), sizeof(buf));
         for (i=0; i<32; i++) {
            if (bit_is_set(i, run->mday)) {
               bsnprintf(num, sizeof(num), "%d ", i);
               bstrncat(buf, num, sizeof(buf));
            }
         }
         bstrncat(buf, "\n", sizeof(buf));
         sendit(buf, strlen(buf), sp);
         bstrncpy(buf, _("      month="), sizeof(buf));
         for (i=0; i<12; i++) {
            if (bit_is_set(i, run->month)) {
               bsnprintf(num, sizeof(num), "%d ", i);
               bstrncat(buf, num, sizeof(buf));
            }
         }
         bstrncat(buf, "\n", sizeof(buf));
         sendit(buf, strlen(buf), sp);
         bstrncpy(buf, _("      wday="), sizeof(buf));
         for (i=0; i<7; i++) {
            if (bit_is_set(i, run->wday)) {
               bsnprintf(num, sizeof(num), "%d ", i);
               bstrncat(buf, num, sizeof(buf));
            }
         }
         bstrncat(buf, "\n", sizeof(buf));
         sendit(buf, strlen(buf), sp);
         bstrncpy(buf, _("      wom="), sizeof(buf));
         for (i=0; i<6; i++) {
            if (bit_is_set(i, run->wom)) {
               bsnprintf(num, sizeof(num), "%d ", i);
               bstrncat(buf, num, sizeof(buf));
            }
         }
         bstrncat(buf, "\n", sizeof(buf));
         sendit(buf, strlen(buf), sp);
         bstrncpy(buf, _("      woy="), sizeof(buf));
         for (i=0; i<54; i++) {
            if (bit_is_set(i, run->woy)) {
               bsnprintf(num, sizeof(num), "%d ", i);
               bstrncat(buf, num, sizeof(buf));
            }
         }
         bstrncat(buf, "\n", sizeof(buf));
         sendit(buf, strlen(buf), sp);
         len = Mmsg(msg, "      mins=%d\n", run->minute);
         sendit(msg.c_str(), len, sp);
         /* If another Run record is chained in, go print it */
         if (run->next) {
            run = run->next;
            goto next_run;
         }
      } else {
         len = Mmsg(msg, "Schedule: name=%s\n", res->res_sched.hdr.name);
         sendit(msg.c_str(), len, sp);
      }
      break;

   default:
      len = Mmsg(msg, "Unknown resource type %d\n", type);
      sendit(msg.c_str(), len, sp);
   }
   ares = GetNextRes(type, ares);
   if (recurse && ares) {
      dump_resource(type, ares, sendit, sp);
   }
}


/*
 * Free memory of resource.
 * NB, we don't need to worry about freeing any references
 * to other resources as they will be freed when that
 * resource chain is traversed.  Mainly we worry about freeing
 * allocated strings (names).
 */
void free_resource(RES *sres, int type)
{
   URES *res = (URES *)sres;

   if (res == NULL) {
      return;
   }

   /* common stuff -- free the resource name */
   if (res->res_dir.hdr.name) {
      free(res->res_dir.hdr.name);
   }
   if (res->res_dir.hdr.desc) {
      free(res->res_dir.hdr.desc);
   }
   switch (type) {
   case R_DIRECTOR:
      if (res->res_dir.dirinfo.password) {
         free(res->res_dir.dirinfo.password);
      }
      if (res->res_dir.dirinfo.address) {
         free(res->res_dir.dirinfo.address);
      }
      if (res->res_dir.dirinfo.tls_ctx) {
         free_tls_context(res->res_dir.dirinfo.tls_ctx);
      }
      if (res->res_dir.dirinfo.psk_ctx) {
         free_psk_context(res->res_dir.dirinfo.psk_ctx);
      }
      if (res->res_dir.dirinfo.tls_ca_certfile) {
         free(res->res_dir.dirinfo.tls_ca_certfile);
      }
      if (res->res_dir.dirinfo.tls_ca_certdir) {
         free(res->res_dir.dirinfo.tls_ca_certdir);
      }
      if (res->res_dir.dirinfo.tls_certfile) {
         free(res->res_dir.dirinfo.tls_certfile);
      }
      if (res->res_dir.dirinfo.tls_keyfile) {
         free(res->res_dir.dirinfo.tls_keyfile);
      }
      if (res->res_dir.dirinfo.tls_dhfile) {
         free(res->res_dir.dirinfo.tls_dhfile);
      }
      if (res->res_dir.dirinfo.tls_allowed_cns) {
         delete res->res_dir.dirinfo.tls_allowed_cns;
      }
      if (res->res_dir.disable_cmds) {
         delete res->res_dir.disable_cmds;
      }
      if (res->res_dir.disabled_cmds_array) {
         free(res->res_dir.disabled_cmds_array);
      }
      break;
   case R_CONSOLE:
      if (res->res_cons.dirinfo.password) {
         free(res->res_cons.dirinfo.password);
      }
      if (res->res_cons.dirinfo.address) {
         free(res->res_cons.dirinfo.address);
      }
      if (res->res_cons.dirinfo.tls_ctx) {
         free_tls_context(res->res_cons.dirinfo.tls_ctx);
      }
      if (res->res_cons.dirinfo.psk_ctx) {
         free_psk_context(res->res_cons.dirinfo.psk_ctx);
      }
      if (res->res_cons.dirinfo.tls_ca_certfile) {
         free(res->res_cons.dirinfo.tls_ca_certfile);
      }
      if (res->res_cons.dirinfo.tls_ca_certdir) {
         free(res->res_cons.dirinfo.tls_ca_certdir);
      }
      if (res->res_cons.dirinfo.tls_certfile) {
         free(res->res_cons.dirinfo.tls_certfile);
      }
      if (res->res_cons.dirinfo.tls_keyfile) {
         free(res->res_cons.dirinfo.tls_keyfile);
      }
      if (res->res_cons.dirinfo.tls_dhfile) {
         free(res->res_cons.dirinfo.tls_dhfile);
      }
      if (res->res_cons.dirinfo.tls_allowed_cns) {
         delete res->res_cons.dirinfo.tls_allowed_cns;
      }
      break;
    case R_CLIENT:
      if (res->res_client.working_directory) {
         free(res->res_client.working_directory);
      }
      if (res->res_client.pid_directory) {
         free(res->res_client.pid_directory);
      }
      if (res->res_client.subsys_directory) {
         free(res->res_client.subsys_directory);
      }
      if (res->res_client.scripts_directory) {
         free(res->res_client.scripts_directory);
      }
      if (res->res_client.plugin_directory) {
         free(res->res_client.plugin_directory);
      }
      if (res->res_client.dedup_index_dir) {
         free(res->res_client.dedup_index_dir);
      }
      if (res->res_client.FDaddrs) {
         free_addresses(res->res_client.FDaddrs);
      }
      if (res->res_client.FDsrc_addr) {
         free_addresses(res->res_client.FDsrc_addr);
      }
      if (res->res_client.snapshot_command) {
         free(res->res_client.snapshot_command);
      }
      if (res->res_client.pki_keypair_file) {
         free(res->res_client.pki_keypair_file);
      }
      if (res->res_client.pki_keypair) {
         crypto_keypair_free(res->res_client.pki_keypair);
      }

      if (res->res_client.pki_signing_key_files) {
         delete res->res_client.pki_signing_key_files;
      }
      if (res->res_client.pki_signers) {
         X509_KEYPAIR *keypair;
         foreach_alist(keypair, res->res_client.pki_signers) {
            crypto_keypair_free(keypair);
         }
         delete res->res_client.pki_signers;
      }

      if (res->res_client.pki_master_key_files) {
         delete res->res_client.pki_master_key_files;
      }

      if (res->res_client.pki_recipients) {
         X509_KEYPAIR *keypair;
         foreach_alist(keypair, res->res_client.pki_recipients) {
            crypto_keypair_free(keypair);
         }
         delete res->res_client.pki_recipients;
      }

      if (res->res_client.tls_ctx) {
         free_tls_context(res->res_client.tls_ctx);
      }
      if (res->res_client.psk_ctx) {
         free_psk_context(res->res_client.psk_ctx);
      }
      if (res->res_client.tls_ca_certfile) {
         free(res->res_client.tls_ca_certfile);
      }
      if (res->res_client.tls_ca_certdir) {
         free(res->res_client.tls_ca_certdir);
      }
      if (res->res_client.tls_certfile) {
         free(res->res_client.tls_certfile);
      }
      if (res->res_client.tls_keyfile) {
         free(res->res_client.tls_keyfile);
      }
      if (res->res_client.disable_cmds) {
         delete res->res_client.disable_cmds;
      }
      if (res->res_client.disabled_cmds_array) {
         free(res->res_client.disabled_cmds_array);
      }
      if (res->res_client.verid) {
         free(res->res_client.verid);
      }
      break;
   case R_MSGS:
      if (res->res_msgs.mail_cmd) {
         free(res->res_msgs.mail_cmd);
      }
      if (res->res_msgs.operator_cmd) {
         free(res->res_msgs.operator_cmd);
      }
      free_msgs_res((MSGS *)res);  /* free message resource */
      res = NULL;
      break;
   case R_COLLECTOR:
      free_collector_resource(res->res_collector);
      break;
   case R_SCHEDULE:
      if (res->res_sched.run) {
         RUNRES *nrun, *next;
         nrun = res->res_sched.run;
         while (nrun) {
            next = nrun->next;
            free(nrun);
            nrun = next;
         }
      }
      break;
   default:
      printf(_("Unknown resource type %d\n"), type);
   }
   /* Common stuff again -- free the resource, recurse to next one */
   if (res) {
      free(res);
   }
}

/* Get the size of a resource object */
int get_resource_size(int type)
{
   int size = -1;
   switch (type) {
      case R_DIRECTOR:
         size = sizeof(DIRRES);
         break;
      case R_CONSOLE:
         size = sizeof(CONSRES);
         break;
      case R_CLIENT:
         size = sizeof(CLIENT);
         break;
      case R_MSGS:
         size = sizeof(MSGS);
         break;
      case R_COLLECTOR:
         size = sizeof(COLLECTOR);
         break;
      case R_SCHEDULE:
         size = sizeof(SCHEDRES);
         break;
      default:
         break;
   }
   return size;
}

/* Save the new resource by chaining it into the head list for
 * the resource. If this is pass 2, we update any resource
 * pointers (currently only in the Job resource).
 */
bool save_resource(CONFIG *config, int type, RES_ITEM *items, int pass)
{
   URES *res;
   int rindex = type - r_first;
   int i, size;
   int error = 0;

   /*
    * Ensure that all required items are present
    */
   for (i=0; items[i].name; i++) {
      if (items[i].flags & ITEM_REQUIRED) {
         if (!bit_is_set(i, res_all.res_dir.hdr.item_present)) {
            Mmsg(config->m_errmsg, _("\"%s\" directive is required in \"%s\" resource, but not found.\n"),
                 items[i].name, resources[rindex].name);
            return false;
         }
      }
   }

   /* During pass 2, we looked up pointers to all the resources
    * referrenced in the current resource, , now we
    * must copy their address from the static record to the allocated
    * record.
    */
   if (pass == 2) {
      switch (type) {
         /* Resources not containing a resource */
         case R_MSGS:
            break;
         case R_SCHEDULE:
            /*
             * Schedule is a bit different in that it contains a RUN record
             * chain which isn't a "named" resource. This chain was linked
             * in by run_conf.c during pass 2, so here we jam the pointer
             * into the Schedule resource.
             */
            if ((res = (URES *)GetResWithName(R_SCHEDULE, res_all.res_sched.name())) == NULL) {
               Mmsg(config->m_errmsg, _("Cannot find Schedule resource %s\n"), res_all.res_sched.name());
               return false;
            }
            res->res_sched.run = res_all.res_sched.run;
            break;

         /* Resources containing another resource */
         case R_DIRECTOR:
            if ((res = (URES *)GetResWithName(R_DIRECTOR, res_all.res_dir.hdr.name)) == NULL) {
               Mmsg(config->m_errmsg, _("Cannot find Director resource %s\n"), res_all.res_dir.hdr.name);
               return false;
            }
            res->res_dir.dirinfo.tls_allowed_cns = res_all.res_dir.dirinfo.tls_allowed_cns;
            res->res_dir.disable_cmds = res_all.res_dir.disable_cmds;
            res->res_dir.console = res_all.res_dir.console;
            res->res_dir.schedule = res_all.res_dir.schedule;
            break;
         /* Resources containing another resource */
         case R_CONSOLE:
            break;
         case R_CLIENT:
            if ((res = (URES *)GetResWithName(R_CLIENT, res_all.res_dir.hdr.name)) == NULL) {
               Mmsg(config->m_errmsg, _("Cannot find Client resource %s\n"), res_all.res_dir.hdr.name);
               return false;
            }
            res->res_client.pki_signing_key_files = res_all.res_client.pki_signing_key_files;
            res->res_client.pki_master_key_files = res_all.res_client.pki_master_key_files;

            res->res_client.pki_signers = res_all.res_client.pki_signers;
            res->res_client.pki_recipients = res_all.res_client.pki_recipients;

            res->res_client.messages = res_all.res_client.messages;
            res->res_client.disable_cmds = res_all.res_client.disable_cmds;
            break;
         case R_COLLECTOR:
            if ((res = (URES *)GetResWithName(R_COLLECTOR, res_all.res_collector.hdr.name)) == NULL) {
               Mmsg(config->m_errmsg, _("Cannot find Statistics resource %s\n"), res_all.res_collector.hdr.name);
               return false;
            }
            res->res_collector.metrics = res_all.res_collector.metrics;
            // Dmsg2(100, "metrics = 0x%p 0x%p\n", res->res_collector.metrics, res_all.res_collector.metrics);
            break;
         default:
            Emsg1(M_ERROR, 0, _("Unknown resource type %d\n"), type);
            error = 1;
            break;
      }
      /* Note, the resource name was already saved during pass 1,
       * so here, we can just release it.
       */
      if (res_all.res_dir.hdr.name) {
         free(res_all.res_dir.hdr.name);
         res_all.res_dir.hdr.name = NULL;
      }
      if (res_all.res_dir.hdr.desc) {
         free(res_all.res_dir.hdr.desc);
         res_all.res_dir.hdr.desc = NULL;
      }
      return true;
   }

   /* The following code is only executed on pass 1 */
   size = get_resource_size(type);

   if (size < 0) {
      printf(_("Unknown resource type %d\n"), type);
      error = 1;
   }

   /* Common */
   if (!error) {
      if (!config->insert_res(rindex, size)) {
         return false;
      }
   }
   return true;
}

bool parse_fd_config(CONFIG *config, const char *configfile, int exit_code)
{
   config->init(configfile, NULL, exit_code, (void *)&res_all, res_all_size,
      r_first, r_last, resources, &res_head);
   return config->parse_config();
}
