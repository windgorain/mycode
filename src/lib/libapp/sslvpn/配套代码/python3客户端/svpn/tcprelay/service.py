import selectors
import socket
import time
import logging

class C_Service(object):
    def __init__(self, mypoller, sLocalName, iLocalPort, sRemoteHost, iRemotePort):
        self.__sLocalName = sLocalName
        self.__iLocalPort = iLocalPort
        self.__sRemoteHost = sRemoteHost
        self.__iRemotePort = iRemotePort
        self.__mypoller = mypoller
        self.__iListenSocket = 0
        self.__fwdMap = {}
        self.__fwdCache = {}
        self.__sHskData = {}
        self.__needReSel = False
        self.__sockClosed = False

    def __addEvent(self, sock, event):
        self.__mypoller.addEvent(sock, event, self.__sockEvent)

    def __delEvent(self, sock, event):
        self.__mypoller.delEvent(sock, event, self.__sockEvent)

    def __fwdStop(self, sock):
        logging.info('closing %s' % sock)
        peer_sock = self.__fwdMap[sock]
        self.__fwdMap[sock] = None
        self.__mypoller.clearEvent(sock)
        sock.close()
        self.__sockClosed = True
        if peer_sock:
            self.__fwdMap[peer_sock] = None
            if self.__fwdCache[peer_sock]:
                self.__delEvent(peer_sock, selectors.EVENT_READ)
            else:
                self.__needReSel = True
                self.__mypoller.clearEvent(peer_sock)
                peer_sock.close()

    def __fwdSendData(self, sock, data):
        if self.__fwdCache[sock]:
            logging.info("Error")

        try:
            sendlen = sock.send(data)
        except BlockingIOError as e:
            sendlen = 0
        except Exception as e:
            self.__fwdStop(sock)
            return

        dataLen = len(data)
        if sendlen == dataLen:
            return

        reservedData = data[sendlen:]
        self.__fwdCache[sock] = reservedData
        self.__addEvent(sock, selectors.EVENT_WRITE)
        peer_sock = self.__fwdMap[sock]
        if peer_sock:
            self.__delEvent(peer_sock, selectors.EVENT_READ)


    def __sockReadEvent(self, sock):
        peer_sock = self.__fwdMap[sock]
        if not peer_sock:
            return

        if self.__fwdCache[peer_sock]:
            self.__delEvent(sock, selectors.EVENT_READ)
            return

        try:
            data = sock.recv(1000)
            if data:
                self.__fwdSendData(self.__fwdMap[sock], data)
            else:
                self.__fwdStop(sock)
        except Exception as e:
            self.__fwdStop(sock)

    def __sockWriteEvent(self, sock):
        data = self.__fwdCache[sock]
        self.__fwdCache[sock] = None
        self.__delEvent(sock, selectors.EVENT_WRITE)
        self.__fwdSendData(sock, data)

        peer_sock = self.__fwdMap[sock]

        if (peer_sock == None) and (self.__fwdCache[sock] == None):
            self.__fwdStop(sock)
        if (peer_sock) and (self.__fwdCache[sock] == None):
            self.__addEvent(peer_sock, selectors.EVENT_READ)

    def __sockEvent(self, sock, event):
        self.__sockClosed = False
        self.__needReSel = False
        if event & selectors.EVENT_READ:
            self.__sockReadEvent(sock)

        if self.__sockClosed:
            return True

        if event & selectors.EVENT_WRITE:
            self.__sockWriteEvent(sock)

        return self.__needReSel

    def __serverHskReply(self, sock, event):
        try:
            data = sock.recv(200)
        except Exception as e:
            self.__fwdStop(sock)
            return

        print(data)

        if (self.__sHskData[sock]):
            self.__sHskData[sock] += data
            data = self.__sHskData[sock]

        if (data.find(b"\r\n\r\n") == -1):
            self.__sHskData[sock] = data
            return

        if (data.find(b"200 OK") == -1):
            self.__fwdStop(sock)
            return

        self.__mypoller.setEvent(self.__fwdMap[sock], selectors.EVENT_READ, self.__sockEvent)
        self.__mypoller.setEvent(sock, selectors.EVENT_READ, self.__sockEvent)

    def __serverHskStart(self, sock, event):
        sRequest = "TCP_RELAY / HTTP/1.1\r\nserver: %s:%d\r\n\r\n" % (self.__sRemoteHost, self.__iRemotePort)
        try:
            data = sock.send(sRequest.encode('ascii'))
        except Exception as e:
            self.__fwdStop(sock)
            return

        self.__mypoller.setEvent(sock, selectors.EVENT_READ, self.__serverHskReply)

    def __conn2server(self):
        sock = socket.socket()
        sock.setblocking(False)
        try:
            sock.connect(('202.118.1.1', 80))
        except Exception as e:
            pass

        self.__mypoller.setEvent(sock, selectors.EVENT_WRITE, self.__serverHskStart)
        return sock

    def __accept(self, sock, event):
        conn, addr = sock.accept()
        logging.info('accepted %s from %s' % (conn, addr))
        server_sock = self.__conn2server()
        self.__fwdMap[server_sock] = conn
        self.__fwdMap[conn] = server_sock
        self.__sHskData[server_sock] = None
        self.__sHskData[conn] = None
        self.__fwdCache[server_sock] = None
        self.__fwdCache[conn] = None

    def start(self):
        self.__iListenSocket = socket.socket()
        sock = self.__iListenSocket
        sock.bind((self.__sLocalName, self.__iLocalPort))
        sock.listen(5)
        sock.setblocking(0)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.__mypoller.addEvent(sock, selectors.EVENT_READ, self.__accept)

if __name__ == '__main__':
    sel = selectors.DefaultSelector()

    service = C_Service(sel, "127.0.0.1", 2300, "tech", 80)
    service.start()

    while True:
        events = sel.select()
        for key, event in events:
            callback = key.data
            if callback(key.fileobj, event):
                break

