
var msxmlhttp = new Array(
	"MSXML2.DOMDocument.6.0",
	"MSXML2.DOMDocument.5.0",
	"MSXML2.DOMDocument.4.0",
	"MSXML2.DOMDocument.3.0",
	"MSXML2.DOMDocument",
	"MSXML.DOMDocument",
	"Microsoft.XMLDOM",
	"Msxml2.XMLHTTP.5.0",
	"Msxml2.XMLHTTP.4.0",
	"Msxml2.XMLHTTP.3.0",
	"Msxml2.XMLHTTP",
	"Microsoft.XMLHTTP");

function xml_createFromFile(fileName)
{
	var xmlDoc = null;
	for (var i = 0; i < msxmlhttp.length; i++) { 
		try { 
			xmlDoc = new ActiveXObject(msxmlhttp[i]);
			xmlDoc.async = false;
			var ret = xmlDoc.load(fileName);
			if (!ret)
				xmlDoc = null;
			return xmlDoc;
		} catch (e) {
			//alert(msxmlhttp[i] + ": " + e.message)
			xmlDoc = null;
		}
	}
	try { //Firefox, Mozilla, Opera, etc.
		xmlDoc = document.implementation.createDocument("", "", null);
		xmlDoc.async = false;
		var ret = xmlDoc.load(fileName);
		if (!ret)
			xmlDoc = null;
	} catch (e) {
		//alert(e.message)
		xmlDoc = null;
	}
	return xmlDoc;
}

function xml_createFromText(text)
{
	var xmlDoc = null;
	for (var i = 0; i < msxmlhttp.length; i++) { 
		try { 
			xmlDoc = new ActiveXObject(msxmlhttp[i]);
			xmlDoc.async = false;
			var ret = xmlDoc.loadXML(text);
			if (!ret)
				xmlDoc = null;
			return xmlDoc;
		} catch (e) {
			//alert(msxmlhttp[i] + ": " + e.message)
			xmlDoc = null;
		}
	}
	
	try { //Firefox, Mozilla, Opera, etc.
		var parser = new DOMParser();
		xmlDoc = parser.parseFromString(text, "text/xml");
	} catch (e) {
		//alert(e.message)
		xmlDoc = null;
	}
	return xmlDoc;
}

function xml_firstChild(x)
{
	var y = x.firstChild;
	while (y.nodeType != 1) {
		y = y.nextSibling;
	}
	return y;
}

function xml_nextSibling(x)
{
	var y = x.nextSibling;
	while (y.nodeType != 1) {
		y = y.nextSibling;
	}
	return y;
}

function GetLanguageText(module, text, xmlDoc)
{
	var y = xmlDoc.getElementsByTagName(module)[0];
	y = y.getElementsByTagName(text)[0];
	return y.getAttribute("txt");
}

function GetLanguageErrorText(errno, xmlDoc)
{
	var y = xmlDoc.getElementsByTagName("ERROR")[0];
	y = y.getElementsByTagName("ERR_"+-errno)[0];
	return y.getAttribute("txt");
}
