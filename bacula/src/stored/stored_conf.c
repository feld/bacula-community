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
 * Configuration file parser for Bacula Storage daemon
 *
 *     Kern Sibbald, March MM
 */

#include "bacula.h"
#include "stored.h"
#include "lib/status-pkt.h"

/* First and last resource ids */
int32_t r_first = R_FIRST;
int32_t r_last  = R_LAST;
RES_HEAD **res_head;

/* We build the current resource here statically,
 * then move it to dynamic memory */
#if defined(_MSC_VER)
extern "C" { // work around visual compiler mangling variables
    URES res_all;
}
#else
URES res_all;
#endif
int32_t res_all_size = sizeof(res_all);

#ifdef SD_DEDUP_SUPPORT
# include "bee_libsd_dedup.h"
#else
#define dedup_check_storage_resource(a) true
#endif

static int const dbglvl = 200;

/* Definition of records permitted within each
 * resource with the routine to process the record
 * information.
 */

/*
 * Globals for the Storage daemon.
 *  name         handler      value       code   flags  default_value
 */
static RES_ITEM store_items[] = {
   {"Name",                  store_name, ITEM(res_store.hdr.name),   0, ITEM_REQUIRED, 0},
   {"Description",           store_str,  ITEM(res_dir.hdr.desc),     0, 0, 0},
   {"SdAddress",             store_addresses_address,  ITEM(res_store.sdaddrs),     0, ITEM_DEFAULT, 9103},
   {"SdAddresses",           store_addresses,  ITEM(res_store.sdaddrs), 0, ITEM_DEFAULT, 9103},
   {"Messages",              store_res,  ITEM(res_store.messages),   R_MSGS, 0, 0},
   {"SdPort",                store_addresses_port,  ITEM(res_store.sdaddrs),     0, ITEM_DEFAULT, 9103},
   {"WorkingDirectory",      store_dir,  ITEM(res_store.working_directory), 0, ITEM_REQUIRED, 0},
   {"PidDirectory",          store_dir,  ITEM(res_store.pid_directory), 0, ITEM_REQUIRED, 0},
   {"SubsysDirectory",       store_dir,  ITEM(res_store.subsys_directory), 0, 0, 0},
   {"PluginDirectory",       store_dir,  ITEM(res_store.plugin_directory), 0, 0, 0},
   {"ScriptsDirectory",      store_dir,  ITEM(res_store.scripts_directory), 0, 0, 0},
#if BEEF
   {"SsdDirectory",          store_dir,  ITEM(res_store.ssd_directory), 0, 0, 0},
#endif
   {"MaximumConcurrentJobs", store_pint32, ITEM(res_store.max_concurrent_jobs), 0, ITEM_DEFAULT, 20},
   {"ClientConnectTimeout",  store_time, ITEM(res_store.ClientConnectTimeout), 0, ITEM_DEFAULT, 60 * 30},
   {"HeartbeatInterval",     store_time, ITEM(res_store.heartbeat_interval), 0, ITEM_DEFAULT, 5 * 60},
#if BEEF
   {"FipsRequire",            store_bool, ITEM(res_store.require_fips), 0, 0, 0},
#endif
   {"TlsAuthenticate",       store_bool,    ITEM(res_store.tls_authenticate), 0, 0, 0},
   {"TlsEnable",             store_bool,    ITEM(res_store.tls_enable), 0, 0, 0},
   {"TlsPskEnable",          store_bool,    ITEM(res_store.tls_psk_enable), 0, ITEM_DEFAULT, tls_psk_default},
   {"TlsRequire",            store_bool,    ITEM(res_store.tls_require), 0, 0, 0},
   {"TlsVerifyPeer",         store_bool,    ITEM(res_store.tls_verify_peer), 1, ITEM_DEFAULT, 1},
   {"TlsCaCertificateFile",  store_dir,       ITEM(res_store.tls_ca_certfile), 0, 0, 0},
   {"TlsCaCertificateDir",   store_dir,       ITEM(res_store.tls_ca_certdir), 0, 0, 0},
   {"TlsCertificate",        store_dir,       ITEM(res_store.tls_certfile), 0, 0, 0},
   {"TlsKey",                store_dir,       ITEM(res_store.tls_keyfile), 0, 0, 0},
   {"TlsDhFile",             store_dir,       ITEM(res_store.tls_dhfile), 0, 0, 0},
   {"TlsAllowedCn",          store_alist_str, ITEM(res_store.tls_allowed_cns), 0, 0, 0},
   {"ClientConnectWait",     store_time,  ITEM(res_store.client_wait), 0, ITEM_DEFAULT, 30 * 60},
   {"VerId",                 store_str,   ITEM(res_store.verid), 0, 0, 0},
   {"CommCompression",       store_bool,  ITEM(res_store.comm_compression), 0, ITEM_DEFAULT, true},
#ifdef SD_DEDUP_SUPPORT
   {"DedupDirectory",        store_dir,   ITEM(res_store.dedup_dir),  0, 0, 0},
   {"DedupIndexDirectory",   store_dir,   ITEM(res_store.dedup_index_dir),  0, 0, 0},
   {"DedupCheckHash",        store_bool,   ITEM(res_store.dedup_check_hash),  0, 0, 0},
   {"DedupScrubMaximumBandwidth", store_speed, ITEM(res_store.dedup_scrub_max_bandwidth), 0, ITEM_DEFAULT, 15*1024*1024},
   {"MaximumContainerSize",  store_size64, ITEM(res_store.max_container_size), 0, 0, 0},
#endif
   {NULL, NULL, {0}, 0, 0, 0}
};


/* Directors that can speak to the Storage daemon */
static RES_ITEM dir_items[] = {
   {"Name",        store_name,     ITEM(res_dir.hdr.name),   0, ITEM_REQUIRED, 0},
   {"Description", store_str,      ITEM(res_dir.hdr.desc),   0, 0, 0},
   {"Password",    store_password, ITEM(res_dir.password),   0, ITEM_REQUIRED, 0},
   {"Monitor",     store_bool,     ITEM(res_dir.monitor),    0, 0, 0},
   {"TlsAuthenticate",      store_bool,    ITEM(res_dir.tls_authenticate), 0, 0, 0},
   {"TlsEnable",            store_bool,    ITEM(res_dir.tls_enable), 0, 0, 0},
   {"TlsPskEnable",         store_bool,    ITEM(res_dir.tls_psk_enable), 0, ITEM_DEFAULT, tls_psk_default},
   {"TlsRequire",           store_bool,    ITEM(res_dir.tls_require), 0, 0, 0},
   {"TlsVerifyPeer",        store_bool,    ITEM(res_dir.tls_verify_peer), 1, ITEM_DEFAULT, 1},
   {"TlsCaCertificateFile", store_dir,       ITEM(res_dir.tls_ca_certfile), 0, 0, 0},
   {"TlsCaCertificateDir",  store_dir,       ITEM(res_dir.tls_ca_certdir), 0, 0, 0},
   {"TlsCertificate",       store_dir,       ITEM(res_dir.tls_certfile), 0, 0, 0},
   {"TlsKey",               store_dir,       ITEM(res_dir.tls_keyfile), 0, 0, 0},
   {"TlsDhFile",            store_dir,       ITEM(res_dir.tls_dhfile), 0, 0, 0},
   {"TlsAllowedCn",         store_alist_str, ITEM(res_dir.tls_allowed_cns), 0, 0, 0},
   {NULL, NULL, {0}, 0, 0, 0}
};

/* Device definition */
static RES_ITEM dev_items[] = {
   {"Name",                  store_name,   ITEM(res_dev.hdr.name),    0, ITEM_REQUIRED, 0},
   {"Description",           store_str,    ITEM(res_dir.hdr.desc),    0, 0, 0},
   {"MediaType",             store_strname,ITEM(res_dev.media_type),  0, ITEM_REQUIRED, 0},
   {"DeviceType",            store_devtype,ITEM(res_dev.dev_type),    0, 0, 0},
   {"ArchiveDevice",         store_strname,ITEM(res_dev.device_name), 0, ITEM_REQUIRED, 0},
   {"AlignedDevice",         store_strname,ITEM(res_dev.adevice_name), 0, 0, 0},
   {"HardwareEndOfFile",     store_bit,  ITEM(res_dev.cap_bits), CAP_EOF,  ITEM_DEFAULT, 1},
   {"HardwareEndOfMedium",   store_bit,  ITEM(res_dev.cap_bits), CAP_EOM,  ITEM_DEFAULT, 1},
   {"BackwardSpaceRecord",   store_bit,  ITEM(res_dev.cap_bits), CAP_BSR,  ITEM_DEFAULT, 1},
   {"BackwardSpaceFile",     store_bit,  ITEM(res_dev.cap_bits), CAP_BSF,  ITEM_DEFAULT, 1},
   {"BsfAtEom",              store_bit,  ITEM(res_dev.cap_bits), CAP_BSFATEOM, ITEM_DEFAULT, 0},
   {"TwoEof",                store_bit,  ITEM(res_dev.cap_bits), CAP_TWOEOF, ITEM_DEFAULT, 0},
   {"ForwardSpaceRecord",    store_bit,  ITEM(res_dev.cap_bits), CAP_FSR,  ITEM_DEFAULT, 1},
   {"ForwardSpaceFile",      store_bit,  ITEM(res_dev.cap_bits), CAP_FSF,  ITEM_DEFAULT, 1},
   {"FastForwardSpaceFile",  store_bit,  ITEM(res_dev.cap_bits), CAP_FASTFSF, ITEM_DEFAULT, 1},
   {"RemovableMedia",        store_bit,  ITEM(res_dev.cap_bits), CAP_REM,  ITEM_DEFAULT, 1},
   {"RandomAccess",          store_bit,  ITEM(res_dev.cap_bits), CAP_RACCESS, 0, 0},
   {"AutomaticMount",        store_bit,  ITEM(res_dev.cap_bits), CAP_AUTOMOUNT,  ITEM_DEFAULT, 0},
   {"LabelMedia",            store_bit,  ITEM(res_dev.cap_bits), CAP_LABEL,      ITEM_DEFAULT, 0},
   {"AlwaysOpen",            store_bit,  ITEM(res_dev.cap_bits), CAP_ALWAYSOPEN, ITEM_DEFAULT, 1},
   {"Autochanger",           store_bit,  ITEM(res_dev.cap_bits), CAP_AUTOCHANGER, ITEM_DEFAULT, 0},
   {"CloseOnPoll",           store_bit,  ITEM(res_dev.cap_bits), CAP_CLOSEONPOLL, ITEM_DEFAULT, 0},
   {"BlockPositioning",      store_bit,  ITEM(res_dev.cap_bits), CAP_POSITIONBLOCKS, ITEM_DEFAULT, 1},
   {"UseMtiocGet",           store_bit,  ITEM(res_dev.cap_bits), CAP_MTIOCGET, ITEM_DEFAULT, 1},
   {"CheckLabels",           store_bit,  ITEM(res_dev.cap_bits), CAP_CHECKLABELS, ITEM_DEFAULT, 0},
   {"RequiresMount",         store_bit,  ITEM(res_dev.cap_bits), CAP_REQMOUNT, ITEM_DEFAULT, 0},
   {"OfflineOnUnmount",      store_bit,  ITEM(res_dev.cap_bits), CAP_OFFLINEUNMOUNT, ITEM_DEFAULT, 0},
   {"BlockChecksum",         store_bit,  ITEM(res_dev.cap_bits), CAP_BLOCKCHECKSUM, ITEM_DEFAULT, 1},
   {"UseLintape",            store_bit,  ITEM(res_dev.cap_bits), CAP_LINTAPE, ITEM_DEFAULT, 0},
   {"Enabled",               store_bool, ITEM(res_dev.enabled), 0, ITEM_DEFAULT, 1},
   {"AutoSelect",            store_bool, ITEM(res_dev.autoselect), 0, ITEM_DEFAULT, 1},
   {"ReadOnly",              store_bool, ITEM(res_dev.read_only), 0, ITEM_DEFAULT, 0},
   {"ChangerDevice",         store_strname,ITEM(res_dev.changer_name), 0, 0, 0},
   {"ControlDevice",         store_strname,ITEM(res_dev.control_name), 0, 0, 0},
   {"ChangerCommand",        store_strname,ITEM(res_dev.changer_command), 0, 0, 0},
   {"AlertCommand",          store_strname,ITEM(res_dev.alert_command), 0, 0, 0},
   {"LockCommand",           store_strname,ITEM(res_dev.lock_command), 0, 0, 0},
   {"WormCommand",           store_strname,ITEM(res_dev.worm_command), 0, 0, 0},
   {"MaximumChangerWait",    store_time,   ITEM(res_dev.max_changer_wait), 0, ITEM_DEFAULT, 5 * 60},
   {"MaximumOpenWait",       store_time,   ITEM(res_dev.max_open_wait), 0, ITEM_DEFAULT, 5 * 60},
   {"MaximumNetworkBufferSize", store_pint32, ITEM(res_dev.max_network_buffer_size), 0, 0, 0},
   {"VolumePollInterval",    store_time,   ITEM(res_dev.vol_poll_interval), 0, ITEM_DEFAULT, 5 * 60},
   {"MaximumRewindWait",     store_time,   ITEM(res_dev.max_rewind_wait), 0, ITEM_DEFAULT, 5 * 60},
   {"MinimumBlockSize",      store_size32, ITEM(res_dev.min_block_size), 0, 0, 0},
   {"MaximumBlockSize",      store_maxblocksize, ITEM(res_dev.max_block_size), 0, 0, 0},
   {"PaddingSize",           store_size32, ITEM(res_dev.padding_size), 0, ITEM_DEFAULT, 4096},
   {"FileAlignment",         store_size32, ITEM(res_dev.file_alignment), 0, ITEM_DEFAULT, 4096},
   {"MinimumAlignedSize",    store_size32, ITEM(res_dev.min_aligned_size), 0, ITEM_DEFAULT, 4096},
   {"MaximumVolumeSize",     store_size64, ITEM(res_dev.max_volume_size), 0, 0, 0},
   {"MaximumFileSize",       store_size64, ITEM(res_dev.max_file_size), 0, ITEM_DEFAULT, 1000000000},
   {"MaximumFileIndex",      store_size64, ITEM(res_dev.max_file_index), 0, ITEM_DEFAULT, 100000000}, /* ***BEEF*** */
   {"VolumeCapacity",        store_size64, ITEM(res_dev.volume_capacity), 0, 0, 0},
   {"MinimumFeeSpace",       store_size64, ITEM(res_dev.min_free_space), 0, ITEM_DEFAULT, 5000000},
   {"MaximumConcurrentJobs", store_pint32, ITEM(res_dev.max_concurrent_jobs), 0, 0, 0},
   {"SpoolDirectory",        store_dir,    ITEM(res_dev.spool_directory), 0, 0, 0},
   {"MaximumSpoolSize",      store_size64, ITEM(res_dev.max_spool_size), 0, 0, 0},
   {"MaximumJobSpoolSize",   store_size64, ITEM(res_dev.max_job_spool_size), 0, 0, 0},
   {"DriveIndex",            store_pint32, ITEM(res_dev.drive_index), 0, 0, 0},
   {"MaximumPartSize",       store_size64, ITEM(res_dev.max_part_size), 0, ITEM_DEFAULT, 0},
   {"MountPoint",            store_strname,ITEM(res_dev.mount_point), 0, 0, 0},
   {"MountCommand",          store_strname,ITEM(res_dev.mount_command), 0, 0, 0},
   {"UnmountCommand",        store_strname,ITEM(res_dev.unmount_command), 0, 0, 0},
   {"WritePartCommand",      store_strname,ITEM(res_dev.write_part_command), 0, 0, 0},
   {"FreeSpaceCommand",      store_strname,ITEM(res_dev.free_space_command), 0, 0, 0},
   {"LabelType",             store_label,  ITEM(res_dev.label_type), 0, 0, 0},
   {"Cloud",                 store_res,    ITEM(res_dev.cloud), R_CLOUD, 0, 0},
#ifdef SD_DEDUP_SUPPORT
   {"Dedupengine",           store_res,    ITEM(res_dev.dedup), R_DEDUP, 0, 0},
#endif
   {"SyncOnClose",           store_bit,    ITEM(res_dev.cap_bits), CAP_SYNCONCLOSE, ITEM_DEFAULT, 0},
   {NULL, NULL, {0}, 0, 0, 0}
};

/* Autochanger definition */
static RES_ITEM changer_items[] = {
   {"Name",              store_name,      ITEM(res_changer.hdr.name),        0, ITEM_REQUIRED, 0},
   {"Description",       store_str,       ITEM(res_changer.hdr.desc),        0, 0, 0},
   {"Device",            store_alist_res, ITEM(res_changer.device),   R_DEVICE, ITEM_REQUIRED, 0},
   {"ChangerDevice",     store_strname,   ITEM(res_changer.changer_name),    0, ITEM_REQUIRED, 0},
   {"ChangerCommand",    store_strname,   ITEM(res_changer.changer_command), 0, ITEM_REQUIRED, 0},
   {"LockCommand",           store_strname,ITEM(res_changer.lock_command), 0, 0, 0},
   {NULL, NULL, {0}, 0, 0, 0}
};

/* Cloud driver definition */
static RES_ITEM cloud_items[] = {
   {"Name",              store_name,      ITEM(res_cloud.hdr.name),        0, ITEM_REQUIRED, 0},
   {"Description",       store_str,       ITEM(res_cloud.hdr.desc),        0, 0, 0},
   {"Driver",            store_cloud_driver, ITEM(res_cloud.driver_type), 0, ITEM_REQUIRED, 0},
   {"HostName",          store_strname,ITEM(res_cloud.host_name), 0, 0, ITEM_REQUIRED}, // Only S3/File
   {"BucketName",        store_strname,ITEM(res_cloud.bucket_name), 0, ITEM_REQUIRED, 0},
   {"Region",            store_strname,ITEM(res_cloud.region), 0, 0, 0},
   {"AccessKey",         store_strname,ITEM(res_cloud.access_key), 0, 0, 0},
   {"SecretKey",         store_strname,ITEM(res_cloud.secret_key), 0, 0, 0},
   {"Protocol",          store_protocol,ITEM(res_cloud.protocol), 0, ITEM_DEFAULT, 0},   /* HTTPS */
   {"BlobEndpoint",      store_strname,ITEM(res_cloud.blob_endpoint), 0, 0, 0},
   {"FileEndpoint",      store_strname,ITEM(res_cloud.file_endpoint), 0, 0, 0},
   {"QueueEndpoint",     store_strname,ITEM(res_cloud.queue_endpoint), 0, 0, 0},
   {"TableEndpoint",     store_strname,ITEM(res_cloud.table_endpoint), 0, 0, 0},
   {"EndpointSuffix",    store_strname,ITEM(res_cloud.endpoint_suffix), 0, 0, 0},
   {"UriStyle",          store_uri_style, ITEM(res_cloud.uri_style), 0, ITEM_DEFAULT, 0}, /* VirtualHost */
   {"TruncateCache",     store_truncate, ITEM(res_cloud.trunc_opt), 0, ITEM_DEFAULT, TRUNC_NO},
   {"Upload",            store_upload,   ITEM(res_cloud.upload_opt), 0, ITEM_DEFAULT, UPLOAD_NO},
   {"MaximumConcurrentUploads", store_pint32, ITEM(res_cloud.max_concurrent_uploads), 0, ITEM_DEFAULT, 3},
   {"MaximumConcurrentDownloads", store_pint32, ITEM(res_cloud.max_concurrent_downloads), 0, ITEM_DEFAULT, 3},
   {"MaximumUploadBandwidth", store_speed, ITEM(res_cloud.upload_limit), 0, 0, 0},
   {"MaximumDownloadBandwidth", store_speed, ITEM(res_cloud.download_limit), 0, 0, 0},
   {"DriverCommand",     store_strname, ITEM(res_cloud.driver_command), 0, 0, 0},
   {"TransferPriority",  store_transfer_priority, ITEM(res_cloud.transfer_priority), 0, ITEM_DEFAULT, 0},
   {"TransferRetention", store_time, ITEM(res_cloud.transfer_retention), 0, ITEM_DEFAULT, 5 * 3600 * 24},
   {NULL, NULL, {0}, 0, 0, 0}
};

/* Message resource */
extern RES_ITEM msgs_items[];

/* Statistics resource */
extern RES_ITEM collector_items[];


/* This is the master resource definition */
/* ATTN the order of the items must match the R_FIRST->R_LAST list in stored_conf.h !!! */
RES_TABLE resources[] = {
   {"Director",      dir_items,        R_DIRECTOR},
   {"Storage",       store_items,      R_STORAGE},
   {"Device",        dev_items,        R_DEVICE},
   {"Messages",      msgs_items,       R_MSGS},
   {"Autochanger",   changer_items,    R_AUTOCHANGER},
   {"Cloud",         cloud_items,      R_CLOUD},
   {"Statistics",    collector_items,  R_COLLECTOR},
#ifdef SD_DEDUP_SUPPORT
   {"Dedupengine",   dedup_items,      R_DEDUP},
#endif
   {NULL,            NULL,             0}
};

/*
 * Device types
 *
 *   device type     device code = token
 */
s_kw dev_types[] = {
   {"File",          B_FILE_DEV},
   {"Tape",          B_TAPE_DEV},
   {"Fifo",          B_FIFO_DEV},
   {"VTape",         B_VTAPE_DEV},
   {"Vtl",           B_VTL_DEV},
   {"Aligned",       B_ALIGNED_DEV},
#ifdef SD_DEDUP_SUPPORT
   {"Dedup",         B_DEDUP_DEV},
#endif
   {"Null",          B_NULL_DEV},
   {"Cloud",         B_CLOUD_DEV},
   {NULL,            0}
};


/*
 * Store Device Type (File, FIFO, Tape, Cloud, ...)
 *
 */
void store_devtype(LEX *lc, RES_ITEM *item, int index, int pass)
{
   bool found = false;

   lex_get_token(lc, T_NAME);
   /* Store the label pass 2 so that type is defined */
   for (int i=0; dev_types[i].name; i++) {
      if (strcasecmp(lc->str, dev_types[i].name) == 0) {
         *(uint32_t *)(item->value) = dev_types[i].token;
         found = true;
         break;
      }
   }
   if (!found) {
      scan_err1(lc, _("Expected a Device Type keyword, got: %s"), lc->str);
   }
   scan_to_eol(lc);
   set_bit(index, res_all.hdr.item_present);
}

/*
 * Cloud drivers
 *
 *  driver     driver code
 */
s_kw cloud_drivers[] = {
   {"S3",           C_S3_DRIVER},
   {"File",         C_FILE_DRIVER},
#if BEEF
   {"Azure",        C_WAS_DRIVER},
   {"Google",       C_GOOGLE_DRIVER},
   {"Oracle",       C_ORACLE_DRIVER},
   {"Generic",      C_GEN_DRIVER},
   {"Swift",        C_SWIFT_DRIVER},
#endif
   {NULL,           0}
};

/*
 * Store Device Type (File, FIFO, Tape, Cloud, ...)
 *
 */
void store_cloud_driver(LEX *lc, RES_ITEM *item, int index, int pass)
{
   bool found = false;

   lex_get_token(lc, T_NAME);
   /* Store the label pass 2 so that type is defined */
   for (int i=0; cloud_drivers[i].name; i++) {
      if (strcasecmp(lc->str, cloud_drivers[i].name) == 0) {
         *(uint32_t *)(item->value) = cloud_drivers[i].token;
         found = true;
         break;
      }
   }
   if (!found) {
      scan_err1(lc, _("Expected a Cloud driver keyword, got: %s"), lc->str);
   }
   scan_to_eol(lc);
   set_bit(index, res_all.hdr.item_present);
}

/*
 * Cloud Truncate cache options
 *
 *   Option       option code = token
 */
s_kw trunc_opts[] = {
   {"No",           TRUNC_NO},
   {"AfterUpload",  TRUNC_AFTER_UPLOAD},
   {"AtEndOfJob",   TRUNC_AT_ENDOFJOB},
   {"ConfDefault",  TRUNC_CONF_DEFAULT}, /* only use as a parameter */
   {NULL,            0}
};

/*
 * Store Cloud Truncate cache option (AfterUpload, AtEndOfJob, No)
 *
 */
void store_truncate(LEX *lc, RES_ITEM *item, int index, int pass)
{
   bool found = false;

   lex_get_token(lc, T_NAME);
   /* Store the label pass 2 so that type is defined */
   for (int i=0; trunc_opts[i].name; i++) {
      if (strcasecmp(lc->str, trunc_opts[i].name) == 0) {
         *(uint32_t *)(item->value) = trunc_opts[i].token;
         found = true;
         break;
      }
   }
   if (!found) {
      scan_err1(lc, _("Expected a Truncate Cache option keyword, got: %s"), lc->str);
   }
   scan_to_eol(lc);
   set_bit(index, res_all.hdr.item_present);
}

/*
 * convert string to truncate option
 * Return true if option matches one of trunc_opts (case insensitive)
 *
 */
bool find_truncate_option(const char* truncate, uint32_t& truncate_option)
{
   for (int i=0; trunc_opts[i].name; i++) {
      if (strcasecmp(truncate, trunc_opts[i].name) == 0) {
         truncate_option = trunc_opts[i].token;
         return true;
      }
   }
   return false;
}

/*
 * Cloud Upload options
 *
 *   Option         option code = token
 */
s_kw upload_opts[] = {
   {"No",        UPLOAD_NO},
   {"Manual",        UPLOAD_NO}, /*identical and preferable to No, No is kept for backward compatibility */
   {"EachPart",      UPLOAD_EACHPART},
   {"AtEndOfJob",    UPLOAD_AT_ENDOFJOB},
   {NULL,            0}
};

/*
 * Store Cloud Upload option (EachPart, AtEndOfJob, No)
 *
 */
void store_upload(LEX *lc, RES_ITEM *item, int index, int pass)
{
   bool found = false;

   lex_get_token(lc, T_NAME);
   /* Store the label pass 2 so that type is defined */
   for (int i=0; upload_opts[i].name; i++) {
      if (strcasecmp(lc->str, upload_opts[i].name) == 0) {
         *(uint32_t *)(item->value) = upload_opts[i].token;
         found = true;
         break;
      }
   }
   if (!found) {
      scan_err1(lc, _("Expected a Cloud Upload option keyword, got: %s"), lc->str);
   }
   scan_to_eol(lc);
   set_bit(index, res_all.hdr.item_present);
}

/*
 * Cloud connection protocol  options
 *
 *   Option       option code = token
 */
s_kw proto_opts[] = {
   {"HTTPS",        0},
   {"HTTP",         1},
   {NULL,            0}
};

/*
 * Store Cloud connect protocol option (HTTPS, HTTP)
 *
 */
void store_protocol(LEX *lc, RES_ITEM *item, int index, int pass)
{
   bool found = false;

   lex_get_token(lc, T_NAME);
   /* Store the label pass 2 so that type is defined */
   for (int i=0; proto_opts[i].name; i++) {
      if (strcasecmp(lc->str, proto_opts[i].name) == 0) {
         *(uint32_t *)(item->value) = proto_opts[i].token;
         found = true;
         break;
      }
   }
   if (!found) {
      scan_err1(lc, _("Expected a Cloud communications protocol option keyword, got: %s"), lc->str);
   }
   scan_to_eol(lc);
   set_bit(index, res_all.hdr.item_present);
}

/*
 * Cloud Uri Style options
 *
 *   Option       option code = token
 */
s_kw uri_opts[] = {
   {"VirtualHost",  0},
   {"Path",         1},
   {NULL,            0}
};

/*
 * Store Cloud Uri Style option
 *
 */
void store_uri_style(LEX *lc, RES_ITEM *item, int index, int pass)
{
   bool found = false;

   lex_get_token(lc, T_NAME);
   /* Store the label pass 2 so that type is defined */
   for (int i=0; uri_opts[i].name; i++) {
      if (strcasecmp(lc->str, uri_opts[i].name) == 0) {
         *(uint32_t *)(item->value) = uri_opts[i].token;
         found = true;
         break;
      }
   }
   if (!found) {
      scan_err1(lc, _("Expected a Cloud Uri Style option keyword, got: %s"), lc->str);
   }
   scan_to_eol(lc);
   set_bit(index, res_all.hdr.item_present);
}

/*
 * Store Maximum Block Size, and check it is not greater than MAX_BLOCK_LENGTH
 *
 */
void store_maxblocksize(LEX *lc, RES_ITEM *item, int index, int pass)
{
   store_size32(lc, item, index, pass);
   if (*(uint32_t *)(item->value) > MAX_BLOCK_LENGTH) {
      scan_err2(lc, _("Maximum Block Size configured value %u is greater than allowed maximum: %u"),
         *(uint32_t *)(item->value), MAX_BLOCK_LENGTH );
   }
}

/*
 * Cloud Glacier restore priority level
 *
 *   Option       option code = token
 */
s_kw restore_prio_opts[] = {
   {"High",       CLOUD_RESTORE_PRIO_HIGH},
   {"Medium",     CLOUD_RESTORE_PRIO_MEDIUM},
   {"Low",        CLOUD_RESTORE_PRIO_LOW},
   {NULL, 0}
};

/*
 * Store Cloud restoration priority 
 */
void store_transfer_priority(LEX *lc, RES_ITEM *item, int index, int pass)
{
   bool found = false;

   lex_get_token(lc, T_NAME);
   /* Store the label pass 2 so that type is defined */
   for (int i=0; restore_prio_opts[i].name; i++) {
      if (strcasecmp(lc->str, restore_prio_opts[i].name) == 0) {
         *(uint32_t *)(item->value) = restore_prio_opts[i].token;
         found = true;
         break;
      }
   }
   if (!found) {
      scan_err1(lc, _("Expected a Cloud Restore Priority Style option keyword (High, Medium, Low), got: %s"), lc->str);
   }
   scan_to_eol(lc);
   set_bit(index, res_all.hdr.item_present);
}

static void dump_store_resource(STORES *store,
      void sendit(const char *msg, int len, STATUS_PKT *sp), STATUS_PKT *sp)
{
   OutputWriter ow(sp->api_opts);
   POOL_MEM tmp;
   char *p;

   ow.start_group("resource");

   ow.get_output(OT_START_OBJ,
                 OT_STRING,   "Name", store->hdr.name,
                 OT_STRING,   "Description", store->hdr.desc,
                 OT_STRING,   "WorkingDirectory", store->working_directory,
                 OT_STRING,   "PidDirectory", store->pid_directory,
                 OT_STRING,   "SubsysDirectory", store->subsys_directory,
                 OT_STRING,   "PluginDirectory", store->plugin_directory,
                 OT_STRING,   "ScriptsDirectory", store->scripts_directory,
#if BEEF
                 OT_BOOL,     "FIPSRequire", store->require_fips,
                 OT_STRING,   "SsdDirectory", store->ssd_directory,
#endif
                 OT_INT,      "SDPort", get_first_port_host_order(store->sdaddrs),
                 OT_STRING,   "SDAddress", get_first_address(store->sdaddrs, tmp.c_str(), PM_MESSAGE),
                 OT_INT64,    "HeartbeatInterval", store->heartbeat_interval,
                 OT_INT32,    "MaximumConcurrentJobs", store->max_concurrent_jobs,
                 OT_INT64,    "ClientConnectTimeout", store->ClientConnectTimeout,
                 OT_BOOL,     "TLSEnable", store->tls_enable,
                 OT_BOOL,     "TLSPSKEnable", store->tls_psk_enable,
                 OT_BOOL,     "TLSRequire", store->tls_require,
                 OT_BOOL,     "TLSAuthenticate", store->tls_authenticate,
                 OT_BOOL,     "TLSVerifyPeer", store->tls_verify_peer,
                 OT_INT64,    "ClientConnectWait", store->client_wait,
                 OT_STRING,   "VerId", store->verid,
                 OT_BOOL,     "CommCommpression", store->comm_compression,
#ifdef SD_DEDUP_SUPPORT
                 OT_STRING,   "DedupDirectory", store->dedup_dir,
                 OT_STRING,   "DedupIndexDirectory", store->dedup_index_dir,
                 OT_BOOL,     "DedupCheckHash", store->dedup_check_hash,
                 OT_INT64,    "DedupScrupMaximumBandwidth", store->dedup_scrub_max_bandwidth,
                 OT_INT64,    "MaximumContainerSize", store->max_container_size,
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

/* Dump contents of resource */
void dump_resource(int type, RES *rres, void sendit(const char *msg, int len, STATUS_PKT *sp), STATUS_PKT *sp)
{
   URES *res = (URES *)rres;
   char buf[1000];
   int recurse = 1, len;
   POOL_MEM msg(PM_MESSAGE);

   if (res == NULL) {
      len = Mmsg(msg, _("Warning: no \"%s\" resource (%d) defined.\n"), res_to_str(type), type);
      sendit(msg.c_str(), len, sp);
      return;
   }

   Dmsg1(dbglvl, "dump_resource type=%d\n", type);

   if (type < 0) {                    /* no recursion */
      type = - type;
      recurse = 0;
   }

   switch (type) {
   case R_DIRECTOR:
      len = Mmsg(msg, "Director: name=%s\n", res->res_dir.hdr.name);
      sendit(msg.c_str(), len, sp);
      break;
   case R_STORAGE:
      {
         STORES *store = (STORES *)rres;
         dump_store_resource(store, sendit, sp);
         break;
      }
   case R_DEVICE:
      len = Mmsg(msg, "Device: name=%s MediaType=%s Device=%s LabelType=%d\n",
         res->res_dev.hdr.name,
         res->res_dev.media_type, res->res_dev.device_name,
         res->res_dev.label_type);
      sendit(msg.c_str(), len, sp);
      len = Mmsg(msg, "        rew_wait=%lld min_bs=%d max_bs=%d chgr_wait=%lld\n",
         res->res_dev.max_rewind_wait, res->res_dev.min_block_size,
         res->res_dev.max_block_size, res->res_dev.max_changer_wait);
      sendit(msg.c_str(), len, sp);
      len = Mmsg(msg, "        max_jobs=%d max_files=%lld max_size=%lld\n",
         res->res_dev.max_volume_jobs, res->res_dev.max_volume_files,
         res->res_dev.max_volume_size);
      len = Mmsg(msg, "        min_block_size=%lld max_block_size=%lld\n",
         res->res_dev.min_block_size, res->res_dev.max_block_size);
      sendit(msg.c_str(), len, sp);
      len = Mmsg(msg, "        max_file_size=%lld capacity=%lld\n",
         res->res_dev.max_file_size, res->res_dev.volume_capacity);
      sendit(msg.c_str(), len, sp);
      len = Mmsg(msg, "        spool_directory=%s\n", NPRT(res->res_dev.spool_directory));
      sendit(msg.c_str(), len, sp);
      len = Mmsg(msg, "        max_spool_size=%lld max_job_spool_size=%lld\n",
         res->res_dev.max_spool_size, res->res_dev.max_job_spool_size);
      sendit(msg.c_str(), len, sp);
      if (res->res_dev.worm_command) {
         len = Mmsg(msg, "         worm command=%s\n", res->res_dev.worm_command);
         sendit(msg.c_str(), len, sp);
      }
      if (res->res_dev.changer_res) {
         len = Mmsg(msg, "         changer=%p\n", res->res_dev.changer_res);
         sendit(msg.c_str(), len, sp);
      }
      bstrncpy(buf, "        ", sizeof(buf));
      if (res->res_dev.cap_bits & CAP_EOF) {
         bstrncat(buf, "CAP_EOF ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_BSR) {
         bstrncat(buf, "CAP_BSR ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_BSF) {
         bstrncat(buf, "CAP_BSF ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_FSR) {
         bstrncat(buf, "CAP_FSR ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_FSF) {
         bstrncat(buf, "CAP_FSF ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_EOM) {
         bstrncat(buf, "CAP_EOM ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_REM) {
         bstrncat(buf, "CAP_REM ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_RACCESS) {
         bstrncat(buf, "CAP_RACCESS ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_AUTOMOUNT) {
         bstrncat(buf, "CAP_AUTOMOUNT ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_LABEL) {
         bstrncat(buf, "CAP_LABEL ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_ANONVOLS) {
         bstrncat(buf, "CAP_ANONVOLS ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_ALWAYSOPEN) {
         bstrncat(buf, "CAP_ALWAYSOPEN ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_CHECKLABELS) {
         bstrncat(buf, "CAP_CHECKLABELS ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_REQMOUNT) {
         bstrncat(buf, "CAP_REQMOUNT ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_OFFLINEUNMOUNT) {
         bstrncat(buf, "CAP_OFFLINEUNMOUNT ", sizeof(buf));
      }
      bstrncat(buf, "\n", sizeof(buf));
      sendit(buf, strlen(buf), sp);  /* Send caps string */
      if (res->res_dev.cloud) {
         len = Mmsg(msg, "   --->Cloud: name=%s\n", res->res_dev.cloud->hdr.name);
         sendit(msg.c_str(), len, sp);
      }
      break;
   case R_CLOUD:
      len = Mmsg(msg, "Cloud: name=%s Driver=%d\n"
         "      HostName=%s\n"
         "      BucketName=%s\n"
         "      AccessKey=%s SecretKey=%s\n"
         "      AuthRegion=%s\n"
         "      Protocol=%d UriStyle=%d\n",
         res->res_cloud.hdr.name, res->res_cloud.driver_type,
         res->res_cloud.host_name,
         res->res_cloud.bucket_name,
         res->res_cloud.access_key, res->res_cloud.secret_key,
         res->res_cloud.region,
         res->res_cloud.protocol, res->res_cloud.uri_style);
      sendit(msg.c_str(), len, sp);
      break;
   case R_DEDUP:
      len = Mmsg(msg, "Dedup: name=%s Driver=%d\n"
         "      DedupDirectory=%s\n",
         res->res_dedup.hdr.name, res->res_dedup.driver_type,
         res->res_dedup.dedup_dir);
      sendit(msg.c_str(), len, sp);
      break;
   case R_AUTOCHANGER:
      DEVRES *dev;
      len = Mmsg(msg, "Changer: name=%s Changer_devname=%s\n      Changer_cmd=%s\n",
         res->res_changer.hdr.name,
         res->res_changer.changer_name, res->res_changer.changer_command);
      sendit(msg.c_str(), len, sp);
      foreach_alist(dev, res->res_changer.device) {
         len = Mmsg(msg, "   --->Device: name=%s\n", dev->hdr.name);
         sendit(msg.c_str(), len, sp);
      }
      break;
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
   default:
      len = Mmsg(msg, _("Warning: unknown resource type %d\n"), type);
      sendit(msg.c_str(), len, sp);
      break;
   }
   rres = GetNextRes(type, rres);
   if (recurse && rres)
      dump_resource(type, rres, sendit, sp);
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

   if (res == NULL)
      return;

   /* common stuff -- free the resource name */
   if (res->res_dir.hdr.name) {
      free(res->res_dir.hdr.name);
   }
   if (res->res_dir.hdr.desc) {
      free(res->res_dir.hdr.desc);
   }


   switch (type) {
   case R_DIRECTOR:
      if (res->res_dir.password) {
         free(res->res_dir.password);
      }
      if (res->res_dir.address) {
         free(res->res_dir.address);
      }
      if (res->res_dir.tls_ctx) {
         free_tls_context(res->res_dir.tls_ctx);
      }
      if (res->res_dir.psk_ctx) {
         free_psk_context(res->res_dir.psk_ctx);
      }
      if (res->res_dir.tls_ca_certfile) {
         free(res->res_dir.tls_ca_certfile);
      }
      if (res->res_dir.tls_ca_certdir) {
         free(res->res_dir.tls_ca_certdir);
      }
      if (res->res_dir.tls_certfile) {
         free(res->res_dir.tls_certfile);
      }
      if (res->res_dir.tls_keyfile) {
         free(res->res_dir.tls_keyfile);
      }
      if (res->res_dir.tls_dhfile) {
         free(res->res_dir.tls_dhfile);
      }
      if (res->res_dir.tls_allowed_cns) {
         delete res->res_dir.tls_allowed_cns;
      }
      break;
   case R_AUTOCHANGER:
      if (res->res_changer.changer_name) {
         free(res->res_changer.changer_name);
      }
      if (res->res_changer.changer_command) {
         free(res->res_changer.changer_command);
      }
      if (res->res_changer.lock_command) {
         free(res->res_changer.lock_command);
      }
      if (res->res_changer.device) {
         delete res->res_changer.device;
      }
      rwl_destroy(&res->res_changer.changer_lock);
      break;
   case R_STORAGE:
      if (res->res_store.sdaddrs) {
         free_addresses(res->res_store.sdaddrs);
      }
      if (res->res_store.sddaddrs) {
         free_addresses(res->res_store.sddaddrs);
      }
      if (res->res_store.working_directory) {
         free(res->res_store.working_directory);
      }
      if (res->res_store.pid_directory) {
         free(res->res_store.pid_directory);
      }
      if (res->res_store.subsys_directory) {
         free(res->res_store.subsys_directory);
      }
      if (res->res_store.plugin_directory) {
         free(res->res_store.plugin_directory);
      }
      if (res->res_store.scripts_directory) {
         free(res->res_store.scripts_directory);
      }
      if (res->res_store.ssd_directory) {
         free(res->res_store.ssd_directory);
      }
      if (res->res_store.tls_ctx) {
         free_tls_context(res->res_store.tls_ctx);
      }
      if (res->res_store.psk_ctx) {
         free_psk_context(res->res_store.psk_ctx);
      }
      if (res->res_store.tls_ca_certfile) {
         free(res->res_store.tls_ca_certfile);
      }
      if (res->res_store.tls_ca_certdir) {
         free(res->res_store.tls_ca_certdir);
      }
      if (res->res_store.tls_certfile) {
         free(res->res_store.tls_certfile);
      }
      if (res->res_store.tls_keyfile) {
         free(res->res_store.tls_keyfile);
      }
      if (res->res_store.tls_dhfile) {
         free(res->res_store.tls_dhfile);
      }
      if (res->res_store.tls_allowed_cns) {
         delete res->res_store.tls_allowed_cns;
      }
      if (res->res_store.verid) {
         free(res->res_store.verid);
      }
      if (res->res_store.dedup_dir) {
         free(res->res_store.dedup_dir);
      }
      if (res->res_store.dedup_index_dir) {
         free(res->res_store.dedup_index_dir);
      }
      break;
   case R_CLOUD:
      if (res->res_cloud.host_name) {
         free(res->res_cloud.host_name);
      }
      if (res->res_cloud.bucket_name) {
         free(res->res_cloud.bucket_name);
      }
      if (res->res_cloud.access_key) {
         free(res->res_cloud.access_key);
      }
      if (res->res_cloud.secret_key) {
         free(res->res_cloud.secret_key);
      }
      if (res->res_cloud.region) {
         free(res->res_cloud.region);
      }
      if (res->res_cloud.blob_endpoint) {
         free(res->res_cloud.blob_endpoint);
      }
      if (res->res_cloud.file_endpoint) {
         free(res->res_cloud.file_endpoint);
      }
      if (res->res_cloud.queue_endpoint) {
         free(res->res_cloud.queue_endpoint);
      }
      if (res->res_cloud.table_endpoint) {
         free(res->res_cloud.table_endpoint);
      }
      if (res->res_cloud.endpoint_suffix) {
         free(res->res_cloud.endpoint_suffix);
      }
      if (res->res_cloud.driver_command) {
         free(res->res_cloud.driver_command);
      }
      break;
   case R_DEDUP:
      if (res->res_dedup.dedup_dir) {
         free(res->res_dedup.dedup_dir);
      }
      if (res->res_dedup.dedup_index_dir) {
         free(res->res_dedup.dedup_index_dir);
      }
      if (res->res_dedup.dedup_err_msg) {
         free(res->res_dedup.dedup_err_msg);
      }
      break;
   case R_DEVICE:
      if (res->res_dev.media_type) {
         free(res->res_dev.media_type);
      }
      if (res->res_dev.device_name) {
         free(res->res_dev.device_name);
      }
      if (res->res_dev.adevice_name) {
         free(res->res_dev.adevice_name);
      }
      if (res->res_dev.control_name) {
         free(res->res_dev.control_name);
      }
      if (res->res_dev.changer_name) {
         free(res->res_dev.changer_name);
      }
      if (res->res_dev.changer_command) {
         free(res->res_dev.changer_command);
      }
      if (res->res_dev.alert_command) {
         free(res->res_dev.alert_command);
      }
      if (res->res_dev.worm_command) {
         free(res->res_dev.worm_command);
      }
      if (res->res_dev.lock_command) {
         free(res->res_dev.lock_command);
      }
      if (res->res_dev.spool_directory) {
         free(res->res_dev.spool_directory);
      }
      if (res->res_dev.mount_point) {
         free(res->res_dev.mount_point);
      }
      if (res->res_dev.mount_command) {
         free(res->res_dev.mount_command);
      }
      if (res->res_dev.unmount_command) {
         free(res->res_dev.unmount_command);
      }
      if (res->res_dev.write_part_command) {
         free(res->res_dev.write_part_command);
      }
      if (res->res_dev.free_space_command) {
         free(res->res_dev.free_space_command);
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
   default:
      Dmsg1(0, _("Unknown resource type %d\n"), type);
      break;
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
      case R_STORAGE:
         size = sizeof(STORES);
         break;
      case R_DEVICE:
         size = sizeof(DEVRES);
         break;
      case R_MSGS:
         size = sizeof(MSGS);
         break;
      case R_AUTOCHANGER:
         size = sizeof(AUTOCHANGER);
         break;
      case R_CLOUD:
         size = sizeof(CLOUD);
         break;
      case R_DEDUP:
         size = sizeof(DEDUPRES);
         break;
      case R_COLLECTOR:
         size = sizeof(COLLECTOR);
         break;
      default:
         break;
   }
   return size;
}

/* Save the new resource by chaining it into the head list for
 * the resource. If this is pass 2, we update any resource
 * or alist pointers.
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
      /* If this triggers, take a look at lib/parse_conf.h */
      if (i >= MAX_RES_ITEMS) {
         Mmsg(config->m_errmsg, _("Too many directives in \"%s\" resource\n"), resources[rindex].name);
         return false;
      }
   }

   /* During pass 2, we looked up pointers to all the resources
    * referrenced in the current resource, , now we
    * must copy their address from the static record to the allocated
    * record.
    */
   if (pass == 2) {
      DEVRES *dev;
      int errstat;
      switch (type) {
      /* Resources not containing a resource */
      case R_MSGS:
      case R_CLOUD:
      case R_DEDUP:
         break;

      /* Resources containing a resource or an alist */
      case R_DIRECTOR:
         if ((res = (URES *)GetResWithName(R_DIRECTOR, res_all.res_dir.hdr.name)) == NULL) {
            Mmsg(config->m_errmsg, _("Cannot find Director resource %s\n"), res_all.res_dir.hdr.name);
            return false;
         }
         res->res_dir.tls_allowed_cns = res_all.res_dir.tls_allowed_cns;
         break;
      case R_STORAGE:
         if ((res = (URES *)GetResWithName(R_STORAGE, res_all.res_dir.hdr.name)) == NULL) {
            Mmsg(config->m_errmsg,  _("Cannot find Storage resource %s\n"), res_all.res_dir.hdr.name);
            return false;
         }
         res->res_store.messages = res_all.res_store.messages;
         res->res_store.tls_allowed_cns = res_all.res_store.tls_allowed_cns;
         break;
      case R_AUTOCHANGER:
         if ((res = (URES *)GetResWithName(type, res_all.res_changer.hdr.name)) == NULL) {
            Mmsg(config->m_errmsg, _("Cannot find AutoChanger resource %s\n"),
                 res_all.res_changer.hdr.name);
            return false;
         }
         /* we must explicitly copy the device alist pointer */
         res->res_changer.device   = res_all.res_changer.device;
         /*
          * Now update each device in this resource to point back
          *  to the changer resource.
          */
         foreach_alist(dev, res->res_changer.device) {
            dev->changer_res = (AUTOCHANGER *)&res->res_changer;
         }
         if ((errstat = rwl_init(&res->res_changer.changer_lock, PRIO_SD_ACH_ACCESS)) != 0) {
            berrno be;
            Mmsg(config->m_errmsg, _("Unable to init lock for Autochanger=%s: ERR=%s\n"),
                 res_all.res_changer.hdr.name, be.bstrerror(errstat));
            return false;
         }
         break;
      case R_DEVICE:
         if ((res = (URES *)GetResWithName(R_DEVICE, res_all.res_dev.hdr.name)) == NULL) {
            Mmsg(config->m_errmsg,  _("Cannot find Device resource %s\n"), res_all.res_dir.hdr.name);
            return false;
         }
         res->res_dev.cloud = res_all.res_dev.cloud;
#ifdef SD_DEDUP_SUPPORT
         res->res_dev.dedup = res_all.res_dev.dedup;
         /* If a dedupengine is required but not defined, look for the
          * "default_legacy_dedupengine" aka the DDE defined in storage
          */
         if (res->res_dev.dev_type == B_DEDUP_DEV && res->res_dev.dedup == NULL) {
            /* The "Dedupengine" is not specified in this Device resource */
            /* If there is only one Dedupengine defined use it */
            int i = 0;
            DEDUPRES *dedup, *prev_dedup = NULL;
            foreach_res(dedup, R_DEDUP) {
               prev_dedup = dedup;
               i++;
            }
            if (i == 1) {
               res->res_dev.dedup = prev_dedup;
               Dmsg2(100, "Select dedupengine %s for device %s\n", prev_dedup->hdr.name, res_all.res_dir.hdr.name);
            } else if (i == 0){
               Mmsg(config->m_errmsg,  _("Cannot find any dedupengine for the device %s\n"), res_all.res_dir.hdr.name);
               return false;
            } else {
               /* i > 1 */
               Mmsg(config->m_errmsg,  _("Dedupengine unspecified for device %s\n"), res_all.res_dir.hdr.name);
               return false;
            }
         }
#endif
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
         printf(_("Unknown resource type %d\n"), type);
         error = 1;
         break;
      }

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
   if (pass == 1 && type == R_STORAGE) {
      if (!dedup_check_storage_resource(config)) {
         return false;
      }
   }
   return true;
}

bool parse_sd_config(CONFIG *config, const char *configfile, int exit_code)
{
   config->init(configfile, NULL, exit_code, (void *)&res_all, res_all_size,
      r_first, r_last, resources, &res_head);
   return config->parse_config();
}
