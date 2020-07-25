#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "ThreadPool.h"
#include "Broker.h"

using namespace std;

const int PORT = 8090;
const string ADDRESS = "127.0.0.1";
const int BUFFSIZE = 10 * 1024;
const int SERVERBACKLOG = 20;
const int THREADPOOLSIZE = 10;

class Server
{
public:
    explicit Server(int th_size) : threadPool(make_unique<ThreadPool>(th_size)), broker(make_unique<Broker>(th_size))
    {
    }

    //check the error, if there are then print and exit .
    void CheckPrint(int val, string error)
    {
        if (val < 0)
        {
            cout << error << endl;
            exit(EXIT_FAILURE);
        }
    }

    void HandleConnection(int client_socket)
    {
        char readBuff[BUFFSIZE] = {'0'};
        int length = 0;
        while ((length = read(client_socket, readBuff, sizeof(readBuff) - 1)) > 0)
        {
            readBuff[length] = 0;
            std::string ss(readBuff);
            int start = 0;
            int pos = 0;
            string delimeter = ";";
            //read line by line and add the whole message to messageQueue.
            while (((pos = ss.find(delimeter, start)) != string::npos))
            {
                string str = ss.substr(start, pos - start);
                broker->AddMessage(str);
                start = pos + delimeter.size();
            }
        }
        close(client_socket);
    }

    void StartServer()
    {
        broker->LoadSubscribers();
        //For Publishing messages after adding all the messages from client.
        broker->StartThreads();
        //For reading the clients messages.
        threadPool->CreateThreadPool();
        int server_fd;
        CheckPrint((server_fd = socket(AF_INET, SOCK_STREAM, 0)), "socket failed");

        struct sockaddr_in server_address = {};
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = INADDR_ANY;
        server_address.sin_port = htons(PORT);
        CheckPrint(::bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)), "bind failed");
        CheckPrint(listen(server_fd, SERVERBACKLOG), "listen failed");
        // accepting clients continuously.
        while (!isExit)
        {
            // To have a better performance, scalable and multiplexing features use epoll.
            // I implemented this epoll some years ago at https://github.com/bandarapusatyam/IO-Multiplexing-with-C
            //level triggered - epollexclusive
            // Read - EPolloneshot
            // close - EPOL - ctl -del
            cout << "Server waiting for new client " << endl;
            int client_socket;
            CheckPrint((client_socket = accept(server_fd, (struct sockaddr *)NULL, NULL)), "Error on accept");
            cout<<"Connection established to the client"<<endl<<endl;
            threadPool->AddTask([this, client_socket]() { HandleConnection(client_socket); });
        }
        // Thread joinable and join.
        threadPool->CloseThreadPool();
        close(server_fd);
    }

private:
    unique_ptr<Broker> broker;
    unique_ptr<ThreadPool> threadPool;
    // set this value to true in case exiting scenario.
    bool isExit = false;
};

int main(int argc, char **argv)
{
    int num_threads = THREADPOOLSIZE;
    if (argc > 1)
    {
        num_threads = atoi(argv[1]);
    }
    Server serv(num_threads);
    serv.StartServer();
    return 0;
}