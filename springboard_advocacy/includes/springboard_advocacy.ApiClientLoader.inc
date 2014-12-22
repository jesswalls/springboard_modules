<?php

/**
 * @file
 * Loads and instantiates an SpringboardAdvocacyApiClient object.
 */

class AdvocacyApiClientLoader 

{
  public function getClient() 
  {
    $lib = libraries_load('springboard_advocacy');
    $domain = variable_get('springboard_advocacy_server_url', '');
    $client = new SpringboardAdvocacyAPIClient($domain);
    $token = $this->getToken($client);
    if (!empty($token->access_token)) {
      $client->setToken($token->access_token);
    }
    return $client;
  }

  private function getToken($client) 
  {
    $token = variable_get('springboard_advocacy_access_token', array());
    if (!empty($token->access_token)) {
      $refresh = ($token->created < (time() - ($token->expires_in - 600)));
    }
    if (empty($token->access_token) || !empty($refresh)) {
      $client_id = variable_get('springboard_advocacy_client_id', '');
      $client_secret = variable_get('springboard_advocacy_client_secret', '');
      $token = $client->getToken($client_id, $client_secret);
      if (!empty($token->access_token)) {
        $token->created = time();
        variable_set('springboard_advocacy_access_token', $token);
        return $token;
      }
    }
    return $token;
  }
}