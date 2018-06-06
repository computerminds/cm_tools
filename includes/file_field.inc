<?php

/**
 * @file
 */

/**
 * Helper function for creating a new file value on a node.
 * Can only handle files that are already on the server.
 *
 * @param $source
 *   A string specifying the filepath or URI of the source file.
 * @param $destination
 *   A URI containing the destination that $source should be copied to. It may
 *   be one of the following:
 *    1. NULL - copy file to the default scheme (probably 'public://'),
 *    2. A pre-existing directory - copy file inside the directory,
 *    3. A non-existant path - copy file to destination exactly.
 *    4. The same as $source - do not copy file, create managed file record for
 *       the $source location itself and return that,
 *   If $destination is a pre-existing filepath then this function will do
 *   nothing and will return FALSE.
 *
 * @param ...
 *   Other metadata about the file.
 */
function cm_tools_file_field_file_add($source, $destination = NULL, $mimetype = 'image/jpeg', $title = '', $alt = '') {
  $file = new stdClass();
  $file->uid = 0;
  $file->uri = $source;
  $file->filemime = $mimetype;
  $file->filename = basename($file->uri);
  if (dirname($file->uri) !== $destination) {
    // Unlike file_copy, we auto-create parent directories as necessary.
    if (isset($destination) && !file_prepare_directory($destination)) {
      $dirname = drupal_dirname($destination);
      file_prepare_directory($dirname, FILE_CREATE_DIRECTORY | FILE_MODIFY_PERMISSIONS);
    }
    $file = file_copy($file, $destination);
  }
  else {
    // If file is not to be copied, check whether file already exists,
    // as file_save() won't do that for us (compare file_copy() and
    // file_save())
    $existing_files = file_load_multiple(array(), array('uri' => $file->uri));
    if (count($existing_files)) {
      $existing = reset($existing_files);
      $file->fid = $existing->fid;
      $file->filename = $existing->filename;
    }
    file_save($file);
  }
  if ($file) {
    $file->title = $title;
    $file->alt = $alt;
    return (array) $file;
  }
  return FALSE;
}
