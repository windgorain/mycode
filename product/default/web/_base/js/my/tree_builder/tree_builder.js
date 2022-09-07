
function TreeBuilder_Create()
{
	var obj = new Object();

	obj.bIsDir = false;
	obj.sAction = null;
	obj.aNodes = new Array();

	obj.AddItem = function (sMPath, sAction)
	{
		/* 如果是以'/'开始,则去掉最开始的'/' */
		if (sMPath.indexOf("/") == 0)
		{
			sMPath = sMPath.substring(1);
		}

		var n = sMPath.indexOf('/');
	    var bIsLeaf = (-1 == n);

	    if(true == bIsLeaf)
	    {
	        var sText = sMPath;
			var oNode = this._treebuilder_GetNode(sText);
			oNode.sAction = sAction;
	    }
		else
		{
			var sText = sMPath.substring(0,n);
			var oNode = this._treebuilder_GetNode(sText);
			oNode.AddItem(sMPath.substring(n+1), sAction);
		}

		this.bIsDir = true;

		return;
	}

	obj.AddClassToAllUL = function (sClass)
	{
		_treebuilder_WalkAllUL(this, _treebuilder_AddULClass, sClass);
	}

	obj.RemoveClassFromAllUL = function (sClass)
	{
		_treebuilder_WalkAllUL(this, _treebuilder_RemoveULClass, sClass);
	}

	obj.AddClassToAllLI = function(sClass)
	{
		_treebuilder_WalkAllLI(this, _treebuilder_AddLIClass, sClass);
	}

	obj.RemoveClassFromAllLI = function(sClass)
	{
		_treebuilder_WalkAllLI(this, _treebuilder_RemoveLIClass, sClass);
	}

	obj.AddStyleToAllUL = function (sStyle)
	{
		_treebuilder_WalkAllUL(this, _treebuilder_AddULStyle, sStyle);
	}

	obj.RemoveStyleFromAllUL = function (sStyle)
	{
		_treebuilder_WalkAllUL(this, _treebuilder_RemoveULStyle, sStyle);
	}

	obj.AddStyleToAllLI = function(sStyle)
	{
		_treebuilder_WalkAllLI(this, _treebuilder_AddLIStyle, style);
	}

	obj.RemoveStyleFromAllLI = function(sStyle)
	{
		_treebuilder_WalkAllLI(this, _treebuilder_RemoveLIStyle, style);
	}

	/* 添加指定路径上的所有UL的Class */
	obj.AddClassToULOnPath = function (sMPath, sClass)
	{
		_treebuilder_WalkULOnPath(this, sMPath, _treebuilder_AddULClass, sClass);
	}

	obj.RemoveClassFromULOnPath = function (sMPath, sClass)
	{
		_treebuilder_WalkULOnPath(this, sMPath, _treebuilder_RemoveULClass, sClass);
	}

	obj.AddClassToLIOnPath = function (sMPath, sClass)
	{
		_treebuilder_WalkLIOnPath(this, sMPath, _treebuilder_AddLIClass, sClass);
	}

	obj.RemoveClassFromLIOnPath = function (sMPath, sClass)
	{
		_treebuilder_WalkLIOnPath(this, sMPath, _treebuilder_RemoveLIClass, sClass);
	}

	obj.AddStyleToULOnPath = function (sMPath, sTyle)
	{
		_treebuilder_WalkULOnPath(this, sMPath, _treebuilder_AddULStyle, sTyle);
	}

	obj.RemoveStyleFromULOnPath = function (sMPath, sTyle)
	{
		_treebuilder_WalkULOnPath(this, sMPath, _treebuilder_RemoveULStyle, sTyle);
	}

	obj.AddStyleToLIOnPath = function (sMPath, sTyle)
	{
		_treebuilder_WalkLIOnPath(this, sMPath, _treebuilder_AddLIStyle, sTyle);
	}

	obj.RemoveStyleFromLIOnPath = function (sMPath, sTyle)
	{
		_treebuilder_WalkLIOnPath(this, sMPath, _treebuilder_RemoveLIStyle, sTyle);
	}

	/* 为路径上所有节点添加Class */
	obj.AddClassToDivOnPath = function (sMPath, sClass)
	{
		_treebuilder_WalkLIOnPath(this, sMPath, _treebuilder_AddDivClass, sClass);
	}

	obj.RemoveClassFromDivOnPath = function (sMPath, sClass)
	{
		_treebuilder_WalkLIOnPath(this, sMPath, _treebuilder_RemoveDivClass, sClass);
	}

	/* 为路径上最后一个节点添加Class */
	obj.AddClassToDivByPath = function (sMPath, sClass)
	{
		var oNode = _treebuilder_GetNodeByPath(this, sMPath);
		if (oNode)
		{
			_treebuilder_AddDivClass(oNode, sClass);
		}
	}

	obj.RemoveClassFromDivByPath = function (sMPath, sClass)
	{
		var oNode = _treebuilder_GetNodeByPath(this, sMPath);
		if (oNode)
		{
			_treebuilder_RemoveDivClass(oNode, sClass);
		}
	}

	/* 获取Html代码 */
	obj.GetHtml = function ()
	{
		var sMsg = "<ul";


		sMsg += _treebuilder_BuildClass(this.ul_class);
		sMsg += _treebuilder_BuildStyle(this.ul_style);
		sMsg += ">";

		for(var i in this.aNodes)
		{
			sMsg += this.aNodes[i]._treebuilder_BuildLi(i);
		}
		sMsg += "</ul>";

		return sMsg;
	}

	obj.Write = function ()
	{
		document.write(this.GetHtml());
	}


	/* private函数,不被外面调用 */
	obj._treebuilder_GetNode = function (sText)
	{
		var oNode = this.aNodes[sText];
		if (!oNode)
		{
			oNode = TreeBuilder_Create();
			this.aNodes[sText] = oNode;
		}

		return oNode;
	}

	obj._treebuilder_BuildActionJs = function ()
	{
		sAction = this.sAction;

		if (! sAction)
		{
			return "";
		}

		/* 解析是否js */
		if (sAction.indexOf("js::") != 0)
		{
			return "";
		}

		return "";
	}

	obj._treebuilder_BuildActionUrl = function (sText)
	{
		var sAction = this.sAction;

		if (! sAction)
		{
			return sText;
		}

		/* 解析是否js */
		if (sAction.indexOf("url::") != 0)
		{
			return sText;
		}

		return "<a href='" + sAction.substring(5) +"'>" + sText + "</a>";
	}

	obj._treebuilder_BuildLi = function (sName)
	{
		var sDirHeader = "<span class='TreeBuilderDirFlag'>+</span>";
		var sLeafHeader = "<span class='TreeBuilderLeafFlag'>-</span>";
		var sHeader = "";
		var oNode = this;

		var sMsg = "<li";

		var sLiClass = "";
		var sDivClass = "";

		if (oNode.bIsDir)
		{
			sLiClass = "TreeBuilderDir ";
			sHeader = sDirHeader;
		}
		else
		{
			sLiClass = "TreeBuilderLeaf ";
			sHeader = sLeafHeader;
		}

		sLiClass = _treebuilder_AddString(oNode.li_class, sLiClass);

		sMsg += _treebuilder_BuildClass(sLiClass);
		sMsg += _treebuilder_BuildStyle(oNode.li_style);
		sMsg += ">";

		var sDiv = "<div" + _treebuilder_BuildClass(oNode.div_class) + ">";
		sDiv += sHeader + sName;
		sDiv += "</div>";

		sMsg += oNode._treebuilder_BuildActionUrl(sDiv);

		if (oNode.bIsDir)
		{
			sMsg += oNode.GetHtml();
		}
		sMsg += "</li>";

		return sMsg;
	}

	function _treebuilder_AddULClass(oNode, param)
	{
		oNode.ul_class = _treebuilder_AddString(oNode.ul_class, param);
	}

	function _treebuilder_RemoveULClass(oNode, param)
	{
		oNode.ul_class = _treebuilder_RemoveString(oNode.ul_class, param);
	}

	function _treebuilder_AddLIClass(oNode, param)
	{
		oNode.li_class = _treebuilder_AddString(oNode.li_class, param);
	}

	function _treebuilder_RemoveLIClass(oNode, param)
	{
		oNode.li_class = _treebuilder_RemoveString(oNode.li_class, param);
	}

	function _treebuilder_AddULStyle(oNode, param)
	{
		oNode.ul_style = _treebuilder_AddString(oNode.ul_style, param);
	}

	function _treebuilder_RemoveULStyle(oNode, param)
	{
		oNode.ul_style = _treebuilder_RemoveString(oNode.ul_style, param);
	}

	function _treebuilder_AddLIStyle(oNode, param)
	{
		oNode.li_style = _treebuilder_AddString(oNode.li_style, param);
	}

	function _treebuilder_RemoveLIStyle(oNode, param)
	{
		oNode.li_style = _treebuilder_RemoveString(oNode.li_style, param);
	}

	function _treebuilder_AddDivClass(oNode, param)
	{
		oNode.div_class = _treebuilder_AddString(oNode.div_class, param);
	}

	function _treebuilder_RemoveDivClass(oNode, param)
	{
		oNode.div_class = _treebuilder_RemoveString(oNode.div_class, param);
	}

	function __treebuilder_UlCall(oNode, obj)
	{
		if (oNode.bIsDir)
		{
			obj.pfFunc(oNode, obj.param);
		}
	}

	function _treebuilder_WalkAllUL (oTreeBuilder, pfFunc, param)
	{
		var obj = new Object();

		obj.pfFunc = pfFunc;
		obj.param = param;

		_treebuilder_WalkAllLI(oTreeBuilder, __treebuilder_UlCall, obj);
	}

	function _treebuilder_WalkAllLI (oTreeBuilder, pfFunc, param)
	{
		pfFunc(oTreeBuilder, param);

		for(var i in oTreeBuilder.aNodes)
		{
			_treebuilder_WalkAllLI(oTreeBuilder.aNodes[i], pfFunc, param);
		}
	}

	function _treebuilder_WalkULOnPath(oTreeBuilder, sMPath, pfFunc, param)
	{
		var obj = new Object();

		obj.pfFunc = pfFunc;
		obj.param = param;

		_treebuilder_WalkLIOnPath(oTreeBuilder, sMPath, __treebuilder_UlCall, obj);
	}

	function _treebuilder_WalkLIOnPath(oTreeBuilder, sMPath, pfFunc, param)
	{
		pfFunc(oTreeBuilder, param);

		/* 如果是以'/'开始,则去掉最开始的'/' */
		if (sMPath.indexOf("/") == 0)
		{
			sMPath = sMPath.substring(1);
		}

		if (sMPath.trim() == "")
		{
			return;
		}

		var n = sMPath.indexOf('/');
	    var bIsLast = (-1 == n);

	    if(true == bIsLast)
	    {
	        var sText = sMPath;
	    }
		else
		{
			var sText = sMPath.substring(0,n);
		}

		if (! oTreeBuilder.aNodes[sText])
		{
			return;
		}

		_treebuilder_WalkLIOnPath(oTreeBuilder.aNodes[sText], sMPath.substring(n+1), pfFunc, param);
	}

	function _treebuilder_GetNodeByPath(oTreeBuilder, sMPath)
	{
		/* 如果是以'/'开始,则去掉最开始的'/' */
		if (sMPath.indexOf("/") == 0)
		{
			sMPath = sMPath.substring(1);
		}

		if (sMPath.trim() == "")
		{
			return null;
		}

		var n = sMPath.indexOf('/');
	    var bIsLast = (-1 == n);

	    if(true == bIsLast)
	    {
	        var sText = sMPath;
	    }
		else
		{
			var sText = sMPath.substring(0,n);
		}

		if (! oTreeBuilder.aNodes[sText])
		{
			return null;
		}

		if (true == bIsLast)
		{
			return oTreeBuilder.aNodes[sText];
		}

		return _treebuilder_GetNodeByPath(oTreeBuilder.aNodes[sText], sMPath.substring(n+1));
	}

	function _treebuilder_AddString (sDst, sSrc)
	{
		if (! sDst)
		{
			sDst = "";
		}

		if (! sSrc)
		{
			sSrc = "";
		}

		sDst += " " + sSrc;

		return sDst.trim();
	}

	function _treebuilder_RemoveString (sDst, sSrc)
	{
		if (! sDst)
		{
			sDst = "";
		}

		if ((! sSrc) || (sSrc == ""))
		{
			return sDst.trim();
		}

		sDst = sDst.replace(sSrc, "");

		return sDst.trim();
	}

	function _treebuilder_BuildClass(sClass)
	{
		if ((sClass) && (sClass != ""))
		{
			return " class='" + sClass + "'";
		}

		return "";
	}

	function _treebuilder_BuildStyle(sTyle)
	{
		if ((sTyle) && (sTyle != ""))
		{
			return " style='" + sTyle + "'";
		}

		return "";
	}

	return obj;
}

