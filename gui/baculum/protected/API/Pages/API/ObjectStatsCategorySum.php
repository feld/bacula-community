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
 * Object category stats endpoint.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
class ObjectStatsCategorySum extends BaculumAPIServer {

	public function get() {
		$misc = $this->getModule('misc');
		$objecttype = null;
		if ($this->Request->contains('objecttype') && $misc->isValidName($this->Request['objecttype'])) {
			$objecttype = $this->Request['objecttype'];
		}
		$objectsource = null;
		if ($this->Request->contains('objectsource') && $misc->isValidName($this->Request['objectsource'])) {
			$objectsource = $this->Request['objectsource'];
		}
		$datestart = null;
		if ($this->Request->contains('datestart') && $misc->isValidBDateAndTime($this->Request['datestart'])) {
			$datestart = $this->Request['datestart'];
		}
		$dateend = null;
		if ($this->Request->contains('dateend') && $misc->isValidBDateAndTime($this->Request['dateend'])) {
			$dateend = $this->Request['dateend'];
		}

		$objects = $this->getModule('object')->getObjectCategorySum(
			$objecttype,
			$objectsource,
			$datestart,
			$dateend
		);
		$this->output = $objects;
		$this->error = ObjectError::ERROR_NO_ERRORS;
	}
}
