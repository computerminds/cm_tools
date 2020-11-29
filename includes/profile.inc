<?php

/**
 * @file
 * Helpers useful when installing or upgrading the site.
 */

/**
 * Enables modules and throws an exception if that can't be done.
 *
 * @param string|array $modules
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
 * @param string|array $modules
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
 * Apply given work_callback to the entities supplied.
 *
 * This function is designed to work in the context of a Drupal update hook, it
 * accepts the sandbox that is passed to the update hook as its first parameter.
 * The entities to iterate over and the state of the iterations are stored in
 * the sandbox.
 *
 * Example usage:
 * @code
 * function node_update_7017(&$sandbox) {
 *   $entity_list_callback = function () {
 *     $select = db_select('node', 'n')
 *       ->condition('n.type', 'article')
 *       ->fields('n', array('nid'));
 *     $result = $select->execute()->fetchCol();
 *     $entities = array();
 *     if (!empty($result)) {
 *       $entities['node'] = $result;
 *     }
 *     return $entities;
 *   };
 *   $work_callback = function ($entity_type, $entity) {
 *     node_save($entity);
 *   };
 *   cm_tools_include('profile');
 *   // Load and save all article nodes.
 *   cm_tools_update_entities_walk($sandbox, $entity_list_callback, $work_callback);
 * }
 * @endcode
 *
 * @param array $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called
 *   from.
 * @param callable $entity_list_callback
 *   A callback that returns an associative array keyed by entity type where the
 *   values are arrays of entity IDs to iterate over. This will be called when
 *   needed to produce the list of entities to iterate over.
 * @param callable $work_callback
 *   The callback to apply to each entity returned from the
 *   $entity_list_callback. The callback is called with two arguments:
 *   - The entity type of the entity in the second argument
 *   - The loaded entity.
 * @param int $entities_per_pass
 *   The number of entities to load and process in each batch iteration.
 */
function cm_tools_update_entities_walk(array &$sandbox, callable $entity_list_callback, callable $work_callback, $entities_per_pass = 10) {
  // On the first pass through this update function compute the work to be done.
  if (!isset($sandbox['_cm_tools_update_entities_walk'])) {
    $sandbox['_cm_tools_update_entities_walk'] = array(
      'current' => 0,
      'total' => 0,
    );
    foreach (call_user_func($entity_list_callback) as $entity_type => $entity_type_entities) {
      $sandbox['_cm_tools_update_entities_walk']['entities'][$entity_type] = array_chunk($entity_type_entities, $entities_per_pass);
      $sandbox['_cm_tools_update_entities_walk']['total'] += count($entity_type_entities);
    }
  }

  if (!empty($sandbox['_cm_tools_update_entities_walk']['entities'])) {
    // Get the first entity type to process.
    reset($sandbox['_cm_tools_update_entities_walk']['entities']);
    $entity_type = key($sandbox['_cm_tools_update_entities_walk']['entities']);
    $entity_ids = array_shift($sandbox['_cm_tools_update_entities_walk']['entities'][$entity_type]);
    foreach (entity_load($entity_type, $entity_ids) as $entity) {
      call_user_func($work_callback, $entity_type, $entity);
      $sandbox['_cm_tools_update_entities_walk']['current']++;
    }
    // Clear up this $entity_type if we've processed all the chunks.
    if (empty($sandbox['_cm_tools_update_entities_walk']['entities'][$entity_type])) {
      unset($sandbox['_cm_tools_update_entities_walk']['entities'][$entity_type]);
    }
  }

  if (!empty($sandbox['_cm_tools_update_entities_walk']['total'])) {
    $sandbox['#finished'] = $sandbox['_cm_tools_update_entities_walk']['current'] / $sandbox['_cm_tools_update_entities_walk']['total'];
  }
}

/**
 * Apply given callback to the entities returned by the given EntityFieldQuery.
 *
 * This function is designed to work in the context of a Drupal update hook, it
 * accepts the sandbox that is passed to the update hook as its first parameter.
 * The results of the query, and the state of the iterations are stored in the
 * sandbox.
 *
 * @param array $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called
 *   from.
 * @param \EntityFieldQuery $query
 *   The EntityFieldQuery to execute and iterate over the results of.
 * @param callable $callback
 *   The callback to apply to each entity returned from the $query. The callback
 *   is called with two arguments:
 *   - The entity type of the entity in the second argument
 *   - The loaded entity.
 * @param int $entities_per_pass
 *   The number of entities to load and process in each batch iteration.
 */
function cm_tools_update_efq_walk(array &$sandbox, EntityFieldQuery $query, callable $callback, $entities_per_pass = 10) {
  $entity_list_callback = function () use ($query) {
    $result = $query->execute();
    $entities = array();
    foreach ($result as $entity_type => $results) {
      $entities[$entity_type] = array_keys($results);
    }
    return $entities;
  };
  cm_tools_update_entities_walk($sandbox, $entity_list_callback, $callback, $entities_per_pass);
}

/**
 * Apply a given callback to all entities of a specific type.
 *
 * This function is designed to work in the context of a Drupal update hook, it
 * accepts the sandbox that is passed to the update hook as its first parameter.
 * The results of the query, and the state of the iterations are stored in the
 * sandbox.
 *
 * @param array $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called
 *   from.
 * @param string $entity_type
 *   The entity type to iterate over.
 * @param callable $callback
 *   The callback to apply to each entity returned from the $query. The callback
 *   is called with two arguments:
 *   - The entity type of the entity in the second argument
 *   - The loaded entity.
 * @param null|string $bundle
 *   The bundle to iterate over, if NULL then all bundles of the given entity
 *   type will be iterated.
 * @param int $entities_per_pass
 *   The number of entities to load and process in each batch iteration.
 */
function cm_tools_update_entity_walk(array &$sandbox, $entity_type, callable $callback, $bundle = NULL, $entities_per_pass = 10) {
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', $entity_type);
  // Skip access checks, as we really want to process all entities.
  $query->addTag('DANGEROUS_ACCESS_CHECK_OPT_OUT');

  if (isset($bundle)) {
    $query->entityCondition('bundle', $bundle);
  }

  $wrapper_callback = function ($entity_type, $entity) use ($callback) {
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
 * Example usage:
 * @code
 * function node_update_7017(&$sandbox) {
 *   cm_tools_include('profile');
 *   cm_tools_update_node_walk($sandbox, function($node) {
 *     // Load and save all article nodes.
 *     node_save($node);
 *   }, 'article');
 * }
 * @endcode
 *
 * @param array $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called
 *   from.
 * @param callable $callback
 *   The callback to apply to each entity returned from the $query. The callback
 *   is called with two arguments:
 *   - The entity type of the entity in the second argument
 *   - The loaded entity.
 * @param null|string $node_type
 *   The node type to iterate over, if NULL then all nodes will be iterated.
 * @param int $nodes_per_pass
 *   The number of nodes to load and process in each batch iteration.
 */
function cm_tools_update_node_walk(array &$sandbox, callable $callback, $node_type = NULL, $nodes_per_pass = 10) {
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
 * @param array $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called
 *   from.
 * @param \EntityFieldQuery $query
 *   The EntityFieldQuery to execute and iterate over the results of.
 * @param int $entities_per_pass
 *   The number of entities to load and process in each batch iteration.
 *
 * @return array
 *   An associative array of entity types mapped to entities to process in the
 *   iteration.
 */
function cm_tools_update_efq(array &$sandbox, EntityFieldQuery $query, $entities_per_pass = 10) {
  $entities = array();

  $callback = function ($entity_type, $entity) use (&$entities) {
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
 * @param array $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called
 *   from.
 * @param string $entity_type
 *   The entity type to iterate over.
 * @param null|string $bundle
 *   The bundle to iterate over, if NULL then all bundles of the given entity
 *   type will be iterated.
 * @param int $entities_per_pass
 *   The number of entities to load and process in each batch iteration.
 *
 * @return array
 *   An array of entities to process in this iteration.
 */
function cm_tools_update_entity(array &$sandbox, $entity_type, $bundle = NULL, $entities_per_pass = 10) {
  $entities = array();

  $callback = function ($entity) use (&$entities) {
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
 * Example usage:
 * @code
 * function node_update_7018(&$sandbox) {
 *   cm_tools_include('profile');
 *   // Load and save all article nodes.
 *   foreach (cm_tools_update_node($sandbox, 'article') as $node) {
 *     node_save($node);
 *   }
 * }
 * @endcode
 *
 * @param array $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called
 *   from.
 * @param null|string $node_type
 *   The node type to iterate over, if NULL then all nodes will be iterated.
 * @param int $nodes_per_pass
 *   The number of nodes to load and process in each batch iteration.
 *
 * @return array
 *   An array of nodes to process in this iteration.
 */
function cm_tools_update_node(array &$sandbox, $node_type = NULL, $nodes_per_pass = 10) {
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
 * Example usage:
 * @code
 * function node_update_7019(&$sandbox) {
 *   cm_tools_include('profile');
 *   // Fill in missing default values in the 'field_new_field' on article nodes.
 *   cm_tools_update_missing_default_field_values_insert($sandbox, 'node', 'article', 'field_new_field');
 * }
 * @endcode
 *
 * @param array $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called
 *   from.
 * @param string $entity_type
 *   The entity type to iterate over.
 * @param null|string $bundle
 *   The bundle to iterate over or NULL to iterate over all bundles for the
 *   given entity type.
 * @param string|array $field_name
 *   A field name or array of field names to fill in the default value of.
 * @param int $entities_per_pass
 *   The number of entities to load and process in each batch iteration.
 */
function cm_tools_update_missing_default_field_values_insert(array &$sandbox, $entity_type, $bundle, $field_name, $entities_per_pass = 10) {

  if (!is_array($field_name)) {
    $field_name = array($field_name);
  }
  $field_names = array_fill_keys(array_values($field_name), TRUE);

  cm_tools_update_missing_default_field_values_insert_with_languages($sandbox, $entity_type, $bundle, $field_names, $entities_per_pass);
}

/**
 * Fill in missing default field values on entities, for specific languages.
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
 * @param array $sandbox
 *   The sandbox passed to the drupal hook hook that this function is called
 *   from.
 * @param string $entity_type
 *   The entity type to iterate over.
 * @param null|string $bundle
 *   The bundle to iterate over or NULL to iterate over all bundles for the
 *   given entity type.
 * @param array $fields
 *   An associative array of fields to set the default value for. Keys are the
 *   field names, and values are either scalar (and then ignored) or an array
 *   of language codes to check and set the default values for.
 * @param int $entities_per_pass
 *   The number of entities to load and process in each batch iteration.
 */
function cm_tools_update_missing_default_field_values_insert_with_languages(array &$sandbox, $entity_type, $bundle, array $fields, $entities_per_pass = 10) {

  cm_tools_update_entity_walk($sandbox, $entity_type, function ($entity) use ($entity_type, $fields) {
    $needs_save = FALSE;
    list($id, , $bundle) = entity_extract_ids($entity_type, $entity);
    foreach ($fields as $field_name => $languages) {
      if (!is_array($languages)) {
        $languages = array(NULL);
      }
      $field = field_info_field($field_name);
      if ($instance = field_info_instance($entity_type, $field_name, $bundle)) {

        foreach ($languages as $language) {
          $langcode = field_language($entity_type, $entity, $field_name, $language);
          if (field_get_items($entity_type, $entity, $field_name, $langcode) === FALSE) {
            $entity->{$field_name}[$langcode] = field_get_default_value($entity_type, $entity, $field, $instance, $langcode);
            $needs_save = TRUE;
          }
        }
      }
    }
    if ($needs_save) {
      // We don't try and save the full entity, instead we
      // Remove all other fields from the entity, then they'll skip being saved.
      $instances = field_info_instances($entity_type, $bundle);
      foreach (array_diff_key($instances, $fields) as $field_name => $instance) {
        unset($entity->$field_name);
      }
      field_attach_presave($entity_type, $entity);
      field_attach_update($entity_type, $entity);
      entity_get_controller($entity_type)->resetCache(array($id));
    }
  }, $bundle, $entities_per_pass);
}
