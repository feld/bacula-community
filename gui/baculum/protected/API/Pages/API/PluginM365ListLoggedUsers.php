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
use Baculum\API\Modules\ConsoleOutputQueryPage;
use Baculum\Common\Modules\Logging;
use Baculum\Common\Modules\Errors\BconsoleError;
use Baculum\Common\Modules\Errors\ClientError;
use Baculum\Common\Modules\Errors\PluginError;
use Baculum\Common\Modules\Errors\PluginM365Error;

/**
 * List logged in M365 plugin users.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
class PluginM365ListLoggedUsers extends ConsoleOutputQueryPage {

	public function get() {
		$misc = $this->getModule('misc');
		$client = null;
		$clientid = $this->Request->contains('id') ? (int)$this->Request['id'] : 0;
		$result = $this->getModule('bconsole')->bconsoleCommand(
			$this->director,
			['.client'],
			null,
			true
		);
		if ($result->exitcode === 0) {
			$client_val = $this->getModule('client')->getClientById($clientid);
			if (is_object($client_val) && in_array($client_val->name, $result->output)) {
				$client = $client_val->name;
			} else {
				$this->output = ClientError::MSG_ERROR_CLIENT_DOES_NOT_EXISTS;
				$this->error = ClientError::ERROR_CLIENT_DOES_NOT_EXISTS;
				return;
			}
		} else {
			$this->output = PluginError::MSG_ERROR_WRONG_EXITCODE;
			$this->error = PluginError::ERROR_WRONG_EXITCODE;
			return;
		}

		$out_format = ConsoleOutputPage::OUTPUT_FORMAT_RAW;
		if ($this->Request->contains('output') && $this->isOutputFormatValid($this->Request['output'])) {
			$out_format = $this->Request['output'];
		}

		$tenantid = $this->Request->contains('tenantid') ? $this->Request['tenantid'] : null;
		if (!empty($tenantid) && !$misc->isValidUUID($tenantid)) {
			$this->output = PluginM365Error::MSG_ERROR_TENANT_DOES_NOT_EXISTS;
			$this->error = PluginM365Error::ERROR_TENANT_DOES_NOT_EXISTS;
			return;
		}
		$cfg = [];
		$fd_plugin_cfg = $this->getModule('fd_plugin_cfg')->getConfig('m365', $client, $tenantid);
		if (!empty($tenantid) && count($fd_plugin_cfg) > 0) {
			$cfg[$tenantid] = $fd_plugin_cfg;
		} else {
			$cfg = $fd_plugin_cfg;
		}

		$output = [];
		$error = 0;
		foreach ($cfg as $tid => $opts) {
			$config_file = key_exists('config_file', $opts) ? $opts['config_file']: '';
			$objectid = key_exists('objectid', $opts) ? $opts['objectid']: '';
			$plugin = 'm365: ';
			if (!empty($config_file)) {
				// Standalone app model
				$plugin .= sprintf(' config_file=\"%s\" ', $config_file);
			} elseif (!empty($objectid)) {
				// Common app model
				$plugin .= sprintf(' objectid=\"%s\" ', $objectid);
			}

			$out = new \StdClass;
			$out->output = [];
			$out->error = BconsoleError::ERROR_NO_ERRORS;
			$params = ['client' => $client, 'plugin' => $plugin];
			if ($out_format === ConsoleOutputPage::OUTPUT_FORMAT_RAW) {
				$out = $this->getRawOutput($params);
			} elseif($out_format === ConsoleOutputPage::OUTPUT_FORMAT_JSON) {
				$out = $this->getJSONOutput($params);
			}

			if ($out->exitcode !== 0) {
				$error = PluginM365Error::ERROR_EXECUTING_PLUGIN_QUERY_COMMAND;
				$output = PluginM365Error::MSG_ERROR_EXECUTING_PLUGIN_QUERY_COMMAND . $out->output;
				$this->getModule('logging')->log(
					Logging::CATEGORY_EXECUTE,
					$output . ", Error=$error"
				);
				continue;
			}
			$output[$tid] = $out->output;
		}
		$this->output = $output;
		$this->error = $error;
	}

	/**
	 * Get M365 logged user list output from console in raw format.
	 *
	 * @param array $params command parameters
	 * @return StdClass object with output and exitcode
	 */
	protected function getRawOutput($params = []) {
		$ret = $this->getModule('bconsole')->bconsoleCommand(
			$this->director,
			[
				'.query',
				'plugin="' . $params['plugin'] . '"',
				'client="' . $params['client'] . '"',
				'parameter="logged-users"'
			]
		);
		if ($ret->exitcode === 0) {
			$ret->output = $this->getUserRows($ret->output);
		} else {
			$ret->output = []; // don't provide errors to output, only in logs
			$this->getModule('logging')->log(
				Logging::CATEGORY_EXECUTE,
				'Wrong output from m365 RAW user list: ' . implode(PHP_EOL, $ret->output)
			);
		}
		return $ret;
	}

	/**
	 * Get show director output in JSON format.
	 *
	 * @param array $params command parameters
	 * @return StdClass object with output and exitcode
	 */
	protected function getJSONOutput($params = []) {
		$ret = $this->getModule('bconsole')->bconsoleCommand(
			$this->director,
			[
				'.query',
				'plugin="' . $params['plugin'] . '"',
				'client="' . $params['client'] . '"',
				'parameter="json|logged-users"'
			]
		);
		if ($ret->exitcode === 0) {
			$users = $this->getUserJSON($ret->output);
			if (count($users) === 1) {
				$ret->output = json_decode($users[0]);
			} else {
				$ret->output = []; // don't provide errors to output, only in logs
				$this->getModule('logging')->log(
					Logging::CATEGORY_EXECUTE,
					'Wrong output from m365 JSON user list: ' . implode(PHP_EOL, $users)
				);
			}
		}
		return $ret;
	}

	private function getUserRows(array $output) {
		$out = [];
		for ($i = 0; $i < count($output); $i++) {
			if (preg_match('/^user=/', $output[$i]) === 1) {
				$out[] = $output[$i];
			}
		}
		return $out;
	}

	private function getUserJSON(array $output) {
		$out = [];
		for ($i = 0; $i < count($output); $i++) {
			if (preg_match('/^=?\[/', $output[$i]) === 1) {
				$out[] = ltrim($output[$i], '=');
			}
		}
		return $out;
	}
}
