import socket
import threading

msgFromClient       = "Hello, server!"
bytesToSend         = str.encode(msgFromClient)
bufferSize          = 1024

def thread_func(event_for_wait, event_for_set):
    for number in range(200):

        TCPClientSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_STREAM)

        TCPClientSocket.connect(('127.0.0.1', 80))

        TCPClientSocket.send(bytesToSend)

        msgFromServer = TCPClientSocket.recv(bufferSize)

        msg = "Message from Server: {}".format(msgFromServer)

        print(msg)

e1 = threading.Event()
e2 = threading.Event()

t1 = threading.Thread(target=thread_func, args=(e1, e2))
t2 = threading.Thread(target=thread_func, args=(e1, e2))

t1.start()
t2.start()

e1.set()

t1.join()
t2.join()