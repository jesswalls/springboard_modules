(function ($) {

  Drupal.behaviors.fundraiserDesignations = {
    attach: function(context, settings) {
        new Drupal.fundraiserDesignations();
    }
  };

  /**
   * Set the amounts on page load
   */
  Drupal.fundraiserDesignations = function() {
    this.fundGroups = $('.designation-group-wrapper');
    this.cart = $('.fundraiser-designation-cart-wrapper');
    this.addon = $('.designation-addon-wrapper');

    this.cartTemplate = '<tr class="cart-fund-row"><td class="fund-cancel">x</td><td class="fund-name"></td><td class="fund-amount"></td> </tr>';
    this.errorTemplate = '<div class="error-message"></div>';
    this.addListeners();
    this.cancelButton();
    this.setWidths();
    var val = this.calcTotal();
    $('#cart_total').val(Drupal.settings.fundraiser.currency.symbol + (val).formatMoney(2, '.', ','));
    var cartVals = $('input[name$="[fund_catcher]"]').val().replace(/&quot;/g, '"');
    if (cartVals.length > 0) {
      cartVals = JSON.parse(cartVals);
      var self = this;
      $.each(cartVals, function(i, item){
        self.repop(item);
      });
    }
  };

  /**
   * Set the amounts on select
   */
  // Iterate over each field in the settings and add listener
  Drupal.fundraiserDesignations.prototype.addListeners = function() {

    var self = this;
    $.each(self.fundGroups, function(key, item) {

      var fundGroup = $(item);
      var fundGroupId = item['id'].replace('designation-group-', '');
      var selector = fundGroup.find('select[name*="funds_select"]');
      var defaultAmts = $('div[id*="default-amounts-"]', fundGroup);
      var recurAmts = $('div[id*="recurring-amounts-"]', fundGroup);
      var otherAmt = $('input[type="text"]', fundGroup);
      var quantity = fundGroup.find('select[name*="funds_quant"]');


      $(window).ready(function () {
        self.validateOtherAmt(fundGroupId);
      });

      otherAmt.keyup(function() {
        $('input[type="radio"]', fundGroup).each(function(){
          $(this).prop('checked', false);
        })
      });

      otherAmt.blur(function() {
        //var message = $('label.error').text();
        $('label.error').remove();
      });

      $.each($('input[type="radio"]', fundGroup), function(){
        $(this).on('click', function(){
          otherAmt.val('');
        });
      });

      $('input[type="submit"]', fundGroup).click(function() {
        var type = 'fund';
        self.addFund(type, fundGroupId, selector, defaultAmts, recurAmts, otherAmt, quantity);
        return false;
      });
    });

    $.each(self.addon, function(key, item) {
      var addOnFundId = item['id'].replace('designation-addon-', '');
      var addOnAmts = $('div[id*="default-amounts-"]', $(item));
      $('input[type="radio"]', $(item)).click(function() {
        var type = 'addon';
        self.addFund(type, addOnFundId, null, addOnAmts, null, null, 1);
      });

    });
  };

  Drupal.fundraiserDesignations.prototype.addFund = function(type, fundGroupId, selector, defaultAmts, recurAmts, otherAmt, quantity) {
    var self = this;

    if (type == 'fund') {
      // Test to see if fund and amount selection has been made.
      var go = self.validateFund(fundGroupId, selector, defaultAmts, recurAmts, otherAmt);
      if (go != 'ok') {
        return;
      }
    }

    // Grab amount.
    var amt = 0;
    if (defaultAmts.length > 0 && defaultAmts.is(':visible')) {
      amt = $(defaultAmts).find('input:checked').val();
      if (type == 'fund') {
        $(defaultAmts).find('input:checked').attr('checked', false)
      }
    }
    if (type == 'fund') {
      if (recurAmts.length > 0 && recurAmts.is(':visible')) {
        amt = $(recurAmts).find('input:checked').val();
        $(recurAmts).find('input:checked').attr('checked', false);
      }
    if (otherAmt.val()) {
      amt = otherAmt.val();
      otherAmt.val('');
    }
  }

    // The displayAmount is currency formatted, and can be different than the line item amount (i.e., totaled)
    var displayAmt = Drupal.settings.fundraiser.currency.symbol + (parseInt(amt)).formatMoney(2, '.', ',');

    // Grab the quantity. Update the display Amount.
    var quant = 1;
    var displayQuant = '';
    if (type == 'fund' && quantity.length > 0) {
      quant = quantity.val();
      displayAmt = amt * quant;
      displayAmt = Drupal.settings.fundraiser.currency.symbol + (parseInt(displayAmt)).formatMoney(2, '.', ',');
      if (quant > 1) {
        displayQuant = ' (' + quant + ' x ' + Drupal.settings.fundraiser.currency.symbol + amt + ')';
      }
    }

    // Grab the fund name and ID.
    if (type =='fund') {
      if (selector.length > 0) {
        var fundId = selector.val();
        var fundName = $('option:selected', selector).text();
      }
      else {
        fundId = $('tr.group-row-' + fundGroupId).attr('data-placeholder-fund-id');
        fundName = $('#funds-placeholder-' + fundGroupId).find('label').text();
      }
    }
    else {
      fundId = fundGroupId;
      fundName = 'One-time add-on donation';
    }
    self.newRow(displayAmt, displayQuant, type, fundId, amt, quant, fundName, fundGroupId);
  };

  /**
   * @param {
   * {fundAmount:string},
   * {fundQuantity:string},
   * {fundId:string},
   * {fundName:string},
   * {fundGroup:string},
   * {addonId:string}
   * } item
   */
  Drupal.fundraiserDesignations.prototype.repop = function(item) {
    var self = this;

    // Grab amount.
    var amt = item.fundAmount;

    // The displayAmount is currency formatted, and can be different than the line item amount (i.e., totaled)
    var displayAmt = Drupal.settings.fundraiser.currency.symbol + (parseInt(amt)).formatMoney(2, '.', ',');

    // Grab the quantity. Update the display Amount.
    var quant = item.fundQuantity;
    var displayQuant = '';
    var type = typeof(item.fundId) != 'undefined' ? 'fund' : 'addon';
    if (type == 'fund') {
      displayAmt = amt * quant;
      displayAmt = Drupal.settings.fundraiser.currency.symbol + (parseInt(displayAmt)).formatMoney(2, '.', ',');
      if (quant > 1) {
        displayQuant = ' (' + quant + ' x ' + Drupal.settings.fundraiser.currency.symbol + amt + ')';
      }
    }

    var fundId = typeof(item.fundId) != 'undefined' ? item.fundId : item.addonId;
    var fundName = item.fundName;
    var fundGroupId = item.fundGroup;

    self.newRow(displayAmt, displayQuant, type, fundId, amt, quant, fundName, fundGroupId);
  };

  Drupal.fundraiserDesignations.prototype.newRow = function (displayAmt, displayQuant, type, fundId, amt, quant, fundName, fundGroupId) {
    // Get the cart row template and add the fund, amount and qunatity html5 attributes.
    var self = this;
    var newRow = $(self.cartTemplate);
    $('.fund-amount', newRow).text(displayAmt);
    $('.fund-name', newRow).text(fundName + displayQuant);
    if (type == 'fund') {
      newRow.attr('data-fund-id', fundId);
    }
    else {
      newRow.attr('data-addon-id', fundId);
    }
    newRow.attr('data-fund-amount', amt);
    newRow.attr('data-fund-quantity', quant);
    newRow.attr('data-fund-name', fundName);
    newRow.attr('data-fund-group', fundGroupId);

    // Check if a row already exists for this fund and donation amount. Replace if so.
    var exists = false;
    $('tr', self.cart).each(function(){
      if($(this).attr('data-fund-id') == fundId && $(this).attr('data-fund-amount') == amt) {

        var oldAmt = parseInt($('.fund-amount', $(this)).text().replace('$', ''));
        var oldQuant = $(this).attr('data-fund-quantity');
        var newQuant = parseInt(oldQuant) + parseInt(quant);
        var newAmt = parseInt($('.fund-amount', newRow).text().replace('$', '')) + oldAmt;

        newRow.attr('data-fund-quantity', newQuant);
        if (newQuant > 1) {
          displayQuant = ' (' +  newQuant + ' x ' + Drupal.settings.fundraiser.currency.symbol + amt + ')';
        }

        $('.fund-name', newRow).text(fundName + displayQuant);

        $('.fund-amount', newRow).text(Drupal.settings.fundraiser.currency.symbol + (parseInt(newAmt)).formatMoney(2, '.', ','));

        $(this).replaceWith(newRow);
        exists = true;
      }
      if($(this).attr('data-addon-id') == fundId) {
        $(this).replaceWith(newRow);
        exists = true;
      }
    });

    // Insert a new row if not exists.
    if (!exists) {
      newRow.insertBefore('.cart-total-row');
      newRow.hide().show(300);
    }

    // Set the json encoded fund values in the hidden field.
    self.setFormValue();
    var cfe = $(".cart-fund-empty");
    if (cfe.is(':visible')) {
      cfe.hide();
    }
    self.cancelButton();
    self.setAmounts();

  };

  // Attaches JSONified data attributes of the funds to a hidden form field.
  Drupal.fundraiserDesignations.prototype.setFormValue = function () {
    var obj = {};
    $('.cart-fund-row:not(".cart-fund-empty")').each(function(i) {
      obj[i] = $(this).data();
    });
    var lineItems = JSON.stringify(obj).replace(/"/g, '&quot;');

    $('input[name$="[fund_catcher]"]').val(lineItems);
  };

  Drupal.fundraiserDesignations.prototype.validateOtherAmt = function(groupId) {
      var field = $('#fd-other-' + groupId);
      if (field[0]) {
        field.rules("add", {
          amount: true,
          min: parseFloat(Drupal.settings.fundraiserWebform.minimum_donation_amount),
          messages: {
            required: "This field is required",
            amount: "Enter a valid amount",
            min: "The amount entered is less than the minimum donation amount."
          }
        });
      }
  };

  Drupal.fundraiserDesignations.prototype.validateFund = function(fundGroupId, selector, defaultAmts, recurAmts, otherAmt) {
    var self = this;
    var fundSel = selector.val() != 0;
    if (defaultAmts.length > 0 && defaultAmts.is(':visible')) {
      var amt = $(defaultAmts).find('input:checked').val();
    }
    if (recurAmts.length > 0 && recurAmts.is(':visible')) {
      amt = $(recurAmts).find('input:checked').val();
    }
    if (otherAmt.val()) {
      amt = otherAmt.val();
    }
    var amtSel = typeof(amt) !== 'undefined';

    var message = 'ok';
    if (!fundSel &&!amtSel) {
      message = 'Please choose a fund and select an amount.';
    }
    else if(!fundSel) {
      message = 'Please choose a fund.';
    }
    else if(!amtSel) {
      message = 'Please select an amount'
    }

    if (amtSel) {
      var amtFloat = parseFloat(amt);
      if (Number.isNaN(amtFloat) || !$.isNumeric(amt)) {
        message += ' Not a valid amount.';
      }
      else {
        var MinAmt = parseFloat(Drupal.settings.fundraiserWebform.minimum_donation_amount);
        if (amtFloat < MinAmt) {
          message += ' Below the minumum amount.'
        }
      }
    }

    var errorContain = $("#designation-group-" + fundGroupId + " .designation-group-title");
    if (message != 'ok') {
      var error = $(self.errorTemplate).text(message);
        errorContain.find('.error-message').remove();
        errorContain.append(error)
    }
    else {
      errorContain.find('.error-message').remove();
    }

    return message;
  };

  /**
   * Set the amounts on select
   */
    // Iterate over each field in the settings and add listener
  Drupal.fundraiserDesignations.prototype.cancelButton = function() {
    var self = this;
    $.each($('tr', self.cart), function(key, row) {
      $('.fund-cancel', this).click(function() {
         $(row).hide(300, function(){
           $(row).remove();
           self.setAmounts();
           if ($('tr.cart-fund-row', self.cart).length == 1) {
             $(".cart-fund-empty").show(100);
           }
         });
      });
    });
  };

  /**
   * Update the various HTML elements with our amounts
   */
  Drupal.fundraiserDesignations.prototype.setAmounts = function() {
    var self = this,
    total = self.calcTotal();
    $('#cart_total').val(Drupal.settings.fundraiser.currency.symbol + (total).formatMoney(2, '.', ','));
    $("input[name='submitted[amount]']").val((total).formatMoney(2, '.', ''));
  };
  /**
   * Calculate the total amount
   */
  Drupal.fundraiserDesignations.prototype.calcTotal = function() {
    var self = this;
    var total = 0;
    $.each($('td.fund-amount', self.cart), function(i, price) {
      total = total + parseInt($(price).text().replace('$', ''));
    });
    total = total > 0 ? total : 0;
    return total;
  };

  Drupal.fundraiserDesignations.prototype.setWidths = function() {
    var selectContain = $('.designation-group-funds-table div.form-type-select');
    var selectContainWidth = 0;
    selectContain.each(function () {
      if ($(this).width() > selectContainWidth) {
        selectContainWidth = $(this).width();
      }
    });

    var fundContain = $('.designation-group-funds-table div[id*="funds-placeholder"]');
    var fundContainWidth = 0;
    fundContain.each(function () {
      if ($(this).width() > fundContainWidth) {
        fundContainWidth = $(this).width();
      }
    });

    var wide = fundContainWidth > selectContainWidth ? fundContainWidth : selectContainWidth;
    selectContain.css('width', wide);
    fundContain.css('width', wide);
  };
  /**
   * Format values as money
   *
   * http://stackoverflow.com/a/149099/1060438
   */
  Number.prototype.formatMoney = function(c, d, t) {
    var n = this, c = isNaN(c = Math.abs(c)) ? 2 : c, d = d == undefined ? "," : d, t = t == undefined ? "." : t, s = n < 0 ? "-" : "", i = parseInt(n = Math.abs(+n || 0).toFixed(c)) + "", j = (j = i.length) > 3 ? j % 3 : 0;
    return s + (j ? i.substr(0, j) + t : "") + i.substr(j).replace(/(\d{3})(?=\d)/g, "$1" + t) + (c ? d + Math.abs(n - i).toFixed(c).slice(2) : "");
  };

})(jQuery);
