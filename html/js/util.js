
function id(x)
{
	if (typeof x == "string")
		return document.getElementById(x);
	return x;
}
