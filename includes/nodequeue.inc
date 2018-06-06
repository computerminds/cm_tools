<?php

/**
 * @file
 */

/**
 * Create a new nodequeue.
 *
 * @code
 * $options = array(
 *   'show_in_links' => 0,
 *   'roles' => array_keys(user_roles(FALSE, 'manipulate queues')),
 *   'types' => array(
 *     'blog',
 *   ),
 * );
 * cm_tools_nodequeue_create('blogs', 'Blogs', $options);
 * @endcode
 *
 * @param $name
 *   The machine name of the nodequeue.
 * @param $title
 *   The human-readable title of the nodequeue.
 * @param $options
 *   An array of options to use when creating the nodequeue.
 *
 * @return int
 *   The nodequeue's qid.
 */
function cm_tools_nodequeue_create($name, $title, $options = array()) {
  $defaults = array(
    'roles' => array(),
    'types' => array(),
    'size' => 0,
    'reverse' => 0,
    'link' => '',
    'link_remove' => '',
    'subqueue_title' => '',
    'owner' => 'nodequeue',
    'show_in_ui' => 1,
    'show_in_tab' => 1,
    'show_in_links' => 1,
    'reference' => 0,
    'i18n' => 1,
  );

  $options += $defaults;
  $options['name'] = $name;
  $options['title'] = $title;

  $nodequeue_schema = drupal_get_schema('nodequeue_queue');

  $qry = db_insert('nodequeue_queue', array('return' => Database::RETURN_INSERT_ID))
    ->fields(array_intersect_key($options, $nodequeue_schema['fields']));
  $qid = $qry->execute();

  if (function_exists('views_invalidate_cache')) {
    views_invalidate_cache();
  }

  // Add each role to the nodequeue.
  foreach ($options['roles'] as $rid) {
    cm_tools_nodequeue_add_role($qid, $rid);
  }

  // Add each content type to the nodequeue.
  foreach ($options['types'] as $type) {
    cm_tools_nodequeue_add_type($qid, $type);
  }

  // Add the subqueue, just pass in the $ref=$qid for now.
  cm_tools_nodequeue_add_sub($qid, $qid, $options['title']);

  // All done.
  return $qid;
}

/**
 * Add a role to a nodequeue.
 *
 * @param $qid
 *   The nodequeue's qid.
 * @param $rid
 *   The user role's rid.
 */
function cm_tools_nodequeue_add_role($qid, $rid) {
  $id = db_insert('nodequeue_roles', array('return' => Database::RETURN_INSERT_ID))
    ->fields(array(
      'qid' => $qid,
      'rid' => $rid,
    ))
    ->execute();
}

/**
 * Add a role to a nodequeue.
 *
 * @param $qid
 *   The nodequeue's qid.
 * @param $type
 *   The content type to add.
 */
function cm_tools_nodequeue_add_type($qid, $type) {
  $id = db_insert('nodequeue_types', array('return' => Database::RETURN_INSERT_ID))
    ->fields(array(
      'qid' => $qid,
      'type' => $type,
    ))
    ->execute();
}

/**
 * Add a subqueue to a nodequeue.
 *
 * @param $qid
 *   The nodequeue's qid.
 * @param $ref
 *   The ?
 * @param $title
 *   The
 *
 * @return
 *   The corresponding sqid (int).
 */
function cm_tools_nodequeue_add_sub($qid, $ref, $title) {
  $qry = db_insert('nodequeue_subqueue', array('return' => Database::RETURN_INSERT_ID))
    ->fields(array(
      'qid' => $qid,
      'reference' => $ref,
      'title' => $title,
    ));
  $sqid = $qry->execute();
  return $sqid;
}
