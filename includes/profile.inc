<?php

/**
 * @file
 * Helpers useful when installing or upgrading the site.
 */

/**
 * Flag used by cm_tools_update_field_tables_walk() -- include data table.
 */
define('CM_TOOLS_FIELD_TABLE_DATA', 1);

/**
 * Flag used by cm_tools_update_field_tables_walk() -- include revision table.
 */
define('CM_TOOLS_FIELD_TABLE_REVISION', 2);

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
 *   is called with a single argument, which is the loaded entity.
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

/**
 * Apply given work_callback to each row of each of the specified tables.
 *
 * This function is designed to work in the context of a Drupal update hook, it
 * accepts the sandbox that is passed to the update hook as its first parameter.
 * The fields to iterate over and the state of the iterations are stored in
 * the sandbox.
 *
 * Example usage:
 * @code
 * function node_update_7018(&$sandbox) {
 *   cm_tools_include('profile');
 *   return cm_tools_tables_walk($sandbox,
 *     // The tables to operate over (with their primary columns).
 *     array(
 *       array('node', array('nid')),
 *       array('node_revision', array('vid')),
 *     ),
 *     // Query: Restrict only to rows that have "foobar" in the title.
 *     function($query, $table_name) {
 *       $query->condition('title', '%' . db_like('foobar') . '%', 'LIKE');
 *     },
 *     // Process row: Remove all occurances of "foobar" from the title.
 *     function(&$row, $table_name) {
 *       $row['title'] = str_replace('foobar', '', $row['title']);
 *     }
 *   );
 * }
 * @endcode
 *
 * @param array $sandbox
 *   The sandbox passed to the drupal update hook that this function is called
 *   from.
 * @param array|callable $tables
 *   Either an array of table entries or a callback that returns the next table
 *   entry on demand. Each entry is an array where the 0th element is the table
 *   name and the 1st element is an array of the primary key columns.
 * @param callable $query_alter_callback
 *   A callback that receives the index query for returning rows from a field
 *   table. Conditions may be added to it to filter down the list of rows
 *   processed. It receives these arguments:
 *   - $query - The DB select query object,
 *   - $table_name - The name of the table being queried.
 * @param callable $work_callback
 *   The callback to apply to each row of each database table. It is called with
 *   these arguments:
 *   - $row - Associative array of column name => values for the row. Alter by
 *            reference,
 *   - $table_name - The name of the table being queried.
 *   The $work_callback can return one of these values:
 *   - NULL (default): Updates the row if a column was changed. Changes to
 *                     columns used in the primary key are ignored.
 *   - TRUE: Forces the row to be updated, even if nothing changed,
 *   - FALSE: Skips row update, even if something changed.
 * @param int $rows_per_pass
 *   The number of table rows to load and process in each batch iteration.
 *
 * @return string
 *   A status message suitable for the return value of your update hook.
 */
function cm_tools_tables_walk(array &$sandbox, $tables, callable $query_alter_callback, callable $work_callback, $rows_per_pass = 50) {
  /*
   *              INITIALISE
   *                   │
   *                   │  ┌───────────────────────────────────┐
   *          move to next table     ┌─────────────────────┐  │
   *                   │  │          │      Next chunk     │  │
   *           ┌───────▼──▼─────┐    │  ┌────────────────┐ │  │
   *           │ GATHER ROWS    │    └──► PROCESS ROWS   ├─┘  │
   *           │ - <table>      ├───────► - <table>      ├────┘
   *           │                │ First │ - <rows chunk> │
   *           │                │ chunk └────────────────┘
   *           └───────┬────────┘
   *                   │ No tables
   *                   │  left
   *                   ▼
   *                  END
   */
  // Convenience reference.
  if (!isset($sandbox['_cm_tools_tables_walk'])) {
    $sandbox['_cm_tools_tables_walk'] = array();
  }
  $s = &$sandbox['_cm_tools_tables_walk'];

  $status = array();

  if (!is_callable($tables)) {
    $move_to_next_table = function () use (&$s) {
      return array_shift($s['tables']);
    };
  }
  else {
    $move_to_next_table = $tables;
  }

  // Initialize state and shared variables.
  if (empty($s)) {
    $s = array(
      'state' => 'GATHER ROWS',
      'tables' => $tables,
      'table' => NULL,
      'rows' => array(),
      'rows_updated' => 0,
    );
    $s['table'] = $move_to_next_table();
  }

  switch ($s['state']) {

    case 'GATHER ROWS':
      if (!isset($s['table'])) {
        $s['state'] = 'END';
        break;
      }
      list($table_name, $primary_columns) = $s['table'];
      $query = db_select($table_name, 'f')
        ->fields('f', $primary_columns);
      if (is_callable($query_alter_callback)) {
        call_user_func($query_alter_callback, $query, $table_name);
      }
      $s['rows'] = $query
        ->execute()
        ->fetchAll(PDO::FETCH_ASSOC);
      $s['state'] = 'PROCESS ROWS';
      $status[] = strtr('Found !row_count rows in !table_name.', array(
        '!row_count' => count($s['rows']),
        '!table_name' => $table_name,
      ));
      // Deliberately fall-through, because why not?
    case 'PROCESS ROWS':
      list($table_name, $primary_columns) = $s['table'];
      $rows = array_splice($s['rows'], 0, $rows_per_pass);
      if (empty($rows)) {
        $status[] = strtr('Updated !row_count rows in !table_name.', array(
          '!row_count' => $s['rows_updated'],
          '!table_name' => $table_name,
        ));
        $s['rows_updated'] = 0;
        $s['state'] = 'GATHER ROWS';
        $s['table'] = $move_to_next_table();
        break;
      }
      // Fetch the values for all the rows in our chunk in the same query.
      $qry = db_select($table_name, 'f')
        ->fields('f');
      $or = db_or();
      foreach ($rows as $row) {
        $and = db_and();
        foreach ($row as $primary_column => $value) {
          $and->condition($primary_column, $value);
        }
        $or->condition($and);
      }
      $rows = $qry
        ->condition($or)
        ->execute()
        ->fetchAll(PDO::FETCH_ASSOC);
      foreach ($rows as $original_row) {
        $altered_row = $original_row;
        $return = call_user_func_array($work_callback, array(
          &$altered_row,
          $table_name,
        ));
        if ($return === FALSE) {
          continue;
        }
        // If a non-primary column changed, update the DB record.
        $changed_columns = array();
        foreach ($altered_row as $column => $value) {
          if (!in_array($column, $primary_columns)) {
            if ($return === TRUE || ($altered_row[$column] !== $original_row[$column])) {
              $changed_columns[$column] = $altered_row[$column];
            }
          }
        }
        if (!empty($changed_columns)) {
          $qry = db_update($table_name)
            ->fields($changed_columns);
          foreach ($original_row as $column => $value) {
            if (in_array($column, $primary_columns)) {
              $qry->condition($column, $value);
            }
          }
          $s['rows_updated'] += $qry->execute();
        }
      }
      break;
  }

  if ($s['state'] === 'END') {
    $sandbox['#finished'] = 1;
  }
  else {
    $sandbox['#finished'] = 0.5;
  }

  if (!empty($status)) {
    return implode("\n", $status);
  }
}

/**
 * Apply given work_callback to each row of each table of the specified fields.
 *
 * This function is designed to work in the context of a Drupal update hook, it
 * accepts the sandbox that is passed to the update hook as its first parameter.
 * The fields to iterate over and the state of the iterations are stored in
 * the sandbox.
 *
 * Example usage:
 * @code
 * function node_update_7018(&$sandbox) {
 *   cm_tools_include('profile');
 *   return cm_tools_field_tables_walk($sandbox,
 *     // Fields: Operate only on text fields.
 *     function($fields, $instances) {
 *       return array_filter($fields, function($field) {
 *         return $field['type'] === 'text';
 *       });
 *     },
 *     // Query: Restrict only to rows that have "foobar" in the field value.
 *     function($query, $field, $table_type) {
 *       $value_column = _field_sql_storage_columnname($sandbox['field']['field_name'], 'value');
 *       $query->condition($value_column, '%' . db_like('foobar') . '%', 'LIKE');
 *     },
 *     // Process row: Remove all occurances of "foobar".
 *     function(&$row, $field, $table_type) {
 *       $value_column = _field_sql_storage_columnname($sandbox['field']['field_name'], 'value');
 *       $row[$value_column] = str_replace('foobar', '', $row[$value_column]);
 *     }
 *   );
 * }
 * @endcode
 *
 * @param array $sandbox
 *   The sandbox passed to the drupal update hook that this function is called
 *   from.
 * @param callable $field_list_filter_callback
 *   A callback that returns an array of field infos. This will be called when
 *   needed to produce the list of fields to iterate over. It is called with
 *   these arguments:
 *   - $fields: The info array for all fields (filter this down and return it).
 *   - $instances: The info array for all field instances.
 * @param callable $query_alter_callback
 *   A callback that receives the index query for returning rows from a field
 *   table. Conditions may be added to it to filter down the list of rows
 *   processed. It receives these arguments:
 *   - $query - The DB select query object,
 *   - $field - The field info array for the current field,
 *   - $table_type - One of CM_TOOLS_FIELD_TABLE_DATA, or
 *                   CM_TOOLS_FIELD_TABLE_REVISION.
 * @param callable $work_callback
 *   The callback to apply to each row of each database table returned from the
 *   $field_list_callback. It is called with these arguments:
 *   - $row - Associative array of column name => values for the row. Alter by
 *            reference,
 *   - $field - The field info array for the current field,
 *   - $table_type - One of CM_TOOLS_FIELD_TABLE_DATA, or
 *                   CM_TOOLS_FIELD_TABLE_REVISION.
 *   The $work_callback can return one of these values:
 *   - NULL (default): Updates the row if a column was changed. Changes to
 *                     columns used in the primary key are ignored.
 *   - TRUE: Forces the row to be updated, even if nothing changed,
 *   - FALSE: Skips row update, even if something changed.
 * @param int $rows_per_pass
 *   The number of table rows to load and process in each batch iteration.
 * @param int $tables
 *   Integer flags indicating which field tables to include. Defaults to both
 *   'data' and 'revision' tables. Possible values are:
 *   - CM_TOOLS_FIELD_TABLE_DATA -- include field data tables only,
 *   - CM_TOOLS_FIELD_TABLE_REVISION -- include field revision tables only.
 *
 * @return string
 *   A status message suitable for the return value of your update hook.
 */
function cm_tools_field_tables_walk(array &$sandbox, callable $field_list_filter_callback, callable $query_alter_callback, callable $work_callback, $rows_per_pass = 50, $tables = CM_TOOLS_FIELD_TABLE_DATA | CM_TOOLS_FIELD_TABLE_REVISION) {
  // Convenience reference.
  if (!isset($sandbox['_cm_tools_field_tables_walk'])) {
    $sandbox['_cm_tools_field_tables_walk'] = array();
  }
  $s = &$sandbox['_cm_tools_field_tables_walk'];

  $status = array();

  // Initialize state, shared variables and gather fields.
  if (empty($s)) {
    $s['fields'] = call_user_func(
      $field_list_filter_callback,
      // We only support fields using the default SQL storage.
      array_filter(field_info_fields(), function ($field) {
        return $field['storage']['module'] == 'field_sql_storage' &&
          $field['storage']['type'] == 'field_sql_storage';
      }),
      field_info_instances()
    );
    $s['fields_count'] = count($s['fields']);
    $s['field'] = NULL;
    $s['table_type'] = 0;
    $tables_str = array();
    if ($tables & CM_TOOLS_FIELD_TABLE_DATA) {
      $tables_str[] = 'data';
    }
    if ($tables & CM_TOOLS_FIELD_TABLE_REVISION) {
      $tables_str[] = 'revision';
    }
    $tables_str = implode(' and ', $tables_str) . (count($tables_str) > 1 ? ' tables' : ' table');
    $status[] = strtr('Processing the !tables for !field_count fields.', array(
      '!tables' => $tables_str,
      '!field_count' => $s['fields_count'],
    ));
  }

  $status[] = cm_tools_tables_walk($sandbox,
    // Move to next table callback.
    function () use (&$s, $tables) {
      if ($tables === CM_TOOLS_FIELD_TABLE_DATA | CM_TOOLS_FIELD_TABLE_REVISION) {
        if ($s['table_type'] === CM_TOOLS_FIELD_TABLE_DATA) {
          $s['table_type'] = CM_TOOLS_FIELD_TABLE_REVISION;
        }
        else {
          $s['field'] = array_shift($s['fields']);
          $s['table_type'] = CM_TOOLS_FIELD_TABLE_DATA;
        }
      }
      else {
        $s['field'] = array_shift($s['fields']);
        $s['table_type'] = $tables;
      }
      if ($s['field']) {
        $table_name = ($s['table_type'] === CM_TOOLS_FIELD_TABLE_DATA)
          ? _field_sql_storage_tablename($s['field'])
          : _field_sql_storage_revision_tablename($s['field']);
        $storage_schema = _field_sql_storage_schema($s['field']);
        $primary_columns = $storage_schema[$table_name]['primary key'];
        return array($table_name, $primary_columns);
      }
    },
    // Query alter callback.
    function ($query, $table_name) use (&$s, $query_alter_callback) {
      if (is_callable($query_alter_callback)) {
        call_user_func_array($query_alter_callback, array(
          $query,
          $s['field'],
          $s['table_type'],
        ));
      }
    },
    // Work callback.
    function (&$row, $table_name) use (&$s, $work_callback) {
      return call_user_func_array($work_callback, array(
        &$row,
        $s['field'],
        $s['table_type'],
      ));
    }
  );

  $status = array_filter($status);
  if (!empty($status)) {
    return implode("\n", $status);
  }
}
