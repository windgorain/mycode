伪造证书服务

提供的Unix和TCP服务，接收JSON格式请求，并生成证书，返回证书名
请求格式: {"hostname":"xxx","port":xxx,"ip":"xxxx"}
其中port和ip是可选的

fakecert服务会根据请求，向服务器发送ssl请求并获取服务器真实证书,并根据真实证书伪造证书

