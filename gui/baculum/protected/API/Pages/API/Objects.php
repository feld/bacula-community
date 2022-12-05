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
use Baculum\API\Modules\ObjectRecord;

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
		$jobids = $this->Request->contains('jobids') && $misc->isValidIdsList($this->Request['jobids']) ? explode(',', $this->Request['jobids']) : [];
		$group_by = $this->Request->contains('groupby') && $misc->isValidColumn($this->Request['groupby']) ? strtolower($this->Request['groupby']) : null;
		$group_limit = $this->Request->contains('group_limit') ? intval($this->Request['group_limit']) : 0;
		$schedtime_from = $this->Request->contains('schedtime_from') && $misc->isValidInteger($this->Request['schedtime_from']) ? (int)$this->Request['schedtime_from'] : null;
		$schedtime_to = $this->Request->contains('schedtime_to') && $misc->isValidInteger($this->Request['schedtime_to']) ? (int)$this->Request['schedtime_to'] : null;
		$starttime_from = $this->Request->contains('starttime_from') && $misc->isValidInteger($this->Request['starttime_from']) ? (int)$this->Request['starttime_from'] : null;
		$starttime_to = $this->Request->contains('starttime_to') && $misc->isValidInteger($this->Request['starttime_to']) ? (int)$this->Request['starttime_to'] : null;
		$endtime_from = $this->Request->contains('endtime_from') && $misc->isValidInteger($this->Request['endtime_from']) ? (int)$this->Request['endtime_from'] : null;
		$endtime_to = $this->Request->contains('endtime_to') && $misc->isValidInteger($this->Request['endtime_to']) ? (int)$this->Request['endtime_to'] : null;
		$realendtime_from = $this->Request->contains('realendtime_from') && $misc->isValidInteger($this->Request['realendtime_from']) ? (int)$this->Request['realendtime_from'] : null;
		$realendtime_to = $this->Request->contains('realendtime_to') && $misc->isValidInteger($this->Request['realendtime_to']) ? (int)$this->Request['realendtime_to'] : null;
		$order_by = $this->Request->contains('order_by') && $misc->isValidColumn($this->Request['order_by']) ? $this->Request['order_by']: 'ObjectId';
		$order_direction = $this->Request->contains('order_direction') && $misc->isValidOrderDirection($this->Request['order_direction']) ? $this->Request['order_direction']: 'DESC';

		$or = new \ReflectionClass('Baculum\API\Modules\ObjectRecord');
		$prop_cols = $or->getProperties();
		$cols_excl = ['jobname'];
		$columns = [];
		if (is_string($group_by)) {
			$columns = [];
			foreach ($prop_cols as $cols) {
				$name = $cols->getName();
				// skip columns not existing in the catalog
				if (in_array($name, $cols_excl)) {
					continue;
				}
				$columns[] = $name;
			}

			if (!in_array($group_by, $columns)) {
				$this->output = ObjectError::MSG_ERROR_INVALID_PROPERTY;
				$this->error = ObjectError::ERROR_INVALID_PROPERTY;
				return;
			}
		}

		$order_by_lc = strtolower($order_by);
		$columns = [];
		foreach ($prop_cols as $cols) {
			$name = $cols->getName();
			// skip columns not existing in the catalog
			if (in_array($name, $cols_excl)) {
				continue;
			}
			$columns[] = $name;
		}
		if (!in_array($order_by_lc, $columns)) {
			$this->output = ObjectError::MSG_ERROR_INVALID_PROPERTY;
			$this->error = ObjectError::ERROR_INVALID_PROPERTY;
			return;
		}


		$params = [];
		if (!empty($objecttype)) {
			$params['Object.ObjectType'] = [];
			$params['Object.ObjectType'][] = [
				'vals' => $objecttype
			];
		}
		if (!empty($objectname)) {
			$params['Object.ObjectName'] = [];
			$params['Object.ObjectName'][] = [
				'vals' => $objectname
			];
		}
		if (!empty($objectcategory)) {
			$params['Object.ObjectCategory'] = [];
			$params['Object.ObjectCategory'][] = [
				'vals' => $objectcategory
			];
		}
		if (!empty($objectsource)) {
			$params['Object.ObjectSource'] = [];
			$params['Object.ObjectSource'][] = [
				'vals' => $objectsource
			];
		}
		if (!empty($objectuuid)) {
			$params['Object.ObjectUUID'] = [];
			$params['Object.ObjectUUID'][] = [
				'vals' => $objectuuid
			];
		}
		if (!empty($objectstatus)) {
			$params['Object.ObjectStatus'] = [];
			$params['Object.ObjectStatus'][] = [
				'vals' => $objectstatus
			];
		}
		if (!empty($jobname)) {
			$params['Job.Name'] = [];
			$params['Job.Name'][] = [
				'vals' => $jobname
			];
		}
		if (count($jobids) > 0) {
			$params['Job.JobId'] = [];
			$params['Job.JobId'][] = [
				'operator' => 'IN',
				'vals' => $jobids
			];
		}

		// Scheduled time range
		if (!empty($schedtime_from) || !empty($schedtime_to)) {
			$params['Job.SchedTime'] = [];
			if (!empty($schedtime_from)) {
				$params['Job.SchedTime'][] = [
					'operator' => '>=',
					'vals' => date('Y-m-d H:i:s', $schedtime_from)
				];
			}
			if (!empty($schedtime_to)) {
				$params['Job.SchedTime'][] = [
					'operator' => '<=',
					'vals' => date('Y-m-d H:i:s', $schedtime_to)
				];
			}
		}

		// Start time range
		if (!empty($starttime_from) || !empty($starttime_to)) {
			$params['Job.StartTime'] = [];
			if (!empty($starttime_from)) {
				$params['Job.StartTime'][] = [
					'operator' => '>=',
					'vals' => date('Y-m-d H:i:s', $starttime_from)
				];
			}
			if (!empty($starttime_to)) {
				$params['Job.StartTime'][] = [
					'operator' => '<=',
					'vals' => date('Y-m-d H:i:s', $starttime_to)
				];
			}
		}

		// End time range
		if (!empty($endtime_from) || !empty($endtime_to)) {
			$params['Job.EndTime'] = [];
			if (!empty($endtime_from)) {
				$params['Job.EndTime'][] = [
					'operator' => '>=',
					'vals' => date('Y-m-d H:i:s', $endtime_from)
				];
			}
			if (!empty($endtime_to)) {
				$params['Job.EndTime'][] = [
					'operator' => '<=',
					'vals' => date('Y-m-d H:i:s', $endtime_to)
				];
			}
		}

		// Real end time range
		if (!empty($realendtime_from) || !empty($realendtime_to)) {
			$params['Job.RealEndTime'] = [];
			if (!empty($realendtime_from)) {
				$params['Job.RealEndTime'][] = [
					'operator' => '>=',
					'vals' => date('Y-m-d H:i:s', $realendtime_from)
				];
			}
			if (!empty($realendtime_to)) {
				$params['Job.RealEndTime'][] = [
					'operator' => '<=',
					'vals' => date('Y-m-d H:i:s', $realendtime_to)
				];
			}
		}

		$objects = $this->getModule('object')->getObjects(
			$params,
			$limit,
			$order_by_lc,
			$order_direction,
			$group_by,
			$group_limit
		);
		$this->output = $objects;
		$this->error = ObjectError::ERROR_NO_ERRORS;
	}
}
