#pragma once

#include <mutex>
#include <memory>
#include <fstream>

using namespace std;
class Subscriber
{
public:
    explicit Subscriber(string id) : subscriber(id)
    {
        CreateFile();
    }

    void CreateFile()
    {
        file = make_unique<ofstream>(subscriber + ".txt");
    }

    ~Subscriber()
    {
        if (file != nullptr && file->is_open())
        {
            file->close();
        }
    }

    void PublishMessage(string msg)
    {
        if (file != nullptr && file->is_open())
        {
            //multiple channels write at the same time. so need lock
            lock_guard<mutex> l(m);
            msg = msg + "\n";
            file->write(msg.c_str(), msg.size());
            file->flush();
            //cout << subscriber << " Subscriber publishing message " << msg;
        }
    }

    string GetId()
    {
        return subscriber;
    }

private:
    string subscriber;
    unique_ptr<ofstream> file;
    mutex m;
};
