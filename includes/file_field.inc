<?php

/**
 * Helper function for creating a new file value on a node.
 * Can only handle files that are already on the server.
 */
function cm_tools_file_field_file_add($source, $destination, $mimetype = 'image/jpeg', $title = '', $alt = '') {
  $file = new stdClass();
  $file->uid = 0;
  $file->uri = $source;
  $file->filemime = $mimetype;
  $file->filename = basename($file->uri);
  if (dirname($file->uri) != $destination) {
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
  $file->title = $title;
  $file->alt = $alt;
  return (array) $file;
}
