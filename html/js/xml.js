
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

function GetLanguageUtf8Text(module, text, xmlDoc)
{
	var module_element = xmlDoc.firstChild.getElementsByTagName(module);
	module_element = module_element[0].getElementsByTagName(text)[0];
	return module_element.attributes.getNamedItem("txt").nodeValue;
}
