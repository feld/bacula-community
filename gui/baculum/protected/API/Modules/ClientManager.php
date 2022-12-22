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

namespace Baculum\API\Modules;

use Prado\Data\ActiveRecord\TActiveRecordCriteria;

/**
 * Client manager module.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category Module
 * @package Baculum API
 */
class ClientManager extends APIModule {

	public function getClients($limit_val = 0, $offset_val = 0) {
		$criteria = new TActiveRecordCriteria;
		if(is_numeric($limit_val) && $limit_val > 0) {
			$criteria->Limit = $limit_val;
		}
		if (is_int($offset_val) && $offset_val > 0) {
			$criteria->Offset = $offset_val;
		}
		return ClientRecord::finder()->findAll($criteria);
	}

	public function getClientByName($name) {
		return ClientRecord::finder()->findByName($name);
	}

	public function getClientById($id) {
		return ClientRecord::finder()->findByclientid($id);
	}
}
?>
