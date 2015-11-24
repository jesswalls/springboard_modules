<?php

/**
 * Callback handler for 'node/%/message-queue' menu item.
 *
 * Displays message queue delivery metrics and form actions for changing queue message items status.
 */
function sba_message_action_queue_page($nid) {

  // CSS assets
  drupal_add_css(drupal_get_path('module', 'sba_message_action') . '/css/message-queues.css');

  // Get the node ID/name for API calls
  $advocacy_id = node_load($nid)->advocacy_id;

  // Get an instance of the advocacy API client.
  $client = springboard_advocacy_api_call();

  if (isset($advocacy_id)) {
    $queue_ready = $client->invokeClientMethod(
      'getQueueMessagesCountByStatus',
      $advocacy_id,
      'ready'
    );
    $queue_paused = $client->invokeClientMethod(
      'getQueueMessagesCountByStatus',
      $advocacy_id,
      'paused'
    );
    $queue_canceled = $client->invokeClientMethod(
      'getQueueMessagesCountByStatus',
      $advocacy_id,
      'canceled'
    );

    $queue_hold = $client->invokeClientMethod(
      'getQueueMessagesCountByStatus',
      $advocacy_id,
      'hold'
    );
  }
  $queue_ready_items = !empty($queue_ready->data->count) ? $queue_ready->data->count : 0;
  $queue_pausedItems = !empty($queue_paused->data->count) ? $queue_paused->data->count : 0;
  $queue_canceled_items = !empty($queue_canceled->data->count) ? $queue_canceled->data->count : 0;
  $queue_hold_items = !empty($queue_hold->data->count) ? $queue_hold->data->count : 0;

  // Create the tabs
  $tabs = theme('messsage_deliverability_tabs', array(
    'nodeId' => $nid
  ));

  // Render the action form for changing queue items status
  if (isset($advocacy_id)) {
    $render_form = drupal_get_form('sba_message_action_queues_form', $advocacy_id);
    $queue_actions_form = drupal_render($render_form);
  }
  else {
    $queue_actions_form = t('No actions are currently available. Advocacy Form ID not found.');
  }

  // Setup the page theme
  $page = theme('messsage_action_queues', array(
    'tabs'               => $tabs,
    'queue_ready_items'    => $queue_ready_items,
    'queue_paused_items'   => $queue_pausedItems,
    'queue_canceled_items' => $queue_canceled_items,
    'queue_hold_items' => $queue_hold_items,
    'queue_actions_form'   => $queue_actions_form
  ));

  // Return the page to the browser
  return $page;
}

/**
 * Form handler for rendering the action form that allows changing message status.
 * @param $form
 * @param $form_state
 * @param $advocacy_id
 * @return mixed
 */
function sba_message_action_queues_form($form, &$form_state, $advocacy_id) {

  // Setup the bounds of the allowed and rendered queue actions
  $active = array('Pause' => t('Pause'), 'Restart' => t('Restart'), 'Cancel' => t('Cancel') );

  // Create the form radios for queue update status
  $form['queue_update_type'] = array(
    '#type' => 'radios',
    '#default_value' => 'Pause',
    '#options' => $active,
    '#description' => t('In the event that you need to immediately modify messages pending in the queue, you can do so here.'),
  );

  // Hidden node id for usage in API calls
  $form['node_id'] = array(
    '#type'          => 'value',
    '#name'          => 'node_id',
    '#title'         => t('Node Id'),
    '#default_value' => $advocacy_id,
    '#required'      => TRUE,
  );

  // Submit button
  $form['submit'] = array(
    '#type'  => 'submit',
    '#value' => t('Update Queue Items'),
  );

  // Return the form
  return $form;
}

/**
 * Form submission handler for the sba message action queues page.
 * Implements hook_submit()
 * @param $form
 * @param $form_state
 */
function sba_message_action_queues_form_submit($form, &$form_state) {
  // Make the API request to queue up the CSV download
  $api_client = springboard_advocacy_api_call();

  if (!form_get_errors()) {

    $update_status_message = '';
    $update_status_message_type = 'status';

    // Check the submitted radio state to provide feedback and run appropriate API calls
    switch ($form_state['input']['queue_update_type']) {
      case 'Pause':
        $api_request = $api_client->invokeClientMethod('modifyQueueMessagesStatus', $form_state['values']['node_id'], 'pause');
        if (isset($api_request->data)) {
          $update_status_message = $api_request->data->items_updated == 0 ?
            t('Their are no "Ready" messages to pause delivery for.') :
            t('@count Queue message/s have been "Paused" and will not be delivered.', array('@count' => $api_request->data->items_updated));
          if ($api_request->data->items_updated == 0) {
            $update_status_message_type = 'warning';
          }
        }
        break;

      case 'Restart':
          $api_request = $api_client->invokeClientMethod('modifyQueueMessagesStatus', $form_state['values']['node_id'], 'restart');
          if (isset($api_request->data)) {
            $update_status_message = $api_request->data->items_updated == 0 ?
              t('Their are no "Paused" messages to restart delivery for.') :
              t('@count Queue message/s are now "Ready" and are scheduled for delivery.',
                array('@count' => $api_request->data->items_updated)
              );
            if ($api_request->data->items_updated == 0) {
              $update_status_message_type = 'warning';
            }
          }
        break;

      case 'Cancel':
        $api_request = $api_client->invokeClientMethod('modifyQueueMessagesStatus', $form_state['values']['node_id'], 'cancel');
        if (isset($api_request->data)) {
          $update_status_message = $api_request->data->items_updated == 0 ?
              t('Their are no "Ready" messages to cancel delivery for.') :
              t('@count Queue message/s have been "Canceled" indefinitely.', array('@count' => $api_request->data->items_updated));
          if ($api_request->data->items_updated == 0) {
              $update_status_message_type = 'warning';
            }
        }
        break;
    }

    // Send the message back the to user for the update attempt and result.
    if (!empty($api_request) && isset($api_request->data)) {
      $advocacy_id = $form_state['values']['node_id'];
      cache_clear_all($advocacy_id, 'cache_deliverability_report');
      drupal_set_message($update_status_message, $update_status_message_type);
    }
  }
}

/**
 * Method to validate the form submission of the sba message action queues page.
 * Implements hook_validate()
 * @param $form
 * @param $form_state
 */
function sba_message_action_queues_form_validate($form, &$form_state) {

}
