<?php

/**
 * @file
 * Drush SQL Santization routines for many common modules.
 */

/**
 * Implements hook_drush_sql_sync_sanitize().
 */
function cm_tools_drush_sql_sync_sanitize($site) {
  _cm_tools_drush_sql_sync_sanitize_user_module($site);

  // Fetch list of all tables.
  $all_tables = drush_sql_get_class()->listTables();

  // All cache tables (from the system module).
  $system_queries = array();

  foreach ($all_tables as $name) {
    if ($name === 'cache' || strpos($name, 'cache_') === 0) {
      $system_queries[] = "DELETE FROM $name";
    }
  }

  if (!empty($system_queries)) {
    drush_sql_register_post_sync_op('cache', dt('Sanitize SQL cache tables.'), implode(';', $system_queries) . ';');
  }

  // Redirect module.
  $redirect_queries = array();

  if (in_array('redirect', $all_tables, TRUE)) {
    // Redirects may contain personal data like usernames etc.
    $redirect_queries[] = "DELETE FROM redirect WHERE redirect LIKE 'user/%'";
  }

  if (!empty($redirect_queries)) {
    drush_sql_register_post_sync_op('redirect', dt('Sanitize redirect to user paths.'), implode(';', $redirect_queries) . ';');
  }

  // Path module.
  $path_queries = array();

  if (in_array('url_alias', $all_tables, TRUE)) {
    // URL aliases may contain personal data like usernames etc.
    $path_queries[] = "DELETE FROM url_alias WHERE source LIKE 'user/%'";
  }

  if (!empty($path_queries)) {
    drush_sql_register_post_sync_op('path', dt('Sanitize URL aliases.'), implode(';', $path_queries) . ';');
  }

  // Comment module.
  $comment_queries = array();

  if (in_array('comment', $all_tables, TRUE)) {
    $comment_queries[] = "UPDATE comment SET hostname = '192.0.2.1'";
    $comment_queries[] = "UPDATE comment SET name = sha(name) WHERE name <> ''";
    $comment_queries[] = "UPDATE comment SET mail = CONCAT(sha(mail), '@example.com') WHERE mail <> ''";
    $comment_queries[] = "UPDATE comment SET homepage = CONCAT(sha(homepage), '.example.com') WHERE homepage <> ''";
  }

  if (!empty($comment_queries)) {
    drush_sql_register_post_sync_op('comment', dt('Sanitize comment information.'), implode(';', $comment_queries) . ';');
  }

  // Commerce order module.
  $commerce_order_queries = array();

  if (in_array('commerce_order', $all_tables, TRUE)) {
    $commerce_order_queries[] = "UPDATE commerce_order SET mail = concat('order-', order_id, '@example.com'), hostname='192.0.2.1'";
  }
  if (in_array('commerce_order_revision', $all_tables, TRUE)) {
    $commerce_order_queries[] = "UPDATE commerce_order_revision SET mail = concat('order-', order_id, '@example.com'), revision_hostname='192.0.2.1'";
  }

  if (!empty($commerce_order_queries)) {
    drush_sql_register_post_sync_op('commerce-order', dt('Sanitize Commerce orders.'), implode(';', $commerce_order_queries) . ';');
  }

  // Commerce payment module.
  $commerce_payment_queries = array();

  if (in_array('commerce_payment_transaction', $all_tables, TRUE)) {
    $commerce_payment_queries[] = "UPDATE commerce_payment_transaction SET payload = 'a:0:{}'";
  }
  if (in_array('commerce_payment_transaction_revision', $all_tables, TRUE)) {
    $commerce_payment_queries[] = "UPDATE commerce_payment_transaction_revision SET data = 'b:0;'";
  }

  if (!empty($commerce_payment_queries)) {
    drush_sql_register_post_sync_op('commerce-payment', dt('Sanitize Commerce payments.'), implode(';', $commerce_payment_queries) . ';');
  }

  // Addressfield module.
  $addressfield_queries = array();

  // Addressfield column names and SQL replacements for them.
  $column_names = array(
    'country' => "'GB'",
    'administrative_area' => "CONCAT('A', LEFT(sha(%column), 7))",
    'sub_administrative_area' => "CONCAT('B', LEFT(sha(%column), 7))",
    'locality' => "CONCAT('C', LEFT(sha(%column), 7))",
    'dependent_locality' => "CONCAT('D', LEFT(sha(%column), 7))",
    'postal_code' => "CONCAT('E', LEFT(sha(%column), 6))",
    'thoroughfare' => "CONCAT('R', LEFT(sha(%column), 9), ' Road')",
    'premise' => "CONCAT('P', LEFT(sha(%column), 9))",
    'sub_premise' => "CONCAT('S', LEFT(sha(%column), 7))",
    'organisation_name' => "CONCAT('O', LEFT(sha(%column), 7))",
    'name_line' => "CONCAT('N', LEFT(sha(%column), 7))",
    'first_name' => "CONCAT('F', LEFT(sha(%column), 7))",
    'last_name' => "CONCAT('L', LEFT(sha(%column), 7))",
  );
  foreach (_cm_tools_drush_get_fields_of_type('addressfield') as $field_name) {
    if (in_array('field_data_' . $field_name, $all_tables, TRUE)) {
      foreach ($column_names as $column_name => $replacement) {
        $addressfield_queries[] = strtr("UPDATE field_data_${field_name} SET %column = $replacement WHERE %column IS NOT NULL AND %column <> ''", array('%column' => $field_name . '_' . $column_name));
      }
    }
    if (in_array('field_revision_' . $field_name, $all_tables, TRUE)) {
      foreach ($column_names as $column_name => $replacement) {
        $addressfield_queries[] = strtr("UPDATE field_revision_${field_name} SET %column = $replacement WHERE %column IS NOT NULL AND %column <> ''", array('%column' => $field_name . '_' . $column_name));
      }
    }
  }

  if (!empty($addressfield_queries)) {
    drush_sql_register_post_sync_op('addressfield', dt('Sanitize Addressfield.'), implode(';', $addressfield_queries) . ';');
  }
}

/**
 * Get field names of a specific field type.
 *
 * @param string $field_type
 *   The field type to search for.
 *
 * @return string[]
 *   An array of field names.
 */
function _cm_tools_drush_get_fields_of_type($field_type) {
  return _cm_tools_drush_db_query("SELECT field_name FROM field_config WHERE type = ':field_type'", array(':field_type' => $field_type));
}

/**
 * Get roles which have a specific permission.
 *
 * @param string $permission
 *   The permission to fetch roles for.
 *
 * @return int[]
 *   An array of role IDs with the given permission
 */
function _cm_tools_drush_get_roles_with_permission($permission) {
  return _cm_tools_drush_db_query("SELECT rid FROM role_permission WHERE permission = ':permission'", array(':permission' => $permission));
}

/**
 * Get users with specified roles.
 *
 * @param int[] $roles
 *   The roles to return users for.
 *
 * @return int[]
 *   An array of user IDs with the specified roles.
 */
function _cm_tools_drush_get_users_with_roles(array $roles) {
  return _cm_tools_drush_db_query('SELECT DISTINCT uid FROM users_roles WHERE rid IN (:roles)', array(':roles' => implode(',', $roles)));
}

/**
 * Execute a SQL query in a limited Drush bootstrap environment.
 *
 * @param string $query
 *   The SQL query to execute.
 * @param array $placeholders
 *   An array of placeholders to replace in the query.
 *
 * @return array
 *   An array of lines returned from executing the SQL query.
 */
function _cm_tools_drush_db_query($query, array $placeholders) {
  $prepared_query = strtr(rtrim($query, ' ;') . ';', $placeholders);
  if (drush_sql_get_class()->query($prepared_query)) {
    return drush_shell_exec_output();
  }
  else {
    return array();
  }
}

/**
 * Get users with specified permissions.
 *
 * @param string $permission
 *   The permission to return users for.
 *
 * @return int[]
 *   An array of users with the specified permissions.
 */
function cm_tools_drush_get_users_with_permission($permission) {
  if ($roles = _cm_tools_drush_get_roles_with_permission($permission)) {
    return _cm_tools_drush_get_users_with_roles($roles);
  }
  return array();
}

/**
 * Copy and enhancement of the built in Drush user sanitization stuff.
 *
 * We allow for keeping some of the users unsanitized via a permission and also
 * sanitize usernames.
 */
function _cm_tools_drush_sql_sync_sanitize_user_module($site) {
  $site_settings = drush_sitealias_get_record($site);
  $databases = sitealias_get_databases_from_record($site_settings);
  $user_table_updates = array();
  $message_list = array();

  // Sanitize passwords.
  $newpassword = drush_get_option(array('sanitize-password', 'destination-sanitize-password'), drush_generate_password());
  if ($newpassword != 'no' && $newpassword !== 0) {
    $core = DRUSH_DRUPAL_CORE;
    include_once $core . '/includes/password.inc';
    include_once $core . '/includes/bootstrap.inc';
    $hash = user_hash_password($newpassword);
    $user_table_updates[] = "pass = '$hash'";
    $message_list[] = "passwords";
  }

  // Sanitize email addresses.
  $newemail = drush_get_option(array('sanitize-email', 'destination-sanitize-email'), 'user+%uid@localhost.localdomain');
  if ($newemail != 'no' && $newemail !== 0) {
    if (strpos($newemail, '%') !== FALSE) {
      // We need a different sanitization query for Postgres and Mysql.
      $db_driver = $databases['default']['default']['driver'];
      if ($db_driver == 'pgsql') {
        $email_map = [
          '%uid' => "' || uid || '",
          '%mail' => "' || replace(mail, '@', '_') || '",
          '%name' => "' || replace(name, ' ', '_') || '",
        ];
        $newmail = "'" . str_replace(array_keys($email_map), array_values($email_map), $newemail) . "'";
      }
      elseif ($db_driver == 'mssql') {
        $email_map = [
          '%uid' => "' + uid + '",
          '%mail' => "' + replace(mail, '@', '_') + '",
          '%name' => "' + replace(name, ' ', '_') + '",
        ];
        $newmail = "'" . str_replace(array_keys($email_map), array_values($email_map), $newemail) . "'";
      }
      else {
        $email_map = [
          '%uid' => "', uid, '",
          '%mail' => "', replace(mail, '@', '_'), '",
          '%name' => "', replace(name, ' ', '_'), '",
        ];
        $newmail = "concat('" . str_replace(array_keys($email_map), array_values($email_map), $newemail) . "')";
      }
      $user_table_updates[] = "mail = $newmail, init = $newmail";
    }
    else {
      $user_table_updates[] = "mail = '$newemail', init = '$newemail'";
    }
    $message_list[] = 'email addresses';
  }

  // Sanitize usernames.
  $newusername = drush_get_option(array('sanitize-username', 'destination-sanitize-username'), 'user-%uid');
  if ($newusername != 'no' && $newusername !== 0) {
    if (strpos($newusername, '%') !== FALSE) {
      // We need a different sanitization query for Postgres and Mysql.
      $db_driver = $databases['default']['default']['driver'];
      if ($db_driver == 'pgsql') {
        $username_map = array(
          '%uid' => "' || uid || '",
          '%mail' => "' || replace(mail, '@', '_') || '",
          '%name' => "' || replace(name, ' ', '_') || '",
        );
        $newuser = "'" . str_replace(array_keys($username_map), array_values($username_map), $newusername) . "'";
      }
      elseif ($db_driver == 'mssql') {
        $username_map = array(
          '%uid' => "' + uid + '",
          '%mail' => "' + replace(mail, '@', '_') + '",
          '%name' => "' + replace(name, ' ', '_') + '",
        );
        $newuser = "'" . str_replace(array_keys($username_map), array_values($username_map), $newusername) . "'";
      }
      else {
        $username_map = array(
          '%uid' => "', uid, '",
          '%mail' => "', replace(mail, '@', '_'), '",
          '%name' => "', replace(name, ' ', '_'), '",
        );
        $newuser = "concat('" . str_replace(array_keys($username_map), array_values($username_map), $newusername) . "')";
      }
      $user_table_updates[] = "name = $newuser";
    }
    else {
      $user_table_updates[] = "name = '$newusername'";
    }
    $message_list[] = 'usernames';
  }

  if (!empty($user_table_updates)) {
    // Find users with our permission: 'preserve during sanitization'.
    $users_to_keep = cm_tools_drush_get_users_with_permission('preserve during sanitization');

    if (empty($users_to_keep)) {
      $sanitize_query = "UPDATE users SET " . implode(', ', $user_table_updates) . " WHERE uid > 0;";
    }
    else {
      $expressions_to_keep = implode(',', $users_to_keep);
      $sanitize_query = "UPDATE users SET " . implode(', ', $user_table_updates) . " WHERE uid > 0 AND uid NOT IN ($expressions_to_keep);";
    }
    drush_sql_register_post_sync_op('user-email', dt('Reset !message in !table table', array(
      '!message' => implode(' and ', $message_list),
      '!table' => 'users',
    )), $sanitize_query);
  }

  // Stop Drush from adding these queries and sanitizing all the users.
  drush_set_option('sanitize-password', 'no');
  drush_set_option('sanitize-email', 'no');
}

/**
 * Produce SQL queries for field sanitization.
 *
 * @param array $fields
 *   An array of fields to sanitize, keys are the field name to sanitize,
 *   values are a specification of how to sanitize the field. Each of these
 *   specifications may include the following keys with associated values:
 *     - 'field_name' - (optional) The name of the field to sanitize, if not
 *       specified then the key of the outer array is used as the field name.
 *     - 'columns' - An array of field column names to sanitize, each key in the
 *       array is a column, and the value is a SQL expression to be used as a
 *       replacement. The following tokens may be used in the replacement
 *       expression:
 *       - '%column' - The full, expanded name of the field column.
 *     - 'entity_type' - (optional) An array of entity types to sanitize the
 *       field data for. If not specified no restrictions will be imposed and
 *       field data for all entity types will be sanitized.
 *
 * @return string
 *   The semi-colon delimited SQL queries.
 */
function cm_tools_drush_generate_field_sanitize_queries(array $fields) {
  $user_fields_sanitize_queries = array();

  // Find users with our permission: 'preserve during sanitization'.
  $users_to_keep = cm_tools_drush_get_users_with_permission('preserve during sanitization');

  foreach ($fields as $k => $spec) {
    $field_name = isset($spec['field_name']) ? $spec['field_name'] : $k;
    // Process each specification into updates to perform on the columns.
    foreach ($spec['columns'] as $column_name => $replacement) {
      $conditions = array(
        '%column IS NOT NULL',
        "%column <> ''",
      );
      // Handle users specifically if there are users to not sanitize.
      if (!empty($spec['entity_type']) && in_array('user', $spec['entity_type'], TRUE) && !empty($users_to_keep)) {
        $sub_conditions = array();
        $other_entity_types = array_diff($spec['entity_type'], array('user'));
        if (!empty($other_entity_types)) {
          $types = array_map(function ($v) {
            return "'$v'";
          }, $other_entity_types);
          $sub_conditions[] = 'entity_type IN (' . implode(',', $types) . ')';
        }

        $expressions_to_keep = implode(',', $users_to_keep);
        $sub_conditions[] = "(entity_type = 'user' AND entity_id NOT IN ($expressions_to_keep))";

        // Build those conditions onto our main conditions.
        $conditions[] = '(' . implode(' OR ', $sub_conditions) . ')';
      }
      elseif (isset($spec['entity_type']) && is_array($spec['entity_type'])) {
        $types = array_map(function ($v) {
          return "'$v'";
        }, $spec['entity_type']);
        $conditions[] = 'entity_type IN (' . implode(',', $types) . ')';
      }

      // Construct the actual queries.
      foreach (array('field_data', 'field_revision') as $type) {
        $user_fields_sanitize_queries[] = strtr("UPDATE ${type}_${field_name} SET %column = $replacement  WHERE " . implode(' AND ', $conditions), ['%column' => $field_name . '_' . $column_name]);
      }
    }
  }
  return implode(';', $user_fields_sanitize_queries) . ';';
}
