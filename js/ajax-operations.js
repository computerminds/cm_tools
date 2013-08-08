(function($) {

  var id_count = 0;

  /**
   * Initiates a Drupal Ajax request to the server
   * outside of the context of a DOM element.
   *
   * @param op
   *   Name of the operation to perform.
   *
   * @param parameters
   *   (Optional) Named parameters to pass up with the operation.
   *   There are some special keys that are not allowed here:
   *
   *     'q'
   *     'js'
   *     Anything beginning with 'ajax'
   *
   * @param callback
   *   (Optional) This function is called for each time the
   *   server responds with a ajax_command_callback() command.
   *   It accepts the following parameters:
   *
   *     data     A map of named parameters returned by the server,
   *     success  Drupal.ajax success value.
   *
   * @param ajax_options
   *   Specify / override ajax options for the $.ajax call.
   *
   * @param path
   *   (Optional) The path to fire the request to (WITH
   *   leading forward-slash /). By default, the request
   *   is handled by CM Tools' Ajax Operations framework.
   *
   * @return XHR object returned by the jQuery.ajax() call.
   */
  Drupal.CMToolsAjaxOperation = function(op, parameters, callback, ajax_options, path) {

    if (!$.isFunction(callback)) {
      callback = function(){};
    }
    parameters = parameters || {}
    ajax_options = ajax_options || {};
    path = path || '/ajax-operation';

    // The only way I can possibly find of making a Drupal 7 Core
    // AJAX request that processes the response as AJAX Commands
    // is by doing it through a DOM Element event.
    var id = 'cm-tools-ajax-operation-' + id_count++;
    var $el = $('<div id="' + id + '"></div>');

    // @see misc/ajax.js
    var element_settings = {};
    element_settings.url = path + '/' + op;
    element_settings.event = 'cm_tools_fake_event';
    element_settings.cm_tools_callback = callback;
    element_settings.submit = parameters;
    element_settings.submit['js'] = true;
    var base = $el.attr('id');
    var ajax = new Drupal.ajax(base, $el, element_settings);

    // We do not allow ajax options passed in to override those
    // provided by Drupal.ajax 'automatically'.
    ajax.options = $.extend({}, ajax_options, ajax.options);

    // We special case Drupal.ajax's callbacks, so the caller
    // *can* override them but they still get called.
    var callbacks = ['beforeSerialize', 'beforeSubmit', 'beforeSend', 'success', 'complete'];
    for (var i = 0; i < callbacks.length; i++) {
      var callback = callbacks[i];
      if (ajax_options[callback]) {
        // Actually set the callback to a function which calls Drupal's original
        // function, and then the new one we want to use.
        (function(original_callback, new_callback){
          ajax.options[callback] = function() {
            var args = Array.prototype.slice.call(arguments);
            var ret;
            ret = original_callback.apply(this, args);
            if (ret !== false) {
              ret = new_callback.apply(this, args);
            }
            return ret;
          }
        })(ajax.options[callback], ajax_options[callback]);
      }
    }

    // Need our own eventResponse callback to capture the XHR
    // object. This is because we want to return that object
    // ourselves.
    ajax.eventResponse = function(element, event) {
      if (this.ajaxing) {
        return false;
      }
      try {
        this.beforeSerialize(this.element, this.options);
        this.xhr = $.ajax(this.options);
      }
      catch (e) {
        this.ajaxing = false;
      }
    }

    // And now trigger our fake event
    $el.trigger('cm_tools_fake_event');
    $el.unbind('cm_tools_fake_event');

    return ajax.xhr;
  }

  // Stick these in a ready to make sure Drupal.ajax is around.
  $(function() {

    // We need the Ajax framework to have been loaded.
    if (!Drupal.ajax) {
      return;
    }

    /**
     * If the initiator of the ajax request provided a cm_tools_callback
     * in the element_settings then we call it directly passing in the
     * data returned from the server.
     *
     * @param ajax
     * @param data
     * @param status
     */
    Drupal.ajax.prototype.commands.cm_tools_callback = function(ajax, data, status) {
      if (ajax.element_settings.cm_tools_callback && $.isFunction(ajax.element_settings.cm_tools_callback)) {
        ajax.element_settings.cm_tools_callback.call(ajax, data.data, status);
      }
    };
  });

})(jQuery);
