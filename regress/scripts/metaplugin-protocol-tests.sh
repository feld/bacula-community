#!/bin/bash
#
# Copyright (C) 2020 Radosław Korzeniewski
# License: BSD 2-Clause; see file LICENSE-FOSS
#
#
# This is a Metaplugin regression tests code functions.
# Author: Radoslaw Korzeniewski, radoslaw@korzeniewski.net
#
# It is expected to use this test just after a proper regression setup, i.e.
#
#  TestName="rhv-plugin-protocol-test"
#  JobBackup1="PluginRHEVTestProtocol1"
#  FilesetBackup1="TestPluginRHEVSetProtocol1"
#  JobBackup2="PluginRHEVTestProtocol2"
#  JobBackup3="PluginRHEVTestProtocol3"
#  JobBackup4="PluginRHEVTestProtocol4"
#  Plugin="rhv:"
#  . scripts/functions
#  scripts/cleanup
#  scripts/copy-rhv-plugin-confs
#  make -C $src/src/plugins/fd/rhv/src install-test-plugin
#  . scripts/metaplugin-protocol-tests.sh
#

if [ "x$JobBackup1" = "x" ] || [ "x$JobBackup2" = "x" ] || [ "x$JobBackup3" = "x" ] || [ "x$JobBackup4" = "x" ] || [ "x$FilesetBackup1" = "x" ]
then
   echo "You have to setup required variables!"
   exit 2
fi

start_test

# first do a backup job using test backend
cat <<END_OF_DATA >${cwd}/tmp/bconcmds
@#
@# Backup simple
@#
@output /dev/null
messages
@$out ${cwd}/tmp/log1.out
status client=$CLIENT
label storage=File volume=TestVolume001
setdebug level=500 client=$CLIENT trace=1
run job=$JobBackup1 yes
wait
status client=$CLIENT
messages
llist job=$JobBackup1
list files jobid=1
@output
quit
END_OF_DATA

run_bacula

# now test backup multiple plugin= parameters
cat <<END_OF_DATA >${cwd}/tmp/bconcmds
@#
@# Backup multiple
@#
@output /dev/null
messages
@$out ${cwd}/tmp/log5.out
status client=$CLIENT
setdebug level=500 client=$CLIENT trace=1
run job=$JobBackup3 yes
wait
status client=$CLIENT
messages
llist job=$JobBackup3
list files jobid=2
@output
quit
END_OF_DATA

run_bconsole

# now backup and verify plugin objects backup
cat <<END_OF_DATA >${cwd}/tmp/bconcmds
@#
@# Backup objects
@#
@output /dev/null
messages
@$out ${cwd}/tmp/log2.out
status client=$CLIENT
setdebug level=500 client=$CLIENT trace=1
run job=$JobBackup2 yes
wait
status client=$CLIENT
messages
llist objects job=$JobBackup2
messages
@output
quit
END_OF_DATA

run_bconsole

# now backup with stderror channel
cat <<END_OF_DATA >${cwd}/tmp/bconcmds
@#
@# Backup stderr
@#
@output /dev/null
messages
@$out ${cwd}/tmp/log6.out
status client=$CLIENT
setdebug level=500 client=$CLIENT trace=1
run job=$JobBackup4 yes
wait
status client=$CLIENT
messages
llist job=$JobBackup4
@output
quit
END_OF_DATA

run_bconsole

# now backup with metadata information
cat <<END_OF_DATA >${cwd}/tmp/bconcmds
@#
@# Backup metadata
@#
@output /dev/null
messages
@$out ${cwd}/tmp/log7.out
status client=$CLIENT
setdebug level=500 client=$CLIENT trace=1
run job=$JobBackup5 yes
wait
status client=$CLIENT
messages
llist job=$JobBackup5
@output
quit
END_OF_DATA

run_bconsole


# now test estimate job
cat <<END_OF_DATA >${cwd}/tmp/bconcmds
@#
@# Estimate
@#
@output /dev/null
messages
@$out ${cwd}/tmp/log3.out
setdebug level=500 client=$CLIENT trace=1
estimate listing job=$JobBackup1 level=Full
messages
@output
quit
END_OF_DATA

run_bconsole

# now test restore job
cat <<END_OF_DATA >${cwd}/tmp/bconcmds
@#
@# Restore
@#
@output /dev/null
messages
@$out ${cwd}/tmp/rlog1.out
setdebug level=500 client=$CLIENT trace=1
restore fileset=$FilesetBackup1 where=${cwd}/tmp select all storage=File done
yes
wait
messages
llist job=RestoreFiles
@output
quit
END_OF_DATA

run_bconsole

# restore with metadata
cat <<END_OF_DATA >${cwd}/tmp/bconcmds
@#
@# Restore
@#
@output /dev/null
messages
@$out ${cwd}/tmp/rlog2.out
setdebug level=500 client=$CLIENT trace=1
restore fileset=$FilesetBackup5 where=${cwd}/tmp select all storage=File done
yes
wait
messages
llist job=RestoreFiles
@output
quit
END_OF_DATA

run_bconsole

# and finally test listing mode
TEST=1
for ppath in / containers containers/bucket1 containers/bucket2
do

cat <<END_OF_DATA >${cwd}/tmp/bconcmds
@#
@# Listing
@#
@output /dev/null
messages
@$out ${cwd}/tmp/llog${TEST}.out
setdebug level=150 client=$CLIENT trace=1
.ls client=$CLIENT plugin="$Plugin" path="${ppath}"
messages
@output
quit
END_OF_DATA
run_bconsole
TEST=$((TEST+1))

done

stop_bacula

RET=$(grep "jobstatus:" ${cwd}/tmp/log1.out | awk '{print $2}')
BFILE1=$(grep "$Plugin/bucket" ${cwd}/tmp/log1.out | grep -c vm2.iso)
BFILE2=$(grep "$Plugin/bucket" ${cwd}/tmp/log1.out | grep -c SHELL)
BFILE3=$(grep "$Plugin/bucket" ${cwd}/tmp/log1.out | grep -c lockfile)
BFILE4=$(grep "$Plugin/bucket" ${cwd}/tmp/log1.out | grep -c file.xattr)
BFILE5=$(grep "$Plugin/bucket" ${cwd}/tmp/log1.out | grep -c vmsnap.iso)
BEND=$(grep -w -c "TESTEND" ${cwd}/tmp/log1.out)
if [ "x$RET" != "xT" ] || [ "$BFILE1" -ne 1 ] || [ "$BFILE2" -ne 1 ] || [ "$BFILE3" -ne 1 ] || [ "$BFILE4" -ne 1 ] || [ "$BFILE5" -ne 1 ] || [ "$BEND" -ne 1 ]
then
   echo "log1" $RET $BFILE1 $BFILE2 $BFILE3 $BFILE4 $BFILE5 $BEND
   bstat=1
fi

# test long fname
ENDOFNAME=$(grep -c ENDOFNAME ${cwd}/tmp/log1.out)
if [ "$ENDOFNAME" -lt 1 ]
then
   echo "ENDOFNAME" "$ENDOFNAME"
   bstat=$((bstat+2))
fi

RET=$(grep -c "objectid:" ${cwd}/tmp/log2.out)
if [ "$RET" -le 1 ]
then
   echo "log2" "$RET"
   bstat=$((bstat+4))
fi

RET=$(grep "jobstatus:" ${cwd}/tmp/log5.out | awk '{print $2}')
BFILE1=$(grep "$Plugin/bucket" ${cwd}/tmp/log5.out | grep -c vm2.iso)
BFILE2=$(grep "$Plugin/bucket" ${cwd}/tmp/log5.out | grep -c SHELL)
BFILE3=$(grep "$Plugin/bucket" ${cwd}/tmp/log5.out | grep -c lockfile)
BFILE4=$(grep "$Plugin/bucket" ${cwd}/tmp/log5.out | grep -c file.xattr)
BFILE5=$(grep "$Plugin/bucket" ${cwd}/tmp/log5.out | grep -c vmsnap.iso)
BFILE6=$(grep "$Plugin/bucket" ${cwd}/tmp/log5.out | grep -c vm222-other-file.iso)
BEND=$(grep -w -c "TESTEND" ${cwd}/tmp/log5.out)
if [ "x$RET" != "xT" ] || [ "$BFILE1" -ne 2 ] || [ "$BFILE2" -ne 2 ] || [ "$BFILE3" -ne 2 ] || [ "$BFILE4" -ne 2 ] || [ "$BFILE5" -ne 2 ] || [ "$BFILE6" -ne 1 ] || [ "$BEND" -ne 2 ]
then
   echo "log5" "$RET" "$BFILE1" "$BFILE2" "$BFILE3" "$BFILE4" "$BFILE5" "$BFILE6" "$BEND"
   bstat=$((bstat+8))
fi

RET=$(grep "jobstatus:" ${cwd}/tmp/log6.out | awk '{print $2}')
COMME=$(grep -c COMM_STDERR ${cwd}/tmp/log6.out)
BEND=$(grep -w -c "TESTEND" ${cwd}/tmp/log6.out)
if [ "x$RET" != "xT" ] || [ "$COMME" -ne 1 ] || [ "$BEND" -ne 1 ]
then
   echo "log6" "$RET" "$COMME" "$BEND"
   bstat=$((bstat+16))
fi

RET=$(grep "jobstatus:" ${cwd}/tmp/log7.out | awk '{print $2}')
META=$(grep -c "TEST14 - backup metadata" ${cwd}/tmp/log7.out)
BEND=$(grep -w -c "TESTEND" ${cwd}/tmp/log7.out)
if [ "x$RET" != "xT" ] || [ "$META" -ne 1 ] || [ "$BEND" -ne 1 ]
then
   echo "log7" "$RET" "$META" "$BEND"
   bstat=$((bstat+32))
fi

EFILE1=$(grep -c vm1.iso ${cwd}/tmp/log3.out)
EFILE2=$(grep -c vm2.iso ${cwd}/tmp/log3.out)
EFILE3=$(grep -c lockfile ${cwd}/tmp/log3.out)
EFILE4=$(grep -c vmsnap.iso ${cwd}/tmp/log3.out)
if [ "$EFILE1" -ne 2 ] || [ "$EFILE2" -ne 1 ] || [ "$EFILE3" -ne 1 ] || [ "$EFILE4" -ne 1 ]
then
   echo "log3" "$EFILE1" "$EFILE2" "$EFILE3"
   estat=1
fi

LFILE1=$(grep "drwxr-xr-x" ${cwd}/tmp/llog1.out | grep -c containers)
LFILE2=$(grep "drwxr-xr-x" ${cwd}/tmp/llog2.out | grep -c bucket)
LFILE3=$(grep "rw-r-----" ${cwd}/tmp/llog3.out | grep -c bucket)
LFILE4=$(grep "rw-r-----" ${cwd}/tmp/llog4.out | grep -c vm2.iso)
LFILE5=$(grep "lrwxrwxrwx" ${cwd}/tmp/llog4.out | grep -c vmsnap.iso)
if [ "$LFILE1" -ne 1 ] || [ "$LFILE2" -ne 2 ] || [ "$LFILE3" -ne 2 ] || [ "$LFILE4" -ne 1 ] || [ "$LFILE5" -ne 1 ]
then
   echo "llog1" "$LFILE1" "$LFILE2" "$LFILE3" "$LFILE4" "$LFILE5"
   dstat=1
fi

RET=$(grep "jobstatus:" ${cwd}/tmp/rlog1.out | awk '{print $2}')
REND=$(grep -w -c "TESTEND" ${cwd}/tmp/rlog1.out)
if [ "x$RET" != "xT" ] || [ "$REND" -ne 1 ]
then
   echo "rlog1" "$RET" "$REND"
   rstat=1
fi

RET=$(grep "jobstatus:" ${cwd}/tmp/rlog2.out | tail -1 | awk '{print $2}')
REND=$(grep -w -c "TESTEND" ${cwd}/tmp/rlog2.out)
if [ "x$RET" != "xT" ] || [ "$REND" -ne 1 ]
then
   echo "rlog2" "$RET" "$REND"
   rstat=1
fi

end_test
