import selectors
import socket

class C_MyPoll():
    def __init__(self):
        self.__selector = selectors.DefaultSelector()

    def run(self):
        while True:
            events = self.__selector.select()
            for key, event in events:
                callback = key.data
                if callback(key.fileobj, event):
                    break

    def setEvent(self, sock, event, pfCallBack):
        try:
            key = self.__selector.get_key(sock)
        except KeyError as e:
            self.__selector.register(sock, event, pfCallBack)
            return
        self.__selector.modify(sock, event, pfCallBack)

    def addEvent(self, sock, event, pfCallBack):
        try:
            key = self.__selector.get_key(sock)
        except KeyError as e:
            self.__selector.register(sock, event, pfCallBack)
            return
        newEvent = event | key.events
        self.__selector.modify(sock, newEvent, pfCallBack)

    def delEvent(self, sock, event, pfCallBack):
        try:
            key = self.__selector.get_key(sock)
        except Exception as e:
            return
        newEvent = ~event & key.events
        if newEvent != 0:
            self.__selector.modify(sock, newEvent, pfCallBack)
        else:
            self.__selector.unregister(sock)

    def clearEvent(self, sock):
        try:
            self.__selector.unregister(sock)
        except Exception as e:
            return



default_poller = C_MyPoll()

def MyPoll_GetDefault():
    global default_poller
    return default_poller
