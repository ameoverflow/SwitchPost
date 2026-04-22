//
// Created by void on 23/03/2026.
//

#include "InpostAPI.h"
#include "Request.h"
#include "json.hpp"
#include <string>
#include <vector>
#include "spdlog/spdlog.h"
#include <fstream>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <curl/curl.h>

std::shared_ptr<ResponseBuffer> InpostAPI::sendSMSCodeBuffer = std::make_shared<ResponseBuffer>();
std::shared_ptr<ResponseBuffer> InpostAPI::verifySMSCodeBuffer = std::make_shared<ResponseBuffer>();
std::shared_ptr<ResponseBuffer> InpostAPI::getPaczkasBuffer = std::make_shared<ResponseBuffer>();
std::shared_ptr<ResponseBuffer> InpostAPI::getPaczkomatStatusBuffer = std::make_shared<ResponseBuffer>();
std::shared_ptr<ResponseBuffer> InpostAPI::openPaczkomatBuffer = std::make_shared<ResponseBuffer>();
std::shared_ptr<ResponseBuffer> InpostAPI::terminatePaczkaBuffer = std::make_shared<ResponseBuffer>();

std::unique_ptr<std::vector<char>> InpostAPI::refreshTokenBuffer = std::make_unique<std::vector<char>>();
std::vector<Package> InpostAPI::packages = {};
std::string InpostAPI::refreshToken;
std::string InpostAPI::authToken;

std::string InpostAPI::baseUrl = std::string(BASE_URL);

std::string InpostAPI::FormatIsoToCustom(const std::string& iso_date) {
    std::istringstream iss{iso_date};
    std::chrono::sys_time<std::chrono::milliseconds> utc_tp;

    iss >> std::chrono::parse("%FT%T%Z", utc_tp);
    if (iss.fail()) {
        return "invalid";
    }

    // europe/warsaw is hardcoded because for some reason switch doesnt return timezone on current_zone()
    std::chrono::local_time t = std::chrono::locate_zone("Europe/Warsaw")->to_local(utc_tp);
    std::chrono::local_days days = std::chrono::floor<std::chrono::days>(t);

    std::chrono::year_month_day ymd{days};
    std::chrono::hh_mm_ss<std::chrono::milliseconds> hms{t - days};

    // 5. use stringstream for the formatting
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << (unsigned)ymd.day() << "."
        << std::setw(2) << (unsigned)ymd.month() << "."
        << (int)ymd.year() << " | "
        << std::setw(2) << hms.hours().count() << ":"
        << std::setw(2) << hms.minutes().count();

    return oss.str();
}


void InpostAPI::SendSMSCode(std::string phone) {
    sendSMSCodeBuffer->data.clear();
    sendSMSCodeBuffer->status = InProgress;
    sendSMSCodeBuffer->code = 0;
    nlohmann::json json;
    json["phoneNumber"] = { { "prefix", "+48"}, {"value", phone.c_str()} };
    Request::QueueRequest(baseUrl + "/v1/account", json.dump(), { "Content-Type: application/json" }, sendSMSCodeBuffer);
}

void InpostAPI::VerifySMSCode(std::string phone, std::string code) {
    verifySMSCodeBuffer->data.clear();
    verifySMSCodeBuffer->status = InProgress;
    verifySMSCodeBuffer->code = 0;
    nlohmann::json json;
    json["phoneNumber"] = { { "prefix", "+48"}, {"value", phone.c_str()} };
    json["smsCode"] = code.c_str();
    json["devicePlatform"] = "Android";
    Request::QueueRequest(baseUrl + "/v1/account/verification", json.dump(), { "Content-Type: application/json" }, verifySMSCodeBuffer);
}

void InpostAPI::GetPaczkas() {
    getPaczkasBuffer->data.clear();
    getPaczkasBuffer->status = InProgress;
    getPaczkasBuffer->code = 0;
    Request::QueueRequest(baseUrl + "/v4/parcels/tracked", "", { "Authorization: " + authToken }, getPaczkasBuffer);
}

// here come my favorite part of the code
// i hope yall ready for this
// edit: this doesnt work, idk why, it *should* work
bool InpostAPI::RefreshTokenSync() {
    refreshTokenBuffer->clear();
    CURL* curl = curl_easy_init();
    curl_slist* headerList = nullptr;
    headerList = curl_slist_append(headerList, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 21.2.0; AW715988204; Nintendo Switch; pl)");
    headerList = curl_slist_append(headerList, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_URL, "https://api-inmobile-pl.easypack24.net/v1/authenticate");
    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);

    nlohmann::json tokenJson;
    tokenJson["refreshToken"] = refreshToken;
    SPDLOG_TRACE("sending data {}", tokenJson.dump());
    std::string requestBody = tokenJson.dump();

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)requestBody.length());

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, refreshTokenBuffer.get());
    CURLcode result = curl_easy_perform(curl);

    int code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    if (result != CURLE_OK) {
        SPDLOG_ERROR("request failed: {}", curl_easy_strerror(result));

        curl_slist_free_all(headerList);
        curl_easy_cleanup(curl);
    } else {
        std::string requestJson(refreshTokenBuffer->begin(), refreshTokenBuffer->end());
        SPDLOG_DEBUG("request done, code: {}", code);
        SPDLOG_TRACE("data: {}", requestJson);

        curl_slist_free_all(headerList);
        curl_easy_cleanup(curl);

        if (code == 200) {
            // we have both refresh code, and auth token, rebuild the fucken json just
            if (nlohmann::json::accept(requestJson)) {
                nlohmann::json data = nlohmann::json::parse(requestJson);
                authToken = data.value("authToken", "");

                if (!authToken.empty() && !refreshToken.empty()) {
                    std::ofstream file("sdmc:/config/switchpost/token.json");
                    if (file.is_open()) {
                        nlohmann::json tokenData;
                        tokenData["refreshToken"] = refreshToken;
                        tokenData["authToken"] = authToken;
                        file << tokenData.dump();
                        SPDLOG_INFO("login data saved to SD");
                        return true; // :sunglasses:
                    } else {
                        SPDLOG_ERROR("couldnt open token.json for writing");
                    }
                }
            }
        } else {
            SPDLOG_ERROR("smth went wrong, see logs");
        }
    }
    return false;
}

size_t InpostAPI::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;

    if (std::vector<char>* buffer = static_cast<std::vector<char>*>(userp)) {
        const unsigned char* data_ptr = static_cast<const unsigned char*>(contents);
        buffer->insert(buffer->end(), data_ptr, data_ptr + realsize);
    }

    return realsize;
}

bool InpostAPI::LoadTokens() {
    // load tokens every time since its not much work and ensures app can load data
    std::ifstream token("sdmc:/config/switchpost/token.json");
    if (token.is_open()) {
        std::stringstream buffer;
        buffer << token.rdbuf();
        if (nlohmann::json::accept(buffer.str())) {
            nlohmann::json data = nlohmann::json::parse(buffer.str());
            refreshToken = data["refreshToken"].get<std::string>();
            authToken = data["authToken"].get<std::string>();
            SPDLOG_INFO("loaded tokens");
            SPDLOG_TRACE("refresh: {}", refreshToken);
            SPDLOG_TRACE("auth: {}", authToken);
            token.close();
            return true;
        } else {
            SPDLOG_ERROR("failed to load tokens");
            SPDLOG_DEBUG("{}", buffer.str());
            token.close();
            return false;
        }
    } else {
        return false;
    }
}

bool InpostAPI::ParsePaczkas(std::string json) {
#ifdef DEBUG
    std::ofstream file("sdmc:/config/switchpost/parcels.json");
    if (file.is_open()) {
        file << json;
        file.close();
    }
#endif

    packages.clear();
    if (!nlohmann::json::accept(json)) return false;

    nlohmann::json parcelsJson = nlohmann::json::parse(json);

    if (!parcelsJson.contains("parcels") || !parcelsJson["parcels"].is_array()) {
        return false;
    }

    nlohmann::json parcelsArray = parcelsJson["parcels"];

    for (nlohmann::json parcel : parcelsArray) {
        Package packageObject;

        packageObject.courier = (parcel.value("shipmentType", "") == "courier");
        packageObject.number = parcel.value("shipmentNumber", "");
        packageObject.statusGroup = parcel.value("statusGroup", "");
        packageObject.parcelSize = parcel.value("parcelSize", "");
        packageObject.status = parcel.value("status", "");

        if (parcel.contains("sender") && !parcel["sender"].is_null()) {
            nlohmann::json sender = parcel["sender"];
            packageObject.senderName = sender.value("name", "");
        }
        if (!packageObject.courier) {
            packageObject.qrCode = parcel.value("qrCode", "");
            if (parcel.contains("pickUpPoint") && !parcel["pickUpPoint"].is_null()) {
                nlohmann::json pup = parcel["pickUpPoint"];
                packageObject.pickupPointName = pup.value("name", "");
                packageObject.imageUrl = pup.value("imageUrl", "");

                if (pup.contains("addressDetails")) {
                    nlohmann::json addr = pup["addressDetails"];
                    packageObject.city = addr.value("city", "");
                    packageObject.street = addr.value("street", "");
                }

                if (pup.contains("location")) {
                    nlohmann::json loc = pup["location"];
                    packageObject.lat = loc.value("latitude", 0.0f);
                    packageObject.lon = loc.value("longitude", 0.0f);
                }
            }
        }

        packageObject.pickupDate = parcel.value("pickUpDate", "");
        packageObject.code = parcel.value("openCode", "");
        packageObject.openable = parcel.contains("openCode") && !parcel["openCode"].is_null();

        if (packageObject.status == "DELIVERED") {
            packageObject.openable = false;
            packageObject.delivered = true;
        }

        if (parcel.contains("receiver") && !parcel["receiver"].is_null()) {
            nlohmann::json receiver = parcel["receiver"];
            if (receiver.contains("phoneNumber")) {
                nlohmann::json phone = receiver["phoneNumber"];
                packageObject.phonePrefix = phone.value("prefix", "");
                packageObject.phoneNumber = phone.value("value", "");
            }
        }

        if (parcel.contains("eventLog") && parcel["eventLog"].is_array()) {
            nlohmann::json eventLog = parcel["events"];
            for (nlohmann::json event : eventLog) {
                PackageEvent pkgEvent;
                pkgEvent.date = FormatIsoToCustom(event.value("date", ""));
                pkgEvent.name = event.value("eventTitle", ""); // matching test_data.json key
                packageObject.events.push_back(pkgEvent);
            }
        }

        packages.push_back(packageObject);
    }
    std::reverse(packages.begin(), packages.end());
    return true;
}

void InpostAPI::GetPaczkomatStatus(std::string shipmentNumber, std::string openCode, std::string receiverPhoneNumber, std::string receiverPhonePrefix, float lat, float lon) {
    nlohmann::ordered_json sendData;

    sendData["parcel"] = nlohmann::json::object();
    sendData["parcel"]["shipmentNumber"] = shipmentNumber;
    sendData["parcel"]["openCode"] = openCode;

    sendData["parcel"]["recieverPhoneNumber"] = nlohmann::json::object();
    sendData["parcel"]["recieverPhoneNumber"]["prefix"] = receiverPhonePrefix;
    sendData["parcel"]["recieverPhoneNumber"]["value"] = receiverPhoneNumber;

    sendData["geoPoint"] = nlohmann::json::object();
    sendData["geoPoint"]["latitude"] = lat;
    sendData["geoPoint"]["longitude"] = lon;
    sendData["geoPoint"]["accuracy"] = 13.365;

    getPaczkomatStatusBuffer->data.clear();
    getPaczkomatStatusBuffer->status = InProgress;
    getPaczkomatStatusBuffer->code = 0;
    Request::QueueRequest(baseUrl + "/v2/collect/validate", sendData.dump(), { "Authorization: " + authToken }, getPaczkomatStatusBuffer);
}

void InpostAPI::OpenPaczkomat(std::string uuid) {
    nlohmann::json sendData;
    sendData["sessionUuid"] = uuid;

    openPaczkomatBuffer->data.clear();
    openPaczkomatBuffer->status = InProgress;
    openPaczkomatBuffer->code = 0;
    Request::QueueRequest(baseUrl + "/v1/collect/compartment/open", sendData.dump(), { "Authorization: " + authToken }, openPaczkomatBuffer);
}

// terminator
void InpostAPI::TerminatePaczka(std::string uuid) {
    nlohmann::json sendData;
    sendData["sessionUuid"] = uuid;

    terminatePaczkaBuffer->data.clear();
    terminatePaczkaBuffer->status = InProgress;
    terminatePaczkaBuffer->code = 0;
    Request::QueueRequest(baseUrl + "/v1/collect/compartment/terminate", sendData.dump(), { "Authorization: " + authToken }, terminatePaczkaBuffer);
}