//
// Created by void on 20/03/2026.
//
// https://www.youtube.com/watch?v=bwP0bZkGwco
//

#include "Request.h"

#include "spdlog/spdlog.h"
#include "InPostAPI.h"
#include <switch.h>
#include <fstream>

#include "Helpers.h"
#include "json.hpp"

std::list<RequestData> queue;
std::mutex queueMutex;
std::condition_variable_any queueCv;
std::jthread worker;
CURL* curl;
std::string userAgent = "User-Agent: InPost-Mobile/4.9.0(40900000) (Horizon " + std::to_string(HOSVER_MAJOR(hosversionGet())) + "." + std::to_string(HOSVER_MINOR(hosversionGet())) + "." + std::to_string(HOSVER_MICRO(hosversionGet())) + "; AW715988204; Nintendo Switch; pl)";
ResponseBuffer reauthBuffer;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;

    if (ResponseBuffer* buffer = static_cast<ResponseBuffer*>(userp)) {
        const unsigned char* data_ptr = static_cast<const unsigned char*>(contents);
        buffer->data.insert(buffer->data.end(), data_ptr, data_ptr + realsize);
    }

    return realsize;
}

CURLcode ExecuteHttp(const std::string& url, const std::string& data, const std::vector<std::string>& headers, ResponseBuffer* response) {
    SPDLOG_DEBUG("CURL: executing a request to {}", url);

    curl_slist* headerList = nullptr;
    for (const std::string& header : headers) {
        headerList = curl_slist_append(headerList, header.c_str());
    }

    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);

    if (!data.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)data.length());
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    if (!data.empty()) {
        SPDLOG_TRACE("data: {}", data);
    }
    SPDLOG_TRACE("headers count: {}", headers.size());
    for (const std::string& h : headers) {
        SPDLOG_TRACE("  {}", h);
    }

    CURLcode result = curl_easy_perform(curl);
    curl_slist_free_all(headerList);

    return result;
}

void Request::DoRequest(const std::string& url, const std::string& data, const std::vector<std::string>& headers, ResponseBuffer* response) {
    std::vector<std::string> headerList;
    headerList.insert(headerList.begin(), headers.begin(), headers.end());
    headerList.push_back(userAgent);
    if (!data.empty()) {
        headerList.push_back("Content-Type: application/json");
    }

    response->result = ExecuteHttp(url, data, headerList, response);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->code);

    if (response->result != CURLE_OK) {
        response->status = Error;
        SPDLOG_ERROR("request failed: {}", curl_easy_strerror(response->result));
        return;
    }

    SPDLOG_INFO("request done, code: {}", response->code);
    SPDLOG_INFO(std::string(response->data.begin(), response->data.end()));
    if (response->code != 401) {
        response->status = Done;
        return;
    }

    SPDLOG_INFO("reauthentication required");
    std::vector<std::string> reauthHeaders;
    reauthHeaders.push_back(userAgent);
    reauthHeaders.push_back("Content-Type: application/json");

    nlohmann::json tokenJson;
    tokenJson["refreshToken"] = InPostAPI::refreshToken;
    std::string requestBody = tokenJson.dump();
    std::string authUrl = InPostAPI::baseUrl + "/v1/authenticate";

    reauthBuffer.data.clear();
    CURLcode result = ExecuteHttp(authUrl, requestBody, reauthHeaders, &reauthBuffer);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &reauthBuffer.code);

    if (result != CURLE_OK || reauthBuffer.code != 200) {
        response->status = Error;
        SPDLOG_ERROR("reauthentication failed, code: {}", reauthBuffer.code);
        SPDLOG_ERROR("return data: {}", std::string(response->data.begin(), response->data.end()));
        return;
    }

    std::string responseData(reauthBuffer.data.begin(), reauthBuffer.data.end());
    if (!nlohmann::json::accept(responseData)) {
        SPDLOG_ERROR("malformed json");
        response->status = Error;
        return;
    }

    nlohmann::json responseJson = nlohmann::json::parse(responseData);
    std::string authToken = responseJson.value("authToken", "");
    if (authToken.empty()) {
        response->status = Error;
        return;
    }

    InPostAPI::authToken = authToken;

    if (!disableSavingToSD) {
        std::ofstream file("sdmc:/config/switchpost/token.json");
        if (file.is_open()) {
            nlohmann::json tokenJsonFile;
            tokenJsonFile["refreshToken"] = InPostAPI::refreshToken;
            tokenJsonFile["authToken"] = authToken;
            file << tokenJsonFile.dump();
            file.close();
        }
    }

    SPDLOG_INFO("retrying request");
    std::vector<std::string> retryHeaders;
    for (const std::string& header : headers) {
        if (header.find("Authorization:") != std::string::npos) {
            retryHeaders.push_back("Authorization: " + InPostAPI::authToken);
        } else {
            retryHeaders.push_back(header);
        }
    }
    retryHeaders.push_back(userAgent);
    if (!data.empty()) {
        retryHeaders.push_back("Content-Type: application/json");
    }

    response->data.clear();
    response->code = 0;
    response->result = ExecuteHttp(url, data, retryHeaders, response);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->code);

    if (response->result != CURLE_OK || response->code == 401) {
        response->status = Error;
        SPDLOG_ERROR("retry failed, code: {}", response->code);
        SPDLOG_ERROR(std::string(response->data.begin(), response->data.end()));
    } else {
        response->status = Done;
        SPDLOG_INFO("retry success, code: {}", response->code);
    }
}

void RequestThreadWorker(const std::stop_token &stoken) {
    ResponseBuffer local_buf = {};
    SPDLOG_DEBUG("worker is working");
    SPDLOG_DEBUG("pinning thread to 2nd core");
    svcSetThreadCoreMask(CUR_THREAD_HANDLE, -1, 1 << 1);
    while (true) {
        std::unique_lock lock(queueMutex);
        if (queue.empty()) SPDLOG_TRACE("queue is empty - going to eep");
        queueCv.wait(lock, stoken, [&]{
            return !queue.empty() || stoken.stop_requested();
        });

        if (stoken.stop_requested()) {
            SPDLOG_DEBUG("stop requested on thread");
            break;
        }

        SPDLOG_DEBUG("some data in queue - working on it");
        RequestData data = std::move(queue.front());
        queue.pop_front();
        lock.unlock();

        Request::DoRequest(data.url, data.data, data.headers, data.responseBuffer);
    }
}

void Request::QueueRequest(const std::string& url, const std::string& data, const std::vector<std::string>& headers, ResponseBuffer* response) {
    RequestData request;

    request.url = url;
    request.data = data;
    request.headers = headers;
    request.responseBuffer = response;
    {
        std::lock_guard lock(queueMutex);
        SPDLOG_DEBUG("putting request to queue");
        queue.push_back(std::move(request));
    }
    SPDLOG_DEBUG("notifying queue");
    queueCv.notify_one();
}

void Request::StartThread() {
    curl = curl_easy_init();
    if (!curl) {
        SPDLOG_ERROR("curl_easy_init() failed");
        return;
    }
    SPDLOG_INFO("starting request thread");
    worker = std::jthread(RequestThreadWorker);
}

void Request::EndThread() {
    curl_easy_cleanup(curl);
    SPDLOG_INFO("stopping request thread");
    worker.request_stop();
}
