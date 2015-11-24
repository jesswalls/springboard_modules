<?php

/**
 * Message action settings page, menu callback.
 *
 * @return array
 */
function sba_message_action_message_preview_page($nid) {
  // $nid already sanitized in the access hook.
  $node = node_load($nid);
  if (isset($_SESSION['messages']['status'])) {
    // Prevent the confirmation message from appearing prematurely.
    $_SESSION['saved_messages']['status'] = $_SESSION['messages']['status'];
    // clear all
    drupal_get_messages('status');
  }
  // Check if the step two form has already been submitted.
  // @todo think about this.
  $completed = isset($_SESSION['action_completed']) ? $_SESSION['action_completed'] : TRUE;
  if($completed == TRUE) {
    drupal_set_message(t("This form has already been submitted. !return to the form.", array('!return' => l('Return', 'node/' . $node->nid))));
    return array();
  }

  // Build page header and multi-step form.
  $build = array();
  $build['content'] = array(
    'page_description_content' => array(
      '#id' => 'message-action-preview-page',
      '#type' => 'markup',
      '#markup' => t('Please review the messages below; or if you prefer, simply <span id="multiflow-send-now-wrapper" style="display:inline-block"><a href="#" id="multiflow-send-now">send now</a></span>.'),
      '#prefix' => '<div class="message-action-preview-page"><p>',
      '#suffix' => '</p></div>',
      '#weight' => -8,
    ),
    'contact_description' => array(
      '#id' => 'message-action-preview-contact',
      '#type' => 'markup',
      '#markup' => t('These message(s) were generated based on the address you entered: ') . check_markup($_SESSION['contact']['full_address'], 'filtered_html')
        . t('If this is incorrect please !click_here to enter a different address.', array('!click_here' => '<a href="#" onclick="history.go(-1);return false;">click here</a>')),
      '#prefix' => '<div class="message-action-preview-contact"><p>',
      '#suffix' => '</p></div>',
      '#weight' => -7,
    ),
    'sba_message_action_preview_form' => drupal_get_form('sba_message_action_preview_form', $node),
  );

  $messages = $build['content']['sba_message_action_preview_form']['messages'];
  $children = array_flip(element_children($messages));
  $subject_required = FALSE;
  $body_required = FALSE;
  $editable_but_not_required = FALSE;
  foreach ($messages as $key => $message) {
    if(isset($children[$key]) && (!$subject_required && !$body_required)) {
      if(!isset($message['subject_show'])) {
        $editable_but_not_required = TRUE;
        $subject_required = $message['message']['subject']['#required'];
      }
      if (isset($message['message']['edited_body'])) {
        $editable_but_not_required = TRUE;
        $body_required = $message['message']['edited_body']['#required'];
      }
    }
  }

  $build['content']['page_description_label']['#prefix'] = '<h4>';
  $build['content']['page_description_label']['#suffix'] = '</h4>';
  $build['content']['page_description_label']['#weight'] = -10;
  $build['content']['page_description_label']['#markup'] = t('Step 2 of 2');


  if ($subject_required || $body_required) {
    $editable_but_not_required = FALSE;
    $build['content']['page_description_content']['#markup'] = t('One or more of the messages below requires editing. Please make your edits and then click "Send Now" at the bottom of the page.');
  }

  if (!empty($editable_but_not_required)) {
    $build['content']['page_description_content']['#markup'] = t('Please review and edit the messages below; or if you prefer, simply <span id="multiflow-send-now-wrapper"><a href="#" id="multiflow-send-now">send now</a></span>.');
  }

  return $build;
}

/**
 * Multi-step form definition.
 *
 * @param $form
 * @param $form_state
 * @param $node
 *
 * @return mixed
 */
function sba_message_action_preview_form($form, $form_state, $node) {

  $form['#attached']['css'][] = drupal_get_path('module', 'sba_message_action') . '/css/message-preview.css';
  $form['#attached']['js'][] = drupal_get_path('module', 'sba_message_action') . '/js/sba_message_preview.js';

  // Build the form content out of the API response stored in session.
  $messages = array();
  if (!empty($_SESSION['delivered_messages']['messages'])) {

    foreach ($_SESSION['delivered_messages']['messages'] as $id => $message) {
      $messages[$node->messages[$message['message_id']]->data['weight'] . $message['person']['organization'] . $message['targets']->last_name] = $message;
      unset($messages[$id]);
    }
  }

  if (!empty($_SESSION['undelivered_messages']['messages'])) {
    foreach ($_SESSION['undelivered_messages']['messages'] as $message) {
      $messages[$node->messages[$message['message_id']]->data['weight'] . $message['person']['organization'] . $message['targets']->last_name] = $message;
    }
  }

  ksort($messages);

  // @todo explore other ways of retrieving these two.
  $contact = $_SESSION['contact'];
  $action = $_SESSION['form_details'];

  $form['action_id'] = array(
    '#type' => 'value',
    '#value' => $action['action_id'],
  );

  if (isset($action['test_mode'])) {
    $form['test_mode'] = array(
      '#type' => 'value',
      '#value' => $action['test_mode'],
    );

    $form['test_mode_email'] = array(
      '#type' => 'value',
      '#value' => $action['test_email'],
    );
  }

  // Maybe use this for cross-check validation on api server.
  $form['account_id'] = array(
    '#type' => 'value',
    '#value' => $action['account_id'],
  );

  $form['contact'] = array(
    '#type' => 'value',
    '#value' => $contact,
  );

  $form['messages'] = array (
    '#type' => 'container',
    '#tree' => TRUE,
    '#attributes' => array(
      'class' => array('message-preview-container'),
    ),
  );

  // Used to send our second step form through the same helper functions as the first step form.
  module_load_include('inc', 'sba_message_action', 'includes/sba_message_action.form');

  foreach ($messages as $key => $message) {

    // Theme the message header.
    $form['messages'][$key]['sent_to'] = array(
      '#markup' => theme('sba_message_action_preview_message_header', array('message' => $message)),
    );

    // Create the message form elements from the entity. Passed by reference
    // to the same helper function as the first step form.
    $message_entity = $node->messages[$message['message_id']];
    $build = array();
    sba_message_action_message_form($build, $message_entity, 'multi_flow');

    // Customize the resulting form to multi_flow format.
    // Remove title.
    unset($build['message'][$message_entity->sba_message_id]['subject_show']['#title']);
    // Format greeting
    $greeting = token_replace($build['message'][$message_entity->sba_message_id]['greeting']['#value'], array('sba_target' => (Object) $message['targets']));
    $build['message'][$message_entity->sba_message_id]['greeting_show']['#markup'] = $greeting;
    unset($build['message'][$message_entity->sba_message_id]['greeting']);
    // Format signature.
    $signature = token_replace($build['message'][$message_entity->sba_message_id]['signature_tokenized']['#value'], array('sba_contact' => (Object) $contact));
    $build['message'][$message_entity->sba_message_id]['signature']['#type'] = 'item';
    $build['message'][$message_entity->sba_message_id]['signature']['#markup'] = check_markup($signature, 'plain_text');
    unset($build['message'][$message_entity->sba_message_id]['signature_tokenized']);
    // Add a custom class.
    $build['message'][$message_entity->sba_message_id]['#attributes']['class'][] = 'message-preview-message-fieldset';
    // Add precedence value.
    $build['message'][$message_entity->sba_message_id]['precedence'] = array(
      '#type' => 'value',
      '#value' => $message['precedence'],
    );
    // Add hold value.
    $build['message'][$message_entity->sba_message_id]['hold'] = array(
      '#type' => 'value',
      '#value' => isset($message['hold']) ? TRUE : FALSE,
    );
    // Build an array of target properties
    $build['message'][$message_entity->sba_message_id]['target'] = array(
      '#type' => 'value',
      '#value' => array('target_id' => $message['targets']->id, 'primary_delivery_method' => $message['targets']->primary_delivery_method, 'salutation' => $message['targets']->salutation, 'first_name' => $message['targets']->first_name, 'last_name' => $message['targets']->last_name),
    );

    $build['message'][$message_entity->sba_message_id]['body_show']['#title'] = '';
    $build['message'][$message_entity->sba_message_id]['subject']['#title'] = '';
    // Check for required fields
    if (isset($build['message'][$message_entity->sba_message_id]['body_required']) && $build['message'][$message_entity->sba_message_id]['body_required']['#value']) {
      $build['message'][$message_entity->sba_message_id]['edited_body']['#required'] = TRUE;
      $build['message'][$message_entity->sba_message_id]['edited_body']['#title'] =  t('You must edit the body of this message before sending');
    }
    if (isset($build['message'][$message_entity->sba_message_id]['subject_required']) && $build['message'][$message_entity->sba_message_id]['subject_required']['#value']) {
      $build['message'][$message_entity->sba_message_id]['subject']['#required'] = TRUE;
      $build['message'][$message_entity->sba_message_id]['subject']['#title'] =  t('You must edit this subject line before sending');
    }
    // Assign the rebuilt messages to the form
    $form['messages'][$key]['message'] = $build['message'][$message_entity->sba_message_id];
  }
  // Add the node object to use in validation and submit.
  $form['#action_node'] = $node;
  // Define the validation function.
  $form['#validate'][] = 'sba_message_action_preview_form_validate';
  // Add submit button
  $form['submit'] = array(
    '#prefix' => '<div id="edit-actions">',
    '#suffix' => '</div>',
    '#type' => 'submit',
    '#value' => t('Send Now'),
    '#weight' => 10,
    '#attributes' => array('class' => 'message-action-preview-submit'),
  );

  return $form;
}

/**
 * Validate the  messages.
 *
 * @param $form
 * @param $form_state
 */
function sba_message_action_preview_form_validate($form, &$form_state) {

  $messages = !empty($form['messages']) ? element_children($form['messages']) : array();
  $changed = FALSE;
  // To prevent duplicate error messages.
  $body_error_once = NULL;
  $subject_error_once = NULL;

  // Loop through the messages and validate whether user is required to edit subject or body.
  // Also set a value for future use by the "user_edit_flag" component.
  foreach ($messages as $id) {

    // The message
    $message = $form['messages'][$id]['message'];

    // The default value for the user-editable body if any.
    $body_default = isset($message['edited_body']['#default_value']) ? $message['edited_body']['#default_value'] : '';
    // The edited value in the user-editable body if any.
    $body_edited = isset($form_state['values']['messages'][$id]['message']['edited_body']) ? $form_state['values']['messages'][$id]['message']['edited_body'] : '';

    // The default value for the user-editable subject if any.
    $subject_default = isset($message['subject']['#default_value']) ? $message['subject']['#default_value'] : '';
    // The edited value in the user-editable subject if any.
    $subject_edited = isset($form_state['values']['messages'][$id]['message']['subject']) ? $form_state['values']['messages'][$id]['message']['subject'] : '';

    // Enforce mandatory editing of the body if set.
    $body_error = !isset($body_error_once) ? t('Please personalize your message before sending.') : '';
    if ($body_edited == $body_default && isset($message['body_required']) && $message['body_required']['#value'] == 1) {
      $body_error_once = TRUE;
      form_set_error("messages][$id][message][edited_body", $body_error);
    }
    elseif ($body_edited != $body_default) {
      // Set change value for use by the "user_edit_flag" component.
      $changed = TRUE;
    }

    // Enforce mandatory editing of the subject if set.
    $subject_error = !isset($subject_error_once) ? t('Please personalize the subject line before sending your message.') : '';
    if ($subject_edited == $subject_default && isset($message['subject_required']) && $message['subject_required']['#value'] == 1) {
      $subject_error_once = TRUE;
      form_set_error("messages][$id][message][subject", $subject_error);
    }
    elseif (!isset($message['subject_show']) && $subject_edited != $subject_default) {
      // Set change value for use by the "user_edit_flag" component.
      $changed = TRUE;
    }
  }
  // Set value for the user_edited_flag
  $form_state['storage']['changed'] = $changed;

  if (!empty($_SESSION['messages']['error'])) {
    foreach ($_SESSION['messages']['error'] as $key => $message) {
      if (strpos($message, 'You must edit the body of this message before sending') !== FALSE) {
        unset($_SESSION['messages']['error'][$key]);
        drupal_set_message('Please personalize your message before sending', 'error', FALSE);
      }
      if (strpos($message, 'You must edit the subject of this message before sending') !== FALSE) {
        unset($_SESSION['messages']['error'][$key]);
        drupal_set_message('Please personalize the subject line before sending', 'error', FALSE);
      }
    }
  }

}


/**
 * Submit the messages to the API endpoint.
 *
 * @param $form
 * @param $form_state
 */
function sba_message_action_preview_form_submit($form, &$form_state) {

  // Get sba_message_action_build_message_bodies().
  module_load_include('inc', 'sba_message_action', 'includes/sba_message_action.form');

  $messages = $form_state['values']['messages'];
  $node = $form['#action_node'];

  // Re-format the messages into the array structure originally returned
  // by the target resolver and expected by the message queue on the API server.
  $formatted_messages = array();
  foreach ($messages as $key => $message) {
    $edited_body = isset($message['message']['edited_body']) ? $message['message']['edited_body'] : '';
    $formatted_messages[$key]['message']['body'] = sba_message_action_build_message_bodies($message['message']['sba_message_id'], $edited_body);
    $formatted_messages[$key]['message']['subject'] = $message['message']['subject'];
    $formatted_messages[$key]['message']['precedence'] = $message['message']['precedence'];
    $formatted_messages[$key]['message']['id'] = $message['message']['message_id'];
    $formatted_messages[$key]['deliverable'] = $message['message']['target'];
    $formatted_messages[$key]['message']['hold'] = $message['message']['hold'];
  }

  // We don't actually want to send the "hold" value to the API server,
  // but we do need it later to update the confirmation messages.
  $remove_hold_index = $formatted_messages;
  foreach ($remove_hold_index as $key => $formatted) {
    unset($remove_hold_index[$key]['message']['hold']);
  }

  // Reconstruct the full array the API server originally sent. New values for
  // multiflow and message subjects and bodies.
  $payload = array(
    'multiflow' => 'step_two',
    'alert_id' => $form_state['values']['action_id'],
    'contact' => $form_state['values']['contact'],
    'messages' => $remove_hold_index,
  );
  if (!empty($form_state['values']['test_mode'])) {
    $payload['test_mode'] = TRUE;
    $payload['test_email'] =  $form_state['values']['test_mode_email'];
  }
  $api_call = springboard_advocacy_api_call();
  $response = $api_call->invokeClientMethod('resolveTargets', $payload);

  if (isset($_SESSION['retries'])) {
    $_SESSION['retries']++;
  }
  else {
    $_SESSION['retries'] = 1;
  }

  // If there's a hiccup with the server, let the user retry sending a couple more times.
  if (!isset($response->status_code) || (isset($response->status_code) && $response->status_code != 200)) {
    if($_SESSION['retries'] < 4) {
      drupal_set_message(t("A problem occurred and your message was not submitted. Please try again. If the problem persists, we should have it fixed shortly. Try back in a few minutes."), 'error');
      return;
    }
    else {
      drupal_set_message(t("A problem occurred. Please try again later."), 'error');
      $form_state['redirect'] = array('node/' . $node->nid);
      $_SESSION['action_completed'] = TRUE;
      unset($_SESSION['retries']);
      return;
    }
  }

  // Add session flag to prevent resubmission of the form
  $_SESSION['action_completed'] = TRUE;
  unset($_SESSION['retries']);

  // If we succeeded, update the confirmation messages stored in the session
  // with the new subject and body values if any.
  // First we need to get the targets into an array keyed by target ID.
  $people = array();
  if(!empty($_SESSION['delivered_messages']['messages'])) {
    foreach ($_SESSION['delivered_messages']['messages'] as $message) {
      $people[$message['targets']->id] = $message['person'];
    }
  }
  if(!empty($_SESSION['undelivered_messages']['messages'])) {
    foreach ($_SESSION['undelivered_messages']['messages'] as $message) {
      $people[$message['targets']->id] = $message['person'];
    }
  }

  // Rebuild the the message session variables.
  $deliverable_messages_for_confirmation = array();
  $undeliverable_messages_for_confirmation = array();
  // Filter the deliverable from the undeliverable
  foreach ($formatted_messages as $key => $f_message) {
    if (empty($f_message['message']['hold'])) {
      $deliverable_messages_for_confirmation[$key] = $formatted_messages[$key]['message'];
      $deliverable_messages_for_confirmation[$key]['targets'] = (Object) $formatted_messages[$key]['deliverable'];
      $deliverable_messages_for_confirmation[$key]['person'] = $people[$formatted_messages[$key]['deliverable']['target_id']];
    }
    else {
      $undeliverable_messages_for_confirmation[$key] = $formatted_messages[$key]['message'];
      $undeliverable_messages_for_confirmation[$key]['targets'] = (Object) $formatted_messages[$key]['deliverable'];
      $undeliverable_messages_for_confirmation[$key]['person'] = $people[$formatted_messages[$key]['deliverable']['target_id']];
    }
  }
  // Update deliverable session.
  if(!empty($_SESSION['delivered_messages']['messages'])) {
    $_SESSION['delivered_messages'] =
      array(
        'messages' => $deliverable_messages_for_confirmation,
        'target_count' => count($deliverable_messages_for_confirmation),
        'message_count' => count($deliverable_messages_for_confirmation),
      );
  }
  // Update undeliverable session.
  if(!empty($_SESSION['undelivered_messages']['messages'])) {
    $_SESSION['undelivered_messages'] =
      array(
        'messages' => $undeliverable_messages_for_confirmation,
        'targets' => '',
      );
  }
  // Redirect to the original confirmation page.
  // $sid is set in sba_message_action_multifow_submit()
  $sid = !empty($_SESSION['action_sid']['sid']) ? $_SESSION['action_sid']['sid'] : FALSE;
  if ($sid) {
    module_load_include('inc', 'webform', 'includes/webform.submissions');
    $submission = webform_get_submission($node->nid, $sid);
    unset($_SESSION['action_sid']);
    if ($node->webform['redirect_url'] == '<confirmation>') {
      $form_state['redirect'] = array(
        'node/' . $node->nid . '/done',
        array('query' => array('sid' => $sid))
      );
    }
    else {
      $redirect = sba_message_action_parse_redirect($node, $submission);
      $form_state['redirect'] = $redirect;
      if (!empty($_SESSION['saved_messages']['status'])) {
        $_SESSION['messages']['status'] =  $_SESSION['saved_messages']['status'];
      }
    }
  }
  else {
    // we don't have a submission identifier, this is not a legit request, redirect to the node
    $form_state['redirect'] = array('node/' . $node->nid);
    if (!empty($_SESSION['saved_messages']['status'])) {
      $_SESSION['messages']['status'] =  $_SESSION['saved_messages']['status'];
    }
  }
  if (!empty($_SESSION['saved_messages']['status'])) {
    unset($_SESSION['saved_messages']['status']);
  }
  // Update the user edit flag.
  if ($form_state['storage']['changed'] == TRUE) {
    if ($sid) {
      $user_edit = db_query('SELECT * FROM {webform_component} WHERE nid = :nid AND form_key = :form_key',
        array(
          ':nid' => $node->nid,
          ':form_key' => 'sba_user_edit_flag'
        ))->fetchObject();

      if (!empty($user_edit->cid)) {
        db_update('webform_submitted_data')->fields(array('data' => 1))
          ->condition('nid', $node->nid, '=')
          ->condition('sid', $sid, '=')
          ->condition('cid', $user_edit->cid, '=')
          ->condition('no', 0, '=')
          ->execute();
      }
    }
  }
  if ($sid) {
    $complete = db_query('SELECT * FROM {webform_component} WHERE nid = :nid AND form_key = :form_key',
      array(
        ':nid' => $node->nid,
        ':form_key' => 'sba_multistep_complete'
      ))->fetchObject();
    if (!empty($complete->cid)) {
      db_update('webform_submitted_data')->fields(array('data' => 1))
        ->condition('nid', $node->nid, '=')
        ->condition('sid', $sid, '=')
        ->condition('cid', $complete->cid, '=')
        ->condition('no', 0, '=')
        ->execute();
    }
  }
  if ($sid) {
    if (empty($submission)) {
      module_load_include('inc', 'webform', 'includes/webform.submissions');
      $submission = webform_get_submission($node->nid, $sid);
    }
    // Update to trigger salesforce resync.
    webform_submission_update($node, $submission);
  }
}

/**
 * Theme the message preview header, i.e., recipient name
 *
 * @param $vars
 * @return string
 */
function theme_sba_message_action_preview_message_header($vars) {
  $intro = t('The message below will be sent to:');
  $output = '<div class="message-preview-header"><div class="message-preview-intro">';
  $output .= $intro;
  $output .= '</div><div class="message-preview-person">';
  $output .= theme('sba_message_action_person_details', $vars['message']['person']);
  $output .= '</div></div>';
  return $output;
}

/**
 * Parse redirect url and filter webform tokens in redirect urls.
 *
 * @param $node
 * @param $submission
 * @return array|mixed
 */
function sba_message_action_parse_redirect($node, $submission) {
  $redirect = NULL;
  if (isset($node->webform['redirect_url'])) {
    $redirect_url = trim($node->webform['redirect_url']);
    $redirect_url = _webform_filter_values($redirect_url, $node, $submission, NULL, FALSE, TRUE);

    // Remove the site domain name from the redirect if present.
    $redirect_url = preg_replace('/^' . preg_quote($GLOBALS['base_url'], '/') . '\//', '', $redirect_url);

    if (valid_url($redirect_url, TRUE)) {
      $redirect = $redirect_url;
    }
    elseif ($redirect_url && strpos($redirect_url, 'http') !== 0) {
      $parts = drupal_parse_url($redirect_url);
      $parts['query'] ? ($parts['query']['sid'] = $submission->sid) : ($parts['query'] = array('sid' => $submission->sid));
      $query = $parts['query'];
      $redirect = array(
        $parts['path'],
        array('query' => $query, 'fragment' => $parts['fragment'])
      );
    }
  }
  return $redirect;
}