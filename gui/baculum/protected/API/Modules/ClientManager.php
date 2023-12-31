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

	/**
	 * Get client list.
	 *
	 * @param mixed $limit_val result limit value
	 * @param int $offset_val result offset value
	 * @param array $criteria SQL criteria to get job list
	 * @return array clients or empty list if no client found
	 */
	public function getClients($limit_val = 0, $offset_val = 0, $criteria = []) {
		$limit = '';
		if(is_int($limit_val) && $limit_val > 0) {
			$limit = ' LIMIT ' . $limit_val;
		}
		$offset = '';
		if (is_int($offset_val) && $offset_val > 0) {
			$offset = ' OFFSET ' . $offset_val;
		}
		$where = Database::getWhere($criteria);

		$sql = 'SELECT *
FROM Client 
' . $where['where'] . $offset . $limit;

		$statement = Database::runQuery($sql, $where['params']);
		return $statement->fetchAll(\PDO::FETCH_OBJ);
	}

	public function getClientByName($name) {
		return ClientRecord::finder()->findByName($name);
	}

	public function getClientById($id) {
		return ClientRecord::finder()->findByclientid($id);
	}
}
?>
