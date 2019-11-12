<?php

/**
 * @file
 * Helper functions for Blocks.
 */

/**
 * Helper function to position a block.
 *
 * @param string $module
 *   The module that provides the block.
 * @param string $delta
 *   The block delta.
 * @param array $block
 *   Options for the block.
 * @param bool $all_themes
 *   Whether the block should be placed in all themes.
 * @param array $types
 *   List of node types the block should display on.
 */
function cm_tools_block_position($module, $delta, array $block = array(), $all_themes = TRUE, $types = array()) {
  $block['module'] = $module;
  $block['delta'] = $delta;
  $block += array(
    'theme' => variable_get('theme_default', 'seven'),
    'weight' => 0,
    'cache' => -1,
    // Only enable the block by default if a region is supplied.
    'status' => (int) !empty($block['region']),
    'title' => '',
    'region' => '',
    'visibility' => BLOCK_VISIBILITY_NOTLISTED,
    'pages' => '',
  );
  if (module_exists('i18n_block')) {
    $block += array(
      'i18n_mode' => 0,
    );
    i18n_block_update_strings($block, $block['i18n_mode']);
  }

  // Set default keys for the db_merge query.
  $merge_keys = array(
    'module' => TRUE,
    'delta' => TRUE,
  );

  // We might not want to position this block for all themes. If so, restrict
  // the merge query by theme.
  if ($all_themes === FALSE) {
    $merge_keys['theme'] = $block['theme'];
  }

  db_merge('block')
    ->key(array_intersect_key($block, $merge_keys))
    ->insertFields(array_intersect_key($block, array(
      'module' => TRUE,
      'delta'  => TRUE,
      'cache'  => TRUE,
      'status' => TRUE,
      'weight' => TRUE,
      'region' => TRUE,
      'theme'  => TRUE,
      'title'  => TRUE,
      'visibility'  => TRUE,
      'pages'  => TRUE,
      'i18n_mode' => TRUE,
    )))
    ->updateFields(array_intersect_key($block, array(
      'cache'  => TRUE,
      'status' => TRUE,
      'weight' => TRUE,
      'region' => TRUE,
      'title' => TRUE,
      'visibility'  => TRUE,
      'pages'  => TRUE,
      'i18n_mode' => TRUE,
    )))
    ->execute();

  if ($types) {
    db_delete('block_node_type')
      ->condition('module', $module)
      ->condition('delta', $delta)
      ->execute();
    $query = db_insert('block_node_type')->fields(array(
      'type',
      'module',
      'delta'
    ));
    foreach (array_filter($types) as $type) {
      $query->values(array(
        'type' => $type,
        'module' => $module,
        'delta' => $delta,
      ));
    }
    $query->execute();
  }
}

/**
 * Retrieve the render array representation for the given block.
 *
 * @param string $module
 *   The module that provides the block.
 * @param string $delta
 *   The block delta.
 *
 * @return array|null
 *   A render array, suitable for passing to render(). If no block is
 *   available, NULL is returned.
 */
function cm_tools_get_block($module, $delta) {
  $block = block_load($module, $delta);
  $block->region = isset($block->region) ? $block->region : NULL;
  if ($blocks = _block_render_blocks(array($block))) {
    return _block_get_renderable_array(array_values($blocks));
  }
  return NULL;
}
