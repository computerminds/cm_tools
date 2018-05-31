<?php

/**
 * Implementation of hook_drush_sql_sync_sanitize().
 */
function cm_tools_drush_sql_sync_sanitize() {
  // Fetch list of all tables.
  $all_tables = drush_sql_get_class()->listTables();


  // User module.
  $queries = array();

  if (in_array('users', $all_tables, TRUE)) {
    $queries[] = "UPDATE users SET name = CONCAT('user-' , uid) WHERE uid > 0";
    $queries[] = "UPDATE users SET mail = CONCAT('user-' , uid, '@example.com'), init = CONCAT('user-' , uid, '@example.com') WHERE uid > 0";
  }

  if (!empty($queries)) {
    $queries[] = '';
    drush_sql_register_post_sync_op('user', dt('Sanitize user information.'), implode(';', $queries));
  }


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
    'thoroughfare' => "CONCAT('R', LEFT(sha(%column), 9), 'Road')",
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

function _cm_tools_drush_get_fields_of_type($field_type) {
  $fields = array();

  $return = drush_sql_get_class()->query("SELECT field_name FROM field_config where type = '$field_type';");
  if ($return) {
    $fields = drush_shell_exec_output();
  }
  return $fields;
}
