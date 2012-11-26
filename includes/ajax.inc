<?php

/**
 * @file
 * Helpers for the Drupal Ajax framework. When in a form context you probably
 * want to include this file as follows:
 *
 *   cm_tools_form_include($form_state, 'ajax');
 */

/**
 * Create a Drupal Ajax command which removes any messages on the page already.
 *
 * @return An array suitable for use with the ajax_render() function.
 */
function ajax_command_remove_messages() {
  // If you want to add anything to the page which gets removed at the same
  // time that messages are removed, give it the 'remove-with-messages' class.
  return ajax_command_remove('.messages, .remove-with-messages');
}

/**
 * Create a Drupal Ajax command which renders messages at the top.
 *
 * @return An array suitable for use with the ajax_render() function.
 */
function ajax_command_prepend_messages() {
  return ajax_command_prepend(NULL, theme('status_messages'));
}

/**
 * A re-usable #ajax callback which allows the triggering element to specify
 * which ajax_commands to run, saving the need for lots and lots of callbacks.
 *
 * If an '#ajax_command' parameter is an array with first element '/' then it is
 * regarded as an #array_parents-esque array indicating part of the form. As
 * such, that part of the rebuilt form will be passed to the ajax_command
 * callback in its place. See example below.
 *
 * The return value of each ajax command callback will be ignored if does not
 * conform to the syntax of a drupal ajax command (i.e. a non-empty array). This
 * allows arbitrary other callbacks to be used also. For example the following
 * entry will silently bin all drupal messages:
 *
 *   'theme' => array('status_messages'),
 *
 * @code
 * <?php
 *   cm_tools_form_include($form_state, 'ajax');
 *
 *   $form['myelement'] = array(
 *     '#type' => 'submit',
 *     '#value' => t('Hello'),
 *     '#ajax' => array(
 *       'wrapper' => $myuniquewrapperhtmlid,
 *       'callback' => 'cm_tools_ajax_callback_commands',
 *     ),
 *     '#executes_submit_callback' => TRUE|FALSE,
 *     '#limit_validation_errors' => array(),
 *
 *     // AJAX commands to return to the browser when this button is clicked
 *     // (These are *not* used when form validation failed - something only
 *     // relevant when '#executes_submit_callback' is TRUE, see below):
 *
 *     '#ajax_commands' => array(
 *
 *       // Replace our ['#ajax']['wrapper'] div with part of the form:
 *       'ajax_command_insert' => array(NULL, array('/', 'path', 'to', 'el')),
 *
 *       // Refresh any drupal messages on the page:
 *       'ajax_command_remove_messages' => array(),
 *       'ajax_command_prepend_messages' => array(),
 *
 *       // OR perhaps we want to just silently bin all drupal messages so they
 *       // do not show here, nor on any subsequent page load:
 *        'theme' => array('status_messages'),
 *     ),
 *
 *     // AJAX commands to return instead of the above when validation failed
 *     // (Only relevant where '#executes_submit_callback' is TRUE):
 *     '#ajax_invalid_commands' => array(
 *     ),
 *   );
 */
function cm_tools_ajax_callback_commands(&$form, &$form_state) {

  $commands = array();

  // Support arbitrary ajax commands.
  $triggering_element_commands = NULL;
  if (isset($form_state['triggering_element']['#ajax_commands'])) {
    $triggering_element_commands = $form_state['triggering_element']['#ajax_commands'];
  }

  // If there were errors (in cases where validation occured), then we take our
  // ajax commands from a different place - #ajax_invalid_commands.
  if (form_get_errors()) {
    $triggering_element_commands = NULL;
    if (isset($form_state['triggering_element']['#ajax_invalid_commands'])) {
      $triggering_element_commands = $form_state['triggering_element']['#ajax_invalid_commands'];
    }
  }

  // If no ajax commands are specified, default to at least printing messages.
  if (!isset($triggering_element_commands)) {
    $triggering_element_commands['ajax_command_remove_messages'] = array();
    $triggering_element_commands['ajax_command_prepend_messages'] = array();
  }

  if (!empty($triggering_element_commands)) {
    foreach ($triggering_element_commands as $function => $parameters) {

      // This line allows people to call the same callback twice in the same
      // array by affixing spaces. A drawback of using array keys to indicate
      // function names.
      $function = trim($function);

      if (is_callable($function)) {

        // Support #array_parents-esque args, these are indicated by an array
        // value with '/' as the first value.
        foreach ($parameters as $k => $p) {
          if (is_array($p) && $p[0] === '/') {
            $single = FALSE;
            if (!is_array($p[1])) {
              $single = TRUE;
              $parameters[$k] = array('/', array_slice($parameters[$k], 1));
            }
            array_shift($parameters[$k]);
            foreach ($parameters[$k] as $pk => $array_parents) {
              $parameters[$k][$pk] = cm_tools_form_get_relative_element($array_parents, $form, $form_state['triggering_element'], TRUE);
              if (empty($parameters[$k][$pk])) {
                $parameters[$k][$pk] = NULL;
              }
              else {
                $parameters[$k][$pk] = drupal_render($parameters[$k][$pk]);
              }
            }
            if ($single) {
              $parameters[$k] = reset($parameters[$k]);
            }
          }
        }

        $command = call_user_func_array($function, $parameters);
        if (!empty($command) && is_array($command)) {
          $commands[] = $command;
        }
      }
    }
  }

  return array(
    '#type' => 'ajax',
    '#commands' => $commands,
  );
}

/**
 * Filters a given FAPI 'parents' property, allowing it to specify
 * it's parents relatively using '.' and '..' prefixes.
 *
 * @param $parents array of the format:
 *       array('path', 'to', 'another_element')
 *   OR  array('..', '..', 'relative', 'path')
 *
 * @param $form
 *   The whole form in which the relative element could reside
 *
 * @param $root_element
 *   The root element from which relative paths should be resolved.
 *
 * @param $return_element
 *   (Optional) If TRUE then the actual form element will be returned
 *   and not just the #parents array describing its location.
 *
 * Note. This function will only work once all elements have had their #parents
 * set. This is after the #process callbacks have been executed.
 *
 * @return The #parents of the element being referenced.
 */
function cm_tools_form_get_relative_element($parents, $form, $root_element, $return_element = FALSE) {

  if (isset($parents)) {

    reset($parents);
    $root = NULL;
    while (!empty($parents) && ($parent = reset($parents)) && substr($parent, 0, 1) == '.') {

      array_shift($parents);

      if (!isset($root)) {
        $root = $root_element['#parents'];
      }

      switch ($parent) {
        case '..':
          if (!empty($root)) {
            array_pop($root);
          }
          break;
      }
    }

    // This means that the path was never relative
    // Just return the $parents directly
    if (empty($root)) {
      $root = array();
    }

    if ($return_element) {
      return drupal_array_get_nested_value($form, array_merge($root, $parents));
    }
    else {
      return array_merge($root, $parents);
    }
  }

  return $parents;
}
