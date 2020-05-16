import sys,os
import configparser
from . import manger

class C_Config:
    def __init__(self, sConfigFilePath):
        self.__sConfigFilePath = sConfigFilePath
        self.serviceList = self.__serviceList()

    def __serviceList(self):
        cf = configparser.ConfigParser()
        cf.read(self.__sConfigFilePath)

        s = cf.sections()

        serverList = []
        for sec in s:
            sLocalHost = cf.get(sec, "local-host")
            iLocalPort = cf.getint(sec, "local-port")
            sRemoteHost = cf.get(sec, "remote-host")
            iRemotePort = cf.getint(sec, "remote-port")
            try:
                sDesc = cf.get(sec, "desc")
            except Exception as e:
                sDesc = ""
            serverList.append({"LocalHost": sLocalHost, \
                               "LocalPort":iLocalPort, \
                               "RemoteHost":sRemoteHost, \
                               "RemotePort":iRemotePort, \
                               "Desc":sDesc})
        return serverList

    def GetServiceList(self):
        return self.serviceList

if __name__ == "__main__":
    f = C_Config("service.ini")
    manger = manger.C_Manger(f.GetServiceList())
    manger.start()
    


