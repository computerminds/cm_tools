<?php

/**
 * Helper function to position a block.
 *
 * @param  $module
 *   The module that provides the block.
 * @param  $delta
 *   The block delta.
 * @param  $block
 *   Options for the block.
 */
function cm_tools_block_position($module, $delta, $block = array(), $all_themes = TRUE) {
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
    'pages' => '',
  );

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
      'pages'  => TRUE,
    )))
    ->updateFields(array_intersect_key($block, array(
      'cache'  => TRUE,
      'status' => TRUE,
      'weight' => TRUE,
      'region' => TRUE,
      'title' => TRUE,
      'pages'  => TRUE,
    )))
    ->execute();
}

/**
 * Retrieve the render array representation for the given block.
 *
 * @param $module
 *   The module that provides the block.
 * @param $delta
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
