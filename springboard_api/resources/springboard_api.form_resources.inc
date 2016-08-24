<?php

/**
 * @file
 * Service endpoint callbacks & access functions.
 */


/**
 * Supplies detailed information on single forms.
 *
 * @param int $nid
 *   Node id of the requested form.
 *
 * @param bool $op
 *   Experimental: if set, function returns rendered markup.
 */
function springboard_api_form_resource_retrieve($nid, $api_key = NULL, $op = NULL) {

  if ($op) {
    return springboard_api_form_action_render($nid);
  }
  $form_details = springboard_api_get_form_details($nid);
  if ($form_details) {
    return $form_details;
    // $messages = $_SESSION['messages'];
   // unset($_SESSION['messages']);
   // return $messages;
  }
  else {
    return services_error("The requested form does not exist.", 404);
  }
}

/**
 * List webforms.
 *
 * @param string $node_type
 *   Optional, filter webform list by node type.
 */
function springboard_api_form_resource_index($node_type = NULL) {
  $form_types = variable_get('webform_node_types', array('webform'));
  // Currently unable to handle social actions, exclude them from list if present.
  $sb_action_types = variable_get('sba_action_types', array());
  if (!empty($sb_action_types) && in_array('sba_social_action', $sb_action_types)) {
    $excluded_types = array('sba_social_action');
  }
  $form_select = db_select('node', 'n');
  $form_select->fields('n', array('nid', 'type', 'title'));
  if (module_exists('fundraiser')) {
    $form_select->leftJoin('field_data_field_fundraiser_internal_name', 'f', 'n.nid = f.entity_id');
    $form_select->addExpression("COALESCE(f.field_fundraiser_internal_name_value, '')", 'internal_name');
  }
  if ($node_type && in_array($node_type, $form_types)) {
    $form_select->condition('type', $node_type, '=');
  }
  else {
    if (!empty($excluded_types)) {
      $form_select->condition('type', $excluded_types, 'NOT IN');
    }
    $form_select->condition('type', $form_types, 'in');
  }
  $results = $form_select->execute();
  return  $results->fetchAll();
}

/**
 * Programmatically submit a webform.
 *
 * @param int $nid
 *   Node id of webform to submit.
 *
 * @param array $submission
 *   Associative array of webform component form_keys and their submitted
 *   values.
 */
function springboard_api_form_action_submit($nid, $app_id = NULL, $submission = array()) {
  if (is_numeric($nid)) {
    $node = node_load($nid);
    // Typecast in case the submission was JSON encoded as an object.
    $submission = (array) $submission;
    // If we have an advocacy type, handle non-webform fields.
    if (module_exists('springboard_advocacy')) {
      if (springboard_advocacy_is_action_type($node->type)) {
        $action_submission = _springboard_api_convert_action_submission($submission, $node);
        if (is_array($action_submission) && !empty($action_submission)) {
          $action_type = TRUE;
        }
      }
    }
    
    // Convert submission values to nested array.
    $submission = _springboard_api_convert_submission($submission, $node);
    // The op value must match the configured submit_text value.
    $submit_text = empty($node->webform['submit_text']) ? t('Submit') : t($node->webform['submit_text']);

    $form_id = 'webform_client_form_' . $nid;
    $form_state['webform_completed'] = 1;
    $form_state['values'] = array(
      'submit' => $submit_text,
      'op' => $submit_text,
      'details' => array(
        'nid' => $nid,
        'sid' => '',
        'uid' => 0,
        'page_num' => 1,
        'page_count' => 1,
        'finished' => 0,
      ),
      'values' => array(),
    );
    if (isset($action_type) && $action_type === TRUE) {
      // Merge in any submitted advocacy fields.
      $form_state['values'] += $action_submission;
    }
    $form_state['clicked_button']['#parents'] = NULL;
    $form_state['values']['submitted'] = springboard_api_submission_tree_build($submission, $node->webform['components'], $tree, 0);
    $webform_submission = array(
      'nid' => $nid,
      'uid' => 0,
      'submitted' => time(),
      'remote_addr' => ip_address(),
      'data' => array(),
    );
    foreach ($node->webform['components'] as $cid => $component) {
      if ($component['type'] != 'fieldset') {
        $fields[] = $component['form_key'];
        if (isset($submission[$component['form_key']]) && !is_array($submission[$component['form_key']])) {
          $webform_submission['data'][$cid]['value'][] = $submission[$component['form_key']];
        }
        elseif (isset($submission[$component['form_key']])) {
          $webform_submission['data'][$cid]['value'] = $submission[$component['form_key']];
        }
        else {
          // Backfill with null data to match what Webform does.
          $webform_submission['data'][$cid]['value'][] = '';
        }
      }
    }
    drupal_form_submit($form_id, $form_state, $node, (object)$webform_submission);
    
    // submission we return a 406 HTTP response code and any errors encountered.
    if ($errors = form_get_errors()) {
      // Strip down error keys to form key.
      $formatted_errors = _springboard_api_reformat_errors($errors);
      return services_error($formatted_errors, 406);
    }
    else {
      // Since $form_state is passed by reference, we have full access to all
      // of the additional data that Webform and Fundraiser have added.
      $sid = $form_state['values']['details']['sid'];
      $response = array(
        'status' => t('Submission successful'),
        'submission_id' => $sid,
      );
      // Allow other modules to alter the response.
      $context = array(
        'node' => $node,
      );
      drupal_alter('springboard_api_response', $response, $context);
      return array($response);
    }
  }
}

/**
 * Reformat form errors.
 *
 * @param array $errors
 *   Associative array of form errors.
 *
 * @return array
 *   Returns array of form errors with error keys stripped down and organized
 *   by form key.
 */
function _springboard_api_reformat_errors($errors) {
  $formatted_errors = array();
  foreach ($errors as $key => $value) {
    if (empty($value)) {
      continue;
    }
    $err = array();
    if (strpos($key, '][') !== FALSE) {
      $parts = explode('][', $key);
      $form_key = end($parts);
      $formatted_errors[] = $form_key . '::' . $value;
    }
    else {
      $formatted_errors[] = $key . '::' . $value;
    }
  }
  return implode(", ", $formatted_errors);
}

/**
 * Render Form markup for a given webform.
 *
 * @return string
 *   Returns the full rendered form HTML.
 */
function springboard_api_form_action_render() {
  $args = func_get_args();
  $nid = $args[0];
  $node = node_load($nid);
  $submission = array();
  $wrapper = isset($args[1]) ? $args[1] : FALSE;

  $form = drupal_get_form('webform_client_form_' . $nid, $node, $submission, $is_draft = FALSE, $filter = TRUE);
  return render($form);
}

/**
 * Build the webform submission tree.
 *
 * @param array $submission
 *   Associative array of webform component form keys and their values.
 *
 * @param array $src
 *   webform components organized by component id.
 *
 * @param array $tree
 *   Output storage.
 *
 * @param mixed $parent
 *   Parent component (if any).
 *
 * @return mixed
 *   Returns the submitted values in a tree structure that Webform can parse.
 */
function springboard_api_submission_tree_build($submission, $src, &$tree, $parent) {

  foreach ($src as $cid => $component) {
    if ($component['pid']) {
      $parent_key = $src[$component['pid']]['form_key'];
      if (!isset($tree[$parent_key])) {
        $tree[$parent_key] = array();
      }
      $tree[$parent_key][$component['form_key']] = isset($submission[$component['form_key']]) ? $submission[$component['form_key']] : NULL;
    }
    else {
      $tree[$component['form_key']] = isset($submission[$component['form_key']]) ? $submission[$component['form_key']] : NULL;
    }

  }
  return $tree;
}
