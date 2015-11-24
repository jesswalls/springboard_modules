    (function ($) {
    Drupal.behaviors.SpringboardTaxonomyPermissionsForm = {
        attach: function(context, settings) {
            var rows = $('table#permissions tr');
            var hidem = false;
            // Hide everything but the rows necessary to administer organization issues.
            rows.each(function(){
                var id = $(this).children('td:first').attr('id');
                if ($(this).children('td:first').hasClass('module') && (id != 'module-taxonomy' && id != 'module-springboard_taxonomy')) {
                    hidem = true;
                }
                else if ($(this).children('td:first').hasClass('module')) {
                    hidem = false;
                }
                if(hidem == true) {
                    $(this).hide();
                }
                else {
                    $(this).children('td:first:contains("Legislative Issues")').parents('tr').hide();
                    $(this).children('td:first:contains("Edit terms in Tags")').parents('tr').hide();
                    $(this).children('td:first:contains("Delete terms from Tags")').parents('tr').hide();
                }
            })
        }
    };
})(jQuery);