<?php
/**
 * @file
 * Helpers useful when installing or upgrading the site.
 */

/**
 * Enables modules and throws an exception if that can't be done.
 *
 * @param $modules
 *   A string containing a module name or an array of module names.
 */
function cm_tools_module_enable($modules) {
  if (is_string($modules)) {
    $modules = array($modules);
  }

  $result = module_enable($modules);

  if (empty($result)) {
    throw new DrupalUpdateException(implode(', ', $modules) . ' or dependencies could not be enabled');
  }
}

/**
 * Creates a new custom block provided by the block module.
 *
 * All caches will usually have to be cleared for your block to start being
 * displayed after calling this function.
 *
 * @param $admin_label
 *  The name of this block in the Block Admin UI.
 * @param $title
 *  The title of the block.
 * @param $body
 *  The text content of the block.
 * @param $body_format
 *  (Optional) The format machine name to render the body in. Defaults to
 *  variable_get('filter_fallback_format').
 * @param $region
 *  (Optional) Which region to position this block in. Defaults to none.
 * @param $weight
 *  (Optional) The block weight. Defaults to 0.
 * @param array $settings
 *  (Optional) Array of further settings for the block. See code for full array
 *  of possible values and defaults. Notable keys are:
 *
 *   'content_types': Show block for certain content types, An array of machine
 *                    names,
 *   'roles':         Provide an array of rids to display this block only for
 *                    certain users,
 *   'visibility':    Block visibility setting (see hook_block_info() docs),
 *   'pages':         List of pages, works alonside 'visibility'. NOTE: This
 *                    function can accept an array here for improved DX,
 *   'cache':         Drupal block caching constant (see hook_block_info() docs).
 *
 * @return
 *   FALSE if a block with that admin label already exists,
 *   $block array (containing 'delta') of the newly created block on success.
 *
 * @see block_add_block_form_submit().
 */
function cm_tools_block_custom_add($admin_label, $block = array()) {

  // Merge in default values for block settings.
  $block += array(
    'title' => '',
    'body' => '',
    'body_format' => variable_get('filter_fallback_format'),
    'region' => BLOCK_REGION_NONE,
    'weight' => 0,
    'content_types' => array(),
    'roles' => array(),
    'visibility' => BLOCK_VISIBILITY_NOTLISTED,
    'pages' => array(),
    'cache' => DRUPAL_NO_CACHE,
    'custom' => 1,
    'module' => 'block',
  );

  if (!isset($block['status'])) {
    $block['status'] = (int) ($block['region'] != BLOCK_REGION_NONE);
  }

  // Support array for pages.
  if (is_array($block['pages'])) {
    $block['pages'] = implode("\n", $block['pages']);
  }

  $custom_block_exists = (bool) db_query_range('SELECT 1 FROM {block_custom} WHERE info = :info', 0, 1, array(':info' => $admin_label))->fetchField();
  if ($custom_block_exists) {
    return FALSE;
  }

  $block['delta'] = $delta = db_insert('block_custom')
    ->fields(array(
      'body' => $body,
      'info' => $admin_label,
      'format' => $body_format,
    ))
    ->execute();

  $query = db_insert('block')->fields(array('visibility', 'pages', 'custom', 'title', 'module', 'theme', 'status', 'weight', 'delta', 'cache', 'region'));
  foreach (list_themes() as $key => $theme) {
    if (!empty($theme->status) || (variable_get('theme_default') == $theme->name)) {
      $query->values(array(
        'visibility' => $block['visibility'],
        'pages' => $block['pages'],
        'custom' => $block['custom'],
        'title' => $block['title'],
        'module' => $block['module'],
        'theme' => $theme->name,
        'status' => $block['status'],
        'weight' => $block['weight'],
        'delta' => $delta,
        'cache' => $block['cache'],
        'region' => $block['region'],
      ));
    }
  }
  $query->execute();

  // Roles
  $query = db_insert('block_role')->fields(array('rid', 'module', 'delta'));
  foreach (array_filter($block['roles']) as $rid) {
    $query->values(array(
      'rid' => $rid,
      'module' => 'block',
      'delta' => $delta,
    ));
  }
  $query->execute();

  // Content-type visibility
  // @see node_form_block_admin_configure_submit().
  db_delete('block_node_type')
    ->condition('module', 'block')
    ->condition('delta', $delta)
    ->execute();
  $query = db_insert('block_node_type')->fields(array('type', 'module', 'delta'));
  foreach (array_filter($block['content_types']) as $type) {
    $query->values(array(
      'type' => $type,
      'module' => 'block',
      'delta' => $delta,
    ));
  }
  $query->execute();

  return $block;
}

/**
 * Delete an existing custom block, provided by the block module.
 *
 * There are two ways of targetting the block to delete:
 *
 *  @param $admin_label
 *   The admin label of the block. Block module mandates that this needs to be
 *   unique, so it's a pretty good key to match off,
 *  @param $delta
 *   That is to say, the value of the 'bid' column in the {block_custom} table
 *   (not the block table!). If this parameter is passed, $admin_label is
 *   ignored.
 *
 *  @return
 *   TRUE  if something was deleted,
 *   FALSE if nothing was deleted, perhaps because no delta could be found to
 *         match the given $admin_label.
 */
function cm_tools_block_custom_delete($admin_label, $delta = NULL) {

  // Get delta.
  if (!isset($delta)) {
    $delta = db_query('SELECT bid FROM {block_custom} WHERE info = :info', array(':info' => $admin_label))->fetchColumn();
  }

  if (empty($delta)) {
    return FALSE;
  }

  // Delete from {block_custom} table
  db_delete('block_custom')
    ->condition('bid', $delta)
    ->execute();

  // Delete from {block} table
  $affected_rows = db_delete('block')
    ->condition('module', 'block')
    ->condition('delta', $delta)
    ->execute();

  // Delete from {block_node_type}
  db_delete('block_node_type')
    ->condition('module', 'block')
    ->condition('delta', $delta)
    ->execute();

  return !empty($affected_rows);
}
