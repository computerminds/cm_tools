<?php

/**
 * @file Render elements provided by CM Tools module.
 */

/**
 * Implements hook_element_info().
 */
function cm_tools_element_info() {
  $types = array();

  /*
   * Thin wrapper around #theme => 'item_list' which allows
   * render arrays to be passed as '#items'.
   */
  $types['cm_item_list'] = array(
    '#theme' => 'item_list',
    '#items' => array(),
    '#list_type' => 'ul',
    '#attributes' => array(),
    '#pre_render' => array('cm_tools_item_list_pre_render'),
  );

  return $types;
}

/**
 * Pre-render callback for the item_list element.
 *
 * We detect whether render arrays being passed under '#items'
 * and if so, render them into strings.
 *
 * @param $elements
 * @return mixed
 */
function cm_tools_item_list_pre_render($elements) {

  // Need to replace the #type since theme('item_list')
  // expects to find 'ul' or 'ol' there, not 'cm_item_list'!
  $elements['#type'] = $elements['#list_type'];

  // Sadly a bit of magic is necessary here since '#items'
  // can legitimately be an array.
  foreach ($elements['#items'] as &$item) {

    // An item can be it's contents directly, or the
    // contents can be found in the 'data' property.
    if (is_array($item) && isset($item['data'])) {
      $item = &$item['data'];
    }

    if (is_array($item) && !isset($item['children'])) {
      $item = drupal_render($item);
    }
  }

  return $elements;
}
