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
 * Disables modules and throws an exception if that can't be done.
 *
 * @param $modules
 *   A string containing a module name or an array of module names.
 */
function cm_tools_module_disable($modules) {
  if (is_string($modules)) {
    $modules = array($modules);
  }

  module_disable($modules);

  // Ensure that the modules have been disabled.
  $result = TRUE;
  foreach ($modules as $module) {
    if (module_exists($module)) {
      $result = FALSE;
    }
  }

  if (empty($result)) {
    throw new DrupalUpdateException(implode(', ', $modules) . ' or dependencies could not be disabled');
  }
}

/**
 * Helper for batch executing a callback across all nodes of a given type.
 */
function cm_tools_update_node_batch($callback, &$sandbox, $node_type = NULL, $nodes_per_pass = 10) {
  return cm_tools_update_entity_batch('node', $callback, $sandbox, $node_type, $nodes_per_pass);
}

/**
 * Helper for batch executing a callback across all entities of a given type.
 */
function cm_tools_update_entity_batch($entity_type, $callback, &$sandbox, $bundle = NULL, $entities_per_pass = 10) {
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', $entity_type);
  // Skip access checks, as we really want to process all entities.
  $query->addTag('DANGEROUS_ACCESS_CHECK_OPT_OUT');

  if (isset($bundle)) {
    $query->entityCondition('bundle', $bundle);
  }

  $wrapper_callback = function($entity_type, $entity) use ($callback) {
    call_user_func($callback, $entity);
  };

  return cm_tools_update_efq_batch($query, $wrapper_callback, $sandbox, $entities_per_pass);
}

/**
 * Helper for batch executing a callback across all nodes of a given type.
 */
function cm_tools_update_efq_batch(EntityFieldQuery $query, $callback, &$sandbox, $entities_per_pass = 10) {
  // If this is the first pass through this update function then work out the work to be done here..
  if (!isset($sandbox['_cm_tools_update_efq_batch'])) {
    $sandbox['_cm_tools_update_efq_batch'] = array(
      'current' => 0,
      'total' => 0,
    );
    $result = $query->execute();
    foreach ($result as $entity_type => $results) {
      $sandbox['_cm_tools_update_efq_batch']['entities'][$entity_type] = array_chunk(array_keys($results), $entities_per_pass);
      $sandbox['_cm_tools_update_efq_batch']['total'] += count($results);
    }
  }

  if (!empty($sandbox['_cm_tools_update_efq_batch']['entities'])) {
    // Get the first entity type to process.
    reset($sandbox['_cm_tools_update_efq_batch']['entities']);
    $entity_type = key($sandbox['_cm_tools_update_efq_batch']['entities']);
    $entity_ids = array_shift($sandbox['_cm_tools_update_efq_batch']['entities'][$entity_type]);
    foreach (entity_load($entity_type, $entity_ids) as $entity) {
      call_user_func($callback, $entity_type, $entity);
      $sandbox['_cm_tools_update_efq_batch']['current']++;
    }
    // Clear up this $entity_type if we've processed all the chunks.
    if (empty($sandbox['_cm_tools_update_efq_batch']['entities'][$entity_type])) {
      unset($sandbox['_cm_tools_update_efq_batch']['entities'][$entity_type]);
    }
  }

  // Set the value for finished. If current == total then finished will be 1, signifying we are done.
  $sandbox['#finished'] = $sandbox['_cm_tools_update_efq_batch']['current'] / $sandbox['_cm_tools_update_efq_batch']['total'];

  return NULL;
}


