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

Prado::using('System.Web.UI.TPage');

/**
 * Base pages module.
 * The module contains methods that are common for all pages (wizards, main
 * page and error pages).
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 */
class BaculumPage extends TPage {

	public function onPreInit($param) {
		parent::onPreInit($param);
		$this->setURLPrefixForSubdir();
	}

	/**
	 * Shortcut method for getting application modules instances by
	 * module name.
	 *
	 * @access public
	 * @param string $name application module name
	 * @return object module class instance
	 */
	public function getModule($name) {
		return $this->getApplication()->getModule($name);
	}

	/**
	 * Redirection to a page.
	 * Page name is given in PRADO notation with "dot", for example: (Home.SomePage).
	 *
	 * @access public
	 * @param string $page_name page name to redirect
	 * @param array $params HTTP GET method parameters in associative array
         * @return none
	 */
	public function goToPage($page_name, $params = null) {
		$url = $this->Service->constructUrl($page_name, $params, false);
		$url = str_replace('/index.php', '', $url);
		header('Location: ' . $url);
                exit();
	}

	/**
	 * Redirection to default page defined in application config.
	 *
	 * @access public
	 * @param array $params HTTP GET method parameters in associative array
	 * @return none
	 */
	public function goToDefaultPage($params = null) {
		$this->goToPage($this->Service->DefaultPage, $params);
	}

	/**
	 * Set prefix when Baculum is running in document root subdirectory.
	 * For example:
	 *   web server document root: /var/www/
	 *   Baculum directory /var/www/baculum/
	 *   URL prefix: /baculum/
	 * In this case to base url is added '/baculum/' such as:
	 * http://localhost:9095/baculum/
	 *
	 * @access private
	 * @return none
	 */
	private function setURLPrefixForSubdir() {
		$full_document_root = preg_replace('#(\/)$#', '', $this->getFullDocumentRoot());
		$url_prefix = str_replace($full_document_root, '', APPLICATION_DIRECTORY);
		if (!empty($url_prefix)) {
			$this->Application->getModule('url_manager')->setUrlPrefix($url_prefix);
		}
	}

	/**
	 * Get full document root directory path.
	 * Symbolic links in document root path are translated to full paths.
	 *
	 * @access private
	 * return string full document root directory path
	 */
	private function getFullDocumentRoot() {
		$root_dir = array();
		$dirs = explode('/', $_SERVER['DOCUMENT_ROOT']);
		for($i = 0; $i < count($dirs); $i++) {
			$document_root_part =  implode('/', $root_dir) . '/' . $dirs[$i];
			if (is_link($document_root_part)) {
				$root_dir = array(readlink($document_root_part));
			} else {
				$root_dir[] = $dirs[$i];
			}
		}

		$root_dir = implode('/', $root_dir);
		return $root_dir;
	}

	/**
	 * Log in as specific user.
	 *
	 * Note, usually after this method call there required is using exit() just
	 * after method execution. Otherwise the HTTP redirection may be canceled on some
	 * web servers.
	 *
	 * @access public
	 * @param string $user user name to log in
	 * @param string $string plain text user's password
	 * @return none
	 */
	public function switchToUser($user, $password) {
		$http_protocol = isset($_SERVER['HTTPS']) && !empty($_SERVER['HTTPS']) ? 'https' : 'http';
		$host = $_SERVER['SERVER_NAME'];
		$port = $_SERVER['SERVER_PORT'];
		$url_prefix = $this->Application->getModule('url_manager')->getUrlPrefix();
		$url_prefix = str_replace('/index.php', '', $url_prefix);
		$location = sprintf("%s://%s:%s@%s:%d%s", $http_protocol, $user, $password, $host, $port, $url_prefix);

		// Log in by header
		header("Location: $location");
	}
}
?>