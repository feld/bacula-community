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

use Baculum\API\Modules\BaculumAPIServer;
use Baculum\Common\Modules\Errors\SourceError;

/**
 * Sources endpoint.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
class Sources extends BaculumAPIServer {

	public function get() {
		$misc = $this->getModule('misc');
		$limit = $this->Request->contains('limit') ? (int)$this->Request['limit'] : 0;
		$job = $this->Request->contains('job') && $misc->isValidName($this->Request['job']) ? $this->Request['job'] : '';
		$client = $this->Request->contains('client') && $misc->isValidName($this->Request['client']) ? $this->Request['client'] : '';
		$fileset = $this->Request->contains('fileset') && $misc->isValidName($this->Request['fileset']) ? $this->Request['fileset'] : '';
		$starttime_from = $this->Request->contains('starttime_from') && $misc->isValidInteger($this->Request['starttime_from']) ? (int)$this->Request['starttime_from'] : null;
		$starttime_to = $this->Request->contains('starttime_to') && $misc->isValidInteger($this->Request['starttime_to']) ? (int)$this->Request['starttime_to'] : null;
		$jobstatus = $this->Request->contains('jobstatus') && $misc->isValidState($this->Request['jobstatus']) ? $this->Request['jobstatus'] : '';
		$hasobject = $this->Request->contains('hasobject') && $misc->isValidBoolean($this->Request['hasobject']) ? $this->Request['hasobject'] : null;

		// @TODO: Fix using sres and jres in this place. It can lead to a problem when sres or jres will be changed in manager.
		$params = [];
		if (!empty($job)) {
			$params['sres.job'] = [[
				'vals' => $job
			]];
		}
		if (!empty($client)) {
			$params['sres.client'] = [[
				'vals' => $client
			]];
		}
		if (!empty($fileset)) {
			$params['sres.fileset'] = [[
				'vals' => $fileset
			]];
		}
		if (!empty($jobstatus)) {
			$params['jres.jobstatus'] = [[
				'vals' => $jobstatus
			]];
		}
		if (!is_null($hasobject)) {
			if ($misc->isValidBooleanTrue($hasobject)) {
				$params['Object.ObjectId'][] = [
					'operator' => 'IS NOT',
					'vals' => 'NULL'
				];
			} elseif ($misc->isValidBooleanFalse($hasobject)) {
				$params['Object.ObjectId'][] = [
					'operator' => 'IS',
					'vals' => 'NULL'
				];
			}
		}

		// Start time range
		if (!empty($starttime_from) || !empty($starttime_to)) {
			$params['ores.starttime'] = [];
			if (!empty($starttime_from)) {
				$params['ores.starttime'][] = [
					'operator' => '>=',
					'vals' => date('Y-m-d H:i:s', $starttime_from)
				];
			}
			if (!empty($starttime_to)) {
				$params['ores.starttime'][] = [
					'operator' => '<=',
					'vals' => date('Y-m-d H:i:s', $starttime_to)
				];
			}
		}

		$sources = $this->getModule('source')->getSources($params, $limit);
		$this->output = $sources;
		$this->error = SourceError::ERROR_NO_ERRORS;
	}
}
?>
