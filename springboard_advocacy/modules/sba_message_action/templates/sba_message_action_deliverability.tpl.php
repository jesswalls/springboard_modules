<?php print $tabs; ?>
<h2>Message Delivery Overview</h2>
<div class="container deliverability-performance-container">
  <div class="row">
    <div class="span3">
      <div class="well">
        <h2 class="text-info"><?php print $total_messages_processed; ?></h2>
        Processed Messages
      </div>
    </div>
    <div class="span3">
      <div class="well">
        <h2 class="text-info"><?php print $delivered; ?></h2>
        Delivered
      </div>
    </div>
    <div class="span3">
      <div class="well">
        <h2 class="text-info"><?php print $undeliverable; ?></h2>
        Undeliverable
      </div>
    </div>
    <div class="span3">
      <div class="well">
        <h2 class="text-info"><?php print $percentage; ?></h2>
        Deliverablity
      </div>
    </div>
  </div>
  <div class="row">
    <div class="span12 messages-generated">
      <?php print $total_messages_generated; ?> messages generated as of <?php print date('m/d/y g:ia T',$deliverability_cache_timestamp); ?>
    </div>
  </div>
</div>
<h2>Individual Target Deliverability</h2>
<?php print $targets_table; ?>