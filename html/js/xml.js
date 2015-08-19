
function xml_createFromFile(fileName)
{
	var xmlDoc;
	try { //Internet Explorer
		xmlDoc = new ActiveXObject("Microsoft.XMLDOM");
	} catch (e) {
		try { //Firefox, Mozilla, Opera, etc.
			xmlDoc = document.implementation.createDocument("", "", null);
		} catch (e) {
			alert(e.message)
		}
	}
	try {
		xmlDoc.async = false;
		xmlDoc.load(fileName);
		//document.write("xmlDoc is loaded, ready for use");
	} catch (e) {
		alert(e.message)
	}
	return xmlDoc;
}

function xml_createFromText(txt)
{
	var xmlDoc;
	try { //Internet Explorer
		xmlDoc = new ActiveXObject("Microsoft.XMLDOM");
		xmlDoc.async = false;
		xmlDoc.loadXML(text);
	} catch (e) {
		try { //Firefox, Mozilla, Opera, etc.
			var parser = new DOMParser();
			xmlDoc = parser.parseFromString(text, "text/xml");
		} catch (e) {
			alert(e.message)
		}
	}
	//document.write("xmlDoc is loaded, ready for use");
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

function GetLanguageUtf8Text(module, text, xmlDoc)
{
	var y = xmlDoc.firstChild.getElementsByTagName(module)[0];
	y = y.getElementsByTagName(text)[0];
	return y.getAttribute("txt");
}
