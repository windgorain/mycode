from . import config
from . import manger

class C_TcpRleay(object):
    def __init__(self):
        self.config = config.C_Config("service.ini")
        self.manger = manger.C_Manger(self.config.GetServiceList())

    def GetServiceList(self):
        return self.config.GetServiceList()

    def Start(self):
        self.manger.Start()

if __name__ == "__main__":
    f = C_TcpRleay()
    print(f.GetServiceList())
    f.Start()
    




