/*!
 * jQuery OAuth via popup window plugin
 *
 * @author  Nobu Funaki @nobuf
 *
 * Dual licensed under the MIT and GPL licenses:
 *   http://www.opensource.org/licenses/mit-license.php
 *   http://www.gnu.org/licenses/gpl.html
 */
(function($){

  $.oauthpopup = function(options)
  {
    if (!options || !options.path) {
      throw new Error("options.path must not be empty");
    }
    options = $.extend({
      windowName: 'ConnectWithOAuth' // should not include space for IE
      , windowOptions: 'location=0,status=0,width=800,height=400'
      , callback: function(){window.location.reload();
      }
    }, options);

    var oauthWindow   = window.open(options.path, options.windowName, options.windowOptions);
    oauthWindow.truepath;
    // Jackson River modifications begin
    oauthWindow.focus();
    var oauthInterval = window.setInterval(function(){
      if (oauthWindow.closed) {
        window.clearInterval(oauthInterval);
        options.callback(oauthWindow.truepath);
      }
    }, 1000);
  };
  // Jackson River modifications end

  //bind to element and pop oauth when clicked
  $.fn.oauthpopup = function(options) {
    $this = $(this);
    $this.click($.oauthpopup.bind(this, options));
  };

})(jQuery);



