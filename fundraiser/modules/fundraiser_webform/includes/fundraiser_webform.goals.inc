<?php

/**
 * @file Webform Goals integration hooks and associated helper functions.
 */

/**
 * Implements hook_webform_goals_list_context().
 *
 * Provide custom tracking metrics and UI sugar for donation forms.
 */
function fundraiser_webform_webform_goals_list_metrics($selected_context, $form_state, $goal = array()) {

  // This metric should only be available to donation forms selected in a "single" context
  // or groups if the node type is also a donation node type
  // or form groups if all forms selected in the group are fundraiser enabled.
  if (in_array($selected_context['id'], array('single', 'group', 'form_group'))) {
    $is_fundraiser_goal = fundraiser_webform_is_fundraiser_goal($selected_context, $form_state, $goal);

    if ($is_fundraiser_goal) {
      $metrics['fundraiser_donation_amount'] = array(
        'id' => 'fundraiser_donation_amount',
        'name' => t('Total donation amount'),
        'description' => t('Goal progress will be calculated based on the total donations received.'),
        'settings_callback' => '',
        'form_text' => array(
          'target_value' => array(
            'title' => t('Total donation amount required to meet this goal'),
            'description' => t('Goal progress represents the sum of all successful donations as a percentage of this number.'),
          ),
          'seed' => array(
            'title' => t('Seed goal progress with this amount'),
            'description' => t('Add this number to the current total donation amount when calculating goal progress.'),
          ),
          'display_threshold' => array(
            'title' => t('Minimum total donation amount required to show progress'),
            'description' => t('The goal progress bar will be hidden until total donations reach this amount.'),
          ),
        ),
      );
      return $metrics;
    }
  }
}

/**
 * Implements hook_webform_goals_track_metric().
 */
function fundraiser_webform_webform_goals_track_metric($goal, $params = array()) {
  $data = array(
    'count' => '',
  );
  if ($goal['metric'] === 'fundraiser_donation_amount') {

    // Create an array of nids that we're tracking.
    if (!empty($goal['extra']['selected_form_id'])) {
      // Needs to be an array for the foreach below.
      $nids = array($goal['extra']['selected_form_id']);
    }
    elseif (!empty($goal['extra']['selected_node_type'])) {
      $nids = _webform_goals_nids_by_type($goal['extra']['selected_node_type']);
    }
    elseif (!empty($goal['extra']['selected_form_ids'])) {
      $nids = $goal['extra']['selected_form_ids'];
    }
    else {
      // Default value.
      $nids = array();
    }

    // Grouping the nids together to optimize for the case of multiple nids.
    // This will perform a single query instead of one per nid.
    if (count($nids)) {
      $data['count'] = fundraiser_get_total_donations_by_nids($nids, $goal['start_date'], $goal['end_date']);
    }

    // Adds the seed value and calculates the related values.
    webform_goals_calculate_progress($data, $goal);
    // Pretty format the donation amount..
    $data['count'] = number_format((float) $data['count'], 2);
    // and the goal target..
    $data['target'] = number_format((float) $goal['target_value'], 2);
    return $data;
  }
}

function fundraiser_webform_is_fundraiser_goal($selected_context, $form_state, $goal = array()) {

  if ($selected_context['id'] == 'form_group') {
    if (!empty($form_state['values']['selected_form_ids'])) {
      $nids = $form_state['values']['selected_form_ids'];
    }
    elseif (!empty($goal['extra']['selected_form_ids'])) {
      $nids = $goal['extra']['selected_form_ids'];
    }
    else {
      $nids = array();
    }

    if (count($nids)) {
      $types = db_query("SELECT DISTINCT type FROM {node} WHERE nid IN (:nids)", array(':nids' => $nids));
    }
    else {
      // no forms selected, so total donation amount is not available.
      return FALSE;
    }
    foreach ($types as $type) {
      if (!fundraiser_is_donation_type($type->type)) {
        return FALSE;
      }
    }
    return TRUE;
  }

  if ($selected_context['id'] == 'single' || $selected_context['id'] == 'group') {
    $type = '';
    // @todo Might need to check $form_state before $goal.  Needs testing.
    if (!empty($goal['extra']['selected_form_id'])) {
      $nid = $goal['extra']['selected_form_id'];
    }
    else {
      $nid = !(empty($form_state['values']['selected_form_id'])) ? $form_state['values']['selected_form_id'] : 0;
    }

    if ($selected_context['id'] == 'single' && $nid) {
      $node = node_load($nid);
      $type = $node->type;
    }
    elseif ($selected_context['id'] == 'group') {
      if (isset($form_state['values']['selected_node_type'])) {
        $type = $form_state['values']['selected_node_type'];
      }
      elseif (!empty($goal['extra']['selected_node_type'])) {
        $type = $goal['extra']['selected_node_type'];
      }
    }

    if ($type && fundraiser_is_donation_type($type)) {
      return TRUE;
    }
  }
  return FALSE;
}
