<?php

/**
 * Class FundraiserSustainersDailySnapshot
 */
class FundraiserSustainersDailySnapshot {

  /**
   * @var DateTime
   * Today.
   */
  protected $today;

  /**
   * @var DateTime
   * The day of this Snapshot.
   */
  protected $date;

  /**
   * @var int
   * The timestamp of the day start. Used for database queries.
   */
  protected $beginTimestamp;

  /**
   * @var int
   * The timestamp of the day start of the following day. Used for database
   * queries.
   */
  protected $endTimestamp;

  /**
   * @var bool
   */
  protected $isNew;

  /**
   * @var bool
   */
  protected $isComplete;

  /**
   * @var int
   * Timestamp when this was last saved.
   */
  protected $lastUpdated;

  /**
   * @var int
   * Number of charges scheduled, including processed.
   */
  protected $scheduledCharges;

  /**
   * @var int
   * Value (in cents) charges scheduled, including processed.
   */
  protected $scheduledValue;

  /**
   * @var int
   */
  protected $retriedCharges;

  /**
   * @var int
   */
  protected $retriedValue;

  /**
   * @var int
   */
  protected $processedCharges;

  /**
   * @var int
   */
  protected $processedValue;

  /**
   * @var int
   */
  protected $rescheduledCharges;

  /**
   * @var int
   */
  protected $rescheduledValue;

  /**
   * @var int
   */
  protected $abandonedCharges;

  /**
   * @var int
   */
  protected $abandonedValue;

  /**
   * @var int
   * Number of successful charges completed so far in this day.
   */
  protected $successes;

  /**
   * @var int
   * Number of failed charges attempted so far in this day.
   */
  protected $failures;

  protected $successValue;

  protected $failureValue;

  /**
   * @param DateTime $date
   */
  public function __construct(DateTime $date) {
    $this->date = $date;
    $this->date->setTime(0, 0, 0);

    $this->beginTimestamp = $this->date->getTimestamp();

    $date_end = clone $this->date;
    $date_end->add(new DateInterval('P1D'));
    $date_end->setTime(0, 0, 0);
    $this->endTimestamp = $date_end->getTimestamp();

    $this->today = new DateTime();
    $this->today->setTime(0, 0, 0);

    $this->isNew = TRUE;
    $this->isComplete = FALSE;

    $this->load();
  }

  /**
   *
   */
  public function save() {
    $this->lastUpdated = REQUEST_TIME;

    // Save to the database.
    $this->saveRow();
  }

  /**
   * @return DateTime
   */
  public function getDate() {
    return $this->date;
  }

  /**
   * @return int
   */
  public function getScheduledCharges() {
    return $this->scheduledCharges;
  }

  /**
   * @return int
   */
  public function getScheduledValue() {
    return $this->scheduledValue;
  }

  /**
   *
   * @return int
   */
  public function getTotalValue() {
    return $this->getScheduledValue();
  }

  /**
   * @return int
   */
  public function getSuccesses() {
    return $this->successes;
  }

  /**
   * @return int
   */
  public function getFailures() {
    return $this->failures;
  }

  /**
   * @return int
   */
  public function getAbandonedCharges() {
    return $this->abandonedCharges;
  }

  /**
   * @return int
   */
  public function getAbandonedValue() {
    return $this->abandonedValue;
  }

  /**
   * @return int
   */
  public function getProcessedCharges() {
    return $this->getSuccesses() + $this->getFailures();
  }

  /**
   * @return int
   */
  public function getProcessedValue() {
    return $this->getSuccessValue() + $this->getFailureValue();
  }

  public function getSuccessValue() {
    return $this->successValue;
  }

  public function getFailureValue() {
    return $this->failureValue;
  }

  /**
   * @return int
   */
  public function getRescheduledCharges() {
    return $this->rescheduledCharges;
  }

  /**
   * @return int
   */
  public function getRescheduledValue() {
    return $this->rescheduledValue;
  }

  /**
   * @return int
   */
  public function getRetriedCharges() {
    return $this->retriedCharges;
  }

  /**
   * @return int
   */
  public function getRetriedValue() {
    return $this->retriedValue;
  }

  /**
   * @return bool
   */
  protected function shouldUseLiveData() {
    return $this->today->format('Y-m-d') == $this->date->format('Y-m-d');
  }

  /**
   *
   */
  protected function load() {
    $this->initializeValues();
    // Look for a record in the DB and load it.
    // If there's no existing record, calculate new values.
    $row = $this->findRow();

    if (is_object($row)) {
      $this->isNew = FALSE;
      $this->loadRow($row);
    }

    if (!$this->isComplete || $this->shouldUseLiveData()) {
      $this->calculateValues();

      if ($this->endTimestamp <= $this->today->getTimestamp()) {
        $this->isComplete = TRUE;
      }
      $this->save();
    }
  }

  protected function loadRow($row) {
    $this->lastUpdated = $row->last_updated;
    $this->isComplete = $row->complete;
    $this->scheduledCharges = $row->scheduled_charges;
    $this->scheduledValue = $row->scheduled_value;
    $this->successes = $row->successes;
    $this->successValue = $row->success_value;
    $this->failures = $row->failures;
    $this->failureValue = $row->failure_value;
    $this->retriedCharges = $row->retried_charges;
    $this->retriedValue = $row->retried_value;
    $this->rescheduledCharges = $row->rescheduled_charges;
    $this->rescheduledValue = $row->rescheduled_value;
    $this->abandonedCharges = $row->abandoned_charges;
    $this->abandonedValue = $row->abandoned_value;
  }

  /**
   * Set up a record array for drupal_write_record.
   */
  protected function saveRow() {

    $record = array(
      'date' => $this->getDate()->format('Y-m-d'),
      'last_updated' => $this->lastUpdated,
      'complete' => $this->isComplete,
      'scheduled_charges' => $this->scheduledCharges,
      'scheduled_value' => $this->scheduledValue,
      'successes' => $this->successes,
      'success_value' => $this->successValue,
      'failures' => $this->failures,
      'failure_value' => $this->failureValue,
      'retried_charges' => $this->retriedCharges,
      'retried_value' => $this->retriedValue,
      'rescheduled_charges' => $this->rescheduledCharges,
      'rescheduled_value' => $this->rescheduledValue,
      'abandoned_charges' => $this->abandonedCharges,
      'abandoned_value' => $this->abandonedValue,
    );

    if ($this->isNew) {
      drupal_write_record('fundraiser_sustainers_insights_snapshot', $record);
    }
    else {
      drupal_write_record('fundraiser_sustainers_insights_snapshot', $record, 'date');
    }
  }

  /**
   * @return array
   */
  protected function findRow() {
    // Query for rows by the Y-m-d date string.
    // return db_query("SELECT * FROM {fundraiser_sustainers_insights_snapshot} WHERE date = :date", array(':date' => $this->getDate()->format('Y-m-d')))->fetchObject();
    return FALSE;
  }

  /**
   *
   */
  protected function calculateValues() {
    // Do the DB queries and math stuff here.
    $this->calculateScheduledProperties();

    $this->calculateOtherProperties();
  }

  protected function getValueFromOrder($did) {
    $order = commerce_order_load($did);
    $wrapper = entity_metadata_wrapper('commerce_order', $order);
//      $currency_code = $wrapper->commerce_order_total->currency_code->value();

    return $wrapper->commerce_order_total->amount->value();
  }

  protected function calculateScheduledProperties() {
    // success, canceled, skipped, failed, retry, processing, NULL
    $replacements = array(
      ':begin' => $this->beginTimestamp,
      ':end' => $this->endTimestamp,
    );

    $query = "SELECT did FROM {fundraiser_sustainers_log} WHERE (new_state = 'scheduled' OR (new_state = 'retry' AND old_state = 'processing')) AND next_charge >= :begin AND next_charge < :end";

    $result = db_query($query, $replacements);

    $count = 0;
    $total = 0;
    foreach ($result as $row) {
      $count++;
      $total += $this->getValueFromOrder($row->did);
    }

    $this->scheduledCharges = $count;
    $this->scheduledValue = $total;
  }

  protected function calculateOtherProperties() {
    $successes = 0;
    $failures = 0;
    $successValue = 0;
    $failureValue = 0;
    $retriedCharges = 0;
    $retriedValue = 0;
    $rescheduledCharges = 0;
    $rescheduledValue = 0;
    $abandonedCharges = 0;
    $abandonedValue = 0;

    $replacements = array(
      ':begin' => $this->beginTimestamp,
      ':end' => $this->endTimestamp,
    );
    // Get all log events that happened on this day.
    // Ordering these so the new transitions get checked first, and earlier
    // ones get ignored if there are multiple transitions per day.
    // @todo Is that a good idea?
    $results = db_query("SELECT did, old_state, new_state FROM {fundraiser_sustainers_log} WHERE timestamp >= :begin AND timestamp < :end ORDER BY lid DESC", $replacements);

    foreach ($results as $row) {

      // Sustainer was born and at best is going into locking and processing.
      // Otherwise the charge might be getting advanced or changed.
      if (is_null($row->old_state)) {
        continue;
      }

      // Special case, master donation.
      if ($row->old_state == 'success' && $row->new_state == 'success') {
        continue;
      }

      // Get the amount value from the related commerce order.
      $value = $this->getValueFromOrder($row->did);
      if ($row->old_state == 'processing') {

        if ($row->new_state == 'success') {
          $successes++;
          $successValue += $value;
        }
        else {
          $failures++;
          $failureValue += $value;

          if ($row->new_state == 'retry') {
            $rescheduledCharges++;
            $rescheduledValue += $value;
          }
          elseif ($row->new_state == 'failed') {
            $abandonedCharges++;
            $abandonedValue += $value;
          }
        }
      }
      elseif ($row->old_state == 'retry' && $row->new_state == 'processing') {
        $retriedCharges++;
        $retriedValue += $value;
      }
    }

    $this->successes = $successes;
    $this->failures = $failures;
    $this->successValue = $successValue;
    $this->failureValue = $failureValue;

    $this->retriedCharges = $retriedCharges;
    $this->retriedValue = $retriedValue;
    $this->rescheduledCharges = $rescheduledCharges;
    $this->rescheduledValue = $rescheduledValue;
    $this->abandonedCharges = $abandonedCharges;
    $this->abandonedValue = $abandonedValue;
  }

  /**
   *
   */
  protected function initializeValues() {
    $this->scheduledCharges = 0;
    $this->scheduledCharges = 0;
    $this->successes = 0;
    $this->failures = 0;
    $this->successValue = 0;
    $this->failureValue = 0;

    $this->abandonedCharges = 0;
    $this->abandonedValue = 0;
    $this->rescheduledCharges = 0;
    $this->rescheduledValue = 0;
    $this->processedCharges = 0;
    $this->processedValue = 0;
    $this->retriedCharges = 0;
    $this->retriedValue = 0;
  }
}