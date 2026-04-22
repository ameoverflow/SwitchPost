//
// Created by void on 20/03/2026.
//
// https://www.youtube.com/watch?v=bwP0bZkGwco
//

#include "Request.h"

#include "spdlog/spdlog.h"
#include "InpostAPI.h"
#include <switch.h>
#include <fstream>
#include "json.hpp"

std::list<RequestData> Request::queue;
std::mutex Request::queueMutex;
std::condition_variable_any Request::queueCv;
std::jthread Request::worker;
CURL* Request::curl;
std::string Request::userAgent = "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 22.1.0; AW715988204; Nintendo Switch; pl)";
std::unique_ptr<ResponseBuffer> reauthBuffer = std::make_unique<ResponseBuffer>();

void Request::LogRequestToFile(const std::string& url, const std::string& data, const std::vector<std::string> &headers) {
    SPDLOG_INFO("request to {}", url);
    if (!data.empty()) {
        SPDLOG_DEBUG("data: {}", data);
    }
    SPDLOG_DEBUG("headers count: {}", headers.size());
    for (const std::string& h : headers) {
        SPDLOG_DEBUG("  {}", h);
    }
}

size_t Request::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;

    if (ResponseBuffer* buffer = static_cast<ResponseBuffer*>(userp)) {
        const unsigned char* data_ptr = static_cast<const unsigned char*>(contents);
        buffer->data.insert(buffer->data.end(), data_ptr, data_ptr + realsize);
    }

    return realsize;
}

// dlaczego ten kod wygląda jak gówno?
// bo nie miałam ochoty użerać się z curlem i gdb bo w jakimś miejscu wypierdala invalid memory access :v
// zajebane z inpost3ds i wygląda jak gówno ale chuj, działa, jak komuś chce się naprawiać to niech jebnie PR na to
void Request::DoRequest(std::string url, std::string data, std::vector<std::string> headers, std::shared_ptr<ResponseBuffer> response) {
    curl_easy_reset(curl);
    curl_slist* headerList = nullptr;
    for (const std::string& header : headers) {
        headerList = curl_slist_append(headerList, header.c_str());
    }
    headerList = curl_slist_append(headerList, userAgent.c_str());
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    if (!data.empty()) {
        headerList = curl_slist_append(headerList, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    }
    LogRequestToFile(url, data, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response.get());
    response->result = curl_easy_perform(curl);

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->code);

    curl_slist_free_all(headerList);

    if (response->result != CURLE_OK) {
        response->status = Error;
        SPDLOG_ERROR("request failed: {}", curl_easy_strerror(response->result));
    } else {
        if (response->code == 401) {
            // reauth
            curl_easy_reset(curl);
            curl_slist* reauthHeaders = nullptr;
            reauthHeaders = curl_slist_append(reauthHeaders, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 21.2.0; AW715988204; Nintendo Switch; pl)");
            reauthHeaders = curl_slist_append(reauthHeaders, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_URL, std::string(InpostAPI::baseUrl + "/v1/authenticate").c_str());
            curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, reauthHeaders);

            nlohmann::json tokenJson;
            tokenJson["refreshToken"] = InpostAPI::refreshToken;
            SPDLOG_TRACE("sending data {}", tokenJson.dump());
            std::string requestBody = tokenJson.dump();

            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)requestBody.length());

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, reauthBuffer.get());
            CURLcode result = curl_easy_perform(curl);

            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &reauthBuffer->code);

            curl_slist_free_all(reauthHeaders);

            if (result == CURLE_OK) {
                std::string requestJson(reauthBuffer->data.begin(), reauthBuffer->data.end());
                SPDLOG_DEBUG("request done, code: {}", reauthBuffer->code);
                SPDLOG_TRACE("data: {}", requestJson);

                if (reauthBuffer->code != 200) return;

                // we have both refresh code, and auth token, rebuild the fucken json just
                if (nlohmann::json::accept(requestJson)) {
                    SPDLOG_DEBUG("trying to save new token to file");
                    nlohmann::json data = nlohmann::json::parse(requestJson);
                    std::string authToken = data.value("authToken", "");

                    if (!authToken.empty() && !InpostAPI::refreshToken.empty()) {
                        std::ofstream file("sdmc:/config/switchpost/token.json");
                        if (file.is_open()) {
                            nlohmann::json tokenData;
                            tokenData["refreshToken"] = InpostAPI::refreshToken;
                            tokenData["authToken"] = authToken;
                            InpostAPI::authToken = authToken;
                            file << tokenData.dump();
                            file.close();
                            SPDLOG_INFO("login data saved to SD");
                        } else {
                            SPDLOG_ERROR("couldnt open file for writing");
                            return;
                        }
                    }
                }

                curl_slist* retryHeaders = nullptr;
                for (const std::string& header : headers) {
                    if (header.find("Authorization:") != std::string::npos) {
                        std::string authHeader = "Authorization: " + InpostAPI::authToken;
                        retryHeaders = curl_slist_append(retryHeaders, authHeader.c_str());
                    } else {
                        retryHeaders = curl_slist_append(retryHeaders, header.c_str());
                    }
                }
                curl_easy_reset(curl);
                retryHeaders = curl_slist_append(retryHeaders, userAgent.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, retryHeaders);

                curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
                curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);

                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                if (!data.empty()) {
                    headerList = curl_slist_append(headerList, "Content-Type: application/json");
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
                }

                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, response.get());

                response->data.clear();
                response->code = 0;
                response->result = curl_easy_perform(curl);
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->code);

                if (response->result != CURLE_OK) {
                    response->status = Error;
                    SPDLOG_ERROR("request failed: {}", curl_easy_strerror(response->result));
                } else {
                    if (response->code == 401) {
                        response->status = Error;
                        SPDLOG_ERROR("request failed after reauthentication, code: {}", response->code);
                        curl_slist_free_all(retryHeaders);
                    } else {
                        response->status = Done; // :3
                        SPDLOG_DEBUG("request done after reauthentication, code: {}", response->code);
                        SPDLOG_TRACE("data: {}", data);
                        curl_slist_free_all(retryHeaders);
                    }
                }
            } else {
                response->status = Error;
                SPDLOG_ERROR("reauthentication failed");
            }
        } else {
            std::string data(response->data.begin(), response->data.end());
            response->status = Done;
            SPDLOG_DEBUG("request done, code: {}", response->code);
            SPDLOG_TRACE("data: {}", data);
        }
    }
}

void Request::RequestThreadWorker(const std::stop_token &stoken) {
    ResponseBuffer local_buf = {};
    SPDLOG_TRACE("worker is working");
    SPDLOG_TRACE("pinning thread to 2nd core");
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

        SPDLOG_TRACE("some data in queue - working on it");
        RequestData data = std::move(queue.front());
        queue.pop_front();
        lock.unlock();

        DoRequest(std::move(data.url), std::move(data.data), std::move(data.headers), data.responseBuffer);
    }
}

void Request::QueueRequest(std::string url, std::string data, std::vector<std::string> headers, std::shared_ptr<ResponseBuffer> response) {
    RequestData request;

    request.url = std::move(url);
    request.data = std::move(data);
    request.headers = std::move(headers);
    request.responseBuffer = std::move(response);
    request.is_binary = false;
    {
        std::lock_guard lock(queueMutex);
        SPDLOG_TRACE("putting request to queue");
        queue.push_back(std::move(request));
    }
    SPDLOG_TRACE("notifying queue");
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
