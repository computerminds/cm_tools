<?php
/**
 * @file Views-view-cm-tools-bare.tpl.php
 * Default simple view template to display a list of rows but without the
 * wrapping markup.
 *
 * @ingroup views_templates
 */
?>
<?php if (!empty($title)): ?>
  <h3><?php print $title; ?></h3>
<?php endif; ?>
<?php foreach ($rows as $id => $row): ?>
  <?php print $row; ?>
<?php endforeach; ?>
