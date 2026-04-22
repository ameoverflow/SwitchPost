#include "SceneMain.h"

#include "raylib.h"
#include "tween.h"
#include "tweeny.h"
#include "Helpers.h"
#include <iostream>
#include "switch.h"
#include <string>
#include "InpostAPI.h"
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>
#include "json.hpp"
#include "SceneManager.h"
#include "AssetLoader.h"
#include "Config.h"
#include "SceneReloadMain.h"
#include "qrcodegen.h"

void SceneMain::ReloadScene() {
    ResetRemoteLockerData();
    inputLock = true;
    inConfirmClosed = false;
    inQR = false;
    UnloadTexture(qrCode);
    ResetRemoteLockerData();
    SceneManager::ChangeScene(std::make_unique<SceneReloadMain>());
}

void SceneMain::ResetRemoteLockerData() {
    InpostAPI::getPaczkomatStatusBuffer->status = NotStarted;
    InpostAPI::openPaczkomatBuffer->status = NotStarted;
    InpostAPI::terminatePaczkaBuffer->status = NotStarted;
}

Texture2D SceneMain::GenerateQrTexture(const char* qrData) {
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

    if (!qrData || strlen(qrData) == 0) return { 0 };

    bool ok = qrcodegen_encodeText(qrData, tempBuffer, qrcode, qrcodegen_Ecc_MEDIUM,
                                   qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
                                   qrcodegen_Mask_AUTO, true);
    if (!ok) return { 0 };

    int qrSize = qrcodegen_getSize(qrcode);
    int scale = 15; // how big each module is in the texture
    int texSize = qrSize * scale;

    int padding = scale * 4;
    int finalSize = texSize + (padding * 2);

    // create a white image
    Image img = GenImageColor(finalSize, finalSize, WHITE);

    for (int y = 0; y < qrSize; y++) {
        for (int x = 0; x < qrSize; x++) {
            if (qrcodegen_getModule(qrcode, x, y)) {
                // draw a black block onto the image
                ImageDrawRectangle(&img, padding + (x * scale), padding + (y * scale), scale, scale, BLACK);
            }
        }
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img); // free the cpu ram, it's on the gpu now
    return tex;
}

void SceneMain::SceneInit() {
    poststamp = LoadTexture(AssetLoader::ResolveResource("sprites/znaczek.png").c_str());
    package = LoadTexture(AssetLoader::ResolveResource("sprites/paczka.png").c_str());
    selectorCorner = LoadTexture(AssetLoader::ResolveResource("sprites/paczka_selector_corner.png").c_str());
    loadingCircle = LoadTexture(AssetLoader::ResolveResource("sprites/loading_circle.png").c_str());
    promptY = LoadTexture(AssetLoader::ResolveResource("sprites/prompts/Switch_Y.png").c_str());
    promptX = LoadTexture(AssetLoader::ResolveResource("sprites/prompts/Switch_X.png").c_str());
    reloadButton = LoadTexture(AssetLoader::ResolveResource("sprites/refresh.png").c_str());
    openButton = LoadTexture(AssetLoader::ResolveResource("sprites/open_button.png").c_str());
    delivered = LoadTexture(AssetLoader::ResolveResource("sprites/delivered.png").c_str());
    readyForPickup = LoadTexture(AssetLoader::ResolveResource("sprites/ready_for_pickup.png").c_str());
    change = LoadSound(AssetLoader::ResolveResource("sounds/change.wav").c_str());
    mainFont = LoadFontEx("romfs:/fonts/Ubuntu-Regular.ttf", 42, 0, 381);
    packageDetails = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    std::string voice = Config::GetProperty("voice");
    if (voice != "none" && !voice.empty() && std::filesystem::exists(AssetLoader::ResolveResource("voice/" + voice + "/confirm_closed.ogg"))) {
        confirmClosed = LoadSound(AssetLoader::ResolveResource("voice/" + voice + "/confirm_closed.ogg").c_str());
    }

    if (voice != "none" && !voice.empty() && std::filesystem::exists(AssetLoader::ResolveResource("voice/" + voice + "/confirm_open.ogg"))) {
        confirmOpen = LoadSound(AssetLoader::ResolveResource("voice/" + voice + "/confirm_open.ogg").c_str());
    }

    // poststamp drop in anim
    poststampFade = tweeny::from((float)-poststamp.height).to(0).during(500).via(tweeny::easing::backOut);

    // packages on bottom of the screen rise in
    packagesFade = tweeny::from(820.0f).to(500.0f).during(500).via(tweeny::easing::backOut);

    // fade in from loading screen
    loadingFade = tweeny::from(1.0f).to(0.0f).during(100);

    // details fade in
    detailsFade = tweeny::from(0.0f).to(1.0f).during(400).via(tweeny::easing::sinusoidalInOut);

    // details scroll up from middle
    detailsScrollUp = tweeny::from(360.0f).to(0.0f).during(400).via(tweeny::easing::sinusoidalInOut);

    // nice selector pulsating anim
    selectorFadePulse = tweeny::from(0.0f).to(5.0f).during(1000).via(tweeny::easing::sinusoidalInOut);

    loadingFade.seek(0);
    poststampFade.seek(0);
    packagesFade.seek(0);
    detailsFade.seek(0);
    detailsScrollUp.seek(0);
    selectorFadePulse.seek(0);
}

void SceneMain::SceneUpdate(float dt) {
    spinnerRotation += 180 * dt;
    if (errorCode != None) {
        switch (errorCode) {
            case NetworkError:
                errorDesc = "Błąd sieci";
                break;
            case JSONError:
                errorDesc = "Błąd danych";
                break;
        }

        return;
    }

    if (isLoaded) {
        if (inDetails)
        {
            float drawOffset = 10;

            // --- render detail mode ---
            {
                BeginTextureMode(packageDetails);

                DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(),
                    {255, 255, 255, 255}, {255, 255, 140, 255});
                if (InpostAPI::packages[selectedPackage].openable) {
                    DrawTextPro(mainFont, "Kod odbioru", {20, scrollOffset + drawOffset}, {0, 0}, 0, 32, 0, GRAY);
                    drawOffset += 37;
                    DrawTextPro(mainFont, InpostAPI::packages[selectedPackage].code.c_str(), {20, scrollOffset + drawOffset}, {0, 0}, 0, 50, 0, BLACK);
                    drawOffset += 55;
                    DrawRectangle(20, scrollOffset + drawOffset, GetScreenWidth() - 40, 5, {255, 204, 0, 255});
                    drawOffset += 20;

                    if (!InpostAPI::packages[selectedPackage].courier) {
                        DrawTexturePro(promptY, {0, 0, 100, 100}, {(float)GetScreenWidth() - 180, scrollOffset + 10, 80, 80}, {0, 0}, 0, WHITE);
                        DrawRectangle(GetScreenWidth() - 90, scrollOffset + 10, 80, 80, {255, 170, 0, 255});
                        Vector2 textSize = MeasureTextEx(mainFont, "QR", 50, 0);
                        DrawTextPro(mainFont, "QR", {(float)GetScreenWidth() - 50, scrollOffset + 50}, {textSize.x / 2, textSize.y / 2}, 0, 50, 0, WHITE);
                    }
                }
                drawOffset += 20;
                Vector2 textSize = MeasureTextEx(mainFont, "Historia zdarzeń", 50, 2);
                DrawTextPro(mainFont, "Historia zdarzeń", {(float)GetScreenWidth()/2, scrollOffset + drawOffset},
            {textSize.x/2, textSize.y/2}, 0, 50, 2, BLACK);
                drawOffset += 32;
                for (PackageEvent event : InpostAPI::packages[selectedPackage].events) {
                    DrawTextPro(mainFont, event.date.c_str(), {20, scrollOffset + drawOffset}, {0, 0}, 0, 32, 0, GRAY);
                    drawOffset += 37;
                    DrawTextPro(mainFont, event.name.c_str(), {20, scrollOffset + drawOffset}, {0, 0}, 0, 32, 0, BLACK);
                    drawOffset += 47;
                    DrawLine(20, scrollOffset + drawOffset, GetScreenWidth() - 20, scrollOffset + drawOffset, GRAY);
                    drawOffset += 10;
                }

                EndTextureMode();
            }

            // --- handle detail mode input ---
            {
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)
                && !inQR && !inOpenPaczkomat && !inConfirmClosed && !inputLock) {
                    detailsFade.backward();
                    detailsScrollUp.backward();
                    inDetails = false;
                }

                if (!inQR && InpostAPI::packages[selectedPackage].openable &&
                    IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) {
                    qrCode = GenerateQrTexture(InpostAPI::packages[selectedPackage].qrCode.c_str());
                    inQR = qrCode.id != 0;
                    }

                if (inQR && !inOpenPaczkomat && !inConfirmClosed && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                    UnloadTexture(qrCode);
                    inQR = false;
                }

                if (inQR && !inOpenPaczkomat && !inConfirmClosed && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP) && InpostAPI::getPaczkomatStatusBuffer->status == NotStarted) {
                    InpostAPI::GetPaczkomatStatus(
                            InpostAPI::packages[selectedPackage].number,
                            InpostAPI::packages[selectedPackage].code,
                            InpostAPI::packages[selectedPackage].phoneNumber,
                            InpostAPI::packages[selectedPackage].phonePrefix,
                            InpostAPI::packages[selectedPackage].lat,
                            InpostAPI::packages[selectedPackage].lon
                            );
                }
            }

            // --- handle touch ---
            {
                if (!inQR && InpostAPI::packages[selectedPackage].openable && GetTouchPointCount() > 0 && !screenTouched) {
                    Vector2 touchPoint = GetTouchPosition(0);
                    screenTouched = true;
                    if (touchPoint.x >= GetScreenWidth() - 90 && touchPoint.y >= scrollOffset + 10 &&
                        touchPoint.x <= GetScreenWidth() - 90 + 80 && touchPoint.y <= scrollOffset + 10 + 80) {
                        qrCode = GenerateQrTexture(InpostAPI::packages[selectedPackage].qrCode.c_str());
                        inQR = qrCode.id != 0;
                        }
                }

                // DrawTextureEx(openButton, {GetScreenWidth()/2 - openButton.width/2, 40 + qrCode.height}, 0, 1, WHITE);
                if (inQR && !inOpenPaczkomat && !inConfirmClosed && GetTouchPointCount() > 0 && !screenTouched) {
                    Vector2 touchPoint = GetTouchPosition(0);
                    screenTouched = true;
                    if (touchPoint.x >= GetScreenWidth()/2 - openButton.width/2 && touchPoint.y >= 40 + qrCode.height &&
                        touchPoint.x <= GetScreenWidth()/2 - openButton.width/2 + openButton.width && touchPoint.y <= 40 + qrCode.height + openButton.height) {
                        InpostAPI::GetPaczkomatStatus(
                                InpostAPI::packages[selectedPackage].number,
                                InpostAPI::packages[selectedPackage].code,
                                InpostAPI::packages[selectedPackage].phoneNumber,
                                InpostAPI::packages[selectedPackage].phonePrefix,
                                InpostAPI::packages[selectedPackage].lat,
                                InpostAPI::packages[selectedPackage].lon
                        );
                    }
                }

                if (GetTouchPointCount() == 0) screenTouched = false;

                if (drawOffset <= 710 || inQR) { // 720 minus that 10px margin
                    scrollOffset = 0;
                } else {
                    // down
                    if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y) < -0.1f) {
                        scrollOffset -= GetMappedAxis(std::abs(GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y)), 700) * dt;
                    }

                    // up
                    if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y) > 0.1f) {
                        scrollOffset += GetMappedAxis(GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y), 700) * dt;
                    }

                    if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) scrollOffset -= 600 * dt;
                    if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) scrollOffset += 600 * dt;

                    static Vector2 previousTouch = { 0 };
                    static bool isTouching = false;

                    if (GetTouchPointCount() > 0) {
                        Vector2 currentTouch = GetTouchPosition(0);

                        if (isTouching) {
                            scrollOffset += (currentTouch.y - previousTouch.y);
                        }

                        previousTouch = currentTouch;
                        isTouching = true;
                    } else {
                        isTouching = false;
                        previousTouch = (Vector2){ 0, 0 };
                    }

                    float scrollMaxOffset = -(drawOffset - 720 + 10);
                    if (scrollOffset < scrollMaxOffset) scrollOffset = scrollMaxOffset;
                    if (scrollOffset > 0) scrollOffset = 0;
                }
            }

            // handle qr mode and requests
            {
                if (inQR && inOpenPaczkomat && !inConfirmClosed && InpostAPI::openPaczkomatBuffer->status == NotStarted && !inputLock) {
                    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                        inOpenPaczkomat = false;
                        ResetRemoteLockerData();
                    } else if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                        InpostAPI::OpenPaczkomat(sessionUuid);
                    }
                }

                if (inQR && !inOpenPaczkomat && !inConfirmClosed && InpostAPI::getPaczkomatStatusBuffer->status == Done) {
                    if (InpostAPI::getPaczkomatStatusBuffer->code == 200) {
                        std::string rawData = std::string(InpostAPI::getPaczkomatStatusBuffer->data.begin(), InpostAPI::getPaczkomatStatusBuffer->data.end());
                        if (nlohmann::json::accept(rawData)) {
                            nlohmann::json statusData = nlohmann::json::parse(rawData);
                            if (statusData.contains("sessionUuid") && !statusData["sessionUuid"].is_null()) {
                                sessionUuid = statusData["sessionUuid"].get<std::string>();
                                inOpenPaczkomat = true;
                                if (confirmOpen.frameCount > 0) {
                                    PlaySound(confirmOpen);
                                }
                            } else {
                                SPDLOG_ERROR("failed to parse session data for parcel {}", InpostAPI::packages[selectedPackage].number);
                                ResetRemoteLockerData();
                            }
                        } else {
                            SPDLOG_ERROR("failed to start open session parcel {}", InpostAPI::packages[selectedPackage].number);
                            SPDLOG_ERROR("http code: {}", InpostAPI::getPaczkomatStatusBuffer->code);
                            ResetRemoteLockerData();
                        }
                    }
                }

                if (inQR && inOpenPaczkomat && !inConfirmClosed && InpostAPI::openPaczkomatBuffer->status == Done) {
                    if (InpostAPI::openPaczkomatBuffer->code == 200) {
                        inOpenPaczkomat = false;
                        inConfirmClosed = true;
                        if (confirmClosed.frameCount > 0) {
                            PlaySound(confirmClosed);
                        }
                    } else {
                        SPDLOG_ERROR("failed to open locker for parcel {}", InpostAPI::packages[selectedPackage].number);
                        SPDLOG_ERROR("http code: {}", InpostAPI::openPaczkomatBuffer->code);
                        inOpenPaczkomat = false;
                        ResetRemoteLockerData();
                    }
                }

                if (inQR && !inOpenPaczkomat && inConfirmClosed && InpostAPI::terminatePaczkaBuffer->status == NotStarted && !inputLock) {
                    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                        inConfirmClosed = false;
                        ResetRemoteLockerData();
                    } else if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                        InpostAPI::TerminatePaczka(sessionUuid);
                    }
                }

                if (inQR && !inOpenPaczkomat && inConfirmClosed && InpostAPI::terminatePaczkaBuffer->status == Done) {
                    if (InpostAPI::terminatePaczkaBuffer->code != 200) {
                        SPDLOG_ERROR("failed to terminate session for parcel {}", InpostAPI::packages[selectedPackage].number);
                        SPDLOG_ERROR("http code: {}", InpostAPI::terminatePaczkaBuffer->code);
                    }
                    ReloadScene();
                }
            }
        }

        // --- handle camera ---
        {
            float selectorWorldX = 5.0f + (package.width + 40) * selectedPackage;
            float selectorWidth = 250;
            float padding = 5;

            if (selectorWorldX - targetOffset + selectorWidth + padding > GetScreenWidth()) {
                targetOffset = selectorWorldX + selectorWidth - GetScreenWidth() + padding;
            }
            else if (selectorWorldX - targetOffset < padding) {
                targetOffset = selectorWorldX - padding;
            }

            if (std::fabs((targetOffset - cameraOffset) * 10.0f * dt) <= 0.1f)
                cameraOffset = targetOffset;

            cameraOffset += (targetOffset - cameraOffset) * 10.0f * dt;
        }

        // --- handle input ---
        {
            float currentStickValue = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);

            if (currentStickValue < -0.5f && !stickMoved &&
            selectedPackage > 0 && std::size(InpostAPI::packages) && !inputLock && !inDetails) {
                stickMoved = true;
                selectorFadePulse.forward();
                selectorFadePulse.seek(0);
                selectedPackage--;
                PlaySound(change);
            }

            if (currentStickValue > 0.5f && !stickMoved &&
            selectedPackage < std::size(InpostAPI::packages) - 1 && std::size(InpostAPI::packages) && !inputLock && !inDetails) {
                stickMoved = true;
                selectorFadePulse.forward();
                selectorFadePulse.seek(0);
                selectedPackage++;
                PlaySound(change);
            }

            if (currentStickValue > -0.3f && currentStickValue < 0.3f) {
                stickMoved = 0;
            }

            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) &&
            selectedPackage < std::size(InpostAPI::packages) - 1 && std::size(InpostAPI::packages)
            && !inDetails && !inputLock) {
                selectorFadePulse.forward();
                selectorFadePulse.seek(0);
                selectedPackage++;
                PlaySound(change);
            }

            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)
                && selectedPackage > 0 && std::size(InpostAPI::packages) > 0
                && !inDetails && !inputLock) {
                selectorFadePulse.forward();
                selectorFadePulse.seek(0);
                selectedPackage--;
                PlaySound(change);
                }

            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)
            && !inDetails && !inputLock && std::size(InpostAPI::packages) > 0) {
                scrollOffset = 0;
                detailsFade.forward();
                detailsScrollUp.forward();
                inDetails = true;
            }

            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP) && !inDetails && !inputLock) {
                inputLock = true;
                SceneManager::ChangeScene(std::make_unique<SceneReloadMain>());
                return;
            }
        }

        // --- handle animations ---
        {
            if (poststampFade.progress() < 1.0f)
                poststampFade.step((int)(dt * 1000));

            if (poststampFade.progress() >= 0.3f && packagesFade.progress() < 1.0f)
                packagesFade.step((int)(dt * 1000));

            if (inDetails) {
                if (detailsFade.progress() < 1.0f)
                    detailsFade.step((int)(dt * 1000));
                if (detailsScrollUp.progress() < 1.0f)
                    detailsScrollUp.step((int)(dt * 1000));
            } else {
                if (detailsFade.progress() > 0.0f)
                    detailsFade.step((int)(dt * 1000));
                if (detailsScrollUp.progress() > 0.0f)
                    detailsScrollUp.step((int)(dt * 1000));
            }

            selectorFadePulse.step((int)(dt * 1000.0f));
            if (selectorFadePulse.progress() >= 1.0f && selectorFadePulse.direction() == 1)
            {
                selectorFadePulse.backward();
            } else if (selectorFadePulse.progress() <= 0.0f && selectorFadePulse.direction() == -1) {
                selectorFadePulse.forward();
            }
        }
    } else {
        if (!tokensLoaded) {
            if (!InpostAPI::LoadTokens()) {
                errorCode = JSONError;
                return;
            } else {
                tokensLoaded = true;
            }
        }

        if (showFakePackages) {
            std::ifstream fakePackages("romfs:/text/test_data.json");
            if (fakePackages.is_open()) {
                std::stringstream buffer;
                buffer << fakePackages.rdbuf();
                if (nlohmann::json::accept(buffer.str())) {
                    if (!InpostAPI::ParsePaczkas(buffer.str())) {
                        errorCode = JSONError;
                    } else {
                        isLoaded = true;
                    }
                } else {
                    SPDLOG_ERROR("failed to load fake packages");
                    SPDLOG_DEBUG("{}", buffer.str());
                    errorCode = JSONError;
                    return;
                }
            } else {
                errorCode = JSONError;
                return;
            }
            fakePackages.close();
        } else {
            // if data is already loaded, then skip the whole shit
            if (InpostAPI::getPaczkasBuffer->status == NotStarted) {
                InpostAPI::GetPaczkas();
            } else if (InpostAPI::getPaczkasBuffer->status == Done && InpostAPI::getPaczkasBuffer->code == 200) {
                if (InpostAPI::ParsePaczkas(std::string(InpostAPI::getPaczkasBuffer->data.begin(), InpostAPI::getPaczkasBuffer->data.end()))) {
                    isLoaded = true;
                } else {
                    errorCode = JSONError;
                    return;
                }
            } else if (InpostAPI::getPaczkasBuffer->status == Error || (InpostAPI::getPaczkasBuffer->status == Done && InpostAPI::getPaczkasBuffer->code != 200)) {
                errorCode = NetworkError;
                return;
            }
        }
    }
}

void SceneMain::SceneDraw() {
    if (isLoaded) {
        if (!inQR) {
            Rectangle source = {0.0f, 0.0f, (float) poststamp.width, (float) poststamp.height};
            Vector2 screenPos = {(float) GetScreenWidth() / 2.0f, 0};
            Rectangle dest = {screenPos.x, poststampFade.peek(), (float) poststamp.width * 2,
                              (float) poststamp.height * 2};
            Vector2 origin = {(float) poststamp.width, 0};
            DrawTexturePro(poststamp, source, {dest.x + 3, dest.y + 3, dest.width, dest.height}, origin, 0.0f,
                           {0, 0, 0, 100});
            DrawTexturePro(poststamp, source, dest, {origin.x + 3, 3}, 0.0f, WHITE);
            for (int i = 0; i < InpostAPI::packages.size(); i++) {
                DrawTextureEx(package,
                              {40.0f + ((float) package.width + 40) * i - cameraOffset + 6, packagesFade.peek() + 6}, 0,
                              1, {0, 0, 0, 100});
                DrawTextureEx(package, {40.0f + ((float) package.width + 40) * i - cameraOffset, packagesFade.peek()},
                              0, 1, WHITE);

                if (InpostAPI::packages[i].openable) {
                    DrawTextureEx(readyForPickup,
                              {40.0f + ((float) package.width + 40) * i - cameraOffset + 3, packagesFade.peek() + 3}, 0,
                              1, {0, 0, 0, 100});
                    DrawTextureEx(readyForPickup, {40.0f + ((float) package.width + 40) * i - cameraOffset, packagesFade.peek()},
                                  0, 1, WHITE);
                } else if (InpostAPI::packages[i].delivered) {
                    DrawTextureEx(delivered,
                              {40.0f + ((float) package.width + 40) * i - cameraOffset + 3, packagesFade.peek() + 3}, 0,
                              1, {0, 0, 0, 100});
                    DrawTextureEx(delivered, {40.0f + ((float) package.width + 40) * i - cameraOffset, packagesFade.peek()},
                                  0, 1, WHITE);
                }
            }

            if (InpostAPI::packages.size() > 0){
                //DrawTextureEx(selector, { 5.0f + ((float)package.width + 40) * selectedPackage - cameraOffset, packagesFade.peek() - 30.0f}, 0, 1, WHITE);
                float x = 5.0f + ((float)package.width + 40) * selectedPackage - cameraOffset;
                float y = packagesFade.peek() - 30.0f;

                DrawTextureEx(selectorCorner, { x - selectorFadePulse.peek(), y - selectorFadePulse.peek() }, 0.0f, 1.0f, WHITE);
                DrawTextureEx(selectorCorner, { x + 250.0f + selectorFadePulse.peek(), y - selectorFadePulse.peek() }, 90.0f, 1.0f, WHITE);
                DrawTextureEx(selectorCorner, { x + 250.0f + selectorFadePulse.peek(), y + 228.0f + selectorFadePulse.peek() }, 180.0f, 1.0f, WHITE);
                DrawTextureEx(selectorCorner, { x - selectorFadePulse.peek(), y + 228.0f + selectorFadePulse.peek() }, 270.0f, 1.0f, WHITE);
            }

            DrawTextureEx(promptX, {5.0f, packagesFade.peek() - 90.0f}, 0, 0.5f, WHITE);
            DrawTextureEx(reloadButton, {60.0f, packagesFade.peek() - 90.0f}, 0, 1, WHITE);

            if (InpostAPI::packages.size() == 0) {
                Vector2 textSize = MeasureTextEx(mainFont, "Brak paczek :c", 42, 1);
                DrawTextOutlineEx(mainFont, "Brak paczek :c", { (float)GetScreenWidth() / 2, poststampFade.peek() + poststamp.height }, {textSize.x / 2, textSize.y / 2}, 42, 0, WHITE, BLACK, 4);
            } else {
                Vector2 textSize = MeasureTextEx(mainFont, InpostAPI::packages[selectedPackage].events[0].name.c_str(), 32, 1);
                DrawTextOutlineEx(mainFont, InpostAPI::packages[selectedPackage].events[0].name.c_str(), {(float)GetScreenWidth()/2, poststampFade.peek() + 170}, {textSize.x/2, textSize.y/2}, 32, 0, WHITE, BLACK, 2);

                std::string paczkaCounter;
                paczkaCounter += std::to_string(selectedPackage + 1);
                paczkaCounter += " / ";
                paczkaCounter += std::to_string(InpostAPI::packages.size());
                textSize = MeasureTextEx(mainFont, paczkaCounter.c_str(), 28, 1);
                DrawTextOutlineEx(mainFont, paczkaCounter.c_str(), {(float)GetScreenWidth()/2 - 190, poststampFade.peek() + 100}, {textSize.x/2, textSize.y/2}, 28, 1, WHITE, BLACK, 2);

                textSize = MeasureTextEx(mainFont, InpostAPI::packages[selectedPackage].events[0].date.c_str(), 32, 0);
                DrawTextOutlineEx(mainFont, InpostAPI::packages[selectedPackage].events[0].date.c_str(), {(float)GetScreenWidth()/2, poststampFade.peek() + 120}, {textSize.x/2, textSize.y/2}, 32, 0, WHITE, BLACK, 2);

                if (InpostAPI::packages[selectedPackage].courier) {
                    textSize = MeasureTextEx(mainFont, "Paczka kurierska", 40, 0);
                    DrawTextOutlineEx(mainFont, "Paczka kurierska", {(float)GetScreenWidth()/2, poststampFade.peek() + poststamp.height}, {textSize.x/2, textSize.y/2}, 40, 0, WHITE, BLACK, 2);
                } else {
                    textSize = MeasureTextEx(mainFont, InpostAPI::packages[selectedPackage].pickupPointName.c_str(), 40, 0);
                    DrawTextOutlineEx(mainFont, InpostAPI::packages[selectedPackage].pickupPointName.c_str(), {(float)GetScreenWidth()/2, poststampFade.peek() + poststamp.height}, {textSize.x/2, textSize.y/2}, 40, 0, WHITE, BLACK, 2);

                    textSize = MeasureTextEx(mainFont, std::string(InpostAPI::packages[selectedPackage].street + ", " + InpostAPI::packages[selectedPackage].city).c_str(), 32, 0);
                    DrawTextOutlineEx(mainFont, std::string(InpostAPI::packages[selectedPackage].street + ", " + InpostAPI::packages[selectedPackage].city).c_str(), {(float)GetScreenWidth()/2, poststampFade.peek() + poststamp.height + 30}, {textSize.x/2, textSize.y/2}, 32, 0, WHITE, BLACK, 2);
                }
                textSize = MeasureTextEx(mainFont, InpostAPI::packages[selectedPackage].senderName.c_str(), 28, 0);
                DrawTextOutlineEx(mainFont, InpostAPI::packages[selectedPackage].senderName.c_str(), {(float)GetScreenWidth()/2, poststampFade.peek() + poststamp.height + 95}, {textSize.x/2, textSize.y/2}, 28, 0, WHITE, BLACK, 2);

                DrawTexturePro(packageDetails.texture, {0, 0, (float)packageDetails.texture.width, (float)-packageDetails.texture.height},
                   (Rectangle){0, detailsScrollUp.peek(), (float)GetScreenWidth(), (float)GetScreenHeight()},
                   {0, 0}, 0, ColorAlpha(WHITE, detailsFade.peek()));
            }
        } else {
            DrawTexturePro(qrCode, {0, 0, (float)qrCode.width, (float)qrCode.height},
                {(float)GetScreenWidth()/2, 20, (float)qrCode.width, (float)qrCode.height},
                {(float)qrCode.width/2, 0}, 0, WHITE);

            DrawTextureEx(openButton, {GetScreenWidth()/2 - openButton.width/2, 40 + qrCode.height}, 0, 1, WHITE);
            DrawTextureEx(promptX, {GetScreenWidth()/2 - openButton.width/2 - 10 - promptX.width, 35 + qrCode.height}, 0, 1, WHITE);

            if (inOpenPaczkomat || inConfirmClosed) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 192});
            }

            if (inOpenPaczkomat) {
                Vector2 textSize = MeasureTextEx(mainFont, "Czy chcesz otworzyć Paczkomat?\n\n(A) Tak      (B) Nie", 32, 0);
                DrawRectangle(GetScreenWidth()/2 - textSize.x/2 - 50, GetScreenHeight()/2 - textSize.y/2 - 50, textSize.x + 100, textSize.y + 100, WHITE);
                DrawTextPro(mainFont, "Czy chcesz otworzyć Paczkomat?\n\n(A) Tak      (B) Nie",
                    {GetScreenWidth()/2, GetScreenHeight()/2}, {textSize.x/2, textSize.y/2}, 0, 32, 0, BLACK);
            }

            if (inConfirmClosed) {
                Vector2 textSize = MeasureTextEx(mainFont, "Czy paczka została odebrana?\n\n(A) Tak      (B) Nie", 32, 0);
                DrawRectangle(GetScreenWidth()/2 - textSize.x/2 - 50, GetScreenHeight()/2 - textSize.y/2 - 50, textSize.x + 100, textSize.y + 100, WHITE);
                DrawTextPro(mainFont, "Czy paczka została odebrana?\n\n(A) Tak      (B) Nie",
                    {GetScreenWidth()/2, GetScreenHeight()/2}, {textSize.x/2, textSize.y/2}, 0, 32, 0, BLACK);
            }

            if (InpostAPI::getPaczkomatStatusBuffer->status == InProgress ||
                InpostAPI::openPaczkomatBuffer->status == InProgress ||
                InpostAPI::terminatePaczkaBuffer->status == InProgress) {

                Rectangle source = { 0.0f, 0.0f, (float)loadingCircle.width, (float)loadingCircle.height };
                Rectangle dest = { 1197, 637, (float)loadingCircle.width/2, (float)loadingCircle.height/2};
                Vector2 origin = { (float)loadingCircle.width/4.0f, (float)loadingCircle.height/4.0f};
                DrawTexturePro(loadingCircle, source, { dest.x, dest.y, dest.width, dest.height}, origin, spinnerRotation, {255, 255, 255, 100});
            }
        }
    } else {
        if (loadingFade.progress() == 0.0f && errorCode == None) {
            DrawRectangleGradientV(0, 0, 1320, 720, ColorAlpha(BLACK, loadingFade.peek()), ColorAlpha({10, 10, 10, 255}, loadingFade.peek()));
            Rectangle source = { 0.0f, 0.0f, (float)loadingCircle.width, (float)loadingCircle.height };
            Rectangle dest = { 1197, 637, (float)loadingCircle.width/2, (float)loadingCircle.height/2};
            Vector2 origin = { (float)loadingCircle.width/4.0f, (float)loadingCircle.height/4.0f};
            DrawTexturePro(loadingCircle, source, { dest.x, dest.y, dest.width, dest.height}, origin, spinnerRotation, {255, 255, 255, 100});

        } else if (errorCode != None) {
            DrawRectangleGradientV(0, 0, 1320, 720, BLACK, {64, 0, 0, 255});
            Vector2 textSize = MeasureTextEx(mainFont, errorDesc.c_str(), 50, 2);
            DrawTextOutlineEx(mainFont, errorDesc.c_str(), {1280/2, 720/2},
                              {textSize.x / 2.0f, textSize.y / 2.0f}, 50, 2, RED, BLACK, 3);
        } else if (loadingFade.progress() > 0.0f) {
            DrawRectangleGradientV(0, 0, 1320, 720, ColorAlpha(BLACK, loadingFade.peek()), ColorAlpha({10, 10, 10, 255}, loadingFade.peek()));
        }
    }
}

void SceneMain::SceneExit() {
    StopSound(confirmClosed);
    UnloadSound(confirmClosed);
    StopSound(confirmOpen);
    UnloadSound(confirmOpen);
    UnloadRenderTexture(packageDetails);
    StopSound(change);
    UnloadSound(change);
    UnloadFont(mainFont);
    UnloadTexture(package);
    UnloadTexture(poststamp);
    UnloadTexture(selectorCorner);
    UnloadTexture(loadingCircle);
    UnloadTexture(promptY);
    UnloadTexture(promptX);
    UnloadTexture(openButton);
    UnloadTexture(reloadButton);
    UnloadTexture(readyForPickup);
    UnloadTexture(delivered);
}