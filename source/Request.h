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
    std::atomic<RequestStatus> status{NotStarted};
};

struct RequestData {
    std::string url;
    std::string data;
    std::vector<std::string> headers;
    ResponseBuffer* responseBuffer;
};

namespace Request {
    void QueueRequest(const std::string& url, const std::string& data, const std::vector<std::string>& headers, ResponseBuffer* response);
    void StartThread();
    void EndThread();
    void DoRequest(const std::string& url, const std::string& data, const std::vector<std::string>& headers, ResponseBuffer* response);
};



#endif //SWITCHPOST_REQUEST_H
