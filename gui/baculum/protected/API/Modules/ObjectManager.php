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
 * Object manager module.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category Module
 * @package Baculum API
 */
class ObjectManager extends APIModule {

	public function getObjects($criteria = array(), $limit_val = null) {
		$sort_col = 'ObjectId';
		$db_params = $this->getModule('api_config')->getConfig('db');
		if ($db_params['type'] === Database::PGSQL_TYPE) {
		    $sort_col = strtolower($sort_col);
		}
		$order = ' ORDER BY ' . $sort_col . ' DESC';
		$limit = '';
		if(is_int($limit_val) && $limit_val > 0) {
			$limit = ' LIMIT ' . $limit_val;
		}

		$where = Database::getWhere($criteria);

		$sql = 'SELECT Object.*, 
Job.Name as jobname 
FROM Object 
LEFT JOIN Job USING (JobId) '
. $where['where'] . $order . $limit;

		return ObjectRecord::finder()->findAllBySql($sql, $where['params']);
	}

	public function getObjectById($objectid) {
		$params = [
			'Object.ObjectId' => [
				'vals' => $objectid,
				'operator' => ''
			]
		];
		$obj = $this->getObjects($params, 1);
		if (is_array($obj) && count($obj) > 0) {
			$obj = array_shift($obj);
		}
		return $obj;
	}
}
