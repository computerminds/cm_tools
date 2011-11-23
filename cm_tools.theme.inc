<?php
/**
 * @file
 * Theme related functions for the Computerminds Tools module.
 */

/**
 * Theme wrapper function for wrapping a render array in an arbitrary HTML tag.
 *
 * For example if you had the following render array:
 *
 * <code>
 *  $render = array(
 *   '#markup' => 'some random markup',
 * );
 * </code>
 *
 * You could wrap it in a div tag by doing:
 * <code>
 *  $wrapped['#theme_wrappers'] = array('cm_tools_html_wrapper');
 *  $wrapped['#tag'] = 'div';
 *  $wrapped[] = $render;
 * </code>
 *
 * You can then give the HTML tag attributes, thusly:
 * <code>
 *  $wrapped['#attributes']['class'][] = 'some-class-to-add';
 * </code>
 */
function theme_cm_tools_html_wrapper($variables) {
  $element = $variables['element'];
  $attributes = isset($element['#attributes']) ? drupal_attributes($element['#attributes']) : '';
  if (!isset($element['#children'])) {
    return '<' . $element['#tag'] . $attributes . " />\n";
  }
  else {
    $output = '<' . $element['#tag'] . $attributes . '>';
    if (isset($element['#value_prefix'])) {
      $output .= $element['#value_prefix'];
    }
    $output .= $element['#children'];
    if (isset($element['#value_suffix'])) {
      $output .= $element['#value_suffix'];
    }
    $output .= '</' . $element['#tag'] . ">\n";
    return $output;
  }
}
