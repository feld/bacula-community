<?php
/*
 * Bacula(R) - The Network Backup Solution
 * Baculum   - Bacula web interface
 *
 * Copyright (C) 2013-2022 Kern Sibbald
 *
 * The main author of Baculum is Marcin Haba.
 * The original author of Bacula is Kern Sibbald, with contributions
 * from many others, a complete list can be found in the file AUTHORS.
 *
 * You may use this file and others of this release according to the
 * license defined in the LICENSE file, which includes the Affero General
 * Public License, v3.0 ("AGPLv3") and some additional permissions and
 * terms pursuant to its AGPLv3 Section 7.
 *
 * This notice must be preserved when any source code is
 * conveyed and/or propagated.
 *
 * Bacula(R) is a registered trademark of Kern Sibbald.
 */

namespace Baculum\API\Modules;

/**
 * Object record module.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category Database
 * @package Baculum API
 */
class ObjectRecord extends APIDbModule {

	const TABLE = 'Object';

	public $objectid;
	public $jobid;
	public $path;
	public $filename;
	public $fileindex;
	public $pluginname;
	public $objecttype;
	public $objectname;
	public $objectsource;
	public $objectuuid;
	public $objectsize;
	public $objectcategory;
	public $objectstatus;
	public $objectcount;

	// external properties
	public $jobname;
	public $joberrors;
	public $jobstatus;

	public static function finder($className = __CLASS__) {
		return parent::finder($className);
	}
}
?>
