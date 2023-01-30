<?php
/*
 * Bacula(R) - The Network Backup Solution
 * Baculum   - Bacula web interface
 *
 * Copyright (C) 2013-2023 Kern Sibbald
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

use Baculum\Common\Modules\Errors\GenericError;
use Baculum\Common\Modules\Errors\StorageError;


/**
 * Cloud storage volume truncate endpoint.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
class StorageCloudTruncate extends BaculumAPIServer {

	public function get() {
		$misc = $this->getModule('misc');
		$storageid = $this->Request->contains('id') ? (int)$this->Request['id'] : 0;
		$storage = $this->getModule('storage')->getStorageById($storageid);
		$volume = $this->Request->contains('volume') && $misc->isValidName($this->Request['volume']) ? $this->Request['volume'] : null;
		$pool = $this->Request->contains('pool') && $misc->isValidName($this->Request['pool']) ? $this->Request['pool'] : null;
		$allpools = $this->Request->contains('allpools') && $misc->isValidBooleanTrue($this->Request['allpools']) ? true : null;
		$allfrompool = $this->Request->contains('allfrompool') && $misc->isValidBooleanTrue($this->Request['allfrompool']) ? true : null;
		$mediatype = $this->Request->contains('mediatype') && $misc->isValidName($this->Request['mediatype']) ? $this->Request['mediatype'] : null;
		$drive = $this->Request->contains('drive') && $misc->isValidInteger($this->Request['drive']) ? (int)$this->Request['drive'] : null;
		$slots = $this->Request->contains('slots') && $misc->isValidRange($this->Request['slots']) ? $this->Request['slots'] : null;
		$result = $this->getModule('bconsole')->bconsoleCommand(
			$this->director,
			array('.storage')
		);

		$storage_exists = false;
		if ($result->exitcode === 0) {
			$storage_exists = (is_object($storage) && in_array($storage->name, $result->output));
		}

		if ($storage_exists == false) {
			// Storage doesn't exist or is not available for user because of ACL restrictions
			$this->output = StorageError::MSG_ERROR_STORAGE_DOES_NOT_EXISTS;
			$this->error = StorageError::ERROR_STORAGE_DOES_NOT_EXISTS;
			return;
		}

		$out = (object)['output' => [], 'error' => 0];
		$params = [
			'storage' => $storage->name
		];
		if (is_string($volume)) {
			$params['volume'] = $volume;
		}
		if (is_bool($allpools)) {
			$params['allpools'] = null;
		}
		if (is_bool($allfrompool)) {
			$params['allfrompool'] = null;
		}
		if (is_string($pool)) {
			$params['pool'] = $pool;
		}
		if (is_string($mediatype)) {
			$params['mediatype'] = $mediatype;
		}
		if (is_int($drive)) {
			$params['drive'] = $drive;
		}
		// Uncomment when slots parameter will be supported
		/*
		if (is_string($slots)) {
			$params['slots'] = $slots;
		}
		*/
		$out = $this->getRawOutput($params);
		$this->output = $out['output'];
		$this->error = $out['error'];
	}

	protected function getRawOutput($params = []) {
		$cmd = ['cloud', 'truncate'];
		foreach ($params as $key => $val) {
			if (is_null($val)) {
				$cmd[] = $key;
			} else {
				$cmd[] = $key . '="' . $val . '"';
			}
		}
		// traditional cloud storage truncate volume output
		$result = $this->getModule('bconsole')->bconsoleCommand(
			$this->director,
			$cmd
		);
		$error = $result->exitcode == 0 ? $result->exitcode : GenericError::ERROR_WRONG_EXITCODE;
		$ret = [
			'output' => $result->output,
			'error' => $error
		];
		return $ret;
	}
}
