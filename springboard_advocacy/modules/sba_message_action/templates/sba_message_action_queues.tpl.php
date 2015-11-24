<?php print $tabs; ?>
<h2>Current Message Queue Activity</h2>
<div class="container queue-overview-container">
  <div class="row">
    <div class="span3">
      <div class="well">
        <h2 class="text-info"><?php print $queue_ready_items; ?></h2>
        Ready
      </div>
    </div>
    <div class="span3">
      <div class="well">
        <h2 class="text-info"><?php print $queue_paused_items; ?></h2>
        Paused
      </div>
    </div>
    <div class="span3">
      <div class="well">
        <h2 class="text-info"><?php print $queue_canceled_items; ?></h2>
        Canceled
      </div>
    </div>
    <div class="span3">
      <div class="well">
        <h2 class="text-info"><?php print $queue_hold_items; ?></h2>
        On-Hold
      </div>
    </div>
  </div>
  <h2>Queue Actions:</h2>
  <div class="row">
    <div class="span12">
      <div class="well">
            <?php print $queue_actions_form; ?>
        </div>
      </div>
    </div>
  </div>
</div>
