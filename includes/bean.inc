<?php

/**
 * @file
 * Helpers for the bean module.
 */

/**
 * Create and return a new bean entity.
 *
 * Example usage:
 *
 * @code
 * // Create a simple bean, but reset the static caches.
 * $bean1 = cm_tools_bean_create('bean_type', 'Label1', 'example1', '', array(), TRUE);
 * $bean1->save(TRUE);
 *
 * // Create a bean that has a value in one of its fields.
 * $bean2 = cm_tools_bean_create('bean_type_field', 'Label2', 'example2');
 * $bean2->field_content[LANGUAGE_NONE][0] = array(
 *   'value' => "This is some placeholder text.",
 *   'format' => 'rich_text',
 * );
 * $bean1->save(TRUE);
 * @endcode
 *
 * @param $bean_type
 *   The type of bean to create (bundle name).
 * @param $label
 *   The label for this bean that will appear in the admin UI.
 * @param $delta
 *   The machine name for this bean.
 * @param $title
 *   The title of the bean, that will be used as the block title.
 * @param $data
 *   Extra data to store.
 * @param $reset
 *   Pass TRUE to reset several static caches that allow you to create a new
 *   bean with a bean type that you've also added in this page request.
 *
 *  @return
 *    A new bean entity, you should make whatever additions you want, fields etc. and then call the save method on this object to save your new bean:
 *    @code
 *    $bean->save(TRUE);
 *    @endcode
 */
function cm_tools_bean_create($bean_type, $label, $delta, $title = '', $data = array(), $reset = FALSE) {
  
  // Reset some static caches so we pick up newly added bean types correctly.
  if ($reset) {
    drupal_static_reset('ctools_plugins');
    drupal_static_reset('ctools_plugin_type_info_loaded');
    drupal_static_reset('ctools_plugin_type_info');
    drupal_static_reset('ctools_plugin_setup');
    drupal_static_reset('ctools_plugin_load_includes');
    drupal_static_reset('ctools_plugin_api_info');
    drupal_static_reset('bean_load_plugin_class');
    drupal_static_reset('bean_fetch_plugin_info');
  }
  
  $bean = bean_create(array('type' => $bean_type));
  $bean->label = $label;
  $bean->title = $title;
  $bean->delta = $delta;
  $bean->data = $data;
  
  return $bean;
  
}
