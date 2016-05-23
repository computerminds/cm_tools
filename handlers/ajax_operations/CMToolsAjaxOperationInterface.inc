<?php

/**
 * @file Interface for CMTools Ajax Operations.
 */
interface CMToolsAjaxOperationInterface {

  /**
   * Sets the CTools plugin info for this handler..
   *
   * @param array $plugin_info
   *
   * @return array
   */
  public function setPluginInfo(array $plugin_info);

  /**
   * Return CTools plugin info associated with this handler.
   *
   * @param null $key
   *   Optionally return the value of a certain key in the info
   *   array.
   *
   * @return array|mixed|null
   */
  public function getPluginInfo($key = NULL);

  /**
   * Determine whether the given user is permitted to access
   * this Ajax operation.
   *
   * @param $account
   *
   * @return boolean
   */
  public function access($account);

  /**
   * Execute your operation and return a standard array of
   * Drupal Ajax commands to execute client side.
   *
   * @param $parameters
   *   Array of parameters passed to your operation.
   *
   * @return array
   */
  public function execute($parameters);

}
