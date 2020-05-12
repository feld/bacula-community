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

#ifndef DEDUP_INTERFACE_H_
#define DEDUP_INTERFACE_H_

class BufferedMsgSD: public DDEConnectionBackup, public BufferedMsgBase
{
public:
   POOL_MEM block; // used for BNET_CMD_STO_BLOCK
   BufferedMsgSD(DedupEngine *dedupengine, JCR *jcr, BSOCK *sock, const char *a_rec_header, int32_t bufsize, int capacity, int con_capacity);
   virtual ~BufferedMsgSD();
   virtual void *do_read_sock_thread(void);
   virtual int commit(POOLMEM *&errmsg, uint32_t jobid)
         { return DDEConnectionBackup::Commit(errmsg, jobid); };
   virtual bool dedup_store_chunk(DEV_RECORD *rec, const char *rbuf, int rbuflen, char *dedup_ref_buf, char *wdedup_ref_buf, POOLMEM *&errmsg);
};


/*
 * DedupStoredInterface is the JCR->dedup component in charge of handling DEDUP
 * and REHYDRATION on the Storage side
 *
 * When doing backup, only the dedup part is in use, the rehydration part is not
 * used but don't take any resources. The same is true for the opposite.
 *
 */
class DedupStoredInterface: public DedupStoredInterfaceBase, DDEConnectionRestore
{
public:
   enum { di_rehydration_srv=1 };
   JCR *jcr;

private:
   bool _is_rehydration_srvside;
   bool _is_thread_started;
   POOLMEM *msgbuf;
   POOLMEM *eblock;

public:

   pthread_mutex_t mutex;
   pthread_cond_t cond;

   bool emergency_exit;
   bool is_eod_sent;  /* end of backup, EOD has been sent */

   /* deduplication */

   /* rehydration */
   int client_rec_capacity;  /* size of the buffer client on the other side, */
                             /* don't send more rec before an ACK */
   int64_t sent_rec_count, recv_ack_count; /* keep count of rec sent and received ACK */
   pthread_t rehydration_thread;

   bool do_checksum_after_rehydration; /* compute checksum after the rehydration ? */
   bool use_index_for_recall;          /* Use index to restore a block (or use the volume information) */
   int rehydra_check_hash;       /* copied from DedupEngine */
   POOL_MEM m_errmsg;           /* error buffer used in record_rehydration */

   struct DedupReference *circular_ref;
   int circular_ref_count; /* number of ref in the circular buffer */
   int circular_ref_pos; /* where to store the next ref sent to the "other" */
   int circular_ref_search; /* pos of the last hit, start searching from here */

   DedupStoredInterface(JCR *jcr, DedupEngine *dedupengine);
   ~DedupStoredInterface();

   /* rehydration / restore */
   int start_rehydration();                      /* Start rehydration thread */
   void *wait_rehydration(bool emergency=false); /* Stop rehydration thread */
   void *do_rehydration_thread(void);            /* Actual thread startup function */
   int handle_rehydration_command(BSOCK *fd);
   bool wait_flowcontrol_rehydration(int free_rec_count, int timeoutms);
   bool do_flowcontrol_rehydration(int free_rec_count, int retry_timeoutms=250);

   void warn_rehydration_eod();        /* tell rehydration than EOD was sent */
   int record_rehydration(DCR *dcr, DEV_RECORD *rec, char *buf, POOLMEM *&errmsg, bool despite_of_error, int *chunk_size);
   int add_circular_buf(DCR *dcr, DEV_RECORD *rec);

   /* Some tools like bextract may want to test the checksum after a rehydration. */
   void set_checksum_after_rehydration(bool val) {
      do_checksum_after_rehydration = val;
   };

   /* Use the index during restore, or the volume information */
   void set_use_index_for_recall(bool val) {
      use_index_for_recall = val;
   };

   POOLMEM *get_msgbuf() { return msgbuf; };
   bool is_rehydration_srvside() { return _is_rehydration_srvside; };
   bool is_thread_started() { return _is_thread_started; };

   void set_rehydration_srvside() { _is_rehydration_srvside = true; };
   void unset_rehydration_srvside() { _is_rehydration_srvside = false; };

};

#endif /* DEDUP_INTERFACE_H_ */
