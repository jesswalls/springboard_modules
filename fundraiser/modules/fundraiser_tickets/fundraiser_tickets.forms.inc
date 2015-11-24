<?php
/**
 * @file
 * fundraiser_tickets.forms.inc - Ticket management form
 */

function fundraiser_tickets_order_ticket_types_form($form, &$form_state, $node){
  global $language;
  $products = array();

  if (! empty($node->fr_tickets_ticket_types)){
    $node_wrapper = entity_metadata_wrapper('node', $node);

    // Create an array from the products
    foreach ($node_wrapper->fr_tickets_ticket_types as $product_wrapper) {
      $products[] = $product_wrapper;
    }

    uasort($products, '_fundraiser_tickets_weight_sort');
  }

  // Build the form
  $form = array();

// create a parent element and use our custom theme
  $form['tickets'] = array(
    '#prefix' => '<div id="ticket-container">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
    '#theme' => 'fundraiser_tickets_theme_order_ticket_types_form'
  );

  if (! empty($products)){

    // create the form elements for each product
    foreach ($products as $key => $product_wrapper) {
      $form['tickets'][$key]['price'] = array(
        '#markup' => commerce_currency_format($product_wrapper->commerce_price->amount->value(), $product_wrapper->commerce_price->currency_code->value(), $product_wrapper->value())
      );
      $form['tickets'][$key]['title'] = array(
        '#markup' => check_plain($product_wrapper->title->value()),
      );
      $form['tickets'][$key]['description'] = array(
        '#markup' => check_plain($product_wrapper->fr_tickets_description->value()),
      );
      $form['tickets'][$key]['quantity'] = array(
        '#markup' => check_plain($product_wrapper->fr_tickets_quantity->value()),
      );
      $form['tickets'][$key]['sold'] = array(
        '#markup' => check_plain($product_wrapper->fr_tickets_quantity_sold->value()),
      );
      $form['tickets'][$key]['sold_out_message'] = array(
        '#markup' => check_plain($product_wrapper->fr_tickets_sold_out_message->value()),
      );
      $form['tickets'][$key]['warning'] = array(
        '#markup' => check_plain($product_wrapper->fr_tickets_threshold->value()),
      );
      $form['tickets'][$key]['edit_link'] = array(
        '#markup' =>  l(t('edit'), 'admin/commerce/products/' . $product_wrapper->product_id->value() . '/edit',
          array('query' => array('destination' => 'node/' . $node->nid . '/tickets/tickets')))
      );
      $form['tickets'][$key]['delete_link'] = array(
        '#markup' =>  l(t('delete'),
          'springboard/node/' . $node->nid . '/tickets/' . $product_wrapper->product_id->value() . '/delete',
          array('query' => array('destination' => 'node/' . $node->nid . '/tickets/tickets')))
      );

      $form['tickets'][$key]['ticket_id'] = array(
        '#type' => 'hidden',
        '#value' => $product_wrapper->product_id->value()
      );

      $form['tickets'][$key]['weight'] = array(
        '#type' => 'textfield',
        '#default_value' => $key,
        '#size' => 3,
        '#attributes' => array('class' => array('rank-weight')), // needed for table dragging
      );
    }

    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Submit'),
    );
  }

  $form['add'] = array(
    '#type' => 'button',
    '#value' => t('Add ticket type'),
  );



  return $form;
}


/**
 * Provides a ticket deletion form separate from Commerce for more granular access.
 */
function fundraiser_tickets_delete_ticket_form($form, &$form_state, $node, $ticket_id) {

  $product = commerce_product_load($ticket_id);
  $form_state['product'] = $product;
  $content = entity_view('commerce_product', array($product->product_id => $product));

  $form = confirm_form($form,
    t('Are you sure you want to delete ticket %title?', array('%title' => $product->title)),
    '',
    drupal_render($content) . '<p>' . t('Deleting this ticket cannot be undone.', array('@sku' => $product->sku)) . '</p>',
    t('Delete'),
    t('Cancel'),
    'confirm'
  );

  // Use commerce code to block deletion if this ticket has any orders:
  commerce_product_reference_form_commerce_product_product_delete_form_alter($form, $form_state);

  // Make text ticket-specific if Commerce blocks form submit due this ticket (product) having orders:
  if (isset($form['actions']['submit']['#disabled']) && $form['actions']['submit']['#disabled'] == TRUE) {
    $form['description']['#markup'] = str_replace(
      t('This product is referenced by a line item on'),
      t('WARNING: This ticket has been purchased via'),
      $form['description']['#markup']
    );
    // No link for disabling ticket is needed so remove that call to action:
    $form['description']['#markup'] = str_replace(
      t('Disable it instead.'),
      '',
      $form['description']['#markup']
    );
  } 

  // Remove confusing and unneeded Commerce message about which nodes this product references:
  $form['description']['#markup'] = preg_replace(
    '/<p>' . t('This product is referenced.') . '*?<div class="item-list">.*?<\/div>/s',
    '', $form['description']['#markup']);

  return $form;
}

/**
 * Process either ticket deletion or redirection if "Cancel" is hit.
 */
function fundraiser_tickets_delete_ticket_form_submit($form, &$form_state) {
  $product = $form_state['product'];

  if (commerce_product_delete($product->product_id)) {
    drupal_set_message(t('Ticket %title has been deleted.', array('%title' => $product->title)));
    watchdog('commerce_product', 'Deleted ticket product %title (SKU: @sku).',
      array('%title' => $product->title, '@sku' => $product->sku), WATCHDOG_NOTICE);
  }
  else {
    drupal_set_message(t('Ticket %title could not be deleted.', array('%title' => $product->title)), 'error');
  }

}


function theme_fundraiser_tickets_theme_order_ticket_types_form($vars) {
  $element = $vars['element'];
  drupal_add_tabledrag('form_id', 'order', 'sibling', 'rank-weight'); // needed for table dragging

  $header = array(
    'price' => t('Price'),
    'title' => t('Title'),
    'description' => t('Description'),
    'quantity' => t('Quantity'),
    'sold' => t('Sold'),
    'sold_out_message' => t('Sold out message'),
    'warning' => t('Warning'),
    'edit_link' => t(''),
    'delete_link' => t(''),
    'weight' => t('Weight'),

  );

  if (!user_access('allow node-specific ticket deletion')) {
    unset($header['delete_link']);
  }

  $rows = array();
  foreach (element_children($element) as $key) {
    $row = array();

    $row['data'] = array();
    foreach ($header as $fieldname => $title) {
      $row['data'][] = drupal_render($element[$key][$fieldname]);
      $row['class'] = array('draggable'); // needed for table dragging
    }
    $rows[] = $row;
  }

  return theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => 'form_id'), // needed for table dragging
  ));
}


function fundraiser_tickets_order_ticket_types_form_validate($form, &$form_state) {
  if ($form_state['values']['op'] == 'Add ticket type'){
    drupal_goto('admin/commerce/products/add/fundraiser-ticket', array('query' => array('destination' => 'node/' . $form_state['build_info']['args'][0]->nid . '/tickets/tickets')));
  }
}

function fundraiser_tickets_order_ticket_types_form_submit($form, &$form_state) {
  foreach ($form_state['values']['tickets'] as $i => $ticket){
    $product_wrapper = entity_metadata_wrapper('commerce_product', commerce_product_load($ticket['ticket_id']));
    if ($product_wrapper->fr_tickets_weight->value() != $ticket['weight']){
      $product_wrapper->fr_tickets_weight->set($ticket['weight']);
      $product_wrapper->save();
    }
  }
}
