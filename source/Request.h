//
// Created by void on 20/03/2026.
//

#ifndef SWITCHPOST_REQUEST_H
#define SWITCHPOST_REQUEST_H

#include "curl/curl.h"
#include <string>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <thread>

enum RequestStatus {
    NotStarted,
    InProgress,
    Done,
    Error
};

struct ResponseBuffer {
    int code;
    std::vector<char> data;
    CURLcode result;
    std::atomic<enum RequestStatus> status{NotStarted};
};

struct RequestData {
    std::string url;
    std::string data;

    std::vector<std::string> headers;

    bool is_binary = false;
    std::shared_ptr<ResponseBuffer> responseBuffer;
};

class Request {
public:
    static void QueueRequest(std::string url, std::string data, std::vector<std::string> headers, std::shared_ptr<ResponseBuffer> response);
    static void StartThread();
    static void EndThread();
    static void DoRequest(std::string url, std::string data, std::vector<std::string> headers, std::shared_ptr<ResponseBuffer> response);
private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static void RequestThreadWorker(const std::stop_token &stoken);
    static void LogRequestToFile(const std::string& url, const std::string& data, const std::vector<std::string>& headers);
    static std::list<RequestData> queue;
    static std::mutex queueMutex;
    static std::condition_variable_any queueCv;
    static std::jthread worker;
    static CURL* curl;
    static std::string userAgent;
};



#endif //SWITCHPOST_REQUEST_H
