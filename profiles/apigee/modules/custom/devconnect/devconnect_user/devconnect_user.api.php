<?php

/**
 * Allows modules to do something immediately after a user is saved to KMS.
 *
 * @param $results
 * @param $edit
 * @param $account
 */
function hook_devconnect_user_save($results, &$edit, $account) {
  $action = ($account->uid ? 'created' : 'updated');
  $user_name = $edit['name'];
  drupal_set_message("User $user_name was $action in KMS.");
}

/**
 * Allows modules to set a customized terms-and-conditions URL
 *
 * @param $user
 * @return string
 */
function hook_tnc_url($user) {
  if (user_access('administer site configuration', $user)) {
    return 'terms-and-conditions-administrator';
  }
  return 'terms-and-conditions';
}