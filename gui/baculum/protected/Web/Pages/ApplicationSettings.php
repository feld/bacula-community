<?php
/*
 * Bacula(R) - The Network Backup Solution
 * Baculum   - Bacula web interface
 *
 * Copyright (C) 2013-2020 Kern Sibbald
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

use Baculum\Web\Modules\WebConfig;
use Baculum\Web\Modules\BaculumWebPage;


/**
 * Application settings class.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category Page
 * @package Baculum Web
 */
class ApplicationSettings extends BaculumWebPage {

	public function onInit($param) {
		parent::onInit($param);
		$this->DecimalBytes->Checked = true;
		if(count($this->web_config) > 0) {
			$this->Debug->Checked = ($this->web_config['baculum']['debug'] == 1);
			$this->MaxJobs->Text = (key_exists('max_jobs', $this->web_config['baculum']) ? intval($this->web_config['baculum']['max_jobs']) : WebConfig::DEF_MAX_JOBS);
			if (key_exists('keep_table_settings', $this->web_config['baculum'])) {
				if ($this->web_config['baculum']['keep_table_settings'] === '-1') {
					// keep settings until end of web browser session
					$this->KeepTableSettingsEndOfSession->Checked = true;
				} elseif ($this->web_config['baculum']['keep_table_settings'] === '0') {
					// keep settings with no time limit (persistent settings)
					$this->KeepTableSettingsNoLimit->Checked = true;
				} else {
					// keep settings for specific time (default 2 hours)
					$this->KeepTableSettingsSpecificTime->Checked = true;
					$this->KeepTableSettingsFor->setDirectiveValue($this->web_config['baculum']['keep_table_settings']);
				}
			} else {
				// default setting
				$this->KeepTableSettingsSpecificTime->Checked = true;
				$this->KeepTableSettingsFor->setDirectiveValue(WebConfig::DEF_KEEP_TABLE_SETTINGS);
			}
			if (key_exists('size_values_unit', $this->web_config['baculum'])) {
				$this->DecimalBytes->Checked = ($this->web_config['baculum']['size_values_unit'] === 'decimal');
				$this->BinaryBytes->Checked = ($this->web_config['baculum']['size_values_unit'] === 'binary');
			}
			if (key_exists('time_in_job_log', $this->web_config['baculum'])) {
				$this->TimeInJobLog->Checked = ($this->web_config['baculum']['time_in_job_log'] == 1);
			}
			if (key_exists('date_time_format', $this->web_config['baculum'])) {
				$this->DateTimeFormat->Text = $this->web_config['baculum']['date_time_format'];
			} else {
				$this->DateTimeFormat->Text = WebConfig::DEF_DATE_TIME_FORMAT;
			}
			$this->EnableMessagesLog->Checked = $this->getModule('web_config')->isMessagesLogEnabled();
			if (key_exists('job_age_on_job_status_graph', $this->web_config['baculum'])) {
				$this->JobAgeOnJobStatusGraph->setDirectiveValue($this->web_config['baculum']['job_age_on_job_status_graph']);
			} else {
				$this->JobAgeOnJobStatusGraph->setDirectiveValue(0);
			}
			$this->JobAgeOnJobStatusGraph->createDirective();
		}
	}


	public function save() {
		if (count($this->web_config) > 0) {
			$this->web_config['baculum']['debug'] = ($this->Debug->Checked === true) ? 1 : 0;
			$max_jobs = intval($this->MaxJobs->Text);
			$keep_table_settings = null;
			if ($this->KeepTableSettingsNoLimit->Checked) {
				$keep_table_settings = '0';
			} elseif ($this->KeepTableSettingsEndOfSession->Checked) {
				$keep_table_settings = '-1';
			} elseif ($this->KeepTableSettingsSpecificTime->Checked) {
				$keep_table_settings = $this->KeepTableSettingsFor->getValue();
			}
			$this->web_config['baculum']['max_jobs'] = $max_jobs;
			$this->web_config['baculum']['keep_table_settings'] = $keep_table_settings;
			$this->web_config['baculum']['size_values_unit'] = $this->BinaryBytes->Checked ? 'binary' : 'decimal';
			$this->web_config['baculum']['time_in_job_log'] = ($this->TimeInJobLog->Checked === true) ? 1 : 0;
			$this->web_config['baculum']['date_time_format'] = $this->DateTimeFormat->Text;
			$this->web_config['baculum']['enable_messages_log'] = ($this->EnableMessagesLog->Checked === true) ? 1 : 0;
			$this->web_config['baculum']['job_age_on_job_status_graph'] = $this->JobAgeOnJobStatusGraph->getValue();
			$this->getModule('web_config')->setConfig($this->web_config);
		}
	}
}
?>
