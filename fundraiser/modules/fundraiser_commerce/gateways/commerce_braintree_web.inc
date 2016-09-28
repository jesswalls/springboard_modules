<?php

/**
 * @file
 * Commerce based hook for commerce_authnet.
 */

/**
 * Implements hook_fundraiser_commerce_fundraiser_gateway_info().
 */
function commerce_braintree_web_fundraiser_commerce_fundraiser_gateway_info() {
  return array(
    'payment_method' => array('credit', 'paypal'),
    'allow_recurring' => array('credit', 'paypal'),
    'allow_refund' => array('credit', 'paypal'),
    'form callback' => 'commerce_braintree_web_fundraiser_commerce_form',
    // 'cleanup callback' => 'commerce_authnet_fundraiser_commerce_cleanup',
    // 'scrub callback' => 'commerce_authnet_fundraiser_commerce_scrub',
    'expire callback' => 'commerce_braintree_web_fundraiser_commerce_expire',
    // 'validate callback' => 'commerce_authnet_fundraiser_commerce_validate',
    'charge callback' => 'commerce_braintree_web_fundraiser_commerce_charge',
    // 'update callback' => 'commerce_authnet_fundraiser_commerce_update',
    'refund callback' => 'commerce_braintree_web_fundraiser_commerce_refund',
    // 'cardonfile callback' => 'commerce_braintree_web_fundraiser_commerce_cardonfile',
    // 'response fields callback' => 'commerce_authnet_fundraiser_commerce_response_fields',
    // 'response save callback' => 'commerce_authnet_fundraiser_commerce_response_save',
  );
}

/**
 * Implements hook_fundraiser_donation_form_config_check().
 */
function commerce_braintree_web_fundraiser_donation_form_config_check($node) {
  $messages = array();
  foreach ($node->gateways as $gateway) {
    if (!empty($gateway['id'])  && !empty($gateway['status'])) {
      $gateway_config = _fundraiser_gateway_info($gateway['id']);
      if ($gateway_config['gateway_details']['module'] == 'commerce_braintree_web') {
        if ($gateway_config['gateway_details']['settings']['environment'] == 'sandbox') {
          $messages[] = t('%gateway is currently using the sandbox environment and will not process live transactions.', array('%gateway' => $gateway_config['title']));
        }
      }
    }
  }
  return $messages;
}

/**
 * Card on file callback.
 *
 * We get payment fields from the transaction payload associated with the master
 * donation id, if one is present.
 */
// function commerce_braintree_web_fundraiser_commerce_cardonfile($donation) {
//   $cardonfile_fields = array();

//   if (isset($donation->master_did)) {
//     $master_donation = fundraiser_donation_get_donation($donation->master_did);

//     $payload = db_query("SELECT payload FROM {commerce_payment_transaction} WHERE remote_id = :txn_id LIMIT 1", array(':txn_id' => $master_donation->txn_id))->fetchField();

//     if (isset($payload)) {
//       // Braintree classes appear to not be available during fundraiser_cron.
//       // Load them here so we can use the object in the payload.
//       if (!class_exists('Braintree\Result\Successful')) {
//         libraries_load('braintree_php');
//       }
//       $braintree_response_object = unserialize($payload);
//       $braintree_response_object = reset($braintree_response_object);
//       // $first_key = key($braintree_response_object);
//       // $transaction = $braintree_response_object[$first_key]->transaction;
//       $transaction = $braintree_response_object->transaction;
//       $cardonfile_fields = array(
//         'card_number' => isset($transaction->creditCard['last4']) ? $transaction->creditCard['last4'] : '',
//         'card_exp_month' => isset($transaction->creditCard['expirationMonth']) ? $transaction->creditCard['expirationMonth'] : '',
//         'card_exp_year' => isset($transaction->creditCard['expirationYear']) ? $transaction->creditCard['expirationYear'] : '',
//         'instance_id' => $donation->gateway['id'],
//         'uid' => $donation->uid,
//       );
//     }
//   }
//   return $cardonfile_fields;
// }

/**
 * Callback function, return the user entered experation dates for this submission.
 */
function commerce_braintree_web_fundraiser_commerce_expire($submission_fields) {
  $expires = array();
  if (isset($submission_fields['txn_id'])) {
    $payload = db_query("SELECT payload FROM {commerce_payment_transaction} WHERE remote_id = :txn_id LIMIT 1", array(':txn_id' => $submission_fields['txn_id']))->fetchField();
    if (isset($payload)) {
      $braintree_response_object = unserialize($payload);
      reset($braintree_response_object);
      $first_key = key($braintree_response_object);
      $transaction = $braintree_response_object[$first_key]->transaction;
      if (isset($transaction->creditCard['expirationMonth'])) {
        $expires['month'] = $transaction->creditCard['expirationMonth'];
      }
      if (isset($transaction->creditCard['expirationYear'])) {
        $expires['year'] = $transaction->creditCard['expirationYear'];
      }
    }
  }
  // If we could not find expiration month and year in the transaction
  // payload (eg paypal), stub an expiration date 24 months in the future.
  if (!isset($expires['month']) && !isset($expires['year'])) {
    $months = variable_get('commerce_braintree_web_sustainer_series_count', 24);
    $exp_timestamp = strtotime('+' . $months . ' months');
    $expires['month'] = date('m', $exp_timestamp);
    $expires['year'] = date('Y', $exp_timestamp);
  }
  return $expires;
}

/**
 * Refund callback for authnet, wraps the admin form used for refund with our own system.
 */
function commerce_braintree_web_fundraiser_commerce_refund($payment_method, $refund) {
  // Retrieve data.
  $refunding_order = commerce_order_load($refund->did);
  $transactions = commerce_payment_transaction_load_multiple(array(), $conditions = array('order_id' => $refunding_order->order_id));
  $transaction = array_shift($transactions); // The first one is the original donation.

  $form = array();
  $form_state = array();
  // Set up the values needed for the transaction to go through.
  $form_state['transaction'] = $transaction;
  $form_state['values']['amount'] = $refund->amount;
  $form_state['order'] = $refunding_order;

  $form_state['payment_method'] = commerce_payment_method_instance_load($transaction->instance_id);
  // Submit the donation form.
  // module_load_include('inc', 'commerce_braintree_web', '/includes/commerce_authnet.admin');
  $result = commerce_braintree_web_refund_form_submit($form, $form_state);
  // At this point the card has been saved on the user profile.
  $success = FALSE;
  if ($result !== FALSE) {
    $success = TRUE;
  }
  return $success;
}

/**
 * Callback function, use this form for the given gateway.
 */
function commerce_braintree_web_fundraiser_commerce_form($payment_method, $config) {
  $payment_method_instance = commerce_payment_method_instance_load($config['id']);
  braintree_initialize($payment_method_instance['settings']);
  // @TODO Possibily replace with _braintree_payment_form().

  global $user;
  $using_existing = FALSE;

  $form = array();
  $form['#attached']['library'][] = array('braintree_payment', 'braintree.js');

  $form['braintree'] = array(
    '#type' => 'container',
    '#id' => 'payment-details',
  );

  $form['braintree']['new'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('braintree-payment-form'),
    ),
    '#prefix' => '<div id="braintree-payment-form-outer">',
    '#suffix' => '</div>',
  );

  _braintree_payment_form_element($form['braintree']['new'], $form, $form_state, $payment_method_instance);

  if ($using_existing && ($user->uid > 0) && module_exists('braintree_vault') && ($customer = braintree_customer_load_by_account($user))) {
    $default_method = braintree_customer_default_payment_method($customer);
    $options = braintree_payment_method_options($customer->paymentMethods);

    if (count($options) > 0) {
      $form['braintree']['using_existing_payment_method'] = array(
        '#type' => 'hidden',
        '#default_value' => 1,
      );

      if (!empty($form_state['values'])) {
        $parent_array[] = 'payment_details';
        $parent_array[] = 'braintree';
        $parent_array[] = 'using_existing_payment_method';

        if ($default_value = drupal_array_get_nested_value($form_state['values'], $parent_array)) {
          $form['braintree']['using_existing_payment_method']['#default_value'] = $default_value;
        }

      }

      $form['braintree']['payment_method_token'] = array(
        '#type' => 'select',
        //'#title' => t('Choose your default payment method'),
        '#options' => $options,
        '#default_value' => $default_method->token,
        '#attributes' => array(
          'class' => array('braintree-primary-payment-method'),
        ),
      );

      $form['braintree']['default_method_token'] = array(
        '#type' => 'value',
        '#value' => $default_method->token,
      );
    }
  }

  $form['#validate'][] = '_braintree_payment_form_validate';
  $form['#submit'][] = '_braintree_payment_form_submit';

  return $form;
}

/**
 * Callback function, charge gateway.
 */
function commerce_braintree_web_fundraiser_commerce_charge($method_instance, $donation) {

  // Translate from donation settings to the correct values for the plugin.
  // module_load_include('inc', 'fundraiser_commerce', 'includes/fundraiser_commerce.credit_card');
  $order = commerce_order_load($donation->did);
  $wrapper = entity_metadata_wrapper('commerce_order', $order);
  $charge = $wrapper->commerce_order_total->value();

  $pane_form = array();

  // We may need to set these values. They are used in
  // commerce_braintree_web_submit_form_submit() below.
  // $pane_values['braintree']['using_existing_payment_method']
  // $pane_values['braintree']['payment_method_token']
  // $pane_values['braintree']['payment_method_token']
  // $pane_values['braintree']['default_method_token'].

  // @TODO this function won't work b/c it is looking at CC field values that
  // do not exist with hosted fields.
  // $pane_values = _fundraiser_commerce_credit_card_pane_values($donation);
    $pane_values = array();

  _fundraiser_commerce_submit_handler_pane_values($pane_values, $donation);

  $success = FALSE;
  $result = commerce_braintree_web_submit_form_submit($method_instance, $pane_form, $pane_values, $order, $charge);
  if ($result !== FALSE) {
    $success = TRUE;
  }

  if (isset($result->card_id)) {
    $donation->data['cardonfile'] = $result->card_id;
  }
  if (isset($result->transaction->paymentInstrumentType) && $result->transaction->paymentInstrumentType == 'credit_card') {
    $donation->donation['payment_fields'] = array(
      'credit' => array(
        'card_number' => $result->transaction->creditCard['last4'],
        'card_type' => $result->transaction->creditCard['cardType'],
        'card_expiration_month' => $result->transaction->creditCard['expirationMonth'],
        'card_expiration_year' => $result->transaction->creditCard['expirationYear'],
      ),
    );
  }

  // Adapted from commerce_authnet_echeck.inc.
  // If this is recurring PayPal payment.
  if ($success == TRUE && isset($donation->recurring) && !empty($donation->recurring) && $method_instance['method_id'] == 'braintree_paypal') {
    // Update expiration date and generate the next donation.
    $card_data = _fundraiser_commerce_donation_cardonfile_card($donation);
    if (!empty($card_data) && $donation->status <> 'failed') {
      // If the next recurring series entry doesn't exist, create one.
      if (_fundraiser_sustainers_count_donations_recurr_remaining($donation->recurring->master_did) < 1) {
        // Move card expiration date ahead one month.
        $new_expire_time = mktime(0, 0, 0, ($card_data->card_exp_month + 1), 1, $card_data->card_exp_year);
        $card_data->card_exp_month = date('m', $new_expire_time);
        $card_data->card_exp_year = date('Y', $new_expire_time);
        $submission_fields = array(
          'payment_fields' => array(
            'braintree_paypal' => array(
              'card_expiration_month' => $card_data->card_exp_month,
              'card_expiration_year' => $card_data->card_exp_year,
            ),
          ),
        );
        // Update card data.
        commerce_cardonfile_save($card_data);
        // Create new orders.
        $master_donation = fundraiser_donation_get_donation($donation->recurring->master_did);
        fundraiser_sustainers_update_billing_info_create_new_donations($master_donation, $donation, $submission_fields);
      }
    }
  }

// _fundraiser_commerce_charge_submit_form_process needs $pane_values[$form_key]['cardonfile_store']
// .... calls _fundraiser_commerce_donation_cardonfile_card($donation)
// ... looks at $donation->data['cardonfile'] and loads the cardonfile.

  // Perform post processing functions.
  _fundraiser_commerce_charge_submit_form_process($success, $method_instance, $pane_values, $donation);

  return $success;

}