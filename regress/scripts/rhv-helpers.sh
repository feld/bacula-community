#
#
# Author: Jorge Gea
# A set of useful functions to test rhv plugin
# This file should be inluced after scripts/functions
#

require_linux

. scripts/plugin-helpers.sh

# It installs the plugin
# It makes C compilation, Java compilation and Binaries copy process
rhv_plugin_install() {
	rhv_kill
    if [ -z "$RHV_NOCOMPILE" ]
    then
        MVN=${BEE_PLUGINS_REPO}/utils/mvn.wrapper
        export MVN

        RHV_JAR=${RHV_REPO}/target/bacula-rhv-plugin.jar
        RHV_BACKEND=${RHV_REPO}/bin/rhv_backend

        mkdir -p ${working}/rhv

        # Compiling java part & generation .jar
        make -C ${RHV_REPO}

        # Installing
        make -C build/src/plugins/fd install-rhv

        cp ${RHV_BACKEND} ${bin}/

        # Replace any base= line with base=${PWD}
        sed -i "/base=/c\base=${PWD}" ${bin}/rhv_backend

        mkdir -p ${lib}/
        cp ${RHV_JAR} ${lib}/
        
        rhv_truststore_check
    fi
}

# Kills any running execution of the plugin. Useful to be sure we don't have any background process that can interfere with tests 
rhv_kill() {
	RHVPID=""
	RHVPID=$(ps ax | grep java | grep rhv-plugin | grep -v grep | awk '{print $1}' | xargs -n1 | head -1)
	if [ "${RHVPID}" != "" ]
    then
    	kill -9 ${RHVPID}
    fi
}

# Based on selected RHV env vars it generates the appropriate plugin line and stores it in rhv_plugin_line var
rhv_setup_plugin_line() {
    rhv_plugin_line="rhv:";
    if [ "${RHV_SERVER}" != "" ]
    then
        rhv_plugin_line="${rhv_plugin_line} server=\\\"${RHV_SERVER}\\\""
    fi
    if [ "${RHV_TRUSTSTORE_FILE}" != "" ]
    then
        rhv_plugin_line="${rhv_plugin_line} truststore_file=\\\"${RHV_TRUSTSTORE_FILE}\\\""
    fi
    if [ "${RHV_TRUSTSTORE_PASSWORD}" != "" ]
    then
        rhv_plugin_line="${rhv_plugin_line} truststore_password=\\\"${RHV_TRUSTSTORE_PASSWORD}\\\""
    fi
    if [ "${RHV_AUTH}" != "" ]
    then
        rhv_plugin_line="${rhv_plugin_line} auth=\\\"${RHV_AUTH}\\\""
    fi
    if [ "${RHV_PROFILE}" != "" ]
    then
        rhv_plugin_line="${rhv_plugin_line} profile=${RHV_PROFILE}"
    fi
    if [ "${RHV_USER}" != "" ]
    then
        rhv_plugin_line="${rhv_plugin_line} user=\\\"${RHV_USER}\\\""
    fi
    if [ "${RHV_PASSWORD}" != "" ]
    then
        rhv_plugin_line="${rhv_plugin_line} password=\\\"${RHV_PASSWORD}\\\""
    fi
    if [ "${RHV_TARGET_VIRTUALMACHINE}" != "" ]
    then
        rhv_plugin_line="${rhv_plugin_line} target_virtualmachine=\\\"${RHV_TARGET_VIRTUALMACHINE}\\\""
    fi
    if [ "${RHV_TARGET_EXCLUDE_DISKS}" != "" ]
    then
        rhv_plugin_line="${rhv_plugin_line} target_exclude_disks=\\\"${RHV_TARGET_EXCLUDE_DISKS}\\\""
    fi
    if [ "${RHV_TARGET_EXCLUDE_VMS}" != "" ]
    then
        rhv_plugin_line="${rhv_plugin_line} target_exclude_vms=\\\"${RHV_TARGET_EXCLUDE_VMS}\\\""
    fi
    if [ "${RHV_TARGET_VIRTUALMACHINE_REGEX}" != "" ]
    then
        rhv_plugin_line="${rhv_plugin_line} target_virtualmachine_regex=\\\"${RHV_TARGET_VIRTUALMACHINE_REGEX}\\\""
    fi
    if [ "${RHV_TARGET_CONFIGURATION_ONLY}" != "" ]
    then
        rhv_plugin_line="${rhv_plugin_line} target_configuration_only=${RHV_TARGET_CONFIGURATION_ONLY}"
    fi
    if [ "${RHV_PROXY_VM}" != "" ]
    then
        rhv_plugin_line="${rhv_plugin_line} proxy_vm=${RHV_PROXY_VM}"
    fi
}

# Generates plugin line (using env vars), generates Fileset definition in $conf/FS_RHV.conf and configures the job
rhv_setup_job() {

    rhv_setup_plugin_line

    echo "" > $conf/FS_RHV.conf

    # Fileset Definition
    cat << EOF >> $conf/FS_RHV.conf

FileSet {
        Name = FS_RHV
        Include {
                Options {
                        signature = MD5
                        compression = LZO
                }
                Plugin = "${rhv_plugin_line}"
        }
}

EOF

    # Delete any FS_RHV line so we are sure only our new FS file is applied
    sed -i '/FS_RHV/d' $conf/bacula-dir.conf
    echo "@$conf/FS_RHV.conf" >> $conf/bacula-dir.conf

    # Turn on auto-label
    sed -i "s%# Label Format%  Label Format%" $conf/bacula-dir.conf

    $bperl -e 'add_attribute("$conf/bacula-dir.conf", "FileSet", "FS_RHV" , "Job", "pluginTest")'
    $bperl -e 'add_attribute("$conf/bacula-dir.conf", "Accurate", "yes", "Job", "pluginTest")'
}

# Wrapper to group common init functions for RHV testing
rhv_init_test() {

    JobName=pluginTest

    scripts/cleanup
    scripts/copy-plugin-confs

    if test "$debug" -eq 1 ; then
        RHV_DEBUG=2
    else
        RHV_DEBUG=0
    fi

	# If we have RHV_NOCOMPILE we don't check java, we don't install and we don't check plugin
    if [ -z "$RHV_NOCOMPILE" ]
    then
        check_java
        rhv_plugin_install
    fi
    
    is_var_defined "$RHV_SERVER" "RHV_SERVER"
    is_var_defined "$RHV_USER" "RHV_USER"
    is_var_defined "$RHV_PASSWORD" "RHV_PASSWORD"

    start_test

    if [ -z "$RHV_NOCOMPILE" ]
    then
        check_plugin_available "rhv-fd"
    fi
}

# Run restore command. First parameter is jobid, second one is directory to restore, third the logname
# Selected env variables control RHV restore configuration
rhv_gen_restore_command() {

    if [ "${RHV_RESTORELOCALPATH}" != "" ]
    then
        where=${RHV_RESTORELOCALPATH}
    else
        where=/
    fi

    echo "" > $tmp/${3}
    cat <<EOF > $tmp/bconcmds
@$out ${cwd}/tmp/${3}
restore jobid=${1} Client=127.0.0.1-fd where="${where}"
cd "$2"
dir
estimate
mark *
done
mod
13
EOF
    # Optional parameters
	if [ "${RHV_VM_DISKS}" != "" ]; then
	    echo "mod" >> $tmp/bconcmds
	    echo "8" >> $tmp/bconcmds
	    echo "${RHV_VM_DISKS}" >> $tmp/bconcmds
	fi
	
	if [ "${RHV_VM_EXCLUDE_DISKS}" != "" ]; then
	    echo "mod" >> $tmp/bconcmds
	    echo "9" >> $tmp/bconcmds
	    echo "${RHV_VM_EXCLUDE_DISKS}" >> $tmp/bconcmds
	fi
	if [ "${RHV_VM_CLUSTER}" != "" ]; then
	    echo "mod" >> $tmp/bconcmds
	    echo "10" >> $tmp/bconcmds
	    echo "${RHV_VM_CLUSTER}" >> $tmp/bconcmds
	fi
	if [ "${RHV_VM_STORAGE}" != "" ]; then
	    echo "mod" >> $tmp/bconcmds
	    echo "11" >> $tmp/bconcmds
	    echo "${RHV_VM_STORAGE}" >> $tmp/bconcmds
	fi
	if [ "${RHV_VM_NAME}" != "" ]; then
	    echo "mod" >> $tmp/bconcmds
	    echo "12" >> $tmp/bconcmds
	    echo "${RHV_VM_NAME}" >> $tmp/bconcmds
	fi
	if [ "${RHV_RESTORE_FORCE_OVERWRITE}" != "" ]; then
	    echo "mod" >> $tmp/bconcmds
	    echo "13" >> $tmp/bconcmds
	    echo "${RHV_RESTORE_FORCE_OVERWRITE}" >> $tmp/bconcmds
	fi
	if [ "${RHV_RESTORE_SWITCH_ON}" != "" ]; then
	    echo "mod" >> $tmp/bconcmds
	    echo "14" >> $tmp/bconcmds
	    echo "${RHV_RESTORE_SWITCH_ON}" >> $tmp/bconcmds
	fi
	if [ "${RHV_RESTORE_DISK_INTERFACE}" != "" ]; then
	    echo "mod" >> $tmp/bconcmds
	    echo "15" >> $tmp/bconcmds
	    echo "${RHV_RESTORE_DISK_INTERFACE}" >> $tmp/bconcmds
	fi
	if [ "${RHV_RESTORE_DISK_ACTIVE}" != "" ]; then
	    echo "mod" >> $tmp/bconcmds
	    echo "16" >> $tmp/bconcmds
	    echo "${RHV_RESTORE_DISK_ACTIVE}" >> $tmp/bconcmds
	fi
	if [ "${RHV_RESTORE_DISK_BOOT}" != "" ]; then
	    echo "mod" >> $tmp/bconcmds
	    echo "17" >> $tmp/bconcmds
	    echo "${RHV_RESTORE_DISK_BOOT}" >> $tmp/bconcmds
	fi
	if [ "${RHV_RESTORE_NICS}" != "" ]; then
	    echo "mod" >> $tmp/bconcmds
	    echo "18" >> $tmp/bconcmds
	    echo "${RHV_RESTORE_NICS}" >> $tmp/bconcmds
	fi
	if [ "${RHV_RESTORE_DISK_NAMES}" != "" ]; then
	    echo "mod" >> $tmp/bconcmds
	    echo "19" >> $tmp/bconcmds
	    echo "${RHV_RESTORE_DISK_NAMES}" >> $tmp/bconcmds
	fi
	if [ "${RHV_RESTORE_TEMPLATE_NAME}" != "" ]; then
	    echo "mod" >> $tmp/bconcmds
	    echo "20" >> $tmp/bconcmds
	    echo "${RHV_RESTORE_TEMPLATE_NAME}" >> $tmp/bconcmds
	fi

    cat <<EOF >> $tmp/bconcmds
yes
yes
wait
quit
EOF
}


# Run restore and check that terminates ok. 
#First parameter is jobid, second one is directory to restore and third
rhv_run_restore_and_check() {
    rhv_gen_restore_command ${1} "${2}" ${3}

    # Run restore
    run_bconsole

    sleep 5

    RESTOREID=$(grep JobId= $tmp/${3} | sed 's/.*=//')

    check_job_ok_termination ${RESTOREID}
}

rhv_truststore_check() {
	
	RHV_TRUSTSTORE_FILE="${working}/rhv/cacerts"
	
	check_file ${JAVA_TRUSTSTORE}
	cp ${JAVA_TRUSTSTORE} ${working}/rhv

	touch ${RHV_TRUSTSTORE_FILE}_tmp
	if [ ! -f ${RHV_TRUSTSTORE_FILE}_tmp ]; then
	    print_debug "Error: Check truststore directory permissions. The directory must be writable"
	    exit 1
	fi
	if [ ! -w ${RHV_TRUSTSTORE_FILE}_tmp ]; then
	    print_debug "Error: Check truststore file permissions. The file must be writable"
	    exit 1
	fi
	curl -sS -o ${RHV_TRUSTSTORE_FILE}_tmp "http://${RHV_SERVER}/ovirt-engine/services/pki-resource?resource=ca-certificate&format=X509-PEM-CA"
	if [ ! -f ${RHV_TRUSTSTORE_FILE}_tmp ]; then
	    print_debug "Error: downloading public certificate!"
	    exit 1
	fi
	EHHO=`${JAVA_KEY_TOOL} -import -alias RHV_ALIAS_TEST -file ${RHV_TRUSTSTORE_FILE}_tmp -keystore ${RHV_TRUSTSTORE_FILE} -storepass ${RHV_TRUSTSTORE_PASSWORD} -noprompt`
	
	check_file ${RHV_TRUSTSTORE_FILE}
	
	print_debug "Truststore generate successfully"
}
