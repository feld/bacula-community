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

namespace Baculum\Web\Portlets;

/**
 * Directive field interface.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category Interfaces
 * @package Baculum Web
 */
interface IDirectiveField {

	public function setDirectiveName($name);

	public function getDirectiveName();

	public function setDirectiveValue($value);

	public function getDirectiveValue();

	public function setLabel($name);

	public function getLabel();

	public function loadValue($value_obj);

	public function createDirective();

	public function saveValue($sender, $param);
}
?>
