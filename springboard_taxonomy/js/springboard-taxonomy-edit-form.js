(function ($) {

    Drupal.behaviors.SpringboardTaxonomyEditForm = {
        attach: function(context, settings) {
            // Do all of this only once, on page load.
            var optionContainer = $('#edit-springboard-taxonomy-options');
            if (optionContainer.hasClass('springboard-taxonomy-edit-form-ready'))
                return;

            // Is the action options or org issues tab not visible? Then we have to engage in some gymnastics
            // for a visibility limitation of jQuery Columnizer.
            if (optionContainer.css('display') == 'none') {
                optionContainer.css({'visibility': 'hidden', 'display': 'block'})
                var hiddenOnLoad = true;
            }
            else {
                hiddenOnLoad = false;
            }

            // Columnize the Legislative Issues list based on the average label width.
            if(optionContainer.has('.field-name-field-sba-legislative-issues').length === 1) {
                var legLabelWidthsTotal = 0;
                var legLabelCount = 0;
                var checkboxes = optionContainer.find('.field-name-field-sba-legislative-issues .form-type-checkbox');
                checkboxes.each(function () {
                    legLabelWidthsTotal += $(this).find('label').outerWidth();
                    legLabelCount++;
                });
                var legCheckboxWidth =
                  optionContainer.find(' .field-name-field-sba-legislative-issues .form-type-checkbox input').outerWidth();
                if (legLabelCount > 0) {
                    var legAverageLabelWidth = legLabelWidthsTotal / legLabelCount;
                    var legBufferWidth = legAverageLabelWidth * 1.2;  // Add 20% buffer.
                    var legColumnWidth = Math.ceil(legAverageLabelWidth + legCheckboxWidth + legBufferWidth);
                    var legContainer = checkboxes.parent();
                    columnizeItems(legContainer, legColumnWidth, hiddenOnLoad);
                }
            }

            // Columnize the Organization Issues list
            if(optionContainer.has('.field-name-field-sba-organization-issues').length === 1) {
                labelWidthsTotal = labelCount = 0;
                if (hiddenOnLoad == true) {
                    optionContainer.css({'visibility': 'hidden', 'display': 'block'})
                }
                optionContainer.find('.field-name-field-sba-organization-issues .form-type-checkbox').each(function () {
                    labelWidthsTotal += $(this).find('label').width();
                    console.log($(this).find('label').css('*'))
                    labelCount++;
                });
                var checkboxWidth =
                  optionContainer.find('.field-name-field-sba-organization-issues .form-type-checkbox input')
                    .outerWidth();
                if (labelCount > 0) {
                    // When a top-level item is changed, update all of its descendants to match.
                    var onChange = function (e) {
                        var isChecked = $(this).parent().find('input:not(.sba-term-indent)').get(0).checked;
                        $(this).parents('.sba-term-group').find('input.sba-term-indent').attr('checked', isChecked);
                    };
                    // Wrap groups and attach click behavior.
                    var orgContainer = optionContainer.find('.field-name-field-sba-organization-issues .form-type-checkbox').parent();

                    optionContainer.find('.field-name-field-sba-organization-issues .form-type-checkbox').each(function () {
                        if (!$(this).find('input').hasClass('sba-term-indent')) {
                            $(this).find('input').change(onChange);
                            // Create a new group for this top-level term.
                            orgContainer.append('<div class="sba-term-group dontsplit"></div>');
                        }
                        $(this).appendTo(orgContainer.find('div.sba-term-group:last'));
                    });

                    var averageLabelWidth = labelWidthsTotal / labelCount;
                    var bufferWidth = averageLabelWidth * 1.3;
                    var columnWidth = Math.ceil(averageLabelWidth + checkboxWidth + bufferWidth);
                    columnizeItems(orgContainer, columnWidth, hiddenOnLoad);
                }
            }
            // Done.
            optionContainer.addClass('springboard-taxonomy-edit-form-ready');
        }
    };

    function columnizeItems (container, columnWidth, hiddenOnLoad) {
        var columnNum = null;

        if (hiddenOnLoad == true) {
            if ($(window).width() < 800) {
                columnNum = 2;
                columnWidth = null;
            }
            else {
                columnNum = null;
            }
            container.columnize({
                columns: columnNum,
                width: columnWidth,
                doneFunc: hideIt,
                ignoreImageLoading: true,
                buildOnce: true

            });
        }
        else {
            container.columnize({
                width: columnWidth,
            });
        }
    }

    function hideIt() {
        $('#edit-springboard-taxonomy-options').css({'visibility': 'visible', 'display': 'none'});
    }

})(jQuery);