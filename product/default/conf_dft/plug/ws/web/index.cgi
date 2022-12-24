<html>

<head>
</head>

<body>
	<span id="domain_list"></span>

	<script language="javascript">
		var domainList = <?cgi domain.list ?>;
		var sUrl = <?cgi domain.jump2url ?>;
		
		for (var i=0; i<domainList.length; i++)
		{
			var sHtml = '<li><a href="/?domain=' + domainList[i].Name + '&url=' + sUrl + '">' + domainList[i].Name + '</a></li>';
			document.getElementById("domain_list").innerHTML += sHtml;
		}
	</script>

	<br>
</body>
	
</html>


