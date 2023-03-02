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

use Baculum\API\Modules\ConsoleOutputJSONPage;
use Baculum\Common\Modules\Logging;
use Baculum\Common\Modules\Errors\ClientError;
use Baculum\Common\Modules\Errors\PluginError;
use Baculum\Common\Modules\Errors\PluginM365Error;

/**
 * List email attachments backed up using Microsoft 365 plugin.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
class PluginM365EmailAttachmentList extends ConsoleOutputJSONPage {

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

		$tenantid = $this->Request->contains('tenantid') ? $this->Request['tenantid'] : null;
		$fd_plugin_cfg = $this->getModule('fd_plugin_cfg')->getConfig('m365', $client, $tenantid);
		if (!empty($tenantid) && (!$misc->isValidUUID($tenantid) || count($fd_plugin_cfg) == 0)) {
			$this->output = PluginM365Error::MSG_ERROR_TENANT_DOES_NOT_EXISTS;
			$this->error = PluginM365Error::ERROR_TENANT_DOES_NOT_EXISTS;
			return;
		}
		$tenant = $fd_plugin_cfg['tenant_name'];

		$params = [
			'client' => $client,
			'tenant' => $tenant
		];
		if ($this->Request->contains('mailbox') && $misc->isValidEmail($this->Request['mailbox'])) {
			$params['mailbox'] = $this->Request['mailbox'];
		}
		if ($this->Request->contains('limit') && $misc->isValidInteger($this->Request['limit'])) {
			$params['limit'] = (int)$this->Request['limit'];
		}
		if ($this->Request->contains('name') && $misc->isValidName($this->Request['name'])) {
			$params['name'] = $this->Request['name'];
		}
		if ($this->Request->contains('contenttype') && $misc->isValidPath($this->Request['contenttype'])) {
			$params['contenttype'] = $this->Request['contenttype'];
		}
		if ($this->Request->contains('minsize') && $misc->isValidInteger($this->Request['minsize'])) {
			$params['minsize'] = $this->Request['minsize'];
		}
		if ($this->Request->contains('maxsize') && $misc->isValidInteger($this->Request['maxsize'])) {
			$params['maxsize'] = $this->Request['maxsize'];
		}
		if ($this->Request->contains('isinline') && $misc->isValidInteger($this->Request['isinline'])) {
			$params['isinline'] =  $this->Request['isinline'];
		}
		$out = $this->getJSONOutput($params);

		$output = [];
		$error = PluginM365Error::ERROR_NO_ERRORS;
		if ($out->exitcode === 0) {
			$output = $out->output;
		} else {
			$error = PluginM365Error::ERROR_WRONG_EXITCODE;
			$output = PluginM365Error::MSG_ERROR_WRONG_EXITCODE . implode(PHP_EOL, $out->output);
		}
		$this->output = $output;
		$this->error = $error;
	}

	/**
	 * Get M365 email attachment list in JSON string format.
	 *
	 * @param array $params command parameters
	 * @return StdClass object with output and exitcode
	 */
	protected function getRawOutput($params = []) {
		$cmd = [
			'.jlist',
			'metadata',
			'type="attachment"',
			'tenant="' . $params['tenant'] . '"'
		];

		if (key_exists('mailbox', $params)) {
			$cmd[] ='owner="' .  $params['mailbox'] . '"';
		}
		if (key_exists('limit', $params)) {
			$cmd[] ='limit="' .  $params['limit'] . '"';
		}
		if (key_exists('name', $params)) {
			$cmd[] ='name="' .  $params['name'] . '"';
		}
		if (key_exists('contenttype', $params)) {
			$cmd[] ='contenttype="' .  $params['contenttype'] . '"';
		}
		if (key_exists('minsize', $params)) {
			$cmd[] ='minsize="' .  $params['minsize'] . '"';
		}
		if (key_exists('maxsize', $params)) {
			$cmd[] ='maxsize="' .  $params['maxsize'] . '"';
		}
		if (key_exists('isinline', $params)) {
			$cmd[] ='isinline="' .  $params['isinline'] . '"';
		}
		$ret = $this->getModule('bconsole')->bconsoleCommand(
			$this->director,
			$cmd
		);

		if ($ret->exitcode !== 0) {
			$this->getModule('logging')->log(
				Logging::CATEGORY_EXECUTE,
				'Wrong output from m365 RAW attachment list: ' . implode(PHP_EOL, $ret->output)
			);
		}
		return $ret;
	}

	/**
	 * Get M365 email attachment list in validated and formatted JSON format.
	 *
	 * @param array $params command parameters
	 * @return StdClass object with output and exitcode
	 */
	protected function getJSONOutput($params = []) {
		$ret = $this->getRawOutput($params);
		if ($ret->exitcode === 0) {
			$ret->output = $this->parseOutput($ret->output);
			if ($ret->output->error === 0) {
				$ret->output = $ret->output->data;
			} else {
				$ret->output = $ret->output->errmsg;
			}
		}
		return $ret;
	}
}
