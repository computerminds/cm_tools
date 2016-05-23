<?php

/**
 * @file Helpful parent class for all Ajax Operations.
 */
class CMToolsAjaxOperation implements CMToolsAjaxOperationInterface {

  protected $plugin_info;

  /**
   * {@inheritdoc}
   */
  public function setPluginInfo(array $plugin_info) {
    $this->plugin_info = $plugin_info;
  }

  /**
   * {@inheritdoc}
   */
  public function getPluginInfo($key = NULL) {
    if (isset($key)) {
      return isset($this->plugin_info[$key]) ? $this->plugin_info[$key] : NULL;
    }
    return isset($this->plugin_info) ? $this->plugin_info : array();
  }

  /**
   * {@inheritdoc}
   */
  public function access($account) {
    return TRUE;
  }

  /**
   * {@inheritdoc}
   */
  public function execute($parameters) {
    return array();
  }
}
