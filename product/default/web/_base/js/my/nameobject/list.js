/* 在jquery datatable的基础上进行的封装 */
function my_datatable_load(oJsonParam)
{
	if (oJsonParam.columnDefs)	{
		oJsonParam.columnDefs[oJsonParam.columnDefs.length] = {
			"targets":0,
			"sWidth":"10px",
			"className": 'dt-body-center',
			"orderable":false,
			"render": function(data,type,full,meta){ return '<input type="checkbox" name=selected_names[] value="'+ data.Name + '">';}
			};
	}
	else
	{
		oJsonParam.columnDefs = [{
			"targets":0,
			"sWidth":"10px",
			"className": 'dt-body-center',
			"orderable":false,
			"render": function(data,type,full,meta){return '<input type="checkbox" name=selected_names[] value="'+ data.Name + '">';}
			}];
	}
	
	return $('#' + oJsonParam.sTableID).DataTable( {
		"bDestroy": true,
		"bAutoWidth": false,
		"sAjaxSource": oJsonParam.sListUrl,
		"order": [[1,'asc']],
		"columns": oJsonParam.columns,
		"columnDefs": oJsonParam.columnDefs,
		"createdRow":function(nRow,oData,iDataIndex){
			if (oJsonParam.supportEdit)	{
				$('td',nRow).eq(1).html('<a href=' + oJsonParam.sEditUrl + '?name='+oData.Name+'>'+$('td',nRow).eq(1).html()+'</a>');
			}
			if (oJsonParam.createdRow) {
				oJsonParam.createdRow(nRow, oData, iDataIndex);
			}
			return nRow;
		}
	} );
}

function my_datatable_init(oJsonParam)
{
	if (!oJsonParam){oJsonParam = new object();}

	/* Table的ID */
	if (!oJsonParam.sTableID){oJsonParam.sTableID = "MyDftTableID";}
	/* 修改页面路径 */
	if (!oJsonParam.sEditUrl){oJsonParam.sEditUrl = "edit.htm";}
	/* 全选复选框的ID */
	if (!oJsonParam.sSelectAllID){oJsonParam.sSelectAllID = "MyDftSelectAllID";}
	/* columns定义 */
	if (!oJsonParam.columns){oJsonParam.columns = [{"data":null,"width":"10px"}, {"data":"Name"} ];}
	
	if (oJsonParam.supportEdit==undefined){oJsonParam.supportEdit = true;}

	return oJsonParam;
}

/* 对外公开函数 */
function MyDataTable_Init(oJsonParam)
{
	oJsonParam = my_datatable_init(oJsonParam);
	
	var oTable = my_datatable_load(oJsonParam);
	
	$("#" + oJsonParam.sSelectAllID).on('click', function(){
		var rows = oTable.rows({'search':'applied'}).nodes();
		$('input[type="checkbox"]', rows).prop('checked', this.checked);
	});
	
	// Handle click on checkbox to set state of "Select all" control
	$('#' + oJsonParam.sTableID + ' tbody').on('change', 'input[type="checkbox"]', function(){
	  // If checkbox is not checked
	  if(!this.checked){
	     var el = $('#' + oJsonParam.sSelectAllID).get(0);
	     // If "Select all" control is checked and has 'indeterminate' property
	     if(el && el.checked && ('indeterminate' in el)){
	        // Set visual state of "Select all" control 
	        // as 'indeterminate'
	        el.indeterminate = true;
	     }
	  }
	});
}

function MyDataTable_DelSelected(oJsonParam)
{
	oJsonParam = my_datatable_init(oJsonParam);
	
	var oTable = $('#' + oJsonParam.sTableID).DataTable();
	var sNames = "";

	oTable.$('input[type="checkbox"]').each(function(){
          if(this.checked){
             sNames += this.value + ",";
          }
      });
  
  $.ajax(
	{ type: "POST",
		url: oJsonParam.sDeleteUrl + "&Delete=" + sNames,
		async: false,
		dataType: "json",
		success: function(oJson) {
			  my_datatable_load(oJsonParam);
			  if (oJson.infoarray){
			  	if (oJsonParam.infoNotify){
			  		oJsonParam.infoNotify(oJson.infoarray);
			  	} else {
			  		var sInfos = "";
				  	for (x in oJson.infoarray)
						{
							sInfos += oJson.infoarray[x].info;
							sInfos += "\r\n";
						}
						window.alert(sInfos);
			  	}
			  }
		}
	});
}

