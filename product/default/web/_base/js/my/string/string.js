
String.prototype.replaceAll  = function(s1, s2)
{   
	return this.replace(new RegExp(s1, "gm") ,s2);
}

/* ɾ���հ��� */
String.prototype.removeBlankLine = function ( )
{
	/* ѹ���հ��ַ� */
	var sTmp = this.replace(/[ \t\f\v]{2,}/g, " ");

	/* \r\n ��Ϊ \n */
	sTmp = sTmp.replace(/(\r\n)+/g, "\n");

	/* ɾ��ÿ��ǰ��Ŀհ� */
	sTmp = sTmp.replace(/\n[ \t\f\v]+/g, "\n");
	sTmp = sTmp.replace(/[ \t\f\v]+\n/g, "\n");
	
	/* ɾ���հ��� */
	sTmp = sTmp.replace(/\n{2,}/g, "\n");
	sTmp = sTmp.replace(/^\n/g, "");
	sTmp = sTmp.replace(/\n$/g, "");

	return sTmp;
}

String.prototype.splitByChar=function(cChar)
{
	return this.split(cChar);
}

String.prototype.byteLength=function()
{
    return this.replace(/[^\x00-\xff]/g,"**").length;
}

String.prototype.paddingString=function(len,pad)
{
    var tmp;
    var i;
    
    tmp = "";
    for (i=this.byteLength(); i<len; i++)
        tmp+=pad;
    return this.toString() + tmp;
}

String.prototype.trim = function ( )
{
    var str = this;
    str = str.replace(/^\s*/, "");
    str = str.replace(/\s*$/, "");
    return str;
}

String.prototype.isNumber = function ( )
{
    var i;

    for(i = 0; i < this.length; i++)
    {
        var c = this.charCodeAt(i);
        if(c < 48 || c > 57)
        {
            return false;
        }
    }
    return i>0;
}

// innerHTML -> innerText
String.prototype.toInnerText = function ( )
{
return this.replace(/&lt;/g,"<")
    .replace(/&gt;/g,">")
    .replace(/&nbsp;/g," ")
    .replace(/&amp;/g,"&");
}


//convert a unitcode string to native string
String.prototype.parseUnicode = function ()
{
    var index,res,start,end,str;
    index = 0;
    res = "";
    str = this;
    while (index < str.length)
    {
        start = str.indexOf("&#", index);
        if (start == -1)
        {
            res += str.substr(index);
            return res;
        }
        end = str.indexOf(";", start);
        if (end == -1)
        {
            res += str.substr(index);
            return res;
        }
        res += str.substring(index, start);
        unicode = str.substring(start+2, end);
        if (unicode.isNumber())
        {
            //It's unicode.
            res += String.fromCharCode(unicode);
        }
        else
        {
            //It's a normal string.
            res += str.substring(start, end+1);
        }
        index = end + 1;
    }
    // process end with &#12345;
    return res;
}

String.prototype.parseCharEntity = function ()
{
    var oPatten = /([\u4e00-\u9fa5])/;
    var res = this,cTemp;
    while(oPatten.test(res) == true)
    {
        res = res.replace(oPatten,"&#[$1];");
        cTemp = res.replace(/.*&#\[(.*)\];.*/,"$1");
        res = res.replace(/&#\[.*\];/,"&#" + cTemp.charCodeAt(0) + ";");
    }
    return res;
}
