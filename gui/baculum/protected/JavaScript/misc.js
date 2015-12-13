var Units = {
	get_decimal_size: function(size) {
		size = parseInt(size, 10);
		var size_unit = 'B';
		var units = ['K', 'M', 'G', 'T', 'P'];
		var unit;
		var dec_size = size.toString() + ((size > 0 ) ? size_unit : '');
		while(size >= 1000) {
			size /= 1000;
			unit = units.shift(units);
		}
		if (unit) {
			dec_size = (Math.floor(size * 10) / 10).toFixed(1);
			dec_size += unit + size_unit;
		}
		return dec_size;
	}
}

var Formatters = {
	formatter: [
		{css_class: 'size', format_func: Units.get_decimal_size}
	],
	set_formatters: function() {
		var elements, formatter, txt;
		for (var i = 0; i < this.formatter.length; i++) {
			elements = document.getElementsByClassName(this.formatter[i].css_class);
			formatter = this.formatter[i].format_func;
			for (var i = 0; i < elements.length; i++) {
				txt = elements[i].firstChild;
				if (txt.nodeType === 3) {
					txt.nodeValue = formatter(txt.nodeValue);
				}
			}
		}
	}
}

var Cookies = {
	default_exipration_time: 31536000000, // 1 year in miliseconds
	set_cookie: function(name, value, expiration) {
		var date = new Date();
		date.setTime(date.getTime() + this.default_exipration_time);
		var expires = 'expires=' + date.toUTCString();
		document.cookie = name + '=' + value + '; ' + expires;
	},
	get_cookie: function(name) {
		name += '=';
		var values = document.cookie.split(';');
		var cookie_val = null;
		var value;
		for (var i = 0; i < values.length; i++) {
			value = values[i];
			while (value.charAt(0) == ' ') {
				value = value.substr(1);
			}
			if (value.indexOf(name) == 0) {
				cookie_val = value.substring(name.length, value.length);
				break;
			}
		}
		return cookie_val;
	}
}
