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

use Baculum\API\Modules\ConsoleOutputPage;
use Baculum\API\Modules\ConsoleOutputQueryPage;
use Baculum\API\Modules\JobManager;
use Baculum\API\Modules\JobRecord;
use Baculum\Common\Modules\Logging;
use Baculum\Common\Modules\Errors\JobError;

/**
 * List jobs with objects.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
class JobsObjects extends BaculumAPIServer {

	public function get() {
		$misc = $this->getModule('misc');
		$jobids = $this->Request->contains('jobids') && $misc->isValidIdsList($this->Request['jobids']) ? $this->Request['jobids'] : '';
		$afterjobid = $this->Request->contains('afterjobid') && $misc->isValidInteger($this->Request['afterjobid']) ? $this->Request['afterjobid'] : 0;
		$limit = $this->Request->contains('limit') && $misc->isValidInteger($this->Request['limit']) ? (int)$this->Request['limit'] : 0;
		$offset = $this->Request->contains('offset') && $misc->isValidInteger($this->Request['offset']) ? (int)$this->Request['offset'] : 0;
		$jobstatus = $this->Request->contains('jobstatus') ? $this->Request['jobstatus'] : '';
		$level = $this->Request->contains('level') && $misc->isValidJobLevel($this->Request['level']) ? $this->Request['level'] : '';
		$type = $this->Request->contains('type') && $misc->isValidJobType($this->Request['type']) ? $this->Request['type'] : '';
		$jobname = $this->Request->contains('name') && $misc->isValidName($this->Request['name']) ? $this->Request['name'] : '';
		$clientid = $this->Request->contains('clientid') ? $this->Request['clientid'] : '';
		$schedtime_from = $this->Request->contains('schedtime_from') && $misc->isValidInteger($this->Request['schedtime_from']) ? (int)$this->Request['schedtime_from'] : null;
		$schedtime_to = $this->Request->contains('schedtime_to') && $misc->isValidInteger($this->Request['schedtime_to']) ? (int)$this->Request['schedtime_to'] : null;
		$starttime_from = $this->Request->contains('starttime_from') && $misc->isValidInteger($this->Request['starttime_from']) ? (int)$this->Request['starttime_from'] : null;
		$starttime_to = $this->Request->contains('starttime_to') && $misc->isValidInteger($this->Request['starttime_to']) ? (int)$this->Request['starttime_to'] : null;
		$endtime_from = $this->Request->contains('endtime_from') && $misc->isValidInteger($this->Request['endtime_from']) ? (int)$this->Request['endtime_from'] : null;
		$endtime_to = $this->Request->contains('endtime_to') && $misc->isValidInteger($this->Request['endtime_to']) ? (int)$this->Request['endtime_to'] : null;
		$realstarttime_from = $this->Request->contains('realstarttime_from') && $misc->isValidInteger($this->Request['realstarttime_from']) ? (int)$this->Request['realstarttime_from'] : null;
		$realstarttime_to = $this->Request->contains('realstarttime_to') && $misc->isValidInteger($this->Request['realstarttime_to']) ? (int)$this->Request['realstarttime_to'] : null;
		$realendtime_from = $this->Request->contains('realendtime_from') && $misc->isValidInteger($this->Request['realendtime_from']) ? (int)$this->Request['realendtime_from'] : null;
		$realendtime_to = $this->Request->contains('realendtime_to') && $misc->isValidInteger($this->Request['realendtime_to']) ? (int)$this->Request['realendtime_to'] : null;
		$age = $this->Request->contains('age') && $misc->isValidInteger($this->Request['age']) ? (int)$this->Request['age'] : null;
		$order_by = $this->Request->contains('order_by') && $misc->isValidColumn($this->Request['order_by']) ? $this->Request['order_by']: 'Job.EndTime';
		$order_direction = $this->Request->contains('order_direction') && $misc->isValidOrderDirection($this->Request['order_direction']) ? $this->Request['order_direction']: 'DESC';
		$view = ($this->Request->contains('view') && $misc->isValidResultView($this->Request['view'])) ? $this->Request['view'] : JobManager::JOB_RESULT_VIEW_FULL;

		$jr = new \ReflectionClass('Baculum\API\Modules\JobRecord');
		$sort_cols = $jr->getProperties();
		$order_by_lc = strtolower($order_by);
		$columns = [];
		foreach ($sort_cols as $cols) {
			$name = $cols->getName();
			$columns[] = $name;
		}
		if (!in_array($order_by_lc, $columns)) {
			$this->output = JobError::MSG_ERROR_INVALID_PROPERTY;
			$this->error = JobError::ERROR_INVALID_PROPERTY;
			return;
		}

		if (!empty($jobids)) {
			/**
			 * If jobids parameter provided, all other parameters are not used.
			 */
			$params['Job.JobId'] = [];
			$params['Job.JobId'][] = [
				'operator' => 'IN',
				'vals' => explode(',', $jobids)
			];
			$result = $this->getModule('job')->getJobsObjectsOverview(
				$params,
				null,
				0,
				$order_by,
				$order_direction,
				$view
			);
			$this->output = $result;
			$this->error = JobError::ERROR_NO_ERRORS;
			return;
		}

		if (!empty($clientid) && !$misc->isValidId($clientid)) {
			$this->output = JobError::MSG_ERROR_CLIENT_DOES_NOT_EXISTS;
			$this->error = JobError::ERROR_CLIENT_DOES_NOT_EXISTS;
			return;
		}

		$client = $this->Request->contains('client') ? $this->Request['client'] : '';
		if (!empty($client) && !$misc->isValidName($client)) {
			$this->output = JobError::MSG_ERROR_CLIENT_DOES_NOT_EXISTS;
			$this->error = JobError::ERROR_CLIENT_DOES_NOT_EXISTS;
			return;
		}

		$params = [];

		if ($afterjobid > 0) {
			$params['Job.JobId'] = [];
			$params['Job.JobId'][] = [
				'operator' => '>',
				'vals' => $afterjobid
			];
		}

		$jobstatuses = array_keys($misc->getJobState());
		$sts = str_split($jobstatus);
		$js_counter = 0;
		for ($i = 0; $i < count($sts); $i++) {
			if (in_array($sts[$i], $jobstatuses)) {
				if (!key_exists('Job.JobStatus', $params)) {
					$params['Job.JobStatus'] = [];
					$params['Job.JobStatus'][$js_counter] = [
						'operator' => 'OR',
						'vals' => []
					];
				}
				$params['Job.JobStatus'][$js_counter]['vals'][] = $sts[$i];
			}
		}
		if (!empty($level)) {
			$params['Job.Level'] = [];
			$params['Job.Level'][] = [
				'vals' => $level
			];
		}
		if (!empty($type)) {
			$params['Job.Type'] = [];
			$params['Job.Type'][] = [
				'vals' => $type
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
		} elseif (!empty($age)) { // Job age (now() - age)
			$params['Job.StartTime'] = [];
			$params['Job.StartTime'][] = [
				'operator' => '>=',
				'vals' => date('Y-m-d H:i:s', (time() - $age))
			];
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

		// Real start time range
		if (!empty($realstarttime_from) || !empty($realstarttime_to)) {
			$params['Job.RealStartTime'] = [];
			if (!empty($realstarttime_from)) {
				$params['Job.RealStartTime'][] = [
					'operator' => '>=',
					'vals' => date('Y-m-d H:i:s', $realstarttime_from)
				];
			}
			if (!empty($realstarttime_to)) {
				$params['Job.RealStartTime'][] = [
					'operator' => '<=',
					'vals' => date('Y-m-d H:i:s', $realstarttime_to)
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

		$result = $this->getModule('bconsole')->bconsoleCommand(
			$this->director,
			['.jobs'],
			null,
			true
		);
		if ($result->exitcode === 0) {
			$vals = [];
			if (!empty($jobname) && in_array($jobname, $result->output)) {
				$vals = [$jobname];
			} else {
				$vals = $result->output;
			}
			if (count($vals) == 0) {
				// no $vals criteria means that user has no job resources assigned.
				$this->output = [];
				$this->error = JobError::ERROR_NO_ERRORS;
				return;
			}

			$params['Job.Name'] = [];
			$params['Job.Name'][] = [
				'operator' => 'IN',
				'vals' => $vals
			];

			$error = false;
			// Client name and clientid filter
			if (!empty($client) || !empty($clientid)) {
				$result = $this->getModule('bconsole')->bconsoleCommand($this->director, array('.client'));
				if ($result->exitcode === 0) {
					array_shift($result->output);
					$cli = null;
					if (!empty($client)) {
						$cli = $this->getModule('client')->getClientByName($client);
					} elseif (!empty($clientid)) {
						$cli = $this->getModule('client')->getClientById($clientid);
					}
					if (is_object($cli) && in_array($cli->name, $result->output)) {
						$params['Job.ClientId'] = [];
						$params['Job.ClientId'][] = [
							'operator' => 'AND',
							'vals' => [$cli->clientid]
						];
					} else {
						$error = true;
						$this->output = JobError::MSG_ERROR_CLIENT_DOES_NOT_EXISTS;
						$this->error = JobError::ERROR_CLIENT_DOES_NOT_EXISTS;
					}
				} else {
					$error = true;
					$this->output = $result->output;
					$this->error = $result->exitcode;
				}
			}

			if ($error === false) {
				$result = $this->getModule('job')->getJobsObjectsOverview(
					$params,
					$limit,
					$offset,
					$order_by,
					$order_direction,
					$view
				);
				$this->output = $result;
				$this->error = JobError::ERROR_NO_ERRORS;
			}
		} else {
			$this->output = $result->output;
			$this->error = $result->exitcode;
		}
	}
}
