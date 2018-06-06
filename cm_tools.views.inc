<?php

/**
 * @file
 */

/**
 * Helper function which returns all views plugins we define.
 */
function _cm_tools_views_plugins() {
  $plugins = array(
    'style' => array(
      // Declare a views style that removes all wrapping altogether.
      // We don't need a separate handler for this, we just declare a unique
      // theme hook name which allows us to theme things ourselves.
      // @see cm_tools/theme/theme.inc
      'cm_tools_bare' => array(
        'title' => t('Bare (no wrapping markup)'),
        'help' => t('Renders the view as normal, with no wrapping markup.'),
        'handler' => 'views_plugin_style_default',
        'theme' => 'views_view_cm_tools_bare',
        'uses row plugin' => TRUE,
        'uses row class' => TRUE,
        'uses grouping' => TRUE,
        'uses options' => FALSE,
        'type' => 'normal',
        'help topic' => 'style-unformatted',
        'theme file' => 'theme.views.inc',
        'theme path' => drupal_get_path('module', 'cm_tools') . '/theme',
      ),
    ),
  );
  return $plugins;
}
