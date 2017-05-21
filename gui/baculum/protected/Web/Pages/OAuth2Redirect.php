<?php
/*
 * Bacula(R) - The Network Backup Solution
 * Baculum   - Bacula web interface
 *
 * Copyright (C) 2013-2016 Kern Sibbald
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

Prado::using('Application.Web.Class.BaculumWebPage');

class OAuth2Redirect extends BaculumWebPage {

	/**
	 * Authorization ID (known also as 'authorization_code') regular expression pattern
	 * allow to set hexadecimal value of the authorization ID with length equal 40 chars.
	 * 
	 * @see http://tools.ietf.org/html/rfc6749#section-1.3.1
	 */
	const AUTHORIZATION_ID_PATTERN = '^[a-fA-F0-9]{40}$';

	const STATE_PATTERN = '^[a-zA-Z0-9]{16}$';

	public function onInit($param) {
		parent::onInit($param);
		$this->Response->appendHeader('Access-Control-Allow-Origin: *');
		$this->Response->appendHeader('Access-Control-Allow-Methods: GET, OPTIONS');
		$this->Response->appendHeader('Access-Control-Allow-Headers: Origin, Content-Type, Location, X-Requested-With');
	}

	public function onPreRender($param) {
		parent::onPreRender($param);
		if (array_key_exists('code', $_GET) && $this->validateAuthId($_GET['code']) === true && array_key_exists('state', $_GET) && $this->validateState($_GET['state']) === true) {
			$this->getModule('api')->getTokens($_GET['code'], $_GET['state']);
		}
	}

	private function validateAuthId($auth_id) {
		return (preg_match('/' . self::AUTHORIZATION_ID_PATTERN . '/', $auth_id) === 1);
	}

	private function validateState($state) {
		return (preg_match('/' . self::STATE_PATTERN . '/', $state) === 1);
	}
}
