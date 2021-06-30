#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

#define BUFSIZE 1024

using namespace std;

enum Status {
    STATUS_FREE = 0,
    STATUS_BUSY
};

volatile sig_atomic_t signalFlag = 0;

queue<int> fds;     //###queue which store file descriptors of clients
mutex fds_mtx;      //###mutex for access to queue

void err_msg(string msg) {
    perror(msg.c_str());
    exit(-1);
};

//###the structure contains variables for notifying the thread about the appearance of a new work
struct StatusThread {
    bool status;
    condition_variable cond;

    StatusThread() {
        status = STATUS_FREE;
    }
};

void task(StatusThread* sthread) {
    while (1) {
        unique_lock<mutex> qMtx(fds_mtx);
        sthread->cond.wait(qMtx, [&]{return sthread->status || signalFlag;});
        
        if (signalFlag) {
            qMtx.unlock();
            return;
        }

        while (fds.empty())
            sthread->cond.wait(qMtx);

        int numbytes;
        char buf[BUFSIZE];
        if ((numbytes = recv(fds.front(), buf, BUFSIZE, 0))==-1) 
            err_msg("recv");

        buf[numbytes] = '\0';

        cout << "thread[" << this_thread::get_id() << "]: server received: " << buf << endl;

        if (send(fds.front(), "Hello, client!", sizeof("Hello, client!"), 0)==-1) 
            err_msg("send");

        if (close(fds.front()) == -1)
            err_msg("close");

        fds.pop();
        sthread->status = STATUS_FREE;
        qMtx.unlock();
    }
}

class ThreadPool {
    private:
        const unsigned int count = thread::hardware_concurrency();
        vector<StatusThread*> sthreads;
        vector<thread> threads;
    public:
        void Initialize() {
            for (int index = 0; index < count; ++index) {
                StatusThread *sthread = new StatusThread();
                sthreads.push_back(sthread);

                thread thr(task, sthreads[index]);
                threads.emplace_back(move(thr));
            }
        }

        void AddWork(int fd) {
            for (auto& sthread : sthreads) {
                if (sthread->status == STATUS_FREE) {
                    sthread->status = STATUS_BUSY;
                    {
                        lock_guard<mutex> qMtx(fds_mtx);
                        fds.push(fd);
                        sthread->cond.notify_one();
                    }
                    break;
                }
            }
        }

        void Join() {
            for (int index = 0; index < count; ++index) {
                sthreads[index]->cond.notify_one();
                cout << "thread[" << threads[index].get_id() << "]: joining" << endl; 
                threads[index].join();
                cout << "success" << endl; 
            } 
        } 
};

#endif