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
    std::string name, date, internalDate;
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
            pickupCode,
            qrCode,
            imageUrl,
            senderName;

    float lat, lon;

    bool courier, openable, delivered;

    std::vector<PackageEvent> events;
};

namespace InPostAPI{
    void SendSMSCode(std::string phone);
    void VerifySMSCode(std::string phone, std::string code);
    void GetPaczkas();
    void GetPaczkomatStatus(std::string shipmentNumber, std::string openCode, std::string receiverPhoneNumber, std::string receiverPhonePrefix, float lat, float lon);
    void OpenPaczkomat(std::string uuid);
    void TerminatePaczka(std::string uuid);
    void GetAccountInfo();
    std::string GetAccountName(std::string json);
    bool ParsePaczkas(std::string json);
    bool LoadTokens();
    inline ResponseBuffer
    sendSMSCodeBuffer,
    verifySMSCodeBuffer,
    getAccountInfoBuffer,
    getPaczkasBuffer,
    getPaczkomatStatusBuffer,
    openPaczkomatBuffer,
    terminatePaczkaBuffer,
    lockerStatusBuffer;
    inline std::vector<Package> packages, packageArchive;
    inline std::string refreshToken;
    inline std::string authToken;
    inline std::string baseUrl = std::string(BASE_URL);
};


#endif //SWITCHPOST_INPOSTAPI_H
