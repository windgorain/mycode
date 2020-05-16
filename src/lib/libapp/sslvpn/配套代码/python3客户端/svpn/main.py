from tcprelay import tcprelay

f = tcprelay.C_TcpRleay()
for s in f.GetServiceList():
    print("LoalHost:%s, LocalPort:%d, RemoteHost:%s, RemotePort:%d, Desc:%s" \
          % (s["LocalHost"], s["LocalPort"], s["RemoteHost"], \
             s["RemotePort"], s["Desc"]))
f.Start()



