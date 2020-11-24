#!/bin/sh
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

if [ "x$JobBackup1" = "x" -o "x$JobBackup2" = "x" -o "x$JobBackup3" = "x" -o "x$JobBackup4" = "x" -o "x$FilesetBackup1" = "x" ]
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
@$out ${cwd}/tmp/log4.out
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

RET=`grep "jobstatus:" ${cwd}/tmp/log1.out | awk '{print $2}'`
BFILE1=`grep "$Plugin/bucket" ${cwd}/tmp/log1.out | grep vm2.iso | wc -l`
BFILE2=`grep "$Plugin/bucket" ${cwd}/tmp/log1.out | grep SHELL | wc -l`
BFILE3=`grep "$Plugin/bucket" ${cwd}/tmp/log1.out | grep lockfile | wc -l`
BFILE4=`grep "$Plugin/bucket" ${cwd}/tmp/log1.out | grep file.xattr | wc -l`
BFILE5=`grep "$Plugin/bucket" ${cwd}/tmp/log1.out | grep vmsnap.iso | wc -l`
BEND=`grep -w "TESTEND" ${cwd}/tmp/log1.out | wc -l`
if [ "x$RET" != "xT" -o $BFILE1 -ne 1 -o $BFILE2 -ne 1 -o $BFILE3 -ne 1 -o $BFILE4 -ne 1 -o $BFILE5 -ne 1 -o $BEND -ne 1 ]
then
   echo "log1" $RET $BFILE1 $BFILE2 $BFILE3 $BFILE4 $BFILE5 $BEND
   bstat=1
fi

RET=`grep "objectid:" ${cwd}/tmp/log2.out | wc -l`
if [ $RET -ne 1 ]
then
   echo "log2" $RET
   bstat=$((bstat+2))
fi

RET=`grep "jobstatus:" ${cwd}/tmp/log5.out | awk '{print $2}'`
BFILE1=`grep "$Plugin/bucket" ${cwd}/tmp/log5.out | grep vm2.iso | wc -l`
BFILE2=`grep "$Plugin/bucket" ${cwd}/tmp/log5.out | grep SHELL | wc -l`
BFILE3=`grep "$Plugin/bucket" ${cwd}/tmp/log5.out | grep lockfile | wc -l`
BFILE4=`grep "$Plugin/bucket" ${cwd}/tmp/log5.out | grep file.xattr | wc -l`
BFILE5=`grep "$Plugin/bucket" ${cwd}/tmp/log5.out | grep vmsnap.iso | wc -l`
BFILE6=`grep "$Plugin/bucket" ${cwd}/tmp/log5.out | grep vm222-other-file.iso | wc -l`
BEND=`grep -w "TESTEND" ${cwd}/tmp/log5.out | wc -l`
if [ "x$RET" != "xT" -o $BFILE1 -ne 2 -o $BFILE2 -ne 2 -o $BFILE3 -ne 2 -o $BFILE4 -ne 2 -o $BFILE5 -ne 2 -o $BFILE6 -ne 1 -o $BEND -ne 2 ]
then
   echo "log5" $RET $BFILE1 $BFILE2 $BFILE3 $BFILE4 $BFILE5 $BFILE6 $BEND
   bstat=$((bstat+4))
fi

RET=`grep "jobstatus:" ${cwd}/tmp/log6.out | awk '{print $2}'`
COMME=`grep COMM_STDERR ${cwd}/tmp/log6.out | wc -l`
BEND=`grep -w "TESTEND" ${cwd}/tmp/log6.out | wc -l`
if [ "x$RET" != "xT" -o $COMME -ne 1 -o $BEND -ne 1 ]
then
   echo "log6" $RET $COMME $BEND
   bstat=$((bstat+8))
fi

EFILE1=`grep vm1.iso ${cwd}/tmp/log3.out | wc -l`
EFILE2=`grep vm2.iso ${cwd}/tmp/log3.out | wc -l`
EFILE3=`grep lockfile ${cwd}/tmp/log3.out | wc -l`
EFILE4=`grep vmsnap.iso ${cwd}/tmp/log3.out | wc -l`
if [ $EFILE1 -ne 2 -o $EFILE2 -ne 1 -o $EFILE3 -ne 1 -o $EFILE4 -ne 1 ]
then
   echo "log3" $EFILE1 $EFILE2 $EFILE3
   estat=1
fi

LFILE1=`grep "drwxr-xr-x" ${cwd}/tmp/llog1.out | grep containers |  wc -l`
LFILE2=`grep "drwxr-xr-x" ${cwd}/tmp/llog2.out | grep bucket |  wc -l`
LFILE3=`grep "rw-r-----" ${cwd}/tmp/llog3.out | grep bucket |  wc -l`
LFILE4=`grep "rw-r-----" ${cwd}/tmp/llog4.out | grep vm2.iso |  wc -l`
LFILE5=`grep "lrwxrwxrwx" ${cwd}/tmp/llog4.out | grep vmsnap.iso |  wc -l`
if [ $LFILE1 -ne 1 -o $LFILE2 -ne 2 -o $LFILE3 -ne 2 -o $LFILE4 -ne 1 -o $LFILE5 -ne 1 ]
then
   echo "llog1" $LFILE1 $LFILE2 $LFILE3 $LFILE4 $LFILE5
   dstat=1
fi

REND=`grep -w "TESTEND" ${cwd}/tmp/log4.out | wc -l`
RET=`grep "jobstatus:" ${cwd}/tmp/log4.out | awk '{print $2}'`
if [ "x$RET" != "xT" -o $REND -ne 1 ]
then
   echo "log4" $RET $REND
   rstat=1
fi

end_test
