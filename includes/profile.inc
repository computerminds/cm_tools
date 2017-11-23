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
 * Apply a given callback to the entities returned by the given EntityFieldQuery.
 *
 * This function is designed to work in the context of a Drupal update hook, it
 * accepts the sandbox that is passed to the update hook as its first parameter.
 * The results of the query, and the state of the iterations are stored in the
 * sandbox.
 *
 * @param $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called from.
 * @param \EntityFieldQuery $query
 *   The EntityFieldQuery to execute and iterate over the results of.
 * @param $callback
 *   The callback to apply to each entity returned from the $query. The callback is called with two arguments:
 *   - The entity type of the entity in the second argument
 *   - The loaded entity.
 * @param int $entities_per_pass
 *   The number of entities to load and process in each batch iteration.
 */
function cm_tools_update_efq_walk(&$sandbox, EntityFieldQuery $query, $callback, $entities_per_pass = 10) {
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
}

/**
 * Apply a given callback to all entities of a specific type, and optionally bundle.
 *
 * This function is designed to work in the context of a Drupal update hook, it
 * accepts the sandbox that is passed to the update hook as its first parameter.
 * The results of the query, and the state of the iterations are stored in the
 * sandbox.
 *
 * @param $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called from.
 * @param string $entity_type
 *   The entity type to iterate over.
 * @param $callback
 *   The callback to apply to each entity returned from the $query. The callback is called with two arguments:
 *   - The entity type of the entity in the second argument
 *   - The loaded entity.
 * @param NULL|string $bundle
 *   The bundle to iterate over, if NULL then all bundles of the given entity type will be iterated.
 * @param int $entities_per_pass
 *   The number of entities to load and process in each batch iteration.
 */
function cm_tools_update_entity_walk(&$sandbox, $entity_type, $callback, $bundle = NULL, $entities_per_pass = 10) {
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

  cm_tools_update_efq_walk($sandbox, $query, $wrapper_callback, $entities_per_pass);
}

/**
 * Apply a given callback to all nodes of a specific type.
 *
 * This function is designed to work in the context of a Drupal update hook, it
 * accepts the sandbox that is passed to the update hook as its first parameter.
 * The results of the query, and the state of the iterations are stored in the
 * sandbox.
 *
 * @param $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called from.
 * @param $callback
 *   The callback to apply to each entity returned from the $query. The callback is called with two arguments:
 *   - The entity type of the entity in the second argument
 *   - The loaded entity.
 * @param NULL|string $node_type
 *   The node type to iterate over, if NULL then all nodes will be iterated.
 * @param int $nodes_per_pass
 *   The number of nodes to load and process in each batch iteration.
 */
function cm_tools_update_node_walk(&$sandbox, $callback, $node_type = NULL, $nodes_per_pass = 10) {
  cm_tools_update_entity_walk($sandbox, 'node', $callback, $node_type, $nodes_per_pass);
}


/**
 * Iterate over a set of entities returned by the given EntityFieldQuery.
 *
 * This function is designed to work in the context of a Drupal update hook, it
 * accepts the sandbox that is passed to the update hook as its first parameter.
 * The results of the query, and the state of the iterations are stored in the
 * sandbox.
 *
 * @param $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called from.
 * @param \EntityFieldQuery $query
 *   The EntityFieldQuery to execute and iterate over the results of.
 * @param int $entities_per_pass
 *   The number of entities to load and process in each batch iteration.
 *
 * @return
 *   An associative array of entity types mapped to entities to process in the
 *   iteration.
 */
function cm_tools_update_efq(&$sandbox, EntityFieldQuery $query, $entities_per_pass = 10) {
  $entities = array();

  $callback = function($entity_type, $entity) use (&$entities) {
    $entities[$entity_type][] = $entity;
  };

  cm_tools_update_efq_walk($sandbox, $query, $callback, $entities_per_pass);

  return $entities;
}

/**
 * Iterate over all entities of a specific type, and optionally bundle.
 *
 * This function is designed to work in the context of a Drupal update hook, it
 * accepts the sandbox that is passed to the update hook as its first parameter.
 * The results of the query, and the state of the iterations are stored in the
 * sandbox.
 *
 * @param $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called from.
 * @param string $entity_type
 *   The entity type to iterate over.
 * @param NULL|string $bundle
 *   The bundle to iterate over, if NULL then all bundles of the given entity type will be iterated.
 * @param int $entities_per_pass
 *   The number of entities to load and process in each batch iteration.
 *
 * @return
 *   An array of entities to process in this iteration.
 */
function cm_tools_update_entity(&$sandbox, $entity_type, $bundle = NULL, $entities_per_pass = 10) {
  $entities = array();

  $callback = function($entity) use (&$entities) {
    $entities[] = $entity;
  };

  cm_tools_update_entity_walk($sandbox, $entity_type, $callback, $bundle, $entities_per_pass);

  return $entities;
}

/**
 * Iterate over all nodes of a specific type.
 *
 * This function is designed to work in the context of a Drupal update hook, it
 * accepts the sandbox that is passed to the update hook as its first parameter.
 * The results of the query, and the state of the iterations are stored in the
 * sandbox.
 *
 * @param $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called from.
 * @param NULL|string $node_type
 *   The node type to iterate over, if NULL then all nodes will be iterated.
 * @param int $nodes_per_pass
 *   The number of nodes to load and process in each batch iteration.
 *
 * @return
 *   An array of nodes to process in this iteration.
 */
function cm_tools_update_node(&$sandbox, $node_type = NULL, $nodes_per_pass = 10) {
  return cm_tools_update_entity($sandbox, 'node', $node_type, $nodes_per_pass);
}

/**
 * Fill in missing default field values on entities.
 *
 * When adding a new field to an existing entity type, instances of those
 * entities will be missing values for that field. Drupal does not ensure that
 * when loaded they get the default, and they won't be able to be queried etc.
 *
 * Entities processed by this function will have the default value set
 * according to the field configuration, so will match the defaults of newly
 * created entities.
 *
 * This function is designed to work in the context of a Drupal update hook, it
 * accepts the sandbox that is passed to the update hook as its first parameter.
 * The results of the query, and the state of the iterations are stored in the
 * sandbox.
 *
 * This function requires the Entity API module to be enabled.
 *
 * @param $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called from.
 * @param string $entity_type
 *   The entity type to iterate over.
 * @param string $bundle
 *   The bundle to iterate over.
 * @param $fields
 *   An associative array of fields to set the default value for. Keys are the field names, and values are either scalar (and then ignored) or an array of language codes to check and set the default values for.
 * @param int $entities_per_pass
 *   The number of entities to load and process in each batch iteration.
 *
 * @throws \DrupalUpdateException
 */
function cm_tools_update_missing_default_field_values_insert(&$sandbox, $entity_type, $bundle, $fields, $entites_per_pass = 10) {

  if (!module_exists('entity')) {
    throw new DrupalUpdateException('Entity API module is required by ' . __FUNCTION__);
  }

  cm_tools_update_entity_walk($sandbox, $entity_type, function($entity) use ($entity_type, $fields) {
    foreach ($fields as $field_name => $languages) {
      if (!is_array($languages)) {
        $languages = array(NULL);
      }
      list(, , $bundle) = entity_extract_ids($entity_type, $entity);
      $field = field_info_field($field_name);
      if ($instance = field_info_instance($entity_type, $field_name, $bundle)) {
        $needs_save = FALSE;
        foreach ($languages as $language) {
          $langcode = field_language($entity_type, $entity, $field_name, $language);
          if (field_get_items($entity_type, $entity, $field_name, $langcode) === FALSE) {
            $entity->{$field_name}[$langcode] = field_get_default_value($entity_type, $entity, $field, $instance, $langcode);
            $needs_save = TRUE;
          }
        }
        if ($needs_save) {
          // @TODO: Determine if the field SQL storage module is in for this field, and do a direct insert.
          entity_save($entity_type, $entity);
        }
      }
    }
  }, $bundle, $entites_per_pass);
}
