<?php

/**
 * @file
 * Functions and pages for facebook oauth with P2P.
 */

/**
 * Page where user goes after connecting with facebook oauth.
 *
 * Checks if the user is already registered for p2p and if not, checks if
 * he has filled out all required fields.  If not, goes to the complete profile
 * page.
 *
 * If the user is already registered, go to the normal destination.
 */
function springboard_p2p_fb_page() {
  global $user;
  $uid = $user->uid;

  if (!springboard_p2p_user_is_registered_for_p2p($uid)) {
    $event = new SpringboardP2pEvents();
    $event->userRegistersForP2P($user);

    if (springboard_p2p_complete_profile_user_has_empty_required_fields($user)) {
      $redirect = springboard_p2p_complete_profile_get_redirect();
      drupal_goto($redirect['path'], $redirect['options']);
    }
  }

  // Faking the form submission so the approval status will get saved
  // if needed and we can find out where to go next.
  $form = array();
  $form_state = array(
    'values' => array(
      'springboard_p2p_campaign_nid' => springboard_p2p_get_campaign_id_from_request(),
    ),
    'uid' => $uid,
  );
  springboard_p2p_set_redirect($form, $form_state);

  drupal_goto($form_state['redirect']);

}