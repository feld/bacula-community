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

use Baculum\Common\Modules\Errors\ClientError;
use Baculum\Common\Modules\Errors\PluginError;
use Baculum\Common\Modules\Errors\PluginM365Error;

/**
 * Microsoft 365 plugin email job list endpoint.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
class PluginM365EmailJobList extends BaculumAPIServer {

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
		$tenantid = $this->Request->contains('tenantid') ? $this->Request['tenantid'] : '';
		$fd_plugin_cfg = $this->getModule('fd_plugin_cfg')->getConfig('m365', $client, $tenantid);
		if (!empty($tenantid) && (!$misc->isValidUUID($tenantid) || count($fd_plugin_cfg) == 0)) {
			$this->output = PluginM365Error::MSG_ERROR_TENANT_DOES_NOT_EXISTS;
			$this->error = PluginM365Error::ERROR_TENANT_DOES_NOT_EXISTS;
			return;
		}
		$tenant = $fd_plugin_cfg['tenant_name'];
		$emailowner = $this->Request->contains('emailowner') && $misc->isValidNameExt($this->Request['emailowner']) ? $this->Request['emailowner'] : '';
		if (empty($emailowner)) {
			$this->output = PluginM365Error::MSG_ERROR_EMAIL_DOES_NOT_EXISTS;
			$this->error = PluginM365Error::ERROR_EMAIL_DOES_NOT_EXISTS;
			return;
		}

		$result = $this->getModule('bconsole')->bconsoleCommand(
			$this->director,
			['.jobs'],
			null,
			true
		);
		if ($result->exitcode === 0) {
			$params['Job.Name'] = [];
			$params['Job.Name'][] = [
				'operator' => 'OR',
				'vals' => $result->output
			];
			$result = $this->getModule('m365')->getJobsByTenantAndOwner(
				$tenant,
				$emailowner,
				$params
			);
			$this->output = $result;
			$this->error = PluginM365Error::ERROR_NO_ERRORS;
		} else {
			$this->output = PluginM365Error::MSG_ERROR_WRONG_EXITCODE . ', Error => ' . $result->exitcode . ' Output => ' . implode(PHP_EOL, $result->output);
			$this->error = PluginM365Error::ERROR_WRONG_EXITCODE;
		}
	}
}
