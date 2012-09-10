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
function cm_tools_block_position($module, $delta, $block = array()) {
  $block['module'] = $module;
  $block['delta'] = $delta;
  $block += array(
    'theme' => variable_get('theme_default', 'seven'),
    'weight' => 0,
    'cache' => -1,
    'status' => 1,
    'title' => '',
    'region' => '',
    'pages' => '',
  );

  db_merge('block')
    ->key(array_intersect_key($block, array(
      'module' => TRUE,
      'delta'  => TRUE,
    )))
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
 * Get a render array for a block, no questions asked.
 *
 * @param $module
 *   The module that provides the block.
 * @param $delta
 *   The block delta.
 */
function cm_tools_get_block($module, $delta) {
  $block = block_load($module, $delta);
  $block->region = isset($block->region) ? $block->region : NULL;
  if ($blocks = _block_render_blocks(array($block))) {
    return _block_get_renderable_array(array_values($blocks));
  }
  return NULL;
}
