<?php

/**
 * @file
 * Helper functions for building message action client forms.
 */

/**
 * Message action webform alterations.
 *
 * Add the message entities to the action webform node view.
 * Attach CSS and JS to Action nodes.
 * If it's a multiple message action, attach the ctools modal view of them.
 * Define custom submit and validate hooks.
 *
 * @param $form
 * @param $form_state
 */
function sba_message_action_additional_elements(&$form, &$form_state) {
  // Override core's theme_form in order to remove <form> tag
  // and use it in a template variable defined in
  // sba_message_action_build_preprocess and
  // theme_message_action_form_layout_form
  if (!empty($form['#theme_wrappers']) && !empty($form['#node']->form_layouts) && $form['#node']->form_layouts != 'one_column') {
    foreach ($form['#theme_wrappers'] as $key => $wrapper) {
      if ($wrapper == 'form') {
        unset($form['#theme_wrappers'][$key]);
      }
    }
    $form['#theme_wrappers'][] = array('message_action_form_layout_form');
  }

  // Attach the JS and CSS.
  $form['#attached']['js'][] = drupal_get_path('module', 'sba_message_action')
    . '/js/sba_message_action.js';
  $form['#attached']['css'][] = drupal_get_path('module', 'sba_message_action')
    . '/css/message-modal.css';

  // Is form layouts enabled?
  if (!empty($form['#node']->form_layouts) && $form['#node']->form_layouts != 'one_column') {
    $form['#attached']['css'][] = drupal_get_path('module', 'sba_message_action')
      . '/css/message-action-form-layouts.css';
  }
  else {
    $form['#attached']['css'][] = drupal_get_path('module', 'sba_message_action')
      . '/css/no-form-layouts.css';
  }

  $form['smarty_geo'] = array(
    '#type' => 'value',
    '#value' => '',
  );

  $form['webform_flat'] = array(
    '#type' => 'value',
    '#value' => array(),
  );

  // Create a container on the form for all messages.
  $form['sba_messages'] = array(
    '#type' => 'container',
    '#tree' => TRUE,
    '#attributes' => array(
      'id' => array('sba-messages-form'),
    ),
  );

  // Multiple messages for this form?
  $node = $form['#node'];
  $multiple = count($node->messages) > 1 ? TRUE : FALSE;
  $node_wrapper = entity_metadata_wrapper('node', $node);
  $multi_flow = $node_wrapper->field_sba_action_flow->value() == 'multi' ? TRUE : FALSE;
  $prompt = field_get_items('node', $node, 'field_sba_multistep_prompt');
  $multi_flow_prompt = !empty($prompt) ? check_markup($prompt[0]['value'], $prompt[0]['format']) : '';
  $action_type = 'single_not_multi_flow';
  if ($multiple && !$multi_flow) {
    $action_type = 'multiple_not_multi_flow';
  }
  elseif ($multi_flow) {
    $action_type = 'multi_flow';
  }

  if ($action_type == 'multiple_not_multi_flow') {
    ctools_include('ajax');
    ctools_include('modal');
    $href = 'messages/nojs/' . arg(1);
    $form['sba_messages']['multiple_not_multi_flow'] = array(
      '#type' => 'fieldset',
      '#title' => t('Your Message'),
      '#attributes' => array('class' => array('sba-message')),
    );
    $form['sba_messages']['multiple_not_multi_flow']['message'] = array(
      '#type' => 'item',
      '#markup' => '<p>' . t('We have crafted multiple messages for this action. We will select the right message or messages for your legislators, or others we are targeting, based on their position on this issue, and on the address you enter.')
        . '</p><p></p>' . ctools_modal_text_button('View all possible messages.', $href, t('View messages'), 'ctools-modal-message-window-style') . '</p>',
    );
  }
  elseif ($action_type == 'multi_flow') {
    $form['sba_messages']['multiflow'] = array(
      '#type' => 'fieldset',
      '#title' => t('Step 1 of 2'),
      '#attributes' => array('class' => array('sba-message')),
    );
    $form['sba_messages']['multiflow']['prompt'] =  array(
      '#markup' => $multi_flow_prompt,
    );
  }

  // Create a set of fields for each message assigned to the form.
  foreach ($form['#node']->messages as $message) {
    sba_message_action_message_form($form['sba_messages'], $message, $action_type);
  }

  $form['#validate'][] = 'sba_message_action_form_validate';
  array_unshift($form['#submit'], 'sba_message_action_form_submit');
}

/**
 * Build a form to represent each message attached to the action webform node.
 *
 * @param array $form
 *   The webform client form.
 * @param object $message
 *   The message entity.
 * @param bool $multiple
 *    Whether this webform has multiple messages.
 */
function sba_message_action_message_form(&$form, $message, $action_type) {
  // Get some message values via an entity wrapper.
  $message_wrapper = entity_metadata_wrapper('sba_message', $message);
  $subject = $message_wrapper->field_sba_subject->value();
  $message_body = check_markup($message_wrapper->field_sba_message->value(), 'plain_text');
  $editable_message = $message_wrapper->field_sba_editable_area->value();
  $body_editable = $message_wrapper->field_sba_user_editable[0]->value() == 'editable' ? TRUE : FALSE;
  $required_body = $message_wrapper->field_sba_editable_area_optional[0]->value() == 'required' ? TRUE : FALSE;
  $placeholder_greeting = check_plain($message_wrapper->field_sba_placeholder_greeting->value());
  $greeting = check_plain($message_wrapper->field_sba_greeting->value());
  $conclusion = check_markup($message_wrapper->field_sba_bottom_conclusion->value(), 'plain_text');
  $subject_editable =  $message_wrapper->field_sba_subject_editable->value();
  $node = $message_wrapper->field_sba_action_id->value();
  $node_wrapper = entity_metadata_wrapper('node', $node);
  $multi_flow = $node_wrapper->field_sba_action_flow->value() == 'multi' ? TRUE : FALSE;

  $preview_form = empty($form) ? TRUE : FALSE;
  $access = TRUE;
  if (!$preview_form && $multi_flow) {
    $access = FALSE;
  }

  // If it's a multiple message form, use a container instead of a fieldset.
  if ($action_type == 'single_not_multi_flow') {
    $form['message'][$message->sba_message_id] = array(
      '#type' => 'fieldset',
      '#title' => t('Your Message'),
      '#attributes' => array('class' => array('sba-message')),
    );
  }
  else {
    $form['message'][$message->sba_message_id] = array(
      '#type' => 'container',
      '#attributes' => array('class' => array('sba-message')),
    );
  }

  $form['message'][$message->sba_message_id]['message_id'] = array(
    '#type' => 'value',
    '#value' => $message->data['message_id'],
  );

  $form['message'][$message->sba_message_id]['sba_message_id'] = array(
    '#type' => 'value',
    '#value' => $message->sba_message_id,
  );

  // The subject is editable.
  if ($subject_editable != 'Not editable'  && $action_type != 'multiple_not_multi_flow') {

    // Is it required to edit the subject?
    if ($subject_editable == 'Editing is required') {
      if (!$multi_flow) {
        $description = t('You must edit the subject line of this message before sending.');
      }
      // Validator needs this value?
      $form['message'][$message->sba_message_id]['subject_required'] = array(
        '#type' => 'value',
        '#value' => 1,
      );
    }

    $form['message'][$message->sba_message_id]['subject'] = array(
      '#type' => 'textfield',
      '#title' => t('Subject'),
      '#default_value' => $subject,
      '#description' => !empty($description) ? $description : '',
      '#access' => $access,
    );

  }
  // The subject is not editable.
  else {
    // Show the non-editable subject if there aren't multiple messages.
   // if (!$multiple && !$multi_flow) {
      if ($action_type != 'multiple_not_multi_flow') {

        $form['message'][$message->sba_message_id]['subject_show'] = array(
        '#type' => 'item',
        '#title' => t('Subject'),
        '#markup' => check_plain($subject),
        '#access' => $access,
      );
    }

    // The subject value field is present in either case.
    // The value here is eventually passed to the API server.
    $form['message'][$message->sba_message_id]['subject'] = array(
      '#type' => 'value',
      '#value' => $subject,
    );
  }

  // Show the greeting field is there aren't multiple messages.
  if ($action_type != 'multiple_not_multi_flow') {
    $form['message'][$message->sba_message_id]['greeting_show'] = array(
      '#type' => 'item',
      '#title' => '',
      '#markup' => $placeholder_greeting,
      '#access' => $access,
    );
  }

  // The greeting value field is present in either case.
  // The value here is eventually passed to the API server.
  $form['message'][$message->sba_message_id]['greeting'] = array(
    '#type' => 'value',
    '#value' => $greeting,
  );

  // The body value field is present in either case.
  // The value here is eventually passed to the API server.
  $form['message'][$message->sba_message_id]['body'] = array(
    '#type' => 'value',
    '#value' => $message_body,
  );

  // If not multiple we show the message body.
  if (!empty($message_body) && $action_type != 'multiple_not_multi_flow') {

    // If the message body is editable, suppress the field title.
    if ($body_editable) {
      $title = '';
    }
    else {
      $title = t('Message');
    }

    $form['message'][$message->sba_message_id]['body_show'] = array(
      '#type' => 'item',
      '#title' => $title,
      '#markup' => $message_body,
      '#access' => $access,
    );
  }

  if ($body_editable == 'editable' && $action_type != 'multiple_not_multi_flow') {

    if ($required_body) {
      $description = !$multi_flow ? t('You must edit the body of this message before sending.') : '';
      $required = TRUE;
    }
    else {
      $description = '';
      $required = FALSE;
    }

    $form['message'][$message->sba_message_id]['edited_body'] = array(
      '#type' => 'textarea',
      '#default_value' => $editable_message,
      '#description' => $description,
      '#access' => $access,
      //'#required' => $required,
    );

    $form['message'][$message->sba_message_id]['body_required'] = array(
      '#type' => 'value',
      '#value' => $required_body,
    );

    if (!empty($conclusion)) {
      $form['message'][$message->sba_message_id]['body_bottom'] = array(
        '#type' => 'item',
        '#markup' => $conclusion,
        '#access' => $access,
      );
    }
  }

  // Format the signature display.
  if ($action_type != 'multiple_not_multi_flow') {
    $sig = strip_tags($message_wrapper->field_sba_signature->value());
    $sig = str_replace('[contact:first_name]', '[First Name]', $sig);
    $sig = str_replace('[contact:last_name]', '[Last Name]', $sig);
    $sig = str_replace('[contact:address]', '[Your Addess]', $sig);
    $form['message'][$message->sba_message_id]['signature'] = array(
      '#markup' => check_markup($sig, 'plain_text'),
      '#access' => $access,
    );
    $form['message'][$message->sba_message_id]['signature_tokenized'] = array(
      '#type' => 'value',
      '#value' => $message_wrapper->field_sba_signature->value(),
    );
  }
}


/**
 * Validation handler for message action forms.
 *
 * Checks whether editing of subject or body is required.
 * Calls Smarty Streets API and injects response values into form state.
 * Flattens the webform submission tree and injects flattened values into
 * form state for use by the message sending functions
 * in sba_message_action.webform.inc
 *
 * @param $form
 * @param $form_state
 */
function sba_message_action_form_validate($form, &$form_state) {
  $node = $form['#node'];
  $node_wrapper = entity_metadata_wrapper('node', $node);
  $multi_flow = $node_wrapper->field_sba_action_flow->value() == 'multi' ? TRUE : FALSE;
  if (isset($form_state['values']['sba_messages'])) {
    $is_single = count($form_state['values']['sba_messages']['message']) == 1 ? TRUE : FALSE;
  }
  if (!empty($is_single) && $multi_flow == FALSE) {
    $child_messages = element_children($form['sba_messages']['message']);
    $message_id = reset($child_messages);
    $message = $form['sba_messages']['message'][$message_id];
    $body_default = isset($message['edited_body']['#default_value']) ? $message['edited_body']['#default_value'] : '';
    $body_edited = isset($form_state['values']['sba_messages']['message'][$message_id]['edited_body']) ? $form_state['values']['sba_messages']['message'][$message_id]['edited_body'] : '';
    $subject_default = isset($message['subject']['#default_value']) ? $message['subject']['#default_value'] : '';
    $subject_edited = isset($form_state['values']['sba_messages']['message'][$message_id]['subject']) ? $form_state['values']['sba_messages']['message'][$message_id]['subject'] : '';
    $changed = FALSE;
    // Enforce mandatory editing.
    if ($body_edited == $body_default && isset($message['body_required']) && $message['body_required']['#value'] == 1) {
        form_set_error('body_editable', t('Please personalize your message before sending.'));
    }
    elseif ($body_edited != $body_default) {
      $changed = TRUE;
    }
    if ($subject_edited == $subject_default && isset($message['subject_required']) && $message['subject_required']['#value'] == 1) {
        form_set_error('subject', t('Please personalize the subject line before sending your message.'));
    }
    elseif (!isset($message['subject_show']) && $subject_edited != $subject_default) {
      $changed = TRUE;
    }
    // Set user_edited_flag
    if ($changed && isset($form['submitted']['sba_user_edit_flag'])) {
      form_set_value($form['submitted']['sba_user_edit_flag'], 1, $form_state);
    }
  }
  form_load_include($form_state, 'inc', 'springboard_advocacy', 'includes/springboard_advocacy.smarty_streets');
  $webform_values_flat = _springboard_advocacy_webform_submission_flatten($form['#node']->nid, $form_state['values']['submitted']);
  form_set_value($form['webform_flat'], $webform_values_flat, $form_state);
  $smarty_geo = springboard_advocacy_smarty_lookup($webform_values_flat);
  if (!empty($smarty_geo) && is_array($smarty_geo)) {
    form_set_value($form['smarty_geo'], $smarty_geo, $form_state);
    if (isset($smarty_geo['zip']) && isset($smarty_geo['plus4'])) {
      form_set_value($form['submitted']['sbp_zip_plus_four'], $smarty_geo['zip'] . '-' . $smarty_geo['plus4'], $form_state);
    }
  }
  else {
    form_set_error('', t('We could not verify your address. Please check that the street, city, state and zip code information is correct.'));
  }
}

/**
 * Submit handler for message action forms.
 *
 * Capture all messages, combine with contact information and
 * submit to API.
 *
 * @param $form
 * @param $form_state
 */
function sba_message_action_form_submit($form, &$form_state) {
  $node = $form['#node'];
  $node_wrapper = entity_metadata_wrapper('node', $node);
  $multi_flow = $node_wrapper->field_sba_action_flow->value() == 'multi' ? TRUE : FALSE;

  $_SESSION['delivered_messages'] = array();
  $_SESSION['undelivered_messages'] = array();
  $_SESSION['contact'] = array();
  $_SESSION['form_details'] = array();
  $_SESSION['action_sid'] = FALSE;
  $_SESSION['action_completed'] = FALSE;
  // Create array of messages to submit for target resolution.
  if (isset($form_state['values']['sba_messages'])) {
    $messages = array();
    foreach ($form_state['values']['sba_messages']['message'] as $id => $message) {
      $edited_body = isset($message['edited_body']) ? $message['edited_body'] : '';
      $messages[$id]['body'] = sba_message_action_build_message_bodies($message['sba_message_id'], $edited_body);
      $messages[$id]['subject'] = $message['subject'];
    }
    form_load_include($form_state, 'inc', 'sba_message_action', 'includes/sba_message_action.webform');
    $data = sba_message_action_send_message($messages, $form_state);
    // Calculate the deliverable count for sending to salesforce
    if (!empty($data->messages)) {
      $deliverable = 0;
      foreach ($data->messages as $key => $message) {
        $deliverable += count($message->deliverable);
      }
      if (isset($form_state['values']['submitted']['sba_deliverable_count'])) {
        $form_state['values']['submitted']['sba_deliverable_count'] = $deliverable;
      }
    }
  }
  else {
    drupal_set_message(t('No messages were configured for this action.'));
  }
}


/**
 * Helper function to build message bodies.
 *
 * Combines user edited message data with other static properties
 * to create the final message to deliver to the transaction server.
 *
 * @param int $sba_message_id
 *   The message ID.
 * @param string $body_editable
 *    The user-editable body.
 *
 * @return string
 *   The message in string format.
 */
function sba_message_action_build_message_bodies($sba_message_id, $body_editable) {
  // Load the message entity using the message entity id. We'll use the entity
  // to construct the full message.
  $message = entity_load('sba_message', array($sba_message_id));
  $message = array_shift($message);
  $wrapper = entity_metadata_wrapper('sba_message', $message);

  $user_message = array();
  $user_message[] = $wrapper->field_sba_greeting->value();
  $user_message[] = $wrapper->field_sba_message->value();
  $user_message[] = $body_editable;
  $user_message[] = $wrapper->field_sba_bottom_conclusion->value();
  $user_message[] = $wrapper->field_sba_signature->value();
  $user_message = implode("\n\n", array_filter($user_message));

  return $user_message;
}
