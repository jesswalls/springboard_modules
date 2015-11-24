<?php

/**
 * @file
 * Commerce based hook for commerce_payment_example.
 */

/**
 * Implements hook_fundraiser_donation_form_config_check().
 */
function commerce_payment_example_fundraiser_donation_form_config_check($node) {
  foreach ($node->gateways as $method => $gateway) {
    if (!empty($gateway['id']) && !empty($gateway['status'])) {
      $gateway_config = _fundraiser_gateway_info($gateway['id']);
      if ($gateway_config['gateway_details']['method_id'] == 'commerce_payment_example') {
        return array(t('This form is using the example credit payment gateway and will not process live transactions.'));
      }
    }
  }
}

function commerce_payment_example_fundraiser_gateway_status_cron_check($details) {
  if ($details['method_id'] == 'commerce_payment_example') {
    return 'example gateway';
  }
}

/**
 * Implements hook_fundraiser_commerce_fundraiser_gateway_info().
 */
function commerce_payment_example_fundraiser_commerce_fundraiser_gateway_info() {
  return array(
    'payment_method' => array('credit'),
    'allow_recurring' => array('credit'),
    'offsite_recurring' => array('credit'),
    'allow_refund' => array('credit'),
    'form callback' => 'commerce_payment_example_fundraiser_commerce_form',
    'cleanup callback' => 'commerce_payment_example_fundraiser_commerce_cleanup',
    'scrub callback' => 'commerce_payment_example_fundraiser_commerce_scrub',
    'expire callback' => 'commerce_payment_example_fundraiser_commerce_expire',
    'charge callback' => 'commerce_payment_example_fundraiser_commerce_charge',
    'refund callback' => 'commerce_payment_example_fundraiser_commerce_refund',
  );
}

/**
 * Callback function, use this form for the given gateway.
 */
function commerce_payment_example_fundraiser_commerce_form($payment_method) {
  module_load_include('inc', 'fundraiser_commerce', 'includes/fundraiser_commerce.credit_card');
  return _fundraiser_commerce_credit_card_form();
}

/**
 * Callback function, clean up the submission results after submission before they are returned to fundraiser.
 */
function commerce_payment_example_fundraiser_commerce_cleanup($submission_fields) {
  module_load_include('inc', 'fundraiser_commerce', 'includes/fundraiser_commerce.credit_card');
  return _fundraiser_commerce_credit_card_cleanup($submission_fields);
}

/**
 * Callback function, scrub the fields before they get saved.
 */
function commerce_payment_example_fundraiser_commerce_scrub($submission_fields, $payment_method) {
  module_load_include('inc', 'fundraiser_commerce', 'includes/fundraiser_commerce.credit_card');
  return _fundraiser_commerce_credit_card_scrub($submission_fields);
}

/**
 * Callback function, return the user entered experation dates for this submission.
 */
function commerce_payment_example_fundraiser_commerce_expire($submission_fields) {
  module_load_include('inc', 'fundraiser_commerce', 'includes/fundraiser_commerce.credit_card');
  return _fundraiser_commerce_credit_card_expire($submission_fields);
}

/**
 * Callback function, charge the gateway.
 */
function commerce_payment_example_fundraiser_commerce_charge($method_instance, $donation) {
  module_load_include('inc', 'fundraiser_commerce', 'includes/fundraiser_commerce.credit_card');

  // Translate donation settings to the correct values for the commerce gateway.
  $order = commerce_order_load($donation->did);
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  $charge = $order_wrapper->commerce_order_total->value();
  $pane_values = _fundraiser_commerce_credit_card_pane_values($donation);

  // Add fundraiser commerce data to the pane values array.
  _fundraiser_commerce_submit_handler_pane_values($pane_values, $donation);

  // Process the transaction. First look for special values that force a failure.
  if (commerce_payment_example_is_simulated_failure($donation)) {
    // Add a payment transaction record for failed payments.
    $transaction = commerce_payment_transaction_new('commerce_payment_example', $order->order_id);
    $transaction->instance_id = $method_instance['instance_id'];
    $transaction->amount = $charge['amount'];
    $transaction->status = COMMERCE_PAYMENT_STATUS_FAILURE;
    $transaction->remote_status = 'remote_failed';
    $transaction->message = t('Failed due to sustainer failure simulation mode.');
    commerce_payment_transaction_save($transaction);
    return FALSE;
  }
  // Pass the values through to the submit function.
  else {
    // During a reference charge the example gateway expects a credit card of a certain length for the transaction message.
    if (!empty($donation->reference_charge)) {
      $pane_values['credit_card']['number']  = str_pad($pane_values['credit_card']['number'], 8, 'x', STR_PAD_LEFT);
    }

    commerce_payment_example_submit_form_submit($method_instance, array(), $pane_values, $order, $charge);
    return TRUE;
  }
}


/**
 * Determines whether the gateway should simulate a payment failure.
 *
 * Failure is simulated by either a designed card number or postal code.
 *
 * @param $donation
 *   The donation being processed.
 */
function commerce_payment_example_is_simulated_failure($donation) {
  if (!$donation->reference_charge && $donation->donation['payment_fields']['credit']['card_number'] == '4222222222222') {
    return TRUE;
  }
  
  if ($donation->reference_charge && $donation->donation['zip'] == '55555') {
    return TRUE;
  }
   
  return FALSE;  
} 


/**
 * Callback function, refund the gateway.
 */
function commerce_payment_example_fundraiser_commerce_refund($payment_method, $refund) {
  // Retrieve data.
  $order = commerce_order_load($refund->did);
  $transactions = commerce_payment_transaction_load_multiple(array(), $conditions = array('order_id' => $order->order_id));
  $transaction = array_shift($transactions); // The first one is the original donation.
  $payment_method = commerce_payment_method_instance_load($transaction->instance_id);
  // Create a new transaction to record the credit.
  $credit_transaction = commerce_payment_transaction_new('commerce_payment_example', $order->order_id);
  $credit_transaction->instance_id = $payment_method['instance_id'];
  $credit_transaction->remote_id = 'remote_id';
  $credit_amount = commerce_currency_decimal_to_amount($refund->amount, $transaction->currency_code);
  $credit_transaction->amount = $credit_amount * -1;
  $credit_transaction->currency_code = $transaction->currency_code;
  $credit_transaction->payload[REQUEST_TIME] = 'success';
  $credit_transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
  $credit_transaction->remote_status = 'remote_success';
  $credit_transaction->message = t('Credited back.');
  // Save the credit transaction.
  commerce_payment_transaction_save($credit_transaction);
  return TRUE;
}
