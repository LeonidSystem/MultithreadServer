#include <csignal>
#include "threadpool.h"

#define PORT "80"
#define BACKLOG 128

using namespace std;

class Server {
    private:
        int sfd;
        struct addrinfo hints;
        string port;
        ThreadPool *threadPool;

    public:
        Server() {
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = AI_PASSIVE;
            port = PORT;
            threadPool = new ThreadPool();
        };

        Server(int ipv, string port) {
            if (ipv == 4)
                hints.ai_family = AF_INET;
            else if (ipv == 6)
                hints.ai_family = AF_INET6;
            else
                hints.ai_family = AF_UNSPEC;

            hints.ai_socktype = SOCK_STREAM;
            this->port = port;
        };

        void Start() {
            struct addrinfo *res, *p;
            int rv;

            if ((rv = getaddrinfo(NULL, port.c_str(), &hints, &res)) != 0) {
                printf("getaddrinfo: %s\n", gai_strerror(rv));
                exit(-1);
            }

            for (p = res; p != nullptr; p = p->ai_next) {
                if ((sfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1) {
                    perror("server: sfd");
                    continue;
                }

                int yes = 1;
                if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1) 
                    err_msg("setsockopt");
                
        
                if (bind(sfd, p->ai_addr, p->ai_addrlen)==-1) {
                    close(sfd);
                    perror("server: bind");
                    continue;
                }

                break;
            }

            if (p == nullptr) {
                printf("p: null pointer\n");
                exit(-1);
            }

            freeaddrinfo(res);

            if (listen(sfd, BACKLOG)==-1) 
                err_msg("listen");
              
        };

        void Stop() {
            if (close(sfd) == -1) 
                err_msg("close");
                
            exit(0);
        };

        void Service() {
            int sendfd;

            threadPool->Initialize();

            while (1) {
                if (signalFlag) {
                    threadPool->Join();
                    Stop();
                }  

                if ((sendfd = accept(sfd, NULL, 0)) == -1) {
                    if (errno == EINTR && signalFlag) {
                        threadPool->Join();
                        Stop();
                    }
                    else 
                        err_msg("accept");
                } 
                    
                cout << "Accept" << endl;
                threadPool->AddWork(sendfd);
                cout << "Add work" << endl;
            }
        };
};

void sighandler(int sig) {
    signalFlag = 1;
}

int main() {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sighandler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(-1);
    }

    Server *serv = new Server();

    cout << "Server start" << endl;
    serv->Start();
    cout << "Server handle requests" << endl;
    serv->Service();
}