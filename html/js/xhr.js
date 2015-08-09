
function xhr_create(callback)
{
	var xhr;
	if (window.XMLHttpRequest) {
		// code for IE7+, Firefox, Chrome, Opera, Safari
		xhr = new XMLHttpRequest();
	} else if (window.ActiveXObject) {
		// code for IE6, IE5
		xhr = new ActiveXObject("Microsoft.XMLHTTP");
	} else {
		return xhr;
	}
	
	xhr.onreadystatechange = function() {
		//alert("ready = "+this.readyState+", status = "+this.status);
		if (this.readyState == 4)
			callback(this, this.status);
	};
	return xhr;
}
