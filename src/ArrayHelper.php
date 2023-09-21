<?php

namespace Drupal\cm_tools;

/**
 * A collection of useful array manipulation functions.
 */
class ArrayHelper {

  /**
   * Renames an array key while leaving it in exactly the same place in the array.
   *
   * @param array $haystack
   *   The array. This is modified by reference.
   * @param int|string $key
   *   The key to rename.
   * @param int|string $new_key
   *   The new name for the key. If this already exists in the array it will be
   *   overwritten.
   *
   * @return bool
   *   Success, either TRUE or FALSE.
   */
  public static function cm_tools_array_rename_key(array &$haystack, $key, $new_key) {
    $success = FALSE;

    if (isset($haystack[$key])) {
      $insertion = array(
        $new_key => $haystack[$key],
      );
      if ($success = static::cm_tools_array_insert_at_key($haystack, $key, $insertion)) {
        if ($key !== $new_key) {
          unset($haystack[$key]);
        }
      }
    }

    return $success;
  }

  /**
   * Inserts a new element into an array, just after the one with the given value.
   *
   * The insert value is found by *strict* comparison. If multiple values are
   * found the insertion will be made next to the first occurance only.
   *
   * @param array $haystack
   *   The array. This is modified by reference.
   * @param mixed $insert_value
   *   Array value after which to insert $insertion. A *strict* comparison is
   *   performed when searching for this value.
   * @param mixed $insertions
   *   Array of new values to insert into the array. If $preserve_keys is TRUE and
   *   an element already exists in the array with the same key as one you are
   *   inserting, the old one(s) will removed. If this parameter is not an array
   *   it will be inserted into the array with the lowest available numeric key.
   * @param bool $insert_before
   *   (Optional) Change the behaviour of this function to insert *before*
   *   $insert_value instead of after it.
   * @param bool $preserve_keys
   *   (Optional) By default this function ignores keys, giving everything a new
   *   numeric index. Pass TRUE here to indicate that all keys should be
   *   preserved.
   *
   * @return bool
   *   Success, either TRUE or FALSE.
   */
  public static function cm_tools_array_insert_at_value(array &$haystack, $insert_value, $insertions, $insert_before = FALSE, $preserve_keys = FALSE) {
    $success = FALSE;
    $insert_key = array_search($insert_value, $haystack, TRUE);
    if ($insert_key !== FALSE) {
      $success = static::cm_tools_array_insert_at_key($haystack, $insert_key, $insertions, $insert_before, $preserve_keys);
    }
    return $success;
  }

  /**
   * Inserts a new element into an array, just after the one with the given key.
   *
   * The insert key is found by *strict* comparison.
   *
   * @param array $haystack
   *   The array. This is modified by reference.
   * @param int|string|array $insert_key
   *   Array key after which to insert $insertion. This can also be an array of
   *   suggested keys, in which case each will be looked for in turn and the
   *   first existing one will be used. A *strict* comparison is performed when
   *   searching for this key.
   * @param mixed $insertions
   *   Array of new values to insert into the array. Unless $preserve_keys is
   *   FALSE if an element already exists in the array with the same key as one
   *   you are inserting, the old one(s) will removed. If this parameter is not an
   *   array it will be inserted into the array with the lowest available numeric
   *   key.
   * @param bool $insert_before
   *   (Optional) Change the behaviour of this function to insert *before*
   *   $insert_key instead of after it.
   * @param bool|true $preserve_keys
   *   (Optional) By default this function always preserves keys, even for
   *   numerically-indexed arrays. Pass FALSE here if using a numerically-indexed
   *   array and you don't want to overwrite elements.
   *
   * @return bool
   *   Success, either TRUE or FALSE.
   */
  public static function cm_tools_array_insert_at_key(array &$haystack, $insert_key, $insertions, $insert_before = FALSE, $preserve_keys = TRUE) {

    $success = FALSE;

    if (!is_array($insert_key)) {
      $insert_key = array($insert_key);
    }

    $haystack_keys = array_keys($haystack);
    $offset = FALSE;
    while (count($insert_key) && $offset === FALSE) {
      $loc = array_shift($insert_key);
      $offset = array_search($loc, $haystack_keys, TRUE);
    }

    if ($offset !== FALSE) {
      if ($insert_before) {
        $offset = $offset - 1;
      }
      $success = static::cm_tools_array_insert_at_offset($haystack, $offset + 1, $insertions, $preserve_keys);
    }

    return $success;
  }

  /**
   * Inserts a new element at an offset in an array.
   *
   * @param array $haystack
   *   The array. This is modified by reference.
   * @param int $offset
   *   Offset after which to insert $insertion.
   * @param mixed $insertions
   *   Array of new values to insert into the array. If $preserve_keys is TRUE and
   *   an element already exists in the array with the same key as one you are
   *   inserting, the old one(s) will removed. If this parameter is not an array
   *   it will be inserted into the array with the lowest available numeric key.
   * @param bool $preserve_keys
   *   (Optional) By default this function ignores keys, giving everything a new
   *   numeric index. Pass TRUE here to indicate that all keys should be
   *   preserved.
   *
   * @return bool
   *   Success, either TRUE or FALSE.
   */
  public static function cm_tools_array_insert_at_offset(array &$haystack, $offset, $insertions, $preserve_keys = FALSE) {

    $success = FALSE;

    if (!is_int($offset)) {
      return FALSE;
    }

    if (!$preserve_keys) {
      $haystack = array_values($haystack);
    }

    // Insertions not given as an array. Give it the lowest available numeric key.
    if (!is_array($insertions)) {
      $numeric_keys = array_filter(array_keys($haystack), 'is_numeric');
      $unique_key = !empty($numeric_keys) ? max($numeric_keys) + 1 : 0;
      $insertions = array($unique_key => $insertions);
    }

    if (!$preserve_keys) {
      $insertions = array_values($insertions);
    }

    // Wipe out any keys we're about to overwrite, and ensure we modify the
    // $offset to take into account the changed $haystack.
    if ($preserve_keys) {
      $o = 0;
      foreach (array_keys($haystack) as $k) {
        if (isset($insertions[$k])) {
          if ($o < $offset) {
            $offset--;
          }
          unset($haystack[$k]);
        }
        $o++;
      }
    }

    $before = array_slice($haystack, 0, $offset, TRUE);
    $after = array_slice($haystack, $offset, count($haystack), TRUE);

    if (!$preserve_keys) {
      $haystack = array_merge($before, $insertions, $after);
      $success = TRUE;
    }
    else {
      $keys = array_merge(array_keys($before), array_keys($insertions), array_keys($after));
      $values = array_merge($before, $insertions, $after);
      $haystack = array_combine($keys, $values);
      $success = TRUE;
    }

    return $success;
  }

  /**
   * Remove element(s) with given value(s) from an array.
   *
   * @param array $haystack
   *   The array. This is modified by reference.
   * @param string|array $values
   *   A string value or array of values to remove from the array.
   */
  public static function cm_tools_array_remove_values(array &$haystack, $values) {
    // Make values an array.
    if (is_string($values)) {
      $values = array($values);
    }

    foreach ($values as $value_to_remove) {
      $key = array_search($value_to_remove, $haystack);
      if ($key !== FALSE) {
        unset($haystack[$key]);
      }
    }
  }

  /**
   * Sort an array by values using a user-defined comparison function.
   *
   * However, unlike usort, equal values maintain their order.
   *
   * The comparison function must return an integer less than, equal to, or
   * greater than zero if the first argument is considered to be
   * respectively less than, equal to, or greater than the second.
   *
   * @param array $array
   *   The input array.
   * @param callable $cmp_function
   *   A comparision function that will abide by the usual semantics of a
   *   comparision callback. See @usort.
   *
   * @return int
   *   The result of the sort.
   */
  public static function cm_tools_stable_usort(array &$array, callable $cmp_function) {
    $index = 0;
    foreach ($array as &$item) {
      $item = array($index++, $item);
    }
    $result = usort($array, function ($a, $b) use ($cmp_function) {
      $result = call_user_func($cmp_function, $a[1], $b[1]);
      return $result == 0 ? $a[0] - $b[0] : $result;
    });
    foreach ($array as &$item) {
      $item = $item[1];
    }
    return $result;
  }

  /**
   * Sort an array by user-defined comparison function, preserving indexes.
   *
   * However, unlike uasort, equal values maintain their order.
   *
   * The comparison function must return an integer less than, equal to, or
   * greater than zero if the first argument is considered to be
   * respectively less than, equal to, or greater than the second.
   *
   * @param array $array
   *   The input array.
   * @param callable $cmp_function
   *   A comparision function that will abide by the usual semantics of a
   *   comparision callback. See @usort.
   *
   * @return int
   *   The result of the sort.
   */
  public static function cm_tools_stable_uasort(array &$array, callable $cmp_function) {
    $index = 0;
    foreach ($array as &$item) {
      $item = array($index++, $item);
    }
    $result = uasort($array, function ($a, $b) use ($cmp_function) {
      $result = call_user_func($cmp_function, $a[1], $b[1]);
      return $result == 0 ? $a[0] - $b[0] : $result;
    });
    foreach ($array as &$item) {
      $item = $item[1];
    }
    return $result;
  }
}
