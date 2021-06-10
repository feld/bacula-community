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
 * @file test_metaplugin_backend.cpp
 * @author Radosław Korzeniewski (radoslaw@korzeniewski.net)
 * @brief This is a dumb and extremely simple backend simulator used for test Metaplugin.
 * @version 2.1.1
 * @date 2021-03-10
 *
 * @copyright Copyright (c) 2020 All rights reserved. IP transferred to Bacula Systems according to agreement.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


#ifndef LOGDIR
#define LOGDIR "/tmp"
#endif

extern const char *PLUGINPREFIX;
extern const char *PLUGINNAME;

int logfd;
pid_t mypid;
char * buf;
char * buflog;

bool regress_error_plugin_params = false;
bool regress_error_start_job = false;
bool regress_error_backup_no_files = false;
bool regress_error_backup_stderr = false;
bool regress_backup_plugin_objects = false;
bool regress_error_backup_abort = false;
bool regress_error_estimate_stderr = false;
bool regress_error_listing_stderr = false;
bool regress_error_restore_stderr = false;
bool regress_backup_other_file = false;
bool regress_metadata_support = false;
bool regress_standard_error_backup = false;


#define BUFLEN             4096
#define BIGBUFLEN          65536

/**
 * @brief saves the log text to logfile
 *
 * @param txt log text to save
 */
void LOG(const char *txt)
{
   char _buf[BUFLEN];

   int p = 0;
   for (int a = 0; txt[a]; a++)
   {
      char c = txt[a];
      if (c == '\n')
      {
         _buf[p++] = '\\';
         _buf[p++] = 'n';
      } else {
         _buf[p++] = c;
      }
   }
   _buf[p++] = '\n';
   _buf[p] = '\0';
   write(logfd, _buf, p);
}

/**
 * @brief Reads the raw packet from plugin.
 *
 * @param buf the memory buffer to save packet payload
 * @return int the size of the packer read
 */
int read_plugin(char * buf)
{
   int len;
   int nread;
   int size;
   char header[8];

   len = read(STDIN_FILENO, &header, 8);
   if (len < 8){
      LOG("#> Err: header too short");
      close(logfd);
      exit(2);
   }
   if (header[0] == 'F'){
      LOG(">> EOD >>");
      return 0;
   }
   if (header[0] == 'T'){
      LOG(">> TERM >>");
      close(logfd);
      exit(0);
   }
   size = atoi(header+1);
   if (size > BIGBUFLEN){
      LOG("#> Err: message too long");
      close(logfd);
      exit(3);
   }

   if (header[0] == 'C'){
      len = read(STDIN_FILENO, buf, size);
      buf[len] = 0;
      snprintf(buflog, BUFLEN, "> %s", buf);
      LOG(buflog);
   } else {
      snprintf(buflog, BUFLEN, "> Data:%i", size);
      LOG(buflog);
      len = 0;
      while (len < size){
         nread = read(STDIN_FILENO, buf, size - len);
         len += nread;
         snprintf(buflog, BUFLEN, "> Dataread:%i", nread);
         LOG(buflog);
      }
   }

   return len;
}

void read_plugin_data_stream()
{
   int len = read_plugin(buf);
   if (len == 0){
      /* empty file to restore */
      LOG("#> Empty data.");
      return;
   }
   bool loopgo = true;
   int fsize = len;
   while (loopgo){
      len = read_plugin(buf);
      fsize += len;
      if (len > 0){
         LOG("#> data stream saved.");
         continue;
      } else {
         loopgo = false;
         snprintf(buflog, 4096, "#> data END = %i", fsize);
      }
   }
}

/**
 * @brief Sends/writes the data to plugin with assembling the raw packet.
 *
 * @param cmd the packet type to sent
 * @param str the text to write
 */
void write_plugin(const char cmd, const char *str)
{
   int len;
   const char * out;

   if (str){
      len = strlen(str);
      out = str;
   } else {
      len = 0;
      out = "";
   }
   printf("%c%06d\n", cmd, len);
   printf("%s", out);
   fflush(stdout);
   snprintf(buflog, BUFLEN, "<< %c%06d:%s", cmd, len, out);
   LOG(buflog);
}

/**
 * @brief Sends/writes the binary data to plugin with assembling the raw packet.
 *
 * @param cmd the packet type to sent
 * @param str the text to write
 */
void write_plugin_bin(const char *str, int len = 0)
{
   const char * out;

   if (str) {
      out = str;
   } else {
      out = "";
   }

   printf("D%06d\n", len);
   int status = fwrite(out, len, 1, stdout);
   fflush(stdout);
   snprintf(buflog, BUFLEN, "<< D%06d:%d:<bindata>", len ,status);
   LOG(buflog);
}

/**
 * @brief Sends the EOD packet to plugin.
 */
void signal_eod(){
   printf("F000000\n");
   fflush(stdout);
   LOG("<< EOD <<");
}

/**
 * @brief Sends the termination packet to plugin.
 */
void signal_term(){
   printf("T000000\n");
   fflush(stdout);
   LOG("<< TERM <<");
}

/**
 * @brief Perform test backup.
 */
void perform_backup()
{
   // This is a test for FileIndex Query
   snprintf(buf, BIGBUFLEN, "FileIndex\n");
   write_plugin('C', buf);
   char firesponse[32] = {0};    // well the file index is int32_t so max 11 chars
   read_plugin(firesponse);
   int fileindex = atoi(firesponse);
   snprintf(buf, BIGBUFLEN, "TEST05 - FileIndex query: %d", fileindex);
   write_plugin('I', buf);

   // here we store the linked.file origin fname
   char fileindex_link[256];
   snprintf(fileindex_link, 256, "%s/bucket/%d/vm1.iso", PLUGINPREFIX, mypid);

   // Backup Loop
   snprintf(buf, BIGBUFLEN, "FNAME:%s\n", fileindex_link);           // we use it here
   write_plugin('C', buf);
   write_plugin('C', "STAT:F 1048576 100 100 100640 2\n");           // this will be the first file hardlinked
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   write_plugin('I', "TEST5");
   signal_eod();
   // here comes a file data contents
   write_plugin('C', "DATA\n");
   write_plugin('D', "/* here comes a file data contents */");
   write_plugin('D', "/* here comes another file line    */");
   write_plugin('D', "/* here comes another file line    */");
   write_plugin('D', "/* here comes another file line    */");
   write_plugin('D', "/* here comes another file line    */");
   signal_eod();
   write_plugin('I', "TEST5Data");
   // and now additional metadata
   write_plugin('C', "ACL\n");
   write_plugin('D', "user::rw-\nuser:root:-wx\ngroup::r--\nmask::rwx\nother::r--\n");
   write_plugin('I', "TEST5Acl");
   signal_eod();

   if (regress_error_backup_no_files) {
      write_plugin('E', "No files found for pattern container1/otherobject\n");
      signal_eod();
      return;
   }

   // next file
   // this files we will restore using Bacula Core functionality, so it is crucial
   write_plugin('I', "TEST6");
   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/etc/issue\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:F 26 200 200 100640 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   write_plugin('C', "PIPE:/etc/issue\n");
   read_plugin(buf);
   signal_eod();
   write_plugin('C', "DATA\n");
   write_plugin('I', "TEST6Data");

   write_plugin('I', "TEST6A");
   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/fileforcore\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:F 27 200 200 100640 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   signal_eod();
   write_plugin('C', "DATA\n");
   write_plugin('I', "TEST6AData");
   write_plugin('D', "/* here comes another file line    */");
   write_plugin('D', "/* here comes another file line    */");
   signal_eod();
   write_plugin('I', "TEST6Axattr");
   write_plugin('C', "XATTR\n");
   write_plugin('D', "bacula.custom.data=Inteos\nsystem.custom.data=Bacula\n");
   signal_eod();

   // next file
   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/vm2.iso\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:F 1048576 200 200 100640 1\n");

   if (regress_error_backup_stderr)
   {
      // test some stderror handling, yes in the middle file parameters
      errno = EACCES;
      perror("I've got some unsuspected error which I'd like to display on stderr (COMM_STDERR)");
      sleep(1);
   }

   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   signal_eod();
   write_plugin('I', "TEST7");
   /* here comes a file data contents */
   write_plugin('C', "DATA\n");
   write_plugin('D', "/* here comes a file data contents */");
   write_plugin('D', "/* here comes another file line    */");
   write_plugin('D', "/* here comes another file line    */");

   if (regress_error_backup_stderr && false)
   {
      // test some stderror handling, yes in the middle of data transfer
      errno = EACCES;
      perror("I've got some unsuspected error which I'd like to display on stderr (COMM_STDERR)");
      sleep(1);
   }

   write_plugin('D', "/* here comes another file line    */");
   write_plugin('D', "/* here comes another file line    */");
   signal_eod();
   write_plugin('I', "TEST7Data");
   write_plugin('C', "XATTR\n");
   write_plugin('D', "bacula.custom.data=Inteos\nsystem.custom.data=Bacula\n");
   signal_eod();

   if (regress_backup_other_file)
   {
      // next file
      snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/vm222-other-file.iso\n", PLUGINPREFIX, mypid);
      write_plugin('C', buf);
      write_plugin('C', "STAT:F 1048576 200 200 100640 1\n");
      write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
      signal_eod();
      write_plugin('I', "TEST7-Other");
      /* here comes a file data contents */
      write_plugin('C', "DATA\n");
      write_plugin('D', "/* here comes a file data contents */");
      write_plugin('D', "/* here comes another file line    */");
      write_plugin('D', "/* here comes another file line    */");
      write_plugin('D', "/* here comes another file line    */");
      write_plugin('D', "/* here comes another file line    */");
      write_plugin('I', "TEST7-Other-End");
      signal_eod();
   }

   // next file
   write_plugin('I', "TEST8");
   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/SHELL\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:F 1099016 0 0 100640 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   write_plugin('C', "PIPE:/bin/bash\n");
   read_plugin(buf);
   signal_eod();
   write_plugin('C', "DATA\n");
   write_plugin('I', "TEST8Data");

   if (regress_standard_error_backup)
   {
      // next file
      snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/standard-error-file\n", PLUGINPREFIX, mypid);
      write_plugin('C', buf);
      write_plugin('C', "STAT:F 1048576 200 200 100640 1\n");
      write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
      signal_eod();
      write_plugin('I', "TEST8-Error-Start");
      /* here comes a file data contents */
      write_plugin('E', "TEST8-Error: Standard IO Error goes Here");
      write_plugin('I', "TEST8-Error-End");
      // signal_eod();
   }

   if (regress_backup_plugin_objects)
   {
      // test Plugin Objects interface
      write_plugin('I', "TEST PluginObject");
      snprintf(buf, BIGBUFLEN, "PLUGINOBJ:%s/images/%d/vm1\n", PLUGINPREFIX, mypid);
      write_plugin('C', buf);
      write_plugin('C', "PLUGINOBJ_CAT:Image\n");
      write_plugin('C', "PLUGINOBJ_TYPE:VM\n");
      snprintf(buf, BIGBUFLEN, "PLUGINOBJ_NAME:%s%d/vm1 - Name\n", PLUGINPREFIX, mypid);
      write_plugin('C', buf);
      snprintf(buf, BIGBUFLEN, "PLUGINOBJ_SRC:%s\n", PLUGINPREFIX);
      write_plugin('C', buf);
      write_plugin('C', "PLUGINOBJ_UUID:c3260b8c560e5e093e8913065fa3cba9\n");
      write_plugin('C', "PLUGINOBJ_SIZE:1024kB\n");
      signal_eod();
      write_plugin('I', "TEST PluginObject - END");
   }

   // next file
   write_plugin('I', "TEST9");
   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/lockfile\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:E 0 300 300 0100640 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   signal_eod();
   write_plugin('I', "TEST9E");
   signal_eod();

   // next file
   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/file.xattr\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:E 0 300 300 0100640 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   signal_eod();
   write_plugin('I', "TEST10");
   signal_eod();
   write_plugin('C', "XATTR\n");
   write_plugin('D', "bacula.custom.data=Inteos\nsystem.custom.data=Bacula\n");
   signal_eod();

   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/vmsnap.iso\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:S 1048576 0 0 100640 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   snprintf(buf, BIGBUFLEN, "LSTAT:bucket/%d/vm1.iso/1508502750.495885/69312986/10485760/\n", mypid);
   write_plugin('C', buf);
   signal_eod();
   write_plugin('I', "TEST11 - segmented object");
   write_plugin('C', "DATA\n");
   write_plugin('D', "/* here comes a file data contents */");
   write_plugin('D', "/* here comes another file line    */");
   write_plugin('D', "/* here comes another file line    */");
   write_plugin('D', "/* here comes another file line    */");
   write_plugin('D', "/* here comes another file line    */");
   signal_eod();

   if (regress_standard_error_backup)
   {
      // next file
      snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/standard-error-file2\n", PLUGINPREFIX, mypid);
      write_plugin('C', buf);
      write_plugin('C', "STAT:F 1048576 200 200 100640 1\n");
      write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
      signal_eod();
      write_plugin('I', "TEST8-Error-Start");
      /* here comes a file data contents */
      write_plugin('C', "DATA\n");
      write_plugin('D', "/* here comes a file data contents */");
      write_plugin('D', "/* here comes another file line    */");
      write_plugin('D', "/* here comes another file line    */");
      write_plugin('D', "/* here comes another file line    */");
      write_plugin('E', "TEST8-Error: Standard IO Error goes Here");
      write_plugin('I', "TEST8-Error-End");
      // signal_eod();
   }

   const int bigfileblock = 100000;
   const int bigfilesize = bigfileblock * 5;
   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/bigfile.raw\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   snprintf(buf, BIGBUFLEN, "STAT:F %d 0 0 100640 1\n", bigfilesize);
   write_plugin('C', buf);
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   signal_eod();
   write_plugin('I', "TEST17 - big file block");
   write_plugin('C', "DATA\n");
   {
      char *bigfileblock_ptr = (char*)malloc(bigfileblock);
      memset(bigfileblock_ptr, 0xA1, bigfileblock);
      for (int s = bigfilesize; s > 0; s -= bigfileblock) {
         write_plugin_bin(bigfileblock_ptr, bigfileblock);
      }
      free(bigfileblock_ptr);
   }
   signal_eod();

   if (regress_error_backup_abort)
   {
      snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/file on error\n", PLUGINPREFIX, mypid);
      write_plugin('C', buf);
      write_plugin('C', "STAT:F 234560 900 900 0100640 1\n");
      write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
      signal_eod();
      write_plugin('A', "Some error...\n");
      return;
   }

   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/data.dir/\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:D 1024 100 100 040755 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   signal_eod();
   write_plugin('I', "TEST15 - backup data dir");
   write_plugin('C', "DATA\n");
   write_plugin('D', "/* here comes a file data contents */");
   write_plugin('D', "/* here comes another file line    */");
   signal_eod();

   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/directory.with.xattrs/\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:D 1024 100 100 040755 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   signal_eod();
   write_plugin('I', "TEST16 - backup dir + xattrs");
   write_plugin('C', "DATA\n");
   write_plugin('D', "/* here comes a file data contents */");
   write_plugin('D', "/* here comes another file line    */");
   signal_eod();
   write_plugin('C', "XATTR\n");
   write_plugin('D', "bacula.custom.data=Inteos\nsystem.custom.data=Bacula\n");
   signal_eod();

   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/acl.dir/\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:D 1024 100 100 040755 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   signal_eod();
   write_plugin('I', "TEST12 - backup dir");
   signal_eod();
   write_plugin('C', "ACL\n");
   write_plugin('D', "user::rwx\ngroup::r-x\nother::r-x\n");
   signal_eod();

   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:D 1024 100 100 040755 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   signal_eod();
   write_plugin('I', "TEST12 - backup dir");
   signal_eod();
   write_plugin('C', "XATTR\n");
   write_plugin('D', "bacula.custom.data=Inteos\nsystem.custom.data=Bacula\n");
   signal_eod();

   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/\n", PLUGINPREFIX);
   write_plugin('C', buf);
   write_plugin('C', "STAT:D 1024 100 100 040755 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   signal_eod();
   write_plugin('I', "TEST12 - backup another dir");
   write_plugin('W', "Make some warning messages.");
   signal_eod();

   const char longfilenamestr[] =
      "cb1e1926239b467c8e9affd7d22cea4993940d1e8f5377a1540d2b58e10be5669888c7e729fc9fe98f1400ca2e68c93075fd26e2806bebd727c71022de47f37b"
      "cb1e1926239b467c8e9affd7d22cea4993940d1e8f5377a1540d2b58e10be5669888c7e729fc9fe98f1400ca2e68c93075fd26e2806bebd727c71022de47f37b"
      "cb1e1926239b467c8e9affd7d22cea4993940d1e8f5377a1540d2b58e10be5669888c7e729fc9fe98f1400ca2e68c93075fd26e2806bebd727c71022de47f37b"
      "cb1e1926239b467c8e9affd7d22cea4993940d1e8f5377a1540d2b58e10be5669888c7e729fc9fe98f1400ca2e68c93075fd26e2806bebd727c71022de47f37b"
      "ENDOFNAME";
   const char *longfilename = longfilenamestr;

   // test for fname > 500c
#if __cplusplus > 201103L
   static_assert(sizeof(longfilenamestr) > 500);
#endif
   snprintf(buf, BIGBUFLEN, "FNAME:%s/%s\n", PLUGINPREFIX, longfilename);
   write_plugin('C', buf);
   write_plugin('C', "STAT:F 234560 901 901 0100640 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   signal_eod();
   write_plugin('I', "TEST13 - long FNAME test");
   write_plugin('C', "DATA\n");
   write_plugin('D', "/* here comes a file data contents */");
   write_plugin('D', "/* here comes another file line    */");
   write_plugin('D', "/* here comes another file line    */");
   write_plugin('D', "/* here comes another file line    */");
   write_plugin('D', "/* here comes another file line    */");
   signal_eod();

   if (regress_metadata_support)
   {
      snprintf(buf, BIGBUFLEN, "FNAME:%s/office/%d/document.docx\n", PLUGINPREFIX, mypid);
      write_plugin('C', buf);
      write_plugin('C', "STAT:F 10240 100 100 040755 1\n");
      write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");

      write_plugin('C', "METADATA_STREAM\n");
         write_plugin('D', "{ \"bacula.custom.data\": \"Inteos\"\n  \"system.custom.data\":\"Bacula\" }\n");
      signal_eod();

      write_plugin('C', "METADATA_STREAM\n");
         write_plugin('D', "This is a binary data!");
      signal_eod();

      // disabled intentionally
      // write_plugin('C', "METADATA_CATALOG\n");
      //    write_plugin('D', "TABLE1: { field1: \"value1\", field2: \"value2\", field3: \"value3\"}");
      // signal_eod();

      // write_plugin('C', "METADATA_CATALOG\n");
      //    write_plugin('D', "TABLE2: { field2: \"value1\", field2: \"value2\", field3: \"value3\"}");
      // signal_eod();

      signal_eod();  // end of file attributes

      write_plugin('I', "TEST14 - backup metadata");
      write_plugin('C', "DATA\n");
      write_plugin('D', "/* here comes a file data contents */");
      write_plugin('D', "/* here comes another file line    */");
      write_plugin('D', "/* here comes another file line    */");
      write_plugin('D', "/* here comes another file line    */");
      write_plugin('D', "/* here comes another file line    */");
      signal_eod();
   }

   snprintf(buf, BIGBUFLEN, "FNAME:%s/office/%d/linked.file\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   snprintf(buf, BIGBUFLEN, "STAT:L 10240 100 100 040755 2 %d\n", fileindex);
   write_plugin('C', buf);
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   snprintf(buf, BIGBUFLEN, "LSTAT:%s\n", fileindex_link);
   write_plugin('C', buf);
   signal_eod();
   signal_eod();

   // this plugin object should be the latest item to backup
   if (regress_backup_plugin_objects)
   {
      // test Plugin Objects interface
      write_plugin('I', "TEST PluginObject Last");
      snprintf(buf, BIGBUFLEN, "PLUGINOBJ:%s/images/%d/last_po_item\n", PLUGINPREFIX, mypid);
      write_plugin('C', buf);
      write_plugin('C', "PLUGINOBJ_CAT:POITEM\n");
      write_plugin('C', "PLUGINOBJ_TYPE:POINTEM\n");
      snprintf(buf, BIGBUFLEN, "PLUGINOBJ_NAME:%s%d/last_po_item - Name\n", PLUGINPREFIX, mypid);
      write_plugin('C', buf);
      snprintf(buf, BIGBUFLEN, "PLUGINOBJ_SRC:%s\n", PLUGINPREFIX);
      write_plugin('C', buf);
      write_plugin('C', "PLUGINOBJ_UUID:09bf8b2a-915d-11eb-8ebb-db6e14058a82\n");
      write_plugin('C', "PLUGINOBJ_SIZE:1024kB\n");
      signal_eod();
      write_plugin('I', "TEST PluginObject Last - END");
   }

   write_plugin('I', "M_INFO test message\n");
   write_plugin('W', "M_WARNING test message\n");
   write_plugin('S', "M_SAVED test message\n");
   write_plugin('N', "M_NOTSAVED test message\n");
   write_plugin('R', "M_RESTORED test message\n");
   write_plugin('P', "M_SKIPPED test message\n");
   write_plugin('O', "M_OPER?MOUNT test message\n");
   write_plugin('V', "M_EVENTS test message\n");
   if (regress_standard_error_backup)
   {
      write_plugin('Q', "M_ERROR test message\n");
   }

   /* this is the end of all data */
   signal_eod();
}

void perform_estimate(){
   /* Estimate Loop (5) */
   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/vm1.iso\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:F 1048576 100 100 100640 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   // write_plugin('I', "TEST5");
   signal_eod();

   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/vm2.iso\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:F 1048576 200 200 100640 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   signal_eod();
   // write_plugin('I', "TEST5A");

   if (regress_error_estimate_stderr)
   {
      // test some stderror handling
      errno = EACCES;
      perror("I've got some unsuspected error which I'd like to display on stderr (COMM_STDERR)");
   }

   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/lockfile\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:E 0 300 300 0100640 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   signal_eod();

   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/vmsnap.iso\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:S 0 0 0 0120777 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   snprintf(buf, BIGBUFLEN, "LSTAT:/bucket/%d/vm1.iso\n", mypid);
   write_plugin('C', buf);
   signal_eod();

   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/%d/\n", PLUGINPREFIX, mypid);
   write_plugin('C', buf);
   write_plugin('C', "STAT:D 1024 100 100 040755 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   signal_eod();

   snprintf(buf, BIGBUFLEN, "FNAME:%s/bucket/\n", PLUGINPREFIX);
   write_plugin('C', buf);
   write_plugin('C', "STAT:D 1024 100 100 040755 1\n");
   write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
   signal_eod();

   /* this is the end of all data */
   signal_eod();
}

/*
 * The listing procedure
 * when:
 * - / - it display           drwxr-x--- containers
 * - containers - it display  drwxr-x--- bucket1
 *                            drwxr-x--- bucket2
 */
void perform_listing(char *listing){
   /* Listing Loop (5) */
   if (strcmp(listing, "containers") == 0){
      /* this is a containers listing */
      write_plugin('C', "FNAME:bucket1/\n");
      write_plugin('C', "STAT:D 1024 100 100 040755 1\n");
      write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
      signal_eod();

   if (regress_error_listing_stderr)
   {
      // test some stderror handling
      errno = EACCES;
      perror("I've got some unsuspected error which I'd like to display on stderr (COMM_STDERR)");
   }

      write_plugin('C', "FNAME:bucket2/\n");
      write_plugin('C', "STAT:D 1024 100 100 040755 1\n");
      write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
      signal_eod();
   } else {
      if (strcmp(listing, "containers/bucket1") == 0){
         snprintf(buf, BIGBUFLEN, "FNAME:bucket1/%d/vm1.iso\n", mypid);
         write_plugin('C', buf);
         write_plugin('C', "STAT:F 1048576 100 100 100640 1\n");
         write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
         signal_eod();

         snprintf(buf, BIGBUFLEN, "FNAME:bucket1/%d/lockfile\n", mypid);
         write_plugin('C', buf);
         write_plugin('C', "STAT:E 0 300 300 0100640 1\n");
         write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
         signal_eod();
      } else
      if (strcmp(listing, "containers/bucket2") == 0){
         snprintf(buf, BIGBUFLEN, "FNAME:bucket2/%d/vm2.iso\n", mypid);
         write_plugin('C', buf);
         write_plugin('C', "STAT:F 1048576 200 200 100640 1\n");
         write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
         signal_eod();

         snprintf(buf, BIGBUFLEN, "FNAME:bucket2/%d/vmsnap.iso\n", mypid);
         write_plugin('C', buf);
         write_plugin('C', "STAT:S 0 0 0 0120777 1\n");
         write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
         snprintf(buf, BIGBUFLEN, "LSTAT:/bucket/%d/vm1.iso\n", mypid);
         write_plugin('C', buf);
         signal_eod();
      } else {
         /* this is a top-level listing, response with a single containers list */
         snprintf(buf, BIGBUFLEN, "FNAME:containers\n");
         write_plugin('C', buf);
         write_plugin('C', "STAT:D 0 0 0 040755 1\n");
         write_plugin('C', "TSTAMP:1504271937 1504271937 1504271937\n");
         // write_plugin('I', "TEST5");
         signal_eod();
      }
   }

   /* this is the end of all data */
   signal_eod();
}

/*
 * The query param procedure
 *    return 3 simple parameters
 */
void perform_queryparam(char *query)
{
   /* Query Loop (5) */
   snprintf(buf, BIGBUFLEN, "%s=test1\n", query);
   write_plugin('C', buf);
   snprintf(buf, BIGBUFLEN, "%s=test2\n", query);
   write_plugin('C', buf);
   snprintf(buf, BIGBUFLEN, "%s=test3\n", query);
   write_plugin('C', buf);

   /* this is the end of all data */
   signal_eod();
}

/*
 * The main and universal restore procedure
 */
void perform_restore()
{
   bool loopgo = true;
   bool restore_skip_create = false;
   bool restore_with_core = false;
   bool restore_skip_metadata = false;

   if (regress_error_restore_stderr) {
      // test some stderror handling
      errno = EACCES;
      perror("I've got some unsuspected error which I'd like to display on stderr (COMM_STDERR)");
   }

   /* Restore Loop (5) */
   LOG("#> Restore Loop.");
   while (true) {
      read_plugin(buf);
      /* check if FINISH job */
      if (strcmp(buf, "FINISH\n") == 0) {
         LOG("#> finish files.");
         break;
      }
      /* check for ACL command */
      if (strcmp(buf, "ACL\n") == 0) {
         while (read_plugin(buf) > 0);
         LOG("#> ACL data saved.");
         write_plugin('I', "TEST5R - acl data saved.");
         write_plugin('C', "OK\n");
         continue;
      }

      /* check for XATTR command */
      if (strcmp(buf, "XATTR\n") == 0) {
         while (read_plugin(buf) > 0);
         LOG("#> XATTR data saved.");
         write_plugin('I', "TEST5R - xattr data saved.");
         write_plugin('C', "OK\n");
         continue;
      }
      /* check if FNAME then follow file parameters */
      if (strncmp(buf, "FNAME:", 6) == 0) {
         restore_with_core = strstr(buf, "/_restore_with_core/") != NULL && (strstr(buf, "/etc/issue") != NULL || strstr(buf, "/fileforcore") != NULL);
         restore_skip_create = strstr(buf, "/_restore_skip_create/") != NULL;
         restore_skip_metadata = strstr(buf, "/_restore_skip_metadata/") != NULL;

         /* we read here a file parameters */
         while (read_plugin(buf) > 0);

         if (restore_skip_create){
            // simple skipall
            write_plugin('I', "TEST5R - create file skipped.");
            write_plugin('C', "SKIP\n");
            continue;
         }

         if (restore_with_core){
            // signal Core
            write_plugin('I', "TEST5R - handle file with Core.");
            write_plugin('C', "CORE\n");
            continue;
         }

         // signal OK
         write_plugin('I', "TEST5R - create file ok.");
         write_plugin('C', "OK\n");
         continue;
      }

      /* check for METADATA stream */
      if (strncmp(buf, "METADATA_STREAM", 15) == 0){
         // handle metadata
         read_plugin_data_stream();
         /* signal OK */
         LOG("#> METADATA_STREAM data saved.");

         if (restore_skip_metadata){
            write_plugin('I', "TEST5R - metadata select skip restore.");
            write_plugin('C', "SKIP\n");
            continue;
         }

         write_plugin('I', "TEST5R - metadata saved.");
         write_plugin('C', "OK\n");
         continue;
      }

      /* check if DATA command, so read the data packets */
      if (strcmp(buf, "DATA\n") == 0){
         int len = read_plugin(buf);
         if (len == 0){
            /* empty file to restore */
            LOG("#> Empty file.");
            continue;
         } else {
            LOG("#> file data saved.");
         }
         loopgo = true;
         int fsize = len;
         while (loopgo){
            len = read_plugin(buf);
            fsize += len;
            if (len > 0){
               LOG("#> file data saved.");
               continue;
            } else {
               loopgo = false;
               snprintf(buflog, 4096, "#> file data END = %i", fsize);
            }
         }
         /* confirm restore ok */
         write_plugin('I', "TEST5R - end of data.");
         write_plugin('C', "OK\n");
      } else {
         write_plugin('E', "Error DATA command required.");
         exit (1);
      }
   }

   /* this is the end of all data */
   signal_eod();
}

/*
 * Start here
 */
int main(int argc, char** argv) {

   int len;
   char *listing;
   char *query;

   buf = (char*)malloc(BIGBUFLEN);
   if (buf == NULL){
      exit(255);
   }
   buflog = (char*)malloc(BUFLEN);
   if (buflog == NULL){
      exit(255);
   }
   listing = (char*)malloc(BUFLEN);
   if (listing == NULL){
      exit(255);
   }
   query = (char*)malloc(BUFLEN);
   if (query == NULL){
      exit(255);
   }

   mypid = getpid();
   snprintf(buf, 4096, "%s/%s_backend_%d.log", LOGDIR, PLUGINNAME, mypid);
   logfd = open(buf, O_CREAT|O_TRUNC|O_WRONLY, 0640);
   if (logfd < 0){
      exit (1);
   }
   //sleep(30);

   /* handshake (1) */
   len = read_plugin(buf);
#if 1
   write_plugin('C',"Hello Bacula\n");
#else
   write_plugin('E',"Invalid Plugin name.");
   goto Term;
#endif

   /* Job Info (2) */
   while ((len = read_plugin(buf)) > 0);
   write_plugin('I', "TEST2");
   signal_eod();

   /* Plugin Params (3) */
   while ((len = read_plugin(buf)) > 0)
   {
      // "regress_error_plugin_params",
      // "regress_error_start_job",
      // "regress_error_backup_no_files",
      // "regress_error_backup_stderr",
      // "regress_error_estimate_stderr",
      // "regress_error_listing_stderr",
      // "regress_error_restore_stderr",
      // "regress_backup_plugin_objects",
      // "regress_error_backup_abort",
      // "regress_standard_error_backup",
      if (strcmp(buf, "regress_error_plugin_params=1\n") == 0){
         regress_error_plugin_params = true;
         continue;
      }
      if (strcmp(buf, "regress_error_start_job=1\n") == 0){
         regress_error_start_job = true;
         continue;
      }
      if (strcmp(buf, "regress_error_backup_no_files=1\n") == 0){
         regress_error_backup_no_files = true;
         continue;
      }
      if (strcmp(buf, "regress_error_backup_stderr=1\n") == 0){
         regress_error_backup_stderr = true;
         continue;
      }
      if (strcmp(buf, "regress_error_estimate_stderr=1\n") == 0){
         regress_error_estimate_stderr = true;
         continue;
      }
      if (strcmp(buf, "regress_error_listing_stderr=1\n") == 0){
         regress_error_listing_stderr = true;
         continue;
      }
      if (strcmp(buf, "regress_error_restore_stderr=1\n") == 0){
         regress_error_restore_stderr = true;
         continue;
      }
      if (strcmp(buf, "regress_backup_plugin_objects=1\n") == 0){
         regress_backup_plugin_objects = true;
         continue;
      }
      if (strcmp(buf, "regress_error_backup_abort=1\n") == 0){
         regress_error_backup_abort = true;
         continue;
      }
      if (strcmp(buf, "regress_backup_other_file=1\n") == 0){
         regress_backup_other_file = true;
         continue;
      }
      if (strcmp(buf, "regress_metadata_support=1\n") == 0){
         regress_metadata_support = true;
         continue;
      }
      if (strcmp(buf, "regress_standard_error_backup=1\n") == 0){
         regress_standard_error_backup = true;
         continue;
      }
      if (sscanf(buf, "listing=%s\n", buf) == 1){
         strcpy(listing, buf);
         continue;
      }
      if (sscanf(buf, "query=%s\n", buf) == 1){
         strcpy(query, buf);
         continue;
      }
   }
   write_plugin('I', "TEST3");
   if (!regress_error_plugin_params){
      signal_eod();
   } else {
      write_plugin('E', "We do not accept your TEST3E! AsRequest.");
   }

   /* Start Backup/Estimate/Restore (4) */
   len = read_plugin(buf);
   write_plugin('I', "TEST4");

   if (regress_error_start_job){
      write_plugin('A', "We do not accept your TEST4E! AsRequest.");
      goto Term;
   }

   signal_eod();

   /* check what kind of Job we have */
   buf[len] = 0;
   if (strcmp(buf, "BackupStart\n") == 0){
      perform_backup();
   } else
   if (strcmp(buf, "EstimateStart\n") == 0){
      perform_estimate();
   } else
   if (strcmp(buf, "ListingStart\n") == 0){
      perform_listing(listing);
   } else
   if (strcmp(buf, "QueryStart\n") == 0){
      perform_queryparam(query);
   } else
   if (strcmp(buf, "RestoreStart\n") == 0){
      perform_restore();
   }

   /* End Job */
   len = read_plugin(buf);
   write_plugin('I', "TESTEND");
   signal_eod();
   len = read_plugin(buf);

Term:
   signal_term();
   LOG("#> Terminating backend.");
   close(logfd);
   free(buf);
   free(buflog);
   free(listing);
   return (EXIT_SUCCESS);
}
