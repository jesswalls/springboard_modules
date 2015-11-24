(function() {

// Localize jQuery variable
    var jQuery;

    /******** Load jQuery if not present *********/
    if (window.jQuery === undefined || window.jQuery.fn.jquery !== '1.10.2') {
        var script_tag = document.createElement('script');
        script_tag.setAttribute("type","text/javascript");
        script_tag.setAttribute("src",
            "http://ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js");
        if (script_tag.readyState) {
            script_tag.onreadystatechange = function () { // For old versions of IE
                if (this.readyState == 'complete' || this.readyState == 'loaded') {
                    scriptLoadHandler();
                }
            };
        } else { // Other browsers
            script_tag.onload = scriptLoadHandler;
        }
        // Try to find the head, otherwise default to the documentElement
        (document.getElementsByTagName("head")[0] || document.documentElement).appendChild(script_tag);
    } else {
        // The jQuery version on the window is the one we want to use
        jQuery = window.jQuery;
        main();
    }

    /******** Called once jQuery has loaded ******/
    function scriptLoadHandler() {
        // Restore $ and window.jQuery to their previous values and store the
        // new jQuery in our local jQuery variable
        jQuery = window.jQuery.noConflict(true);
        // Call our main function
        main();
    }

    /******** Our main function ********/
    function main() {

        jQuery(document).ready(function() {
            var ids = [];
            items = jQuery('.wg-goal').map(function(){
                jQuery(this).hide();
                ids.push(jQuery(this).data('gid'));
            });

            goal_ids = ids.join(',');
            if (goal_ids) {
                jQuery.ajax({
                    dataType: 'jsonp',
                    data: 'goal_ids=' + goal_ids,
                    jsonp: 'jsonp_callback',
                    url: 'http://DOMAIN/webform_goals_jsonp',
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
                    },
                });
            }
        });
    }

})(); // We call our anonymous function immediately
