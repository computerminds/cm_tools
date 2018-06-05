<?php

/**
 * Implementation of hook_drush_sql_sync_sanitize().
 */
function cm_tools_drush_sql_sync_sanitize($site) {
  _cm_tools_drush_sql_sync_sanitize_user_module($site);

  // Fetch list of all tables.
  $all_tables = drush_sql_get_class()->listTables();

  // Redirect module.
  $queries = array();

  if (in_array('redirect', $all_tables, TRUE)) {
    // Redirects may contain personal data like usernames etc.
    $queries[] = "DELETE FROM redirect WHERE redirect LIKE 'user/%'";
  }

  if (!empty($queries)) {
    $queries[] = '';
    drush_sql_register_post_sync_op('redirect', dt('Sanitize redirect to user paths.'), implode(';', $queries));
  }


  // Path module.
  $queries = array();

  if (in_array('url_alias', $all_tables, TRUE)) {
    // URL aliases may contain personal data like usernames etc.
    $queries[] = "DELETE FROM url_alias WHERE source LIKE 'user/%'";
  }

  if (!empty($queries)) {
    $queries[] = '';
    drush_sql_register_post_sync_op('path', dt('Sanitize URL aliases.'), implode(';', $queries));
  }

  // Comment module.
  $queries = array();

  if (in_array('comment', $all_tables, TRUE)) {
    $queries[] = "UPDATE comment SET hostname = '192.0.2.1'";
    $queries[] = "UPDATE comment SET name = sha(name) WHERE name <> ''";
    $queries[] = "UPDATE comment SET mail = CONCAT(sha(mail), '@example.com') WHERE mail <> ''";
    $queries[] = "UPDATE comment SET homepage = CONCAT(sha(homepage), '.example.com') WHERE homepage <> ''";
  }

  if (!empty($queries)) {
    $queries[] = '';
    drush_sql_register_post_sync_op('comment', dt('Sanitize comment information.'), implode(';', $queries));
  }


  // Commerce order module.
  $queries = array();

  if (in_array('commerce_order', $all_tables, TRUE)) {
    $queries[] = "UPDATE commerce_order SET mail = concat('order-', order_id, '@example.com'), hostname='192.0.2.1'";
  }
  if (in_array('commerce_order_revision', $all_tables, TRUE)) {
    $queries[] = "UPDATE commerce_order_revision SET mail = concat('order-', order_id, '@example.com'), revision_hostname='192.0.2.1'";
  }

  if (!empty($queries)) {
    $queries[] = '';
    drush_sql_register_post_sync_op('commerce-order', dt('Sanitize Commerce orders.'), implode(';', $queries));
  }

  // Commerce payment module.
  $queries = array();

  if (in_array('commerce_payment_transaction', $all_tables, TRUE)) {
    $queries[] = "UPDATE commerce_payment_transaction SET payload = 'a:0:{}'";
  }
  if (in_array('commerce_payment_transaction_revision', $all_tables, TRUE)) {
    $queries[] = "UPDATE commerce_payment_transaction_revision SET data = 'b:0;'";
  }

  if (!empty($queries)) {
    $queries[] = '';
    drush_sql_register_post_sync_op('commerce-payment', dt('Sanitize Commerce payments.'), implode(';', $queries));
  }

  // Addressfield module.
  $queries = array();

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
//    'data' => '',
  );
  foreach (_cm_tools_drush_get_fields_of_type('addressfield') as $field_name) {
    if (in_array('field_data_' . $field_name, $all_tables, TRUE)) {
      foreach ($column_names as $column_name => $replacement) {
        $queries[] = strtr("UPDATE field_data_${field_name} SET %column = $replacement WHERE %column IS NOT NULL AND %column <> ''", array('%column' => $field_name . '_' . $column_name));
      }
    }
    if (in_array('field_revision_' . $field_name, $all_tables, TRUE)) {
      foreach ($column_names as $column_name => $replacement) {
        $queries[] = strtr("UPDATE field_revision_${field_name} SET %column = $replacement WHERE %column IS NOT NULL AND %column <> ''", array('%column' => $field_name . '_' . $column_name));
      }
    }
  }

  if (!empty($queries)) {
    $queries[] = '';
    drush_sql_register_post_sync_op('addressfield', dt('Sanitize Addressfield.'), implode(';', $queries));
  }
}

/**
 * Get field names of a specific field type.
 *
 * @param $field_type
 *
 * @return string[]
 */
function _cm_tools_drush_get_fields_of_type($field_type) {
  $fields = array();

  $return = drush_sql_get_class()->query("SELECT field_name FROM field_config WHERE type = '$field_type';");
  if ($return) {
    $fields = drush_shell_exec_output();
  }
  return $fields;
}

/**
 * Get roles which have a specific permission.
 *
 * @param string $permission.
 *   The permission to fetch roles for.
 *
 * @return int[]
 *   An array of role IDs with the given permission
 */
function _cm_tools_drush_get_roles_with_permission($permission) {
  $rids = array();

  $return = drush_sql_get_class()->query("SELECT rid FROM role_permission WHERE permission = '${permission}';");
  if ($return) {
    $rids = drush_shell_exec_output();
  }
  return $rids;
}

/**
 * Get users with specified roles.
 *
 * @param $roles
 *   The roles to return users for.
 *
 * @return int[]
 *   An array of user IDs with the specified roles.
 */
function _cm_tools_drush_get_users_with_roles($roles) {
  $users = array();

  $in_expression = implode(',', $roles);

  $return = drush_sql_get_class()->query("SELECT DISTINCT uid FROM users_roles WHERE rid IN ($in_expression);");
  if ($return) {
    $users = drush_shell_exec_output();
  }
  return $users;
}

/**
 * Get users with specified permissions.
 *
 * @param $permission
 *   The roles to return users for.
 *
 * @return int[]
 *   An array of users with the specified permissions.
 */
function _cm_tools_drush_get_users_with_permission($permission) {
  if ($roles = _cm_tools_drush_get_roles_with_permission($permission)) {
    return _cm_tools_drush_get_users_with_roles($roles);
  }
  return array();
}

/**
 * Copy and enhancement of the built in Drush user sanitization stuff.
 *
 * Here we allow for keeping some of the users unsanitized via a permission.
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
    $message_list[] =  "passwords";
  }

  // Sanitize email addresses.
  $newemail = drush_get_option(array('sanitize-email', 'destination-sanitize-email'), 'user+%uid@localhost.localdomain');
  if ($newemail != 'no' && $newemail !== 0) {
    if (strpos($newemail, '%') !== FALSE) {
      // We need a different sanitization query for Postgres and Mysql.

      $db_driver = $databases['default']['default']['driver'];
      if ($db_driver == 'pgsql') {
        $email_map = array('%uid' => "' || uid || '", '%mail' => "' || replace(mail, '@', '_') || '", '%name' => "' || replace(name, ' ', '_') || '");
        $newmail =  "'" . str_replace(array_keys($email_map), array_values($email_map), $newemail) . "'";
      }
      elseif ($db_driver == 'mssql') {
        $email_map = array('%uid' => "' + uid + '", '%mail' => "' + replace(mail, '@', '_') + '", '%name' => "' + replace(name, ' ', '_') + '");
        $newmail =  "'" . str_replace(array_keys($email_map), array_values($email_map), $newemail) . "'";
      }
      else {
        $email_map = array('%uid' => "', uid, '", '%mail' => "', replace(mail, '@', '_'), '", '%name' => "', replace(name, ' ', '_'), '");
        $newmail =  "concat('" . str_replace(array_keys($email_map), array_values($email_map), $newemail) . "')";
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
        $email_map = array('%uid' => "' || uid || '", '%mail' => "' || replace(mail, '@', '_') || '", '%name' => "' || replace(name, ' ', '_') || '");
        $newuser =  "'" . str_replace(array_keys($email_map), array_values($email_map), $newusername) . "'";
      }
      elseif ($db_driver == 'mssql') {
        $email_map = array('%uid' => "' + uid + '", '%mail' => "' + replace(mail, '@', '_') + '", '%name' => "' + replace(name, ' ', '_') + '");
        $newuser =  "'" . str_replace(array_keys($email_map), array_values($email_map), $newusername) . "'";
      }
      else {
        $email_map = array('%uid' => "', uid, '", '%mail' => "', replace(mail, '@', '_'), '", '%name' => "', replace(name, ' ', '_'), '");
        $newuser =  "concat('" . str_replace(array_keys($email_map), array_values($email_map), $newusername) . "')";
      }
      $user_table_updates[] = "name = $newuser";
    }
    else {
      $user_table_updates[] = "name = '$newusername'";
    }
    $message_list[] = 'usernames';
  }

  if (!empty($user_table_updates)) {
    // Find users with our permission: 'preserve during sanitization'
    $users_to_keep = _cm_tools_drush_get_users_with_permission('preserve during sanitization');

    if (empty($users_to_keep)) {
      $sanitize_query = "UPDATE users SET " . implode(', ', $user_table_updates) . " WHERE uid > 0;";
    }
    else {
      $expressions_to_keep = implode(',', $users_to_keep);
      $sanitize_query = "UPDATE users SET " . implode(', ', $user_table_updates) . " WHERE uid > 0 AND uid NOT IN ($expressions_to_keep);";
    }
    drush_sql_register_post_sync_op('user-email', dt('Reset !message in !table table', array('!message' => implode(' and ', $message_list), '!table' => 'users')), $sanitize_query);
    drush_print_r($sanitize_query);
  }

  // Stop Drush from adding these queries and sanitizing all the users.
  drush_set_option('sanitize-password', 'no');
  drush_set_option('sanitize-email', 'no');
}
