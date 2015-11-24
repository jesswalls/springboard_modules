(function ($) {
   Drupal.behaviors.webform_goals = {
    attach: function (context, settings) {
        var ids = [];
    items = jQuery('.wg-goal').map(function(){
      jQuery(this).hide();
      ids.push(jQuery(this).data('gid'));
    });
    // TODO: add site namespacing so goals can be shared to external Drupal sites that have this module installed.
    goal_ids = ids.join(',');
    if (goal_ids) {
    jQuery.ajax({
      dataType: 'jsonp',
      data: 'goal_ids=' + goal_ids,
      jsonp: 'jsonp_callback',

      url: Drupal.settings.webform_goals.loader_url,
      success: function (data) {
        // unpack data and place accordingly.
        //  data format:
        //    - status
        //    - id (eg 14)
        //      - markup (widget markup)
        //      - tokens
        //         - token (eg. [webform_goal:progress-raw]) => token replacement value (eg 11)
        for (i = 0; i < ids.length; ++i) {
          jQuery(".wg-goal").each(function() {

              if (jQuery(this).data('gid') === ids[i]) {
                  if (jQuery(this).is(':empty') || jQuery(this).html() === "\n" || jQuery(this).html() === "&nbsp;") {
                 // embed widget mode
               jQuery(this).html(data[ids[i]].markup);
             }
             else {
               element = jQuery(this);
               // token replacement mode
               tokens = data[ids[i]].tokens;
               jQuery.each(tokens, function(index, value) {
                 html = element.html().replace(index, value);
                 element.html(html);
               });
             }
             jQuery(this).show();
           }
          });
        }
      }
    });
    } // end if
    }
  };
})(jQuery);

