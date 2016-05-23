<?php

/**
 * @file AjaxOperation class used in case of error.
 */

class CMToolsAjaxOperationBroken extends CMToolsAjaxOperation {

  /**
   * {@inheritdoc}
   */
  public function access($account) {
    return FALSE;
  }
}
