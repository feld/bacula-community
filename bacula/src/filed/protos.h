/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2009 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 *   Version $Id$
 */

extern bool blast_data_to_storage_daemon(JCR *jcr, char *addr);
extern void do_verify_volume(JCR *jcr);
extern void do_restore(JCR *jcr);
extern int authenticate_director(JCR *jcr);
extern int authenticate_storagedaemon(JCR *jcr);
extern int make_estimate(JCR *jcr);

/* From verify.c */
int digest_file(JCR *jcr, FF_PKT *ff_pkt, DIGEST *digest);
void do_verify(JCR *jcr);

/* From heartbeat.c */
void start_heartbeat_monitor(JCR *jcr);
void stop_heartbeat_monitor(JCR *jcr);
void start_dir_heartbeat(JCR *jcr);
void stop_dir_heartbeat(JCR *jcr);

/* From acl.c */
bool build_acl_streams(JCR *jcr, FF_PKT *ff_pkt);
bool parse_acl_stream(JCR *jcr, int stream);

/* from accurate.c */
bool accurate_send_deleted_list(JCR *jcr);
bool accurate_check_file(JCR *jcr, FF_PKT *ff_pkt);
bool accurate_mark_file_as_seen(JCR *jcr, char *fname);

/* from backup.c */
bool encode_and_send_attributes(JCR *jcr, FF_PKT *ff_pkt, int &data_stream);
void strip_path(FF_PKT *ff_pkt);
void unstrip_path(FF_PKT *ff_pkt);

/* from xattr.c */
bool build_xattr_streams(JCR *jcr, FF_PKT *ff_pkt);
bool parse_xattr_stream(JCR *jcr, int stream);
