//
// Created by void on 23/03/2026.
//

#include "InPostAPI.h"
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

std::string FormatIsoToCustom(const std::string& iso_date) {
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

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << (unsigned)ymd.day() << "."
        << std::setw(2) << (unsigned)ymd.month() << "."
        << (int)ymd.year() << " | "
        << std::setw(2) << hms.hours().count() << ":"
        << std::setw(2) << hms.minutes().count();

    return oss.str();
}

void InPostAPI::SendSMSCode(std::string phone) {
    sendSMSCodeBuffer.data.clear();
    sendSMSCodeBuffer.status = InProgress;
    sendSMSCodeBuffer.code = 0;
    nlohmann::json json;
    json["phoneNumber"] = { { "prefix", "+48"}, {"value", phone.c_str()} };
    Request::QueueRequest(baseUrl + "/v1/account", json.dump(), { "Content-Type: application/json" }, &sendSMSCodeBuffer);
}

void InPostAPI::VerifySMSCode(std::string phone, std::string code) {
    verifySMSCodeBuffer.data.clear();
    verifySMSCodeBuffer.status = InProgress;
    verifySMSCodeBuffer.code = 0;
    nlohmann::json json;
    json["phoneNumber"] = { { "prefix", "+48"}, {"value", phone.c_str()} };
    json["smsCode"] = code.c_str();
    json["devicePlatform"] = "Android";
    Request::QueueRequest(baseUrl + "/v1/account/verification", json.dump(), { "Content-Type: application/json" }, &verifySMSCodeBuffer);
}

void InPostAPI::GetPaczkas() {
    getPaczkasBuffer.data.clear();
    getPaczkasBuffer.status = InProgress;
    getPaczkasBuffer.code = 0;
    Request::QueueRequest(baseUrl + "/v4/parcels/tracked", "", { "Authorization: " + authToken }, &getPaczkasBuffer);
}

bool InPostAPI::LoadTokens() {
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
        }
        SPDLOG_ERROR("failed to load tokens");
        SPDLOG_DEBUG("{}", buffer.str());
        token.close();
        return false;
    }
    return false;
}

bool InPostAPI::ParsePaczkas(std::string json) {
#ifdef DEBUG
    std::ofstream file("sdmc:/config/switchpost/parcels.json");
    if (file.is_open()) {
        file << json;
        file.close();
    }
#endif

    packages.clear();
    packageArchive.clear();
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
        packageObject.pickupCode = parcel.value("openCode", "");

        // this covers a lot of cases i think, except paczkopunkts
        packageObject.openable = parcel.contains("openCode") && !parcel["openCode"].is_null() && (
            packageObject.status == "READY_TO_PICKUP" ||
            packageObject.status == "PICKUP_REMINDER_SENT" || packageObject.status == "PICKUP_TIME_EXPIRED" ||
            packageObject.status == "OUT_FOR_DELIVERY_TO_ADDRESS" || packageObject.status == "PICKUP_REMINDER_SENT_ADDRESS"
            );
        packageObject.delivered = packageObject.status == "DELIVERED";

        if (parcel.contains("receiver") && !parcel["receiver"].is_null()) {
            nlohmann::json receiver = parcel["receiver"];
            if (receiver.contains("phoneNumber")) {
                nlohmann::json phone = receiver["phoneNumber"];
                packageObject.phonePrefix = phone.value("prefix", "");
                packageObject.phoneNumber = phone.value("value", "");
            }
        }

        if (parcel.contains("eventLog") && parcel["eventLog"].is_array()) {
            nlohmann::json events = parcel["eventLog"];
            for (nlohmann::json event : events) {
                PackageEvent pkgEvent;
                pkgEvent.date = FormatIsoToCustom(event.value("date", ""));
                pkgEvent.internalDate = event.value("date", "");
                pkgEvent.name = event.value("name", ""); // matching test_data.json key
                packageObject.events.push_back(pkgEvent);
            }
        }

        if (packageObject.delivered || packageObject.status == "RETURNED_TO_SENDER") {
            // add parcel accordingly to archive or current parcels
            SPDLOG_TRACE(packageObject.events[0].internalDate);
            std::istringstream iss{packageObject.events[0].internalDate};
            std::chrono::sys_time<std::chrono::milliseconds> utc_tp;

            iss >> std::chrono::parse("%Y-%m-%dT%H:%M:%S", utc_tp);
            if (iss.fail()) {
                packages.push_back(packageObject);
                continue;
            }

            std::chrono::sys_time<std::chrono::milliseconds> now_ms =
                std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());

            std::chrono::milliseconds diff = now_ms - utc_tp;

            if (diff > std::chrono::hours(24)) {
                SPDLOG_TRACE("adding to archive");
                packageArchive.push_back(packageObject);
            } else {
                SPDLOG_TRACE("adding to regular parcels");
                packages.push_back(packageObject);
            }
        } else {
            packages.push_back(packageObject);
        }
    }
    if (!packages.empty()) {
        SPDLOG_TRACE("reversing packages");
        std::reverse(packages.begin(), packages.end());
    }
    if (!packageArchive.empty()) {
        SPDLOG_TRACE("reversing archive");
        std::reverse(packageArchive.begin(), packageArchive.end());
    }
    return true;
}

void InPostAPI::GetPaczkomatStatus(std::string shipmentNumber, std::string openCode, std::string receiverPhoneNumber, std::string receiverPhonePrefix, float lat, float lon) {
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

    getPaczkomatStatusBuffer.data.clear();
    getPaczkomatStatusBuffer.status = InProgress;
    getPaczkomatStatusBuffer.code = 0;
    Request::QueueRequest(baseUrl + "/v2/collect/validate", sendData.dump(), { "Authorization: " + authToken }, &getPaczkomatStatusBuffer);
}

void InPostAPI::OpenPaczkomat(std::string uuid) {
    nlohmann::json sendData;
    sendData["sessionUuid"] = uuid;

    openPaczkomatBuffer.data.clear();
    openPaczkomatBuffer.status = InProgress;
    openPaczkomatBuffer.code = 0;
    Request::QueueRequest(baseUrl + "/v1/collect/compartment/open", sendData.dump(), { "Authorization: " + authToken }, &openPaczkomatBuffer);
}

// terminator
void InPostAPI::TerminatePaczka(std::string uuid) {
    nlohmann::json sendData;
    sendData["sessionUuid"] = uuid;

    terminatePaczkaBuffer.data.clear();
    terminatePaczkaBuffer.status = InProgress;
    terminatePaczkaBuffer.code = 0;
    Request::QueueRequest(baseUrl + "/v1/collect/compartment/terminate", sendData.dump(), { "Authorization: " + authToken }, &terminatePaczkaBuffer);
}