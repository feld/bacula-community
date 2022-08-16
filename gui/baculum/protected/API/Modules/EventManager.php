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
 * Event manager module.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category Module
 * @package Baculum API
 */
class EventManager extends APIModule {

	/**
	 * Get event list.
	 *
	 * @param array $criteria list of optional query criterias
	 * @param array $time_scope time range for events time
	 * @param int|null $limit_val limit results value
	 */
	public function getEvents($criteria = [], $time_scope = [], $limit_val = null) {
		$sort_col = 'EventsId';
		$db_params = $this->getModule('api_config')->getConfig('db');
		if ($db_params['type'] === Database::PGSQL_TYPE) {
		    $sort_col = strtolower($sort_col);
		}
		$order = ' ORDER BY ' . $sort_col . ' DESC';
		$limit = '';
		if(is_int($limit_val) && $limit_val > 0) {
			$limit = ' LIMIT ' . $limit_val;
		}

		$where = Database::getWhere($criteria, true);

		$wh = [];
		if (key_exists('eventstimestart', $time_scope)) {
			$wh[] = " Events.EventsTime >= '{$time_scope['eventstimestart']} 00:00:00' ";
		}
		if (key_exists('eventstimeend', $time_scope)) {
			$wh[] = " Events.EventsTime <= '{$time_scope['eventstimeend']} 23:59:59' ";
		}
		$where['where'] .= implode(' AND ', $wh);
		if (!empty($where['where'])) {
			$where['where'] = ' WHERE ' . $where['where'];
		}

		$sql = 'SELECT Events.* FROM Events ' . $where['where'] . $order . $limit;

		return EventRecord::finder()->findAllBySql($sql, $where['params']);
	}

	/**
	 * Get single event record by eventid.
	 *
	 * @param integer $eventid event identifier
	 * @return EventRecord single event record or null on failure
	 */
	public function getEventById($eventid) {
		$params = [
			'Events.EventsId' => [
				'vals' => $eventid,
			]
		];
		$obj = $this->getEvents($params, [], 1);
		if (is_array($obj) && count($obj) > 0) {
			$obj = array_shift($obj);
		}
		return $obj;
	}
}
