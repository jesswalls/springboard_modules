<?php

/**
 * @file
 * Manages Webform to Salesforce mappings.
 */

/**
 * @name core_drupal_hooks
 * @{
 */

/**
 * Implements hook_form_id_alter().
 */
function webform_form_salesforce_genmap_map_form_alter(&$form, &$form_state) {
  // Check to see if this form has been designated as a webform user form. If
  // so, we add an additional submit handler that will scan the fields in the
  // mapped Salesforce object for a foreign key to the contact object.
  if (module_exists('fundraiser')) {
    $fundraiser = fundraiser_is_donation_type($form['#node']->type);
  }
  else {
    $fundraiser = FALSE;
  }
  if (module_exists('webform_user') && !$fundraiser) {
    if (!empty($form['#node']) && _webform_user_is_webform_user_node($form['#node'])) {
      $sobject_type = $form['salesforce_object_info']['salesforce_object_type']['#default_value'];
      if (!empty($sobject_type)) {
        $sfapi = salesforce_get_api();
        $options = array();
        $options[''] = '-- Please Select --';
        if ($sfapi->isAuthorized()) {
          $object_info = $sfapi->objectDescribe($sobject_type);
          foreach ($object_info['fields'] as $field) {
            if ($field['type'] == 'reference' && $field['referenceTo'][0] == 'Contact') {
              $options[$field['name']] = $field['label'];
            }
          }
        }
        if (!empty($options) && count($options) > 1) {
          $form['webform_user'] = array(
            '#type' => 'fieldset',
            '#title' => 'Webform User Configuration',
          );
          $default = '';
          $webform_user_reference_fields = variable_get('webform_user_reference_fields', array());
          if (array_key_exists($form['#node']->nid, $webform_user_reference_fields)) {
            $default = $webform_user_reference_fields[$form['#node']->nid];
          }
          $form['webform_user']['webform_user_foreign_key'] = array(
            '#type' => 'select',
            '#title' => t('Contact Field'),
            '#options' => $options,
            '#required' => TRUE,
            '#default_value' => $default,
            '#description' => t('The user that is created during the form submission will be exported to Salesforce as a contact. Choose the field on the %type object that will relate the submission to the contact.', array('%type' => $sobject_type)),
          );
        }
      }
      $form['#submit'][] = 'webform_user_assign_foreign_key';
    }
  }
}

/**
 * Implements hook_node_insert().
 *
 * If the node is being cloned make a copy of it's fieldmap if the node is a
 * webform enabled type and it has been mapped.
 */
function salesforce_genmap_node_insert($node) {
  $webform_types = variable_get('webform_node_types', array('webform'));
  if (!empty($node->clone_from_original_nid) && in_array($node->type, $webform_types)) {
    $nid = $node->clone_from_original_nid;
    $webform_user_reference_fields = variable_get('webform_user_reference_fields', array());
    if (array_key_exists($nid, $webform_user_reference_fields)) {
      $webform_user_reference_fields[$node->nid] = $webform_user_reference_fields[$nid];
      variable_set('webform_user_reference_fields', $webform_user_reference_fields);
    }
    $map = salesforce_genmap_load_map($nid, 'webform');
    if (!empty($map)) {
      // Copy the map to the node.
      unset($map->mid);
      $map->nid = $node->nid;
      salesforce_genmap_save_map($map, 'webform');
    }
  }
}

/**
 * @} core_drupal_hooks
 */

/**
 * @name salesforce_genmap_hooks
 * @{
 */

/**
 * Implements hook_salesforce_genmap_info().
 */
function webform_salesforce_genmap_info() {
  $webform_types = webform_variable_get('webform_node_types');
  $items['webform'] = array(
    'type' => 'webform',
    'label' => 'Webform',
    'access callback' => 'webform_map_page_access',
    'supported content types' => $webform_types,
    'file' => 'includes/modules/webform.inc',
  );
  return $items;
}

/**
 * Implements hook_salesforce_genmap_map_fields_alter().
 */
function webform_salesforce_genmap_map_fields_alter(&$fields, $context) {
  // First check to see if the module creating the object is saleforce_webform
  // and that we're dealing with a webform_user submission.
  if ($context['module'] == 'webform' && isset($context['node']->is_webform_user) && $context['node']->is_webform_user) {
    // Look up the foreign key for this webform user.
    $webform_user_reference_fields = variable_get('webform_user_reference_fields', array());
    if (array_key_exists($context['node']->nid, $webform_user_reference_fields)) {
      $foreign_key_field = $webform_user_reference_fields[$context['node']->nid];
      // Insert a token that contains the uid of the use that submitted the
      // webform.
      $fields[$foreign_key_field] = sprintf('[Contact:user:%d]', $context['object']->uid);
    }
  }
}

/**
 * Implements hook_salesforce_genmap_map_fields().
 */
function webform_salesforce_genmap_map_fields(stdClass $submission, Entity $map, $node, $op) {
  $fields = array();
  // Salesforce webform map.
  foreach ($submission->data as $cid => $values) {
    $wf_field = $node->webform['components'][$cid]['form_key'];
    if (isset($map->field_map['webform_map'][$wf_field])) {
      $sf_field = $map->field_map['webform_map'][$wf_field];
      $fields[$sf_field] = $values['value'][0];
    }
  }
  // Node properties map.
  if (isset($map->field_map['salesforce_node_map'])) {
    if (isset($map->field_map['salesforce_node_map']['nid'])) {
      $sf_field = $map->field_map['salesforce_node_map']['nid'];
      $fields[$sf_field] = $node->nid;
    }
    if (isset($map->field_map['salesforce_node_map']['title'])) {
      $sf_field = $map->field_map['salesforce_node_map']['title'];
      $fields[$sf_field] = $node->title;
    }
  }
  // Submission properties map.
  if (isset($map->field_map['salesforce_submission_map'])) {
    if (isset($map->field_map['salesforce_submission_map']['sid'])) {
      $sf_field = $map->field_map['salesforce_submission_map']['sid'];
      $fields[$sf_field] = $submission->sid;
    }
    if (isset($map->field_map['salesforce_submission_map']['date'])) {
      $sf_field = $map->field_map['salesforce_submission_map']['date'];
      $fields[$sf_field] = REQUEST_TIME;
    }
  }
  return $fields;
}

/**
 * Implements hook_salesforce_genmap_map_field_info().
 */
function webform_salesforce_genmap_map_field_info($node) {
  // Build an array of fields to map against.
  // Nested by category. If the category allows for dedupes, note that.
  $fields = array();
  // Webform component fields.
  $fields['webform_map'] = array(
    '#type' => 'webform',
    '#label' => t('Webform Component'),
    '#dedupe' => TRUE,
    '#fields' => array(),
  );
  $wf_components = $node->webform['components'];
  foreach ($wf_components as $cid => $component) {
    if ($component['type'] != 'fieldset') {
      $form_key = $component['form_key'];
      $fields['webform_map']['#fields'][$form_key] = $component['name'] .
        ' (' . $component['type'] . ')';
    }
  }
  // Node fields.
  $fields['salesforce_node_map'] = array(
    '#type' => 'node',
    '#label' => t('Node Property'),
    '#dedupe' => FALSE,
    '#fields' => array(
      'nid' => t('Node ID'),
      'title' => t('Node Title'),
    ),
  );
  // Submission fields.
  $fields['salesforce_submission_map'] = array(
    '#type' => 'submission',
    '#label' => t('Submission Property'),
    '#dedupe' => FALSE,
    '#fields' => array(
      'sid' => t('Submission ID'),
      'date' => t('Submission Date'),
    ),
  );
  // And return the fields for the map to generate the form.
  return $fields;
}

/**
 * Implements hook_salesforce_genmap_delete_map().
 */
function webform_salesforce_genmap_delete_map($nid, $module) {
  // Remove all queued items when a map is deleted.
  if (module_exists('salesforce_queue')) {
    db_delete('salesforce_queue')
      ->condition('module', 'webform')
      ->condition('delta', 'webform:' . $nid)
      ->execute();
  }
}

/**
 * Implements hook_salesforce_genmap_permission().
 */
function webform_salesforce_genmap_permission() {
  $permissions = array(
    'map salesforce to webform' => array(
      'title' => t('Map Salesforce objects to webforms'),
      'description' => t('Configure the mapping of Salesforce objects with
        webforms so submissions can sync with Salesforce.'),
    ),
  );
  return $permissions;
}

/**
 * @} salesforce_genmap_hooks
 */

/**
 * @name salesforce_queue_hooks
 * @{
 */

/**
 * Implements hook_salesforce_queue_refresh_queue_item().
 */
function webform_salesforce_queue_refresh_queue_item($item) {
  // Ensure the item is a webform submission.
  if ($item->module == 'webform') {
    // Load the node.
    $nid = $item->delta;
    $node = node_load($nid);

    // Load the submission.
    module_load_include('inc', 'webform', 'includes/webform.submissions');
    $sid = $item->drupal_id;
    $submission = webform_get_submission($nid, $sid, TRUE);

    if (!empty($submission)) {
      // Requeue the submission.
      switch ($item->operation) {
        case 'CREATE':
        case 'UPSERT':
          salesforce_genmap_webform_submission_insert($node, $submission);
          break;

        case 'UPDATE':
          salesforce_genmap_webform_submission_update($node, $submission);
          break;

        case 'DELETE':
          salesforce_genmap_webform_submission_delete($node, $submission);
          break;
      }

      // Assume it succeeded because salesforce_genmap_send_object_to_queue
      // returns nothing.
      return TRUE;
    }
    // Submission not found.
    return FALSE;
  }
}

/**
 * @} salesforce_queue_hooks
 */

/**
 * Access callback to reach the mapping page.
 */
function webform_map_page_access($node) {
  // %webform_menu returns FALSE if not valid.
  if ($node !== FALSE
     && user_access('map salesforce to webform')
    ) {
    return TRUE;
  }
  return FALSE;
}

/**
 * Implements hook_webform_submission_insert().
 */
function salesforce_genmap_webform_submission_insert($node, $submission) {
  salesforce_genmap_send_object_to_queue('webform', 'insert', $node, $submission->sid, $submission, $node->nid);
}

/**
 * Implements hook_webform_submission_update().
 */
function salesforce_genmap_webform_submission_update($node, $submission) {
  salesforce_genmap_send_object_to_queue('webform', 'update', $node, $submission->sid, $submission, $node->nid);
}

/**
 * Implements hook_webform_submission_delete().
 */
function salesforce_genmap_webform_submission_delete($node, $submission) {
  salesforce_genmap_send_object_to_queue('webform', 'delete', $node, $submission->sid, $submission, $node->nid);
}

/**
 * Implements hook_entity_update().
 */
function webform_entity_update($entity, $type) {
  // Updating the mapping entity will require having all of the queue items
  // regenerated so the new mapping configuration can be applied to them.
  if ($type == 'salesforce_genmap_map' && $entity->map_handler == 'webform') {
    module_load_include('inc', 'webform', 'includes/webform.submissions');
    $node = node_load($entity->nid);
    $result = db_select('salesforce_queue', 'q')
      ->fields('q')
      ->condition('module', 'webform')
      ->condition('delta', $entity->nid)
      ->condition('object_type', $entity->salesforce_object_type)
      ->execute();
    foreach ($result as $item) {
      // Get the original submission and requeue it.
      $submission = webform_get_submission($node->nid, $item->drupal_id);
      $op = '';
      if (in_array($item->operation, array('UPSERT', 'UPDATE'))) {
        $op = 'update';
      }
      elseif ($item->operation == 'CREATE') {
        $op = 'insert';
      }
      elseif ($item->operation == 'DELETE') {
        $op = 'delete';
      }
      salesforce_genmap_send_object_to_queue('webform', $op, $node, $item->drupal_id, $submission, $node->nid);
    }
  }
}

/**
 * Sets the foreign key field for webform_user integration.
 *
 * Scan the mapped Salesforce object's fields for a contact foreign key that
 * will be used to relate the user who submitted the form to the submission in
 * Salesforce.
 */
function webform_user_assign_foreign_key($form, $form_state) {
  $nid = $form_state['build_info']['args'][0]->nid;
  $field = isset($form_state['values']['webform_user']['webform_user_foreign_key']) ? $form_state['values']['webform_user']['webform_user_foreign_key'] : FALSE;
  $webform_user_reference_fields = variable_get('webform_user_reference_fields', array());

  if ($field && !empty($field)) {
    // Add the field name to the webform_user_reference_fields array.
    $webform_user_reference_fields[$nid] = $field;
    variable_set('webform_user_reference_fields', $webform_user_reference_fields);
  }
  elseif (isset($webform_user_reference_fields[$nid])) {
    unset($webform_user_reference_fields[$nid]);
    variable_set('webform_user_reference_fields', $webform_user_reference_fields);
  }
}
