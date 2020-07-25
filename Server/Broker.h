#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <memory>
#include <queue>
#include <unordered_map>
#include <regex>
#include <iostream>

#include "Channel.h"

using namespace std;
class Broker
{
public:
    explicit Broker(int th_size) : thread_pool_size(th_size), isExit(false)
    {
    }
    ~Broker()
    {
        isExit = true;
        CloseThreads();
    }

    void StartThreads()
    {
        for (int i = 0; i < thread_pool_size; i++)
        {
            threads.push_back(move(thread(&Broker::ProcessMessages, this)));
        }
    }

    void CloseThreads()
    {
        cv.notify_all();
        for (int i = 0; i < thread_pool_size; i++)
        {
            if (threads[i].joinable())
            {
                threads[i].join();
            }
        }
    }

    void AddMessage(string msg)
    {
        unique_lock<mutex> l(m);
        messagesQueue.push(msg);
        cv.notify_all();
        l.unlock();
    }

    void ProcessMessages()
    {
        while (!isExit)
        {
            string msg;
            unique_lock<mutex> l(m);
            while (messagesQueue.empty())
            {
                cv.wait(l);
            }
            msg = messagesQueue.front();
            messagesQueue.pop();
            //parse message into topic, text and priority.
            //topic
            int pos2 = msg.find(':');
            int pos3 = msg.find(':', pos2 + 1);
            string topic = msg.substr(0, pos2);
            // removes double quotes.
            topic.erase(remove_if(topic.begin(), topic.end(), [](char ch) { return (ch == '"'); }), topic.end());
            //removes extra spaces before and after.
            topic = std::regex_replace(topic, std::regex("^ +| +$|( ) +"), "$1");

            //message
            string txt = msg.substr(pos2 + 1, pos3 - pos2 - 1);
            txt.erase(remove_if(txt.begin(), txt.end(), [](char ch) { return (ch == '"'); }), txt.end());
            //priority
            string prio = msg.substr(pos3 + 1, pos3 - msg.size() - 1);
            // if (channels.find(topic) != channels.end())
            //{
                channels[topic]->AddMessage(txt, stoi(prio));
            //}
            //else
            //{
            // cout << "No channel found for " << topic << endl;
            //}
            cv.notify_all();
            l.unlock();
        }
    }

    void LoadSubscribers()
    {
        unique_ptr<ifstream> file = make_unique<ifstream>("subscribers.config");
        if (file != nullptr)
        {
            string line;
            while (getline(*file, line))
            {
                //get id and topic
                int pos = line.find('"');
                string id = line.substr(0, pos - 1);
                string topic = line.substr(pos + 1, line.size() - pos - 1);
                //remove trailing spaces from the id.
                id = std::regex_replace(id, std::regex("^ +| +$|( ) +"), "$1");

                //open file and attached to each subscriber.
                if (subscribers.find(id) == subscribers.end())
                {
                    subscribers[id] = make_shared<Subscriber>(id);
                }
                //remove trailing double quotes and spaces from the topic.
                topic = std::regex_replace(topic, std::regex("^ +| +$|( ) +"), "$1");
                topic.erase(remove_if(topic.begin(), topic.end(), [](char ch) { return (ch == '"'); }), topic.end());
                // add subscribers for each topic.
                if (channels.find(topic) == channels.end())
                {
                    channels[topic] = make_shared<Channel>(topic);
                }
                channels[topic]->AddSubscriber(subscribers[id]);
            }

            file->close();
        }
        else
        {
            cout << "Can't open the file " << endl;
        }
    }

    //for dynamically adding subscribers. Not necessary in this project.
    void AddSubscriber() {}

    //for dynamically removing subscribers. Not necessary in this project.
    void RemoveSubscriber() {}

private:
    //channesl are for topic.
    unordered_map<string, shared_ptr<Channel>> channels;
    unordered_map<string, shared_ptr<Subscriber>> subscribers;

    //if the following messageQueue is custom implementation
    // of ring buffer using lock free, wait free then makes
    // queue reading much faster and no need for locks.
    queue<string> messagesQueue;
    mutex m;
    condition_variable cv;
    vector<thread> threads;
    int thread_pool_size;
    bool isExit;
};
