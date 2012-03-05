<?php
/**
 * @file
 * Helpers useful when installing or upgrading the site.
 */

class CMProfile {

  /**
   * Enables modules and throws an exception if that can't be done.
   *
   * @param $modules
   *   A string containing a module name or an array of module names.
   */
  public static function moduleEnable($modules) {
    if (is_string($modules)) {
      $modules = array($modules);
    }

    $result = module_enable($modules);

    if (empty($result)) {
      throw new DrupalUpdateException(implode(', ', $modules) . ' or dependencies could not be enabled');
    }
  }
}
