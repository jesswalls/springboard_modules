<?php
/**
 * @file
 * Secure prepopulate admin functions
 */

/**
 * System settings form for secure prepopulate.
 */
function secure_prepopulate_admin_settings($form, &$form_state) {
  $form = array();
  $form['secure_prepopulate_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Encryption key'),
    '#description' => t('To use 128 bit encryption enter a 16 character (128 bit) AES key. 256 bit encryption requires a 32 character key (256 bits).'),
    '#default_value' => variable_get('secure_prepopulate_key', ''),
  );
  $form['secure_prepopulate_iv'] = array(
    '#type' => 'textfield',
    '#title' => t('Initialization vector'),
    '#description' => t('A 16 character (128 bit) initialization vector. The initialization vector (IV) is a fixed-size input to a cryptographic primitive ' .
      'that is typically required to be random or pseudorandom.'),
    '#default_value' => variable_get('secure_prepopulate_iv', ''),
  );
  $form['secure_prepopulate_precedence'] = array(
    '#type' => 'radios',
    '#title' => t('Pre-populate precedence'),
    '#description' => t('Determines which set of values take precedence when the user is already logged in.'),
    '#options' => array(
      'url' => 'Encrypted URL values',
      'drupal' => 'Drupal values',
    ),
    '#default_value' => variable_get('secure_prepopulate_precedence', 'url'),
  );
  $form['secure_prepopulate_expiration_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Pre-populated token expiration settings'),
  );
  $form['secure_prepopulate_expiration_settings']['secure_prepopulate_toggle_expiration']= array(
    '#type' => 'checkbox',
    '#title' => t('Pre-populated tokens never expire'),
    '#description' => t('Check this setting to allow the pre-population token to be used on multiple site visits.'),
    '#default_value' => variable_get('secure_prepopulate_toggle_expiration', 0),
  );
  $form['secure_prepopulate_expiration_settings']['secure_prepopulate_expired_date_limit'] = array(
    '#type' => 'textfield',
    '#size' => 6,
    '#title' => t('Number of days before entries are cleared from the expired table'),
    '#description' => t('Clean out entries from the expired table that are older than the given amount of days.'),
    '#default_value' => variable_get('secure_prepopulate_expired_date_limit', 30),
    '#states' => array( // Disable this field if "pre-populated tokens never expire" is checked; otherwise enable.
      'disabled' => array(
        ':input[name="secure_prepopulate_toggle_expiration"]' => array('checked' => TRUE),
      ),
      'enabled' => array(
        ':input[name="secure_prepopulate_toggle_expiration"]' => array('checked' => FALSE),
      ),
    ),
  );
  return system_settings_form($form);
}

/**
 * Settings validation routine.
 */
function secure_prepopulate_admin_settings_validate(&$form, $form_state) {
  if (mb_strlen($form_state['values']['secure_prepopulate_key']) != 32 && mb_strlen($form_state['values']['secure_prepopulate_key']) != 16) {
    form_set_error('secure_prepopulate_key', 'Key must be either 128 bits (16 bytes) or 256 bits (32 bytes) in length.');
  }
  if (mb_strlen($form_state['values']['secure_prepopulate_iv']) != 16) {
    form_set_error('secure_prepopulate_iv', 'Initialization vector must be either 128 bits (16 bytes) in length.');
  }
  if (!is_numeric($form_state['values']['secure_prepopulate_expired_date_limit'])) {
    form_set_error('secure_prepopulate_expired_date_limit', 'Expired date limit must be numeric.');
  }
}


/**
 * Token Generator form.
 */
function secure_prepopulate_token_generator($form, $form_state) {

  $default_country = isset($form_state['values']['country']) ? $form_state['values']['country'] : 'US';
  $state_options = array();

  $form['secure_prepopulate'] = array(
    '#type' => 'fieldset',
    '#title' => t('Secure Pre-populate Token'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  // TODO: add user select box to import settings from existing user account.

  $form['secure_prepopulate']['first_name'] = array(
    '#type' => 'textfield',
    '#title' => t('First name'),
    '#size' => 20,
  );
  // last_name
  $form['secure_prepopulate']['last_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Last name'),
    '#size' => 20,
  );
  // email
  $form['secure_prepopulate']['email'] = array(
    '#type' => 'textfield',
    '#title' => t('Email'),
    '#size' => 50,
  );
  // address
  $form['secure_prepopulate']['address'] = array(
    '#type' => 'textfield',
    '#title' => t('Address'),
  );
  // address_line_2
  $form['secure_prepopulate']['address_line_2'] = array(
    '#type' => 'textfield',
    '#title' => t('Address 2'),
  );
  // city
  $form['secure_prepopulate']['city'] = array(
    '#type' => 'textfield',
    '#title' => t('City'),
  );

  // country
  $form['secure_prepopulate']['country'] = array(
    '#type' => 'textfield',
    '#title' => t('Country'),
  );

  // state
  $form['secure_prepopulate']['state'] = array(
    '#type' => 'textfield',
    '#title' => t('State or province'),
  );

  // zip
  $form['secure_prepopulate']['zip'] = array(
    '#type' => 'textfield',
    '#title' => t('Postal code'),
    '#size' => 10,
  );

  if (module_exists('dynamic_gift_strings')) {
    $form['dynamic_gift_strings'] = array(
      '#type' => 'fieldset',
      '#title' => t('Dynamic Gift Strings Token'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['dynamic_gift_strings']['amounts'] = array(
      '#title' => t('Ask amounts'),
      '#type' => 'textfield',
      '#description' => t('Enter the ask amounts you want to show here. Values should be comma-separated. Example: 27,47,97'),
      '#size' => 50,
    );
    $form['dynamic_gift_strings']['default_amount'] = array(
      '#title' => t('Default amount'),
      '#type' => 'textfield',
      '#description' => t('If you want one of the supplied amounts to be selected by default, enter that value here.'),
      '#size' => 5,
    );
  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Generate token'),
  );
  $form['#submit'][] = 'secure_prepopulate_token_generator_submit';
  return $form;
}


function secure_prepopulate_token_generator_submit($form, $form_state) {

  if ($af = secure_prepopulate_generate_af_string($form_state['values'])) {
    drupal_set_message('af token generated:');
    drupal_set_message($af);
  }
  if (module_exists('dynamic_gift_strings')) {
    if ($gs = secure_prepopulate_generate_gs_string($form_state['values'])) {
      drupal_set_message('gs token generated:');
      drupal_set_message($gs);
    }
  }
}


/**
 * Generate an af query string based on values submitted by the token generator form.
 */
function secure_prepopulate_generate_af_string($input) {
  $keys = array(
    'first_name',
    'last_name',
    'email',
    'address',
    'address_line_2',
    'city',
    'country',
    'state',
    'zip',
  );
  $query_str = array();
  foreach ($keys as $key) {
    if (isset($input[$key]) && $input[$key]) {
      $query_str[] = $key . '=' . $input[$key];
    }
  }
  if (!count($query_str)) {
    return FALSE;
  }
  $query_str = implode('&', $query_str);
  $crypt = secure_prepopulate_encrypt($query_str);
  return 'af=' . urlencode($crypt);
}

function secure_prepopulate_generate_gs_string($input) {
  $query = array();
  if (isset($input['amounts']) && $input['amounts']) {
    $query_str = 'amounts=' . str_replace(',', '|', $input['amounts']);
    if (isset($input['default_amount'])) {
      $query_str .= '&default=' . $input['default_amount'];
    }
    $crypt = secure_prepopulate_encrypt($query_str);
    return 'gs=' . urlencode($crypt);
  }
  return FALSE;
}
