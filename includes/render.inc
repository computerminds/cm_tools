<?php
/**
 * @file
 * Helper functions relating to render arrays and rendering.
 */

class CMRender {

  /**
   * Return a render array that wraps the given render array in a given HTML tag.
   *
   * @see theme_cm_tools_html_wrapper.
   */
  public static function wrap($tag = 'div', $render, $classes = array(), $id = '') {
    $wrapped = array(
      $render,
      '#theme_wrappers' => array('cm_tools_html_wrapper'),
      '#tag' => $tag,
      '#attributes' => array(),
      '#pre_render' => array('drupal_pre_render_conditional_comments'),
    );

    // Allow passing a single class.
    if (is_string($classes) && !empty($classes)) {
      $classes = array($classes);
    }

    if (!empty($classes)) {
      $wrapped['#attributes']['class'] = $classes;
    }

    if (!empty($id)) {
      $wrapped['#attributes']['id'] = $id;
    }

    return $wrapped;
  }

}
