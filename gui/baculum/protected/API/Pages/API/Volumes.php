<?php
/*
 * Bacula(R) - The Network Backup Solution
 * Baculum   - Bacula web interface
 *
 * Copyright (C) 2013-2019 Kern Sibbald
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
use Baculum\Common\Modules\Errors\VolumeError;

/**
 * Volumes endpoint.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
class Volumes extends BaculumAPIServer {
	public function get() {
		$misc = $this->getModule('misc');
		$limit = $this->Request->contains('limit') && $misc->isValidInteger($this->Request['limit']) ? (int)$this->Request['limit'] : 0;
		$offset = $this->Request->contains('offset') && $misc->isValidInteger($this->Request['offset']) ? (int)$this->Request['offset'] : 0;
		$volumename = $this->Request->contains('volumename') && $misc->isValidName($this->Request['volumename']) ? $this->Request['volumename'] : null;
		$enabled = null;
		if ($this->Request->contains('enabled') && $misc->isValidBoolean($this->Request['enabled'])) {
			$enabled = $misc->isValidBooleanTrue($this->Request['enabled']) ? 1 : 0;
		}
		$voltype = $this->Request->contains('voltype') && $misc->isValidVolType($this->Request['voltype']) ? $this->Request['voltype'] : null;
		$pool = $this->Request->contains('pool') && $misc->isValidName($this->Request['pool']) ? $this->Request['pool'] : null;
		$storage = $this->Request->contains('storage') && $misc->isValidName($this->Request['storage']) ? $this->Request['storage'] : null;

		$params = $props = [];

		if (is_string($volumename)) {
			$params['Media.VolumeName'][] = [
				'vals' => $volumename
			];
		}

		if (is_int($enabled)) {
			$params['Media.Enabled'][] = [
				'vals' => $enabled
			];
		}

		if (is_string($voltype)) {
			$props['voltype'] = $voltype;
		}

		if (is_string($pool)) {
			$props['pool'] = $pool;
		}

		if (is_string($storage)) {
			$props['storage'] = $storage;
		}

		$result = $this->getModule('volume')->getVolumes(
			$params,
			$props,
			$limit,
			$offset
		);
		$this->output = $result;
		$this->error = VolumeError::ERROR_NO_ERRORS;
	}
}
?>
