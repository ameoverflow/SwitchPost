//
// Created by void on 23/03/2026.
//

#ifndef SWITCHPOST_INPOSTAPI_H
#define SWITCHPOST_INPOSTAPI_H

#include <string>
#include "Request.h"
#include <vector>
#include "curl/curl.h"

struct PackageEvent {
    std::string name, date;
};

struct Package {
    std::string
            number,
            status,
            statusGroup,
            parcelSize,
            storedDate,
            pickupDate,
            pickupPointName,
            city,
            street,
            phonePrefix,
            phoneNumber,
            code,
            qrCode,
            imageUrl,
            senderName;

    float lat, lon;

    bool courier, openable, delivered;

    std::vector<PackageEvent> events;
};

class InpostAPI{
public:
    static void SendSMSCode(std::string phone);
    static void VerifySMSCode(std::string phone, std::string code);
    static void GetPaczkas();
    static void GetWyslanedPaczkas(std::string token);
    static void GetReturnedPaczkas(std::string token);
    static void GetPaczkomatStatus(std::string shipmentNumber, std::string openCode, std::string receiverPhoneNumber, std::string receiverPhonePrefix, float lat, float lon);
    static void OpenPaczkomat(std::string uuid);
    static void TerminatePaczka(std::string uuid);
    static void LockerStatus(std::string token, std::string uuid, bool opened);
    static void GetPaczkomatImage(std::string url);
    static bool ParsePaczkas(std::string json);
    static bool LoadTokens();
    static std::shared_ptr<ResponseBuffer>
    sendSMSCodeBuffer,
    verifySMSCodeBuffer,
    getPaczkasBuffer,
    getWyslanedPaczkasBuffer,
    getReturnedPaczkasBuffer,
    getPaczkomatStatusBuffer,
    openPaczkomatBuffer,
    terminatePaczkaBuffer,
    lockerStatusBuffer;
    static std::vector<Package> packages;
    static std::string refreshToken;
    static std::string authToken;
    static bool RefreshTokenSync();
    static std::string baseUrl;
private:
    static std::string FormatIsoToCustom(const std::string& iso_date);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static std::unique_ptr<std::vector<char>> refreshTokenBuffer;
};


#endif //SWITCHPOST_INPOSTAPI_H
