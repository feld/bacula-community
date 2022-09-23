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

use Baculum\API\Modules\ConsoleOutputPage;
use Baculum\API\Modules\ConsoleOutputLlistPage;
use Baculum\Common\Modules\Errors\JobError;

/**
 * Get console output for llist pluginrestoreconf bconsole command.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
class LlistPluginRestoreConf extends ConsoleOutputLlistPage {

	public function get() {
		$misc = $this->getModule('misc');
		$jobids = $this->Request->contains('jobids') && $misc->isValidIdsList($this->Request['jobids']) ? $this->Request['jobids'] : 0;
		$out_format = $this->Request->contains('output') && $this->isOutputFormatValid($this->Request['output']) ? $this->Request['output'] : ConsoleOutputPage::OUTPUT_FORMAT_RAW;

		if ($jobids === 0) {
			$this->output = JobError::MSG_ERROR_JOB_DOES_NOT_EXISTS;
			$this->error = JobError::ERROR_JOB_DOES_NOT_EXISTS;
			return;
		}

		if ($out_format === ConsoleOutputPage::OUTPUT_FORMAT_RAW) {
			$out = $this->getRawOutput(['jobids' => $jobids]);
		} elseif($out_format === ConsoleOutputPage::OUTPUT_FORMAT_JSON) {
			$out = $this->getJSONOutput(['jobids' => $jobids]);
		}
		$this->output = $out->output;
		$this->error = $out->exitcode;
	}

	/**
	 * Get llist output from console in raw format.
	 *
	 * @param array $params command  parameters
	 * @return StdClass object with output and exitcode
	 */
	protected function getRawOutput($params = []) {
		return $this->getModule('bconsole')->bconsoleCommand(
			$this->director,
			[
				'llist',
				'pluginrestoreconf',
				'jobid="' . $params['jobids'] . '"'
			]
		);
	}

	/**
	 * Get llist output in JSON format.
	 *
	 * @param array $params command  parameters
	 * @return StdClass object with output and exitcode
	 */
	protected function getJSONOutput($params = []) {
		$result = (object)[
			'output' => [],
			'exitcode' => 0
		];
		$output = $this->getRawOutput($params);
		if ($output->exitcode === 0) {
			array_shift($output->output);
			$result->output = $this->parseOutput($output->output);
		}
		$result->exitcode = $output->exitcode;
		return $result;
	}
}
