
var g_ulFmtTblCount = 0;
var g_aFmtTblArray = new Array();

function FmtTbl_Create(szTitle, aArrayHeader)
{
    var obj = new Object();
    
    obj.ulFmtTblCount = g_ulFmtTblCount;
    g_aFmtTblArray[g_ulFmtTblCount] = obj;
    g_ulFmtTblCount = g_ulFmtTblCount + 1;
    obj.selectAll_code = "";
    obj.selectAll_tip = "Select All";
    obj.bSelectAll = false;
    obj.aNodes = new Array();
    obj.tbl = TBL_Create(szTitle, aArrayHeader);
        
    obj.AddNode = function (aArrayNode, bHightlight)
    {
        var i = 0;
        var aFmtArray = new Array();
        var szKey = aArrayNode[0];
        var szHightLightA = "";
        var szHightLightB = "";
        
        this.aNodes[this.aNodes.length] = szKey;

        if (arguments.length == 1)
        {
            bHightLight = false;
        }
        
        if (bHightlight == true)
        {
            szHightLightA = "<font class = hightlight_font_class>";
            szHightLightB = "</font>";
        }
        
        aFmtArray[0] = "<input type=\"checkbox\" name=\"check[" + szKey + "]\" id=\""
                         + szKey +"\" value=\"" + szKey +"\"><label for=\"" + szKey + "\">";
        aFmtArray[0] += szHightLightA + szKey + szHightLightB + "</label>";
        
        for (i=1; i<aArrayNode.length; i++)
        {
            aFmtArray[i] = aArrayNode[i];
        }
        
        this.tbl.AddNode(aFmtArray);
        
        this.selectAll_code += "\tthis.document.getElementById(\"" + szKey + "\").checked = true;\n";
    }
    
    obj.selectAll = function ()
    {
        if (this.bSelectAll == false)
        {
            this.bSelectAll = true;
        }
        else
        {
            this.bSelectAll = false;
        }
        
        for (var i=0; i<this.aNodes.length; i++)
        {
            document.getElementById(this.aNodes[i]).checked = this.bSelectAll;
        }
    }
    
    obj.setSelectAllTip = function (szTip)
    {
        this.selectAll_tip = szTip;
    }
    
    obj.ShowTbl = function ()
    {
        this.tbl.ShowTbl();
        
        var szCheckAll = "<br><input type=\"checkbox\" name=\"fmttbl_checkAll\" onClick=\"javascript:g_aFmtTblArray[";
        szCheckAll += this.ulFmtTblCount;
        szCheckAll += "].selectAll()\" id=fmttbl_checkAll" + this.ulFmtTblCount + ">";
        szCheckAll += "<label for=fmttbl_checkAll" + this.ulFmtTblCount + ">" + this.selectAll_tip + "</label>";

        document.write(szCheckAll);
    }

    return obj;
}
