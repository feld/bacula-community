<?php
/*
 * Bacula(R) - The Network Backup Solution
 * Baculum   - Bacula web interface
 *
 * Copyright (C) 2013-2019 Kern Sibbald
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

use Baculum\API\Modules\BaculumAPIServer;
use Baculum\Common\Modules\Errors\StorageError;

/**
 * Storages endpoint.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
class Storages extends BaculumAPIServer {

	public function get() {
		$misc = $this->getModule('misc');
		$limit = $this->Request->contains('limit') ? intval($this->Request['limit']) : null;
		$offset = $this->Request->contains('offset') && $misc->isValidInteger($this->Request['offset']) ? (int)$this->Request['offset'] : null;
		$storages = $this->getModule('storage')->getStorages();
		$result = $this->getModule('bconsole')->bconsoleCommand(
			$this->director,
			array('.storage')
		);
		if ($result->exitcode === 0) {
			array_shift($result->output);
			$storages_output = array();
			foreach($storages as $storage) {
				if(in_array($storage->name, $result->output)) {
					$storages_output[] = $storage;
				}
			}
			if (!is_int($offset) || $offset < 0) {
				$offset = 0;
			}
			if ($limit < 0) {
				$limit = 0;
			}
			/**
			 * Slice needs to be done here instead in the db because in the catalog can be storages
			 * that do not longer exist in the configuration.
			 */
			$storages_output = array_slice($storages_output, $offset, $limit);
			$this->output = $storages_output;
			$this->error = StorageError::ERROR_NO_ERRORS;
		} else {
			$this->output = $result->output;
			$this->error = $result->exitcode;
		}
	}
}
?>
