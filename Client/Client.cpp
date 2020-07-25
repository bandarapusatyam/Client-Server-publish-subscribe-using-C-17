#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <algorithm>
using namespace std;

const int PORT = 8090;
const string ADDRESS = "127.0.0.1";
const string FILENAME = "publish_data.txt";

class Client
{
public:
    //check the error, print and exit if there are.
    void CheckPrint(int val, string error)
    {
        if (val < 0)
        {
            cout << error << endl;
            exit(EXIT_FAILURE);
        }
    }

    void ReadingFile(vector<string> &content)
    {
        unique_ptr<ifstream> file = make_unique<ifstream>(FILENAME);
        if (file != nullptr)
        {
            string line;
            while (getline(*file, line))
            {
                content.push_back(line);
            }
            file->close();
        }
        else
        {
            cout << "Unable to read the file";
        }
    }

    void StartClient()
    {
        vector<string> content;
        ReadingFile(content);
        int client_fd = 0;
        CheckPrint((client_fd = socket(AF_INET, SOCK_STREAM, 0)), "Socket not created ");
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);
        serv_addr.sin_addr.s_addr = inet_addr(ADDRESS.c_str());

        CheckPrint(connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)), "Connection failed due to port and ip problems");
        cout<<"Connected to the server"<<endl;
        //publish data to server
        for (string str : content)
        {
            stringstream s;
            //for differentiating the each lines.
            str += ";";
            write(client_fd, str.c_str(), str.size());
        }
        close(client_fd);
    }
};

int main(int argc, char **argv)
{
    int numClients = 1;
    if (argc > 1)
    {
        numClients = atoi(argv[1]);
    }
    // start clients.
    vector<thread> th;
    for (int i = 0; i < numClients; i++)
    {
        Client cl;
        th.push_back(move(thread(&Client::StartClient, &cl)));
    }

    for (int i = 0; i < numClients; i++)
    {
        if (th[i].joinable())
            th[i].join();
    }

    return 0;
}