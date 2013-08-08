<?php

/**
 * @file CMTools Ajax Operation Framework.
 *
 * Provides a mechanism for javascript to make arbitrary Ajax
 * requests to the server. It consists of two components:
 *
 *   1. Client-side Drupal.CMToolsAjaxOperation javascript method.
 *
 *      Custom javascript code can call this method to invoke
 *      a Drupal.ajax request outside of the context of a DOM
 *      Element. Such code passes an 'op' name which CMTools
 *      resolves to an AjaxOperation handler server side. E.g.
 *
 *        var xhr = Drupal.CMToolsAjaxOperation('mymodule_operation_name', {param: 'value'}, function(data) {
 *          // This is a direct callback which is invoked for each instance
 *          // of cm_tools_ajax_command_callback() returned by the server.
 *        });
 *
 *      The CMTools Ajax Operations javascript will need to have
 *      been already included on the page for this method to be
 *      present. This can be achieved with the following code:
 *
 *        drupal_add_library('system', 'drupal.ajax');
 *        cm_tools_add_js('ajax-operations');
 *
 *      See cm_tools/js/ajax-operations.js for full details of the
 *      parameters accepted by Drupal.CMToolsAjaxOperation(). In
 *      most cases it is not too hard to replace out some custom
 *      javascript's call to jQuery.ajax() with this method.
 *
 *   2. Server-side AjaxOperation handler.
 *
 *     You will need to declare a handler for your operation. This is done
 *     CTools-stylee as follows:
 *
 *       In mymodule.module:
 *
 *       /**
 *        * Implements hook_ctools_plugin_api().
 *        * /
 *       function mymodule_ctools_plugin_api($owner, $api) {
 *         if ($owner === 'cm_tools' && $api === 'ajax_operation') {
 *           return 1;
 *         }
 *       }
 *
 *       In mymodule.ajax_operations.inc:
 *
 *       /**
 *        * Implements hook_cm_tools_ajax_operations().
 *        * /
 *       function mymodule_cm_tools_ajax_operations() {
 *         $ops = array();
 *
 *         $ops['mymodule_operation_name'] = array(
 *           'label' => t('My Module Operation'),
 *
 *           // Named parameters you expect, and default
 *           // values for when they are not included.
 *           'parameters' => array(),
 *
 *           // Details of where to find your handler.
 *           'handler' => array(
 *             'class' => 'MyModuleAjaxOperationNAME',
 *             'file' => 'MyModuleAjaxOperationNAME.inc',
 *             'path' => drupal_get_path('module', 'mymodule') . '/handlers/ajax_operations',
 *           ),
 *         );
 *         return $ops;
 *       }
 *
 *       And finally your handler
 *       (in e.g. mymodule/handlers/ajax_operations/MyModuleAjaxOperationNAME.inc):
 *
 *         class MyModuleAjaxOperationNAME extends CMToolsAjaxOperation {
 *
 *           /**
 *            * {@inheritdoc}
 *            * /
 *           public function execute($parameters) {
 *             $commands = array();
 *
 *             // Do whatever you need to do and return
 *             // an array of ajax commands.
 *
 *             // You may use the special cm_tools_callback command
 *             // which directly calls the callback passed to
 *             // Drupal.CMToolsAjaxOperation with an arbitrary array
 *             // of parameters.
 *             $commands[] = cm_tools_ajax_command_callback($params_to_return);
 *
 *             return $commands;
 *           }
 *         }
 *
 *   NOTE: Any module may alter the array of commands returned for any operation
 *   by implementing the following hooks:
 *
 *     // Generally
 *     hook_cm_tools_ajax_operation_commands_alter(&$commands, &$context).
 *
 *     // Or more specifically
 *     hook_cm_tools_ajax_operation_OPNAME_commands_alter(&$commands, &$context).
 */

/**
 * Menu callback for an Ajax operation.
 *
 * @param CMToolsAjaxOperationInterface $op
 *   The Ajax operation to execute.
 *
 * @return array of Drupal Ajax commands.
 */
function cm_tools_ajax_operation_callback(CMToolsAjaxOperationInterface $op) {

  $all_parameters = $_GET + $_POST;
  unset($all_parameters['js']);
  unset($all_parameters['q']);
  unset($all_parameters['ajax_page_state']);
  unset($all_parameters['ajax_html_ids']);

  // Only the pass the requested parameters to the op.
  $parameters = array_intersect_key($all_parameters, $op->getPluginInfo('parameters'));
  // Use default values where no parameter was passed.
  $parameters += $op->getPluginInfo('parameters');

  $commands = $op->execute($parameters);

  // Allow others to alter the result.
  $context = array(
    'op' => $op,
    'operation_name' => $op->getPluginInfo('name'),
    'parameters' => &$parameters,
  );
  drupal_alter(array(
    'cm_tools_ajax_operation_commands',
    'cm_tools_ajax_operation_' . $op->getPluginInfo('name') . '_commands'
  ), $commands, $context);

  return array(
    '#type' => 'ajax',
    '#commands' => $commands,
  );
}

/**
 * Directly calls the Ajax request's initiator's callback.
 *
 * This will only work if the request was initiated by a call
 * (in javascript) to Drupal.CMToolsAjaxOperation() (OR some
 * other intelligently customized invocation of Drupal.Ajax
 * which provided a ctools_custom_callback.
 *
 * @param array $parameters
 *   Arbitrary array of parameters to pass to the callback.
 *
 * @return array
 */
function cm_tools_ajax_command_callback($parameters) {
  cm_tools_add_js('ajax-operations');
  return array(
    'command' => 'cm_tools_callback',
    'data' => $parameters,
  );
}
