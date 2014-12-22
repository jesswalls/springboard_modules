<?php

/**
 * @file springbord_advocacy_views_plugin_query.inc
 *
 * Override views default db query with advocacy REST API query
 */

class springboard_advocacy_views_plugin_query extends views_plugin_query {

  /**
   * Advocay API client object
   *
   * @var client
   */
  protected $client;

  function init($base_table, $base_field, $options) {
    parent::init($base_table, $base_field, $options);
    $loader = new AdvocacyApiClientLoader();
    $this->client = $loader->getClient();
  }

  function build(&$view) {
    $view->init_pager($view);
    $this->pager->query();
    $view->build_info['springboard_advocacy_request'] = $this->client;
    // if (isset($view->query->search_terms)) {
    //   $view->build_info['springboard_advocacy_search'] = $view->query->search_terms;  
    // } 
  }

  function execute(&$view) {

    $client = $view->build_info['springboard_advocacy_request'];

    $start = microtime(true);
    $terms = !empty($view->query->search_terms) ? $view->query->search_terms : NULL;

    // If a non-exposed class_name search is present and it is the only search
    // suppress results because no search has actually been performed yet
    if ((count($terms) == 1) && isset($terms['class_name'])) {
      $suppress_results = $view->filter['search_class_name']->options['exposed'] == FALSE;
    }

    if (!empty($terms) && empty($suppress_results)) {
      foreach ($terms as $key =>$term) {
        // is it a multiple value checkbox?
        // then format in a the the api expects
        if (is_array($term)) {
          $terms[$key] = implode('|', $term);
        }
      }
      
      // Get the targets
      $targets = $client->searchTargets($terms);
   
      // Build the pager
      if (!empty($targets->data))  {
        if ($this->pager->use_pager()) {
          $this->pager->total_items = count($targets->data);
          if (!empty($this->pager->options['offset'])) {
            $this->pager->total_items -= $this->pager->options['offset'];
          }
          $this->pager->update_page_info();
        }
       
        if (!empty($this->limit) || !empty($this->offset)) {
          $limit  = intval(!empty($this->limit) ? $this->limit : 999999);
          $offset = intval(!empty($this->offset) ? $this->offset : 0);
          $targets->data = array_slice($targets->data, $offset, $limit, TRUE);
        }
      }
      else {
        $targets->data = array();
      }

      $view->result = array();
      $view->result = $targets->data;

      $this->pager->post_execute($view->result);
      
      if ($this->pager->use_pager()) {
        $view->total_rows = $this->pager->get_total_items();
      }

      $view->execute_time = microtime(true) - $start;
    }
  }
}