#
#
# Author: Jorge Gea
# A set of useful functions to test File Daemon plugin
# This file should be inluced in any {plugin}_helper.sh file
#

require_linux

# Get joblog and store it in $tmp/joblog_jobid.out. ${1} must be jobid
llist_joblog() {
    JOBLOG=joblog_${1}.out
    echo "" > $tmp/$JOBLOG
    cat <<END_OF_DATA >$tmp/bconcmds
@$out ${cwd}/tmp/${JOBLOG}
llist joblog jobid=${1}
quit
END_OF_DATA

    #job log
    run_bconsole
}

# Run bconsole reload
bconsole_reload() {
    cat <<END_OF_DATA >$tmp/bconcmds
	reload
END_OF_DATA

    run_bconsole
}

# Run cancel all in bconsole
bconsole_cancelall() {
    cat <<END_OF_DATA >$tmp/bconcmds
	cancel all yes
END_OF_DATA

    run_bconsole
}

# Receives in first parameter the jobid, and checks termination status is OK, based on the joblog
check_job_ok_termination() {

    llist_joblog ${1}

    # check job ok
    grep "  Termination:" $tmp/$JOBLOG | grep -iv Warning | grep -iv Error | grep OK > /dev/null
    if [ $? != 0 ]
    then
        print_debug "ERROR: Job ${1} ended in error according to $tmp/$JOBLOG and it should not!"
        print_debug "Listing error messages:"
        grep "Error" $tmp/$JOBLOG
        estat=1
    else
        print_debug "Job ${1} ended successfully"
    fi
}

# Receives in first parameter the jobid, and checks termination status is NOT OK, based on the joblog
check_job_fail_termination() {

    llist_joblog ${1}

    # check job failed
    grep "  Termination:" $tmp/$JOBLOG | grep -iv Warning | grep -iv Error | grep OK > /dev/null
    if [ $? != 0 ]
    then
        print_debug "OK: Job ${1} ended in error according to $tmp/$JOBLOG as expected"
        print_debug "Listing error messages:"
        grep -i "Error" $tmp/$JOBLOG
    else
        print_debug "Job ${1} ended successfully and it should not!"
        estat=1
    fi
}

# Runs job called $JobName and stores session in runjob_DATENOW.out
run_job() {
    NOW=$(date +%H%M%S)
    BACKLOG=runjob_$NOW.out

	# By default we wait the job, but receiving $2 will make a non waiting execution
    WAIT="wait"
    if [ "$2" != "" ]
    then
        WAIT=""
    fi

    echo "" > $tmp/bconcmds

	# If Debug is enabled, it enables debug also in the FD
    DEBUG_CLIENT=""
    if test "$debug" -eq 1 ; then
        DEBUG_CLIENT="setdebug level=600 trace=1 options=t client=127.0.0.1-fd"
    fi

    echo "" > $tmp/$BACKLOG
    cat <<END_OF_DATA >$tmp/bconcmds
@$out ${cwd}/tmp/$BACKLOG
$DEBUG_CLIENT
run job=$JobName level=$1 yes
$WAIT
status client
quit
END_OF_DATA

	# If bacula is already running, just run bconsole
    if ! check_bacula_running
    then
        run_bacula
    else
        run_bconsole
    fi

    BACKUPID=$(grep JobId= $tmp/$BACKLOG | sed 's/.*=//')
}

# Run jobs and check it is ok. Level is first parameter. Stores in BACKUPID the jobid
run_job_and_check() {
    run_job $1
    check_job_ok_termination ${BACKUPID}
}

# Run jobs and check it is not ok. Level is first parameter. Stores in BACKUPID the jobid
run_job_and_check_fail() {
    run_job $1
    check_job_fail_termination ${BACKUPID}
}

# Run job and cancel it after 5 secs
run_job_and_cancel() {
    run_job $1 "noWait"
    sleep 25
    bconsole_cancelall
    sleep 5
    llist_joblog ${BACKUPID}
}

# Generate a new unique name, usually used for directories
new_unique_dirname() {
    TODAY=$(date +%Y%m%d%H%M%S)
    echo "REGRESS_${TODAY}"
}

# Check we have $2 files at least in path $1 
check_n_files_in_local_path() {
    NUMRESTORED=$(find "${1}" -type f | wc -l)
    if [ ${NUMRESTORED} -lt ${2} ]
    then
        print_debug "ERROR: Not enough files found in ${1}."
        estat=1
    else
        print_debug "OK: We found enough files in ${1}: ${NUMRESTORED}"
    fi
}

# First parameter is string to look for, second is log and third is the number of files
check_n_files_in_log() {
    NUMRESTORED=$(grep -i "${1}" $tmp/${2} | wc -l)
    if [ ${NUMRESTORED} -lt ${3} ]
    then
        print_debug "ERROR: Not enough ${1} found in $tmp/${2}. Less than ${3} required"
        estat=1
    else
        print_debug "OK: We found enough ${1} in $tmp/${2}: ${NUMRESTORED}"
    fi
}

# Check $1 is not found in log $2
check_no_files_in_log() {
    NUMRESTORED=$(grep "${1}" $tmp/${2} | wc -l)
    if [ ${NUMRESTORED} -gt 1 ]
    then
        print_debug "ERROR: Files ${1} were found in $tmp/${2} and should not!"
        estat=1
    else
        print_debug "OK: File ${1} was not found in $tmp/${2}"
    fi
}

# Run ESTIMATE job, configuring previously the job, using env vars
run_estimate() {
    m365_setup_job
    
    bconsole_reload

    cat <<END_OF_DATA >$tmp/bconcmds
@$out ${cwd}/tmp/$1
estimate job=pluginTest listing
quit
END_OF_DATA

    run_bconsole
}

# Run a false restore, only to show what files are inside the backup
show_backup_contents() {
    echo "" > $tmp/${3}
    cat <<EOF > $tmp/bconcmds
@$out ${cwd}/tmp/${3}
restore jobid=${1} Client=127.0.0.1-fd where="/"
cd "$2"
dir
estimate
done
EOF

    run_bconsole
}