#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <algorithm>
#include <memory>

#include "Subscriber.h"

using namespace std;
class Channel
{
public:
    explicit Channel(string t) : topic(t), pub_thread(), isExit(false)
    {
        pub_thread = thread(&Channel::ProcessChannel, this);
    }

    ~Channel()
    {
        isExit = true;
        cv.notify_all();
        if (pub_thread.joinable())
        {
            pub_thread.join();
        }
    }

    string GetTopic()
    {
        return topic;
    }

    void AddMessage(string msg, int priority)
    {
        unique_lock<mutex> l(m);
        messages.push_back(make_pair<>(msg, priority));
        cv.notify_all();
        l.unlock();
    }

    void AddSubscriber(shared_ptr<Subscriber> sub)
    {
        //if the subscriber is dynamically added then need to handle the race condition.
        //in this project, subscribers are statically added. so, race condition not necessary.
        //unique_lock<mutex> l(m);
        subscribers.push_back(sub);
        //cv.notify_all();
        //l.unlock();
    }

    void RemoveSubscriber(shared_ptr<Subscriber> sub)
    {
        //if the subscriber is dynamically removed then need to handle the race condition.
        //in this project, subscribers are not deleted at all. so, following code may not necessary.
        subscribers.erase(remove(subscribers.begin(), subscribers.end(), sub), subscribers.end());
    }

    void ProcessChannel()
    {
        while (!isExit)
        {
            unique_lock<mutex> l(m);
            while (messages.empty())
            {
                cv.wait(l);
            }
            auto start = chrono::steady_clock::now();
            //sort based on the priority.
            sort(messages.begin(), messages.end(), [](const pair<string, int> &elem1, const pair<string, int> &elem2) { return elem1.second > elem2.second; });
            //send messages based to the publishers.
            vector<pair<string, int>>::iterator it = messages.begin();
            while (it != messages.end())
            {
                for (auto &sub : subscribers)
                {
                    sub->PublishMessage((*it).first);
                }
                it = messages.erase(it);
            }
            cv.notify_all();
            l.unlock();

            //update the subscribers after every 100 Milli seconds
            auto end = chrono::steady_clock::now();
            long long duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
            if(duration<100){
                this_thread::sleep_for(chrono::microseconds(100 - duration));
            }
        }
    }

private:
    string topic;
    //messages based on the priority.
    vector<pair<string, int>> messages;
    vector<shared_ptr<Subscriber>> subscribers;
    mutex m;
    condition_variable cv;
    //thread to publish to the publishers every 100 milliseconds.
    thread pub_thread;
    bool isExit;
};
