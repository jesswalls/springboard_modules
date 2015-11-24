(function ($) {
    Drupal.behaviors.multstepForm = {
        attach: function (context, settings) {
            var editSubmit = $('#sba-message-action-preview-form').find('#edit-submit');
            var send = $('#multiflow-send-now');

            editSubmit.once(function() {
                editSubmit.click(function() {
                    submitOnce(editSubmit, send, this);
                });
            });

            send.once(function() {
                send.click(function() {
                    submitOnce(editSubmit, send, this);
                    $('#sba-message-action-preview-form').submit();
                    return false;
                });
            });

        } // attach.function
    } // drupal.behaviors

    $(document).ready(function () {
        $('textarea').each(function (e) {
            $(this).height(this.scrollHeight)
        });
        $('label').has('.form-required').addClass('required');
    });

    function submitOnce(editSubmit, send, clicked) {

        if($('#sba-message-action-preview-form')[0].checkValidity) {
            if($('#sba-message-action-preview-form')[0].checkValidity()) {
                editSubmit.hide();
                var link = send.replaceWith('<span id="multiflow-send-now-processed">send now</span>');
                if (clicked.id == 'multiflow-send-now') {
                    rotate();
                }
                editSubmit.after('<div class="webform-user-wrapper">' +
                    '<p class="webform-user-thank-you">Thank you, your submission is processing.</p>' +
                    '<div class="webform-user-spinner"></div>' +
                    '</div>');
            }
        }
        else {
            editSubmit.hide();
            var link = send.replaceWith('<span id="multiflow-send-now-processed">send now</span>');
            if (clicked.id == 'multiflow-send-now') {
                rotate();
            }
            editSubmit.after('<div class="webform-user-wrapper">' +
                '<p class="webform-user-thank-you">Thank you, your submission is processing.</p>' +
                '<div class="webform-user-spinner"></div>' +
                '</div>');
        }
    }
    function rotate(num) {
        console.log('d')
        num = typeof num !== 'undefined' ? num : 10;
        $('#multiflow-send-now-wrapper').css({ 'transform': 'rotate(' + num + 'deg)'});
        var spinner = setTimeout(function() { rotate(num+10); }, 10);
        if (num >= 360) {
            clearTimeout(spinner);
        }
    }

})(jQuery);