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

use Baculum\Common\Modules\Errors\ObjectError;

/**
 * Objects endpoint.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
class Objects extends BaculumAPIServer {

	public function get() {
		$misc = $this->getModule('misc');
		$limit = $this->Request->contains('limit') ? intval($this->Request['limit']) : 0;
		$objecttype = $this->Request->contains('objecttype') && $misc->isValidName($this->Request['objecttype']) ? $this->Request['objecttype'] : null;
		$objectname = $this->Request->contains('objectname') && $misc->isValidName($this->Request['objectname']) ? $this->Request['objectname'] : null;
		$objectcategory = $this->Request->contains('objectcategory') && $misc->isValidName($this->Request['objectcategory']) ? $this->Request['objectcategory'] : null;
		$objectsource = $this->Request->contains('objectsource') && $misc->isValidName($this->Request['objectsource']) ? $this->Request['objectsource'] : null;
		$objectuuid = $this->Request->contains('objectuuid') && $misc->isValidName($this->Request['objectuuid']) ? $this->Request['objectuuid'] : null;
		$objectstatus = $this->Request->contains('objectstatus') && $misc->isValidState($this->Request['objectstatus']) ? $this->Request['objectstatus'] : null;
		$jobname = $this->Request->contains('jobname') && $misc->isValidName($this->Request['jobname']) ? $this->Request['jobname'] : null;

		$params = [];
		if (!empty($objecttype)) {
			$params['Object.ObjectType']['operator'] = '';
			$params['Object.ObjectType']['vals'] = $objecttype;
		}
		if (!empty($objectname)) {
			$params['Object.ObjectName']['operator'] = '';
			$params['Object.ObjectName']['vals'] = $objectname;
		}
		if (!empty($objectcategory)) {
			$params['Object.ObjectCategory']['operator'] = '';
			$params['Object.ObjectCategory']['vals'] = $objectcategory;
		}
		if (!empty($objectsource)) {
			$params['Object.ObjectSource']['operator'] = '';
			$params['Object.ObjectSource']['vals'] = $objectsource;
		}
		if (!empty($objectuuid)) {
			$params['Object.ObjectUUID']['operator'] = '';
			$params['Object.ObjectUUID']['vals'] = $objectuuid;
		}
		if (!empty($objectstatus)) {
			$params['Object.ObjectStatus']['operator'] = '';
			$params['Object.ObjectStatus']['vals'] = $objectstatus;
		}
		if (!empty($jobname)) {
			$params['Job.Name']['operator'] = '';
			$params['Job.Name']['vals'] = $jobname;
		}
		$objects = $this->getModule('object')->getObjects($params, $limit);
		$this->output = $objects;
		$this->error = ObjectError::ERROR_NO_ERRORS;
	}
}
