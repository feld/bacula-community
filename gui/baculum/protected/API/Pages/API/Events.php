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

use Baculum\Common\Modules\Errors\EventError;

/**
 * Events endpoint.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
class Events extends BaculumAPIServer {

	public function get() {
		$misc = $this->getModule('misc');
		$limit = $this->Request->contains('limit') ? intval($this->Request['limit']) : 0;
		$offset = $this->Request->contains('offset') && $misc->isValidInteger($this->Request['offset']) ? (int)$this->Request['offset'] : 0;
		$eventscode = $this->Request->contains('eventscode') && $misc->isValidName($this->Request['eventscode']) ? $this->Request['eventscode'] : null;
		$eventstype = $this->Request->contains('eventstype') && $misc->isValidName($this->Request['eventstype']) ? $this->Request['eventstype'] : null;
		$eventstimestart = $this->Request->contains('eventstimestart') && $misc->isValidBDate($this->Request['eventstimestart']) ? $this->Request['eventstimestart'] : null;
		$eventstimeend = $this->Request->contains('eventstimeend') && $misc->isValidBDate($this->Request['eventstimeend']) ? $this->Request['eventstimeend'] : null;
		$eventsdaemon = $this->Request->contains('eventsdaemon') && $misc->isValidName($this->Request['eventsdaemon']) ? $this->Request['eventsdaemon'] : null;
		$eventssource = $this->Request->contains('eventssource') && $misc->isValidNameExt($this->Request['eventssource']) ? $this->Request['eventssource'] : null;
		$eventsref = $this->Request->contains('eventsref') && $misc->isValidName($this->Request['eventsref']) ? $this->Request['eventsref'] : null;
		$eventstext = $this->Request->contains('eventstext') && $misc->isValidName($this->Request['eventstext']) ? $this->Request['eventstext'] : null;

		$params = $time_scope =  [];
		if (!empty($eventscode)) {
			$params['Events.EventsCode'] = [[
				'vals' => $eventscode
			]];
		}
		if (!empty($eventstype)) {
			$params['Events.EventsType'] = [[
				'vals' => $eventstype
			]];
		}
		if (!empty($eventsdaemon)) {
			$params['Events.EventsDaemon'] = [[
				'vals' => $eventsdaemon
			]];
		}
		if (!empty($eventssource)) {
			$params['Events.EventsSource']= [[
				'vals' => $eventssource
			]];
		}
		if (!empty($eventsref)) {
			$params['Events.EventsRef'] = [[
				'vals' => $eventsref
			]];
		}
		if (!empty($eventstext)) {
			$params['Events.EventsText'] = [[
				'vals' => $eventstext
			]];
		}
		if (!empty($eventstimestart)) {
			$time_scope['eventstimestart'] = $eventstimestart;
		}
		if (!empty($eventstimeend)) {
			$time_scope['eventstimeend'] = $eventstimeend;
		}

		$events = $this->getModule('event')->getEvents($params, $time_scope, $limit, $offset);
		$this->output = $events;
		$this->error = EventError::ERROR_NO_ERRORS;
	}
}
