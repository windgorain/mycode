import socket
from . import service
from . import mypoll

class C_Manger(object):
    def __init__(self, serviceList):
        self.__serviceList = serviceList
        self.__selector = None
        self.__service = []

    def Start(self):
        mypoller = mypoll.MyPoll_GetDefault()

        for s in self.__serviceList:
            ser = service.C_Service(mypoller, s["LocalHost"], \
                                    s["LocalPort"], \
                                    s["RemoteHost"], \
                                    s["RemotePort"])
            ser.start()
            self.__service.append(ser)

        mypoller.run()

if __name__ == '__main__':
    manger = C_Manger([{"LocalHost":"127.0.0.1", "LocalPort":2300, "RemoteHost":"l03755b", "RemotePort":80}, \
                       {"LocalHost":"127.0.0.1", "LocalPort":2301, "RemoteHost":"l03755b", "RemotePort":80}])
    manger.Start()


