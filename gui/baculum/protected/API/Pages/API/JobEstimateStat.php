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

use Baculum\API\Modules\BaculumAPIServer;
use Baculum\Common\Modules\Errors\JobError;

/**
 * Estimate job statistics.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
class JobEstimateStat extends BaculumAPIServer {

	public function get() {
		$misc = $this->getModule('misc');
		$job = $this->Request->contains('name') && $misc->isValidName($this->Request['name']) ? $this->Request['name'] : null;
		$level = $this->Request->contains('level') && $misc->isValidJobLevel($this->Request['level']) && in_array($this->Request['level'], $misc->backupJobLevels) ? $this->Request['level'] : null;

		$result = $this->getModule('bconsole')->bconsoleCommand(
			$this->director,
			['.jobs'],
			null,
			true
		);
		if (is_string($job) && $result->exitcode === 0 && !in_array($job, $result->output)) {
			// Job not allowed for specific user
			$job = null;
		}

		if (is_null($job)) {
			$this->output = JobError::ERROR_JOB_DOES_NOT_EXISTS;
			$this->error = JobError::MSG_ERROR_JOB_DOES_NOT_EXISTS;
			return;
		}
		if (is_null($level)) {
			$this->output = JobError::ERROR_INVALID_JOBLEVEL;
			$this->error = JobError::MSG_ERROR_INVALID_JOBLEVEL;
			return;
		}
		$this->output = $this->getModule('job')->getJobEstimatation($job, $level);
		$this->error = JobError::ERROR_NO_ERRORS;
	}
}

?>
