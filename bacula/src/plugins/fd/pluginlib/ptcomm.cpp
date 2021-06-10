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
 * @file ptcomm.cpp
 * @author Radosław Korzeniewski (radoslaw@korzeniewski.net)
 * @brief This is a process communication lowlevel library for Bacula plugin.
 * @version 2.0.0
 * @date 2020-11-20
 *
 * @copyright Copyright (c) 2020 All rights reserved. IP transferred to Bacula Systems according to agreement.
 */

#include "ptcomm.h"
#include <sys/stat.h>
#include <signal.h>


/*
 * libbac uses its own sscanf implementation which is not compatible with
 * libc implementation, unfortunately.
 * use bsscanf for Bacula sscanf flavor
 */
#ifdef sscanf
#undef sscanf
#endif
// #define NEED_REVIEW

/*
 * Closes external pipe if available (opened).
 *
 * in:
 *    bpContext - for Bacula debug and jobinfo messages
 * out:
 *    true - when closed without a problem
 *    false - when got any error
 */
bool PTCOMM::close_extpipe(bpContext *ctx)
{
   /* close expipe if used */
   if (extpipe > 0){
      int rc = close(extpipe);
      extpipe = -1;
      if (rc != 0){
         berrno be;
         DMSG(ctx, DERROR, "Cannot close ExtPIPE. Err=%s\n", be.bstrerror());
         JMSG(ctx, M_ERROR, "Cannot close ExtPIPE. Err=%s\n", be.bstrerror());
         return false;
      }
   }
   return true;
}

/*
 * Terminate the connection represented by BPIPE object.
 *    it shows a debug and job messages when connection close is unsuccessful
 *    and when ctx is available only.
 *
 * in:
 *    bpContext - Bacula Plugin context required for debug/job messages to show,
 *                it could be NULL in this case no messages will be shown
 * out:
 *    none
 */
void PTCOMM::terminate(bpContext *ctx)
{
   if (is_closed())
      return;

   pid_t worker_pid = bpipe->worker_pid;
   int status = close_bpipe(bpipe);

   bpipe = NULL;     // indicte closed bpipe

   if (status && ctx)
   {
      /* error during close */
      berrno be;
      DMSG(ctx, DERROR, "Error closing backend. Err=%s\n", be.bstrerror(status));
      JMSG(ctx, M_ERROR, "Error closing backend. Err=%s\n", be.bstrerror(status));
   }

   if (worker_pid)
   {
      /* terminate the backend */
      kill(worker_pid, SIGTERM);
   }

   if (extpipe > 0)
      close_extpipe(ctx);
};

/**
 * @brief Reads `nbytes` of data from backend into a buffer `buf`.
 *
 * This is a dedicated method for reading raw data from backend.
 * It reads exact `nbytes` number of bytes and stores it at `buf`.
 * It will not return until all requested data is ready or got error.
 * You have to use it when you known exact number of bytes to read from
 * the backend. The method handles errors and timeout reading data.
 *
 * @param ctx - for Bacula debug jobinfo messages
 * @param buf - the memory buffer where we will read data
 * @param nbytes - the exact number of bytes to read into `buf`
 * @return true - when read was successful
 * @return false - on any error
 */
bool PTCOMM::recvbackend_data(bpContext *ctx, char *buf, int32_t nbytes)
{
   int status;
   int rbytes = 0;

   _timeout.tv_sec = PTCOMM_DEFAULT_TIMEOUT;
   _timeout.tv_usec = 0;

   while (nbytes)
   {
      fd_set rfds;

      FD_ZERO(&rfds);
      FD_SET(rfd, &rfds);
      FD_SET(efd, &rfds);

      status = select(maxfd, &rfds, NULL, NULL, &_timeout);
      if (status == 0)
      {
         // this means timeout waiting
         f_error = true;
         DMSG1(ctx, DERROR, "BPIPE read timeout=%d.\n", PTCOMM_DEFAULT_TIMEOUT);
         JMSG1(ctx, is_fatal() ? M_FATAL : M_ERROR, "BPIPE read timeout=%d.\n", PTCOMM_DEFAULT_TIMEOUT);
         return false;
      }

      // check if any data on error channel
      if (FD_ISSET(efd, &rfds))
      {
         // do read of error channel
         f_error = true;
         status = read(efd, errmsg.c_str(), errmsg.size() - 1);
         errmsg.c_str()[status] = '\0'; // terminate string
         strip_trailing_junk(errmsg.c_str());
         if (status < 0)
         {
            /* show any error during message read */
            berrno be;
            DMSG(ctx, DERROR, "BPIPE read error on error channel: ERR=%s\n", be.bstrerror());
            JMSG(ctx, is_fatal() ? M_FATAL : M_ERROR, "BPIPE read error on error channel: ERR=%s\n", be.bstrerror());
         } else {
            // got data on error channel, report it
            DMSG1(ctx, DERROR, "Backend reported error: %s\n", errmsg.c_str());
            JMSG1(ctx, is_fatal() ? M_FATAL : M_ERROR, "Backend reported error: %s\n", errmsg.c_str());
         }
      }

      // check if data descriptor is ready
      if (FD_ISSET(rfd, &rfds))
      {
         // do read of data
         status = read(rfd, buf + rbytes, nbytes);
         if (status < 0)
         {
            /* show any error during data read */
            berrno be;
            f_error = true;
            DMSG(ctx, DERROR, "BPIPE read error: ERR=%s\n", be.bstrerror());
            JMSG(ctx, is_fatal() ? M_FATAL : M_ERROR, "BPIPE read error: ERR=%s\n", be.bstrerror());
            return false;
         }
         if (status == 0){
            /* the backend closed the connection without terminate signal 'T' */
            f_error = true;
            DMSG0(ctx, DERROR, "Backend closed the connection.\n");
            JMSG0(ctx, is_fatal() ? M_FATAL : M_ERROR, "Backend closed the connection.\n");
            return false;
         }
         nbytes -= status;
         rbytes += status;
      }
   }

   return true;
}

/**
 * @brief
 *
 * @param ctx
 * @param buf
 * @param nbytes
 * @return true
 * @return false
 */
bool PTCOMM::sendbackend_data(bpContext *ctx, POOLMEM *buf, int32_t nbytes)
{
   int status;
   int wbytes = 0;

   _timeout.tv_sec = PTCOMM_DEFAULT_TIMEOUT;
   _timeout.tv_usec = 0;

   while (nbytes)
   {
      fd_set rfds;
      fd_set wfds;

      FD_ZERO(&rfds);
      FD_ZERO(&wfds);
      FD_SET(efd, &rfds);
      FD_SET(wfd, &wfds);

      status = select(maxfd, &rfds, &wfds, NULL, &_timeout);
      if (status == 0)
      {
         // this means timeout waiting
         f_error = true;
         DMSG1(ctx, DERROR, "BPIPE write timeout=%d.\n", _timeout.tv_sec);
         JMSG1(ctx, is_fatal() ? M_FATAL : M_ERROR, "BPIPE write timeout=%d.\n", _timeout.tv_sec);
         return false;
      }

      // check if any data on error channel
      if (FD_ISSET(efd, &rfds))
      {
         // do read of error channel
         f_error = true;
         status = read(efd, errmsg.c_str(), errmsg.size());
         if (status < 0)
         {
            /* show any error during message read */
            berrno be;
            DMSG(ctx, DERROR, "BPIPE read error on error channel: ERR=%s\n", be.bstrerror());
            JMSG(ctx, is_fatal() ? M_FATAL : M_ERROR, "BPIPE read error on error channel: ERR=%s\n", be.bstrerror());
         } else {
            // got data on error channel, report it
            DMSG1(ctx, DERROR, "Backend reported error: %s\n", errmsg.c_str());
            JMSG1(ctx, is_fatal() ? M_FATAL : M_ERROR, "Backend reported error: %s\n", errmsg.c_str());
         }
      }

      // check if data descriptor is ready
      if (FD_ISSET(wfd, &wfds))
      {
         // do write of data
         status = write(wfd, buf + wbytes, nbytes);
         if (status < 0)
         {
            /* show any error during data write */
            berrno be;
            f_error = true;
            DMSG(ctx, DERROR, "BPIPE write error: ERR=%s\n", be.bstrerror());
            JMSG(ctx, is_fatal() ? M_FATAL : M_ERROR, "BPIPE write error: ERR=%s\n", be.bstrerror());
            return false;
         }
         nbytes -= status;
         wbytes += status;
      }
   }

   return true;
}

/**
 * @brief Reads a protocol header from backend and return payload length.
 *
 * This method should be used at the start of every read from backend.
 * It handles a full protocol chatting, i.e. error, warning and information
 * messages besides EOD or termination.
 *
 * @param ctx - for Bacula debug jobinfo messages
 * @param cmd - an expected command to read: `C` or `D`
 * @return int32_t - the size of the packet payload
 */
int32_t PTCOMM::recvbackend_header(bpContext *ctx, char cmd)
{
   if (is_closed()){
      DMSG0(ctx, DERROR, "BPIPE to backend is closed, cannot receive data.\n");
      JMSG0(ctx, is_fatal() ? M_FATAL : M_ERROR, "BPIPE to backend is closed, cannot receive data.\n");
      return -1;
   }

   PTHEADER header;
   bool workdone = false;

   f_eod = f_error = f_fatal = false;
   int32_t nbytes = sizeof(PTHEADER);

   while (!workdone){
      if (!recvbackend_data(ctx, (char*)&header, nbytes))
      {
         DMSG0(ctx, DERROR, "PTCOMM cannot get packet header from backend.\n");
         JMSG0(ctx, M_FATAL, "PTCOMM cannot get packet header from backend.\n");
         f_eod = f_error = f_fatal = true;
         return -1;
      }

      // some packet commands require data
      header.length[6] = 0; /* end of string */

      DMSG2(ctx, DDEBUG, "HEADERRECV: %c %s\n", header.status, header.length);

      /* check for protocol status */
      if (header.status == 'F'){
         /* signal EOD */
         f_eod = true;
         return 0;
      }

      if (header.status == 'T'){
         /* backend signaled a connection termination */
         terminate(ctx);
         return 0;
      }

      // convert packet length from ASCII to binary
      int32_t msglen = atoi(header.length);

      if (header.status == 'C' || header.status == 'D')
      {
         if (header.status != cmd)
         {
            DMSG2(ctx, DERROR, "Protocol error. Expected packet: %c got: %c\n", cmd, header.status);
            JMSG2(ctx, M_FATAL, "Protocol error. Expected packet: %c got: %c\n", cmd, header.status);
            return -1;
         }

         // this means no additional handling required
         return msglen;
      }

      // need a space for nul and newline char at the end of the message
      errmsg.check_size(msglen + 2);

      // read the rest of the package
      if (!recvbackend_data(ctx, errmsg.c_str(), msglen))
      {
         DMSG0(ctx, DERROR, "PTCOMM cannot get message from backend.\n");
         JMSG0(ctx, M_FATAL, "PTCOMM cannot get message from backend.\n");
         return -1;
      }

      // ensure error message is terminated with newline and terminated with standard c-string nul
      scan_and_terminate_str(errmsg, msglen);

      switch (header.status)
      {
      /* backend signal errors */
      case 'E':
      case 'A':
         /* setup error flags */
         f_error = true;
         f_fatal = header.status == 'A';

         /* show error to Bacula */
         DMSG(ctx, DERROR, "Backend Error: %s", errmsg.c_str());
         JMSG(ctx, f_fatal ? M_FATAL : M_ERROR, "%s", errmsg.c_str());
         workdone = true;
         break;

      // handle warning and info messages below
      case 'W':
         // handle warning message
         DMSG(ctx, DERROR, "%s", errmsg.c_str());
         JMSG(ctx, M_WARNING, "%s", errmsg.c_str());
         continue;

      case 'I':
         // handle information message
         DMSG(ctx, DINFO, "%s", errmsg.c_str());
         JMSG(ctx, M_INFO, "%s", errmsg.c_str());
         continue;

      case 'S':
         // handle saved message
         DMSG(ctx, DDEBUG, "%s", errmsg.c_str());
         JMSG(ctx, M_SAVED, "%s", errmsg.c_str());
         continue;

      case 'N':
         // handle not-saved message
         DMSG(ctx, DDEBUG, "%s", errmsg.c_str());
         JMSG(ctx, M_NOTSAVED, "%s", errmsg.c_str());
         continue;

      case 'R':
         // handle restored message
         DMSG(ctx, DINFO, "%s", errmsg.c_str());
         JMSG(ctx, M_RESTORED, "%s", errmsg.c_str());
         continue;

      case 'P':
         // handle skipped message
         DMSG(ctx, DINFO, "%s", errmsg.c_str());
         JMSG(ctx, M_SKIPPED, "%s", errmsg.c_str());
         continue;

      case 'O':
         // handle operator message (now it is only M_MOUNT)
         DMSG(ctx, DINFO, "%s", errmsg.c_str());
         JMSG(ctx, M_MOUNT, "%s", errmsg.c_str());
         continue;

      case 'V':
         // handle event message
         DMSG(ctx, DINFO, "%s", errmsg.c_str());
         JMSG(ctx, M_EVENTS, "%s", errmsg.c_str());
         continue;

      case 'Q':
         // handle event message
         DMSG(ctx, DERROR, "%s", errmsg.c_str());
         JMSG(ctx, M_ERROR, "%s", errmsg.c_str());
         continue;

      default:
         DMSG1(ctx, DERROR, "Protocol error. Unknown packet: %c\n", header.status);
         JMSG1(ctx, M_FATAL, "Protocol error. Unknown packet: %c\n", header.status);
         return -1;
      }
   }

   return -1;
}

/**
 * @brief Handles a receive (read) packet header.
 *
 * @param ctx bpContext - for Bacula debug jobinfo messages
 * @param cmd
 * @return int32_t
 */
int32_t PTCOMM::handle_read_header(bpContext *ctx, char cmd)
{
   // first read is the packet header where we will have info about data
   // which is sent to us; the packet header is 8 chars/bytes length fixed
   // nbytes shows how many bytes we expects to read
   int32_t length = recvbackend_header(ctx, cmd);
   if (length < 0)
   {
      // error
      DMSG0(ctx, DERROR, "PTCOMM cannot get packet header from backend.\n");
      JMSG0(ctx, is_fatal() ? M_FATAL : M_ERROR, "PTCOMM cannot get packet header from backend.\n");
      f_eod = f_error = f_fatal = true;
      return -1;
   }

   return length;
}

/**
 * @brief Handles a payload (message) which comes after the header.
 *
 * @param ctx bpContext - for Bacula debug jobinfo messages
 * @param buf - the POOLMEM buffer we will read data
 * @param nbytes - the size of the fized buffer
 * @return int32_t
 *    0: when backend sent signal, i.e. EOD or Term
 *    -1: when we've got any error; the function will report it to Bacula when
 *        ctx is not NULL
 *    <n>: the size of received message
 */
int32_t PTCOMM::handle_payload(bpContext *ctx, char *buf, int32_t nbytes)
{
   // handle raw data read as payload
   if(!recvbackend_data(ctx, buf, nbytes)){
      // error
      DMSG0(ctx, DERROR, "PTCOMM cannot get packet payload from backend.\n");
      JMSG0(ctx, is_fatal() ? M_FATAL : M_ERROR, "PTCOMM cannot get packet payload from backend.\n");
      f_eod = f_error = f_fatal = true;
      return -1;
   }
   char bindata[17];
   DMSG1(ctx, DDEBUG, "RECV> %s\n", asciidump(buf, nbytes, bindata, 17));

   return nbytes;
}

/**
 * @brief Receive a packet from the backend.
 *
 * The caller expects a packet of a particular type (`cmd`) and we return
 * from function only when we will receive this kind of packet or get any error.
 * The `buf` will be extended if message extent current buffer size.
 *
 * @param ctx bpContext - for Bacula debug jobinfo messages
 * @param cmd the packet type expected
 * @param buf the POOL_MEM buffer we will read data
 * @return int32_t
 *    0: when backend sent signal, i.e. EOD or Term
 *    -1: when we've got any error; the function will report it to Bacula when
 *        ctx is not NULL
 *    <n>: the size of received message
 */
int32_t PTCOMM::recvbackend(bpContext *ctx, char cmd, POOL_MEM &buf)
{
   // handle header
   int32_t length = handle_read_header(ctx, cmd);
   if (length < 0)
      return -1;

   // handle data payload
   if (length > 0)
   {
      // check requested buffer size
      buf.check_size(length + 1);
      return handle_payload(ctx, buf.c_str(), length);
   }

   return 0;
}

/**
 * @brief Receive a packet from the backend.
 *
 * The caller expects a packet of a particular type (`cmd`) and we return
 * from function when we will receive this kind of packet or get any error.
 * The `buf` is fixed size, so it won't be extended for larger messages.
 * In this case you have to make more calls to get all data.
 *
 * @param ctx bpContext - for Bacula debug jobinfo messages
 * @param cmd - the packet type expected
 * @param buf - the POOLMEM buffer we will read data
 * @param bufsize - the size of the fized buffer
 * @return int32_t
 *    0: when backend sent signal, i.e. EOD or Term
 *    -1: when we've got any error; the function will report it to Bacula when
 *        ctx is not NULL
 *    <n>: the size of received message
 */
int32_t PTCOMM::recvbackend_fixed(bpContext *ctx, char cmd, char *buf, int32_t bufsize)
{
   int32_t length = remaininglen;

   if (!f_cont){
      // handle header
      length = handle_read_header(ctx, cmd);
      if (length < 0)
         return -1;
   }

   // handle data payload
   if (length > 0){
      // we will need subsequent call to handle remaining data only when `buf` to short
      f_cont = length > bufsize;
      int32_t nbytes = f_cont * bufsize + (!f_cont) * length;
      remaininglen = f_cont * (length - bufsize);
      return handle_payload(ctx, buf, nbytes);
   }

   return 0;
}

/*
 * Sends packet to the backend.
 *    The protocol allows sending no more than 999999 bytes of data in one packet.
 *    If you require to send more data you have to split it in more packages, and
 *    backend has to assemble it into a larger chunk of data.
 *
 * in:
 *    bpContext - for Bacula debug and jobinfo messages
 *    cmd - the packet status to send
 *    buf - the packet contents
 *    len - the length of the contents
 * out:
 *    -1 - when encountered any error
 *    <n> - the number of bytes sent, success
 */
int32_t PTCOMM::sendbackend(bpContext *ctx, char cmd, POOLMEM *buf, int32_t len)
{
   int status;
   PTHEADER *header;
   PTHEADER myheader;

   if (is_closed()){
      DMSG0(ctx, DERROR, "BPIPE to backend is closed, cannot send data.\n");
      JMSG0(ctx, is_fatal() ? M_FATAL : M_ERROR, "BPIPE to backend is closed, cannot send data.\n");
      return -1;
   }

   if (len > 999999){
      /* message length too long, cannot send it */
      DMSG(ctx, DERROR, "Message length %i too long, cannot send data.\n", len);
      JMSG(ctx, M_FATAL, "Message length %i too long, cannot send data.\n", len);
      return -1;
   }

#ifdef NEED_REVIEW
   // The code at NEED_REVIEW uses POOLMEM abufhead reserved space for
   // packet header rendering in the same way as bsock.c do. The code was tested
   // and is working fine. No memory leakage or corruption encountered.
   // The only pros for this code is a single fwrite call for a whole message
   // instead of two fwrites (header + data) for a standard method.
   if (buf){
      // we will prepare POOLMEM for sending data so we can render header here
      header = (PTHEADER*) (buf - sizeof(PTHEADER));
   } else {
      // we will send header only
      header = &myheader;
   }
#else
   header = &myheader;
#endif
   header->status = cmd;

   if (bsnprintf(header->length, sizeof(header->length), "%06i\n", len) != 7){
      /* problem rendering packet header */
      DMSG0(ctx, DERROR, "Problem rendering packet header for command.\n");
      JMSG0(ctx, M_FATAL, "Problem rendering packet header for command.\n");
      return -1;
   }
   header->length[6] = '\n';

   char hlendata[17];
   char bindata[17];
   DMSG2(ctx, DDEBUG, "SENT: %s %s\n", asciidump((char*)header, sizeof(PTHEADER), hlendata, sizeof(hlendata)), asciidump(buf, len, bindata, sizeof(bindata)));

#ifdef NEED_REVIEW
   status = sendbackend_data(ctx, (char*)header, len + sizeof(PTHEADER));
   status -= sizeof(PTHEADER);
#else
   status = write(wfd, header, sizeof(PTHEADER));
   if (buf){
      /* we have some data or command to send */
      status = write(wfd, buf, len);
   }
#endif
   if (status < 0){
      // error
      DMSG0(ctx, DERROR, "PTCOMM cannot write packet to backend.\n");
      JMSG0(ctx, is_fatal() ? M_FATAL : M_ERROR, "PTCOMM cannot write packet to backend.\n");
      f_eod = f_error = f_fatal = true;
      return -1;
   }

#ifdef NEED_REVIEW
   // correct real payload data size
   status -= sizeof(PTHEADER);
#endif

   return status;
}

/**
 * @brief Reads the next command message from the backend communication channel.
 *
 * It expects the command, so returned data will be null terminated string
 * stripped on any unwanted junk, i.e. '\n' or 'space'.
 *
 * @param ctx bpContext - for Bacula debug and jobinfo messages
 * @param buf buffer allocated for command
 * @return int32_t
 *    -1 - when encountered any error
 *    0 - when backend sent signal, i.e. EOD or Term
 *    <n> - the number of bytes received, success
 */
int32_t PTCOMM::read_command(bpContext *ctx, POOL_MEM &buf)
{
   int32_t status = recvbackend(ctx, 'C', buf);
   if (status > 0)
   {
      /* mark end of string because every command is a string */
      buf.c_str()[status] = '\0';
      /* strip any junk in command like '\n' or trailing spaces */
      strip_trailing_junk(buf.c_str());
   }

   return status;
}

/*
 * Reads the next data message from the backend.
 *    The number of bytes received will not exceed the buffer length even when
 *    backend will send more data. In this case next call to read_data() will
 *    return the next part of the message.
 *
 * in:
 *    bpContext - for Bacula debug and jobinfo messages
 *    buf - buffer allocated for data
 *    len - the size of the allocated buffer
 * out:
 *    -1 - when encountered any error
 *    0 - when backend sent signal, i.e. EOD or Term
 *    <n> - the number of bytes received, success
 *    buf - the command string received from backend
 */
int32_t PTCOMM::read_data(bpContext *ctx, POOL_MEM &buf)
{
   int32_t status;

   if (extpipe > 0){
      status = read(extpipe, buf.c_str(), buf.size());
   } else {
      status = recvbackend(ctx, 'D', buf);
   }

   return status;
}

/*
 * Reads the next data message from the backend.
 *    The number of bytes received will not exceed the buffer length even when
 *    backend will send more data. In this case next call to read_data() will
 *    return the next part of the message.
 *
 * in:
 *    bpContext - for Bacula debug and jobinfo messages
 *    buf - buffer allocated for data
 *    len - the size of the allocated buffer
 * out:
 *    -1 - when encountered any error
 *    0 - when backend sent signal, i.e. EOD or Term
 *    <n> - the number of bytes received, success
 *    buf - the command string received from backend
 */
int32_t PTCOMM::read_data_fixed(bpContext *ctx, char *buf, int32_t len)
{
   int32_t status;

   if (extpipe > 0){
      status = read(extpipe, buf, len);
   } else {
      status = recvbackend_fixed(ctx, 'D', buf, len);
   }

   return status;
}

/*
 * Receive an acknowledge from backend (the EOD package).
 *
 * in:
 *    bpContext - for Bacula debug and jobinfo messages
 * out:
 *    True when acknowledge received
 *    False when got any error
 */
bool PTCOMM::read_ack(bpContext *ctx)
{
   POOL_MEM buf(PM_FNAME);

   if (recvbackend(ctx, 'F', buf) == 0 && f_eod)
   {
      f_eod = false;
      return true;
   }

   return false;
}

/*
 * Sends a command to the backend.
 *    The command has to be a nul terminated string.
 *
 * in:
 *    bpContext - for Bacula debug and jobinfo messages
 *    buf - a message buffer contains command to send
 * out:
 *    -1 - when encountered any error
 *    <n> - the number of bytes sent, success
 */
int32_t PTCOMM::write_command(bpContext *ctx, POOLMEM *buf)
{
   int32_t len;
   len = buf ? strlen(buf) : 0;
   return sendbackend(ctx, 'C', buf, len);
}

/*
 * Sends a raw data to backend.
 *    The length of the data should not exceed max packet size which is 999999 Byes.
 *
 * in:
 *    bpContext - for Bacula debug and jobinfo messages
 *    buf - a message buffer contains data to send
 *    len - the length of the data to send
 * out:
 *    -1 - when encountered any error
 *    <n> - the number of bytes sent, success
 */
int32_t PTCOMM::write_data(bpContext *ctx, POOLMEM *buf, int32_t len)
{
   int32_t status;

   if (extpipe > 0){
      status = write(extpipe, buf, len);
   } else {
      status = sendbackend(ctx, 'D', buf, len);
   }
   return status;
}

/*
 * Sends acknowledge to the backend which consist of the following flow:
 *    -> EOD
 *    <- OK
 *    or
 *    <- Error
 *
 * in:
 *    bpContext - for Bacula debug and jobinfo messages
 * out:
 *    True when acknowledge sent successful
 *    False when got any error
 */
bool PTCOMM::send_ack(bpContext *ctx)
{
   POOL_MEM buf(PM_FNAME);

   if (signal_eod(ctx) < 0){
      // error
      return false;
   }

   if (read_command(ctx, buf) < 0){
      // error
      return false;
   }

   // check if backend response with OK
   if (bstrcmp(buf.c_str(), "OK")){
      // great ACK confirmed
      return true;
   }

   return false;
}

/**
 * @brief Send a handshake procedure to the backend using PLUGINNAME and PLUGINAPI.
 *
 * @param ctx bpContext - for Bacula debug and jobinfo messages
 * @param pluginname - the plugin name part of the handshake
 * @param pluginapi - the protocol version of the plugin
 * @return true - when handshake successful
 * @return false - when not
 */
bool PTCOMM::handshake(bpContext *ctx, const char *pluginname, const char * pluginapi)
{
   POOL_MEM cmd(PM_FNAME);

   Mmsg(cmd, "Hello %s %s\n", pluginname, pluginapi);
   int32_t status = write_command(ctx, cmd);
   if (status > 0){
      status = read_command(ctx, cmd);
      if (status > 0){
         if (bstrcmp(cmd.c_str(), "Hello Bacula")){
            /* handshake successful */
            return true;
         } else {
            DMSG(ctx, DERROR, "Wrong backend response to Hello command, got: %s\n", cmd.c_str());
            JMSG(ctx, is_fatal() ? M_FATAL : M_ERROR, "Wrong backend response to Hello command, got: %s\n", cmd.c_str());
         }
      }
   }

   return false;
}
