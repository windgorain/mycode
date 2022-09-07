
document.write('<link rel="stylesheet" type="text/css" href="/_base/js/my/sidebar/sidebar.css">');

function SideBar_Create()
{
	var obj = new Object();

	var treeBuilder = TreeBuilder_Create();

	obj.treeBuilder = treeBuilder;

	obj.AddItem = function (sMPath, sAction)
	{
		this.treeBuilder.AddItem(sMPath, sAction);
	}

	obj.AddItemFinish = function ()
	{
		this.treeBuilder.AddClassToULOnPath("/", "SideBar");
	}

	obj.HideAll = function ()
	{
		this.treeBuilder.AddStyleToAllUL("display:none;");
	}

	obj.Expand = function (sMPath)
	{
		this.treeBuilder.RemoveStyleFromULOnPath(sMPath, "display:none;");
	}

	obj.ExpandAll = function ()
	{
		this.treeBuilder.RemoveStyleFromAllUL("display:none;");
	}

	obj.Select = function (sMPath)
	{
		this.treeBuilder.AddClassToDivByPath(sMPath, "Selected");
	}

	obj.GetHtml = function ()
	{
		return this.treeBuilder.GetHtml();
	}
	
	obj.Write = function ()
	{
		this.treeBuilder.Write();
	}

	return obj;
}

