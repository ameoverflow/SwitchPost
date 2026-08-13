#include "SceneMain.h"

#include "raylib.h"
#include "tween.h"
#include "tweeny.h"
#include "Helpers.h"
#include <iostream>
#include "switch.h"
#include <string>
#include "InPostAPI.h"
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>
#include "json.hpp"
#include "SceneManager.h"
#include "AssetLoader.h"
#include "Config.h"
#include "i18n.h"
#include "MusicManager.h"
#include "qrcodegen.h"
#include "SceneLoading.h"
#include "SceneOptions.h"
#include "SoundManager.h"

void SceneMain::ReloadScene() {
    ResetRemoteLockerData();
    inputLock = true;
    inConfirmClosed = false;
    inQR = false;
    UnloadTexture(qrCode);
    ResetRemoteLockerData();
    SceneManager::ChangeScene(std::make_unique<SceneLoading>());
}

void SceneMain::ResetRemoteLockerData() {
    InPostAPI::getPaczkomatStatusBuffer.status = NotStarted;
    InPostAPI::openPaczkomatBuffer.status = NotStarted;
    InPostAPI::terminatePaczkaBuffer.status = NotStarted;
}

Texture2D SceneMain::GenerateQrTexture(const char* qrData) {
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

    if (!qrData || strlen(qrData) == 0) return { 0 };

    if (!qrcodegen_encodeText(qrData, tempBuffer, qrcode, qrcodegen_Ecc_MEDIUM,
                                   qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
                                   qrcodegen_Mask_AUTO, true)) return { 0 };

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


// yyyyhhhhh
std::string ToLowercase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return text;
}

void SceneMain::GenerateSenderNameRenderTexture() {
    if (std::size((*currentDisplay)) > 0) {
        std::string selectedPackageName = Config::openedFile.parcelNames[(*currentDisplay)[selectedPackage].number];
        Vector2 textSize;

        if (!selectedPackageName.empty()) {
            textSize = MeasureTextEx(mainFont, selectedPackageName.c_str(), 28, 0);
            // DrawTextOutlineEx(mainFont, selectedPackageName.c_str(), {(float)GetScreenWidth()/2, poststampFade.peek() + modeChangeAnim.peek() + poststamp.height + 95}, {textSize.x/2, textSize.y/2}, 28, 0, WHITE, BLACK, 2);
        } else {
            textSize = MeasureTextEx(mainFont, (*currentDisplay)[selectedPackage].senderName.c_str(), 28, 0);
            //DrawTextOutlineEx(mainFont, (*currentDisplay)[selectedPackage].senderName.c_str(), {(float)GetScreenWidth()/2, poststampFade.peek() + modeChangeAnim.peek() + poststamp.height + 95}, {textSize.x/2, textSize.y/2}, 28, 0, WHITE, BLACK, 2);
        }

        if (senderName.id > 0) UnloadRenderTexture(senderName);
        senderName = LoadRenderTexture(textSize.x + 4, textSize.y + 4);

        BeginTextureMode(senderName);
        ClearBackground(BLANK);
        if (!selectedPackageName.empty()) {
            DrawTextOutlineEx(mainFont, selectedPackageName.c_str(), {senderName.texture.width/2, senderName.texture.height/2}, {textSize.x/2, textSize.y/2}, 28, 0, WHITE, BLACK, 2);
        } else {
            DrawTextOutlineEx(mainFont, (*currentDisplay)[selectedPackage].senderName.c_str(), {senderName.texture.width/2, senderName.texture.height/2}, {textSize.x/2, textSize.y/2}, 28, 0, WHITE, BLACK, 2);
        }
        EndTextureMode();

        textScrollAnim = 0;
        textScrollDirection = true;
        textScrollAnimDelay = 5.0f;

        SPDLOG_DEBUG("generated render texture for string {}", selectedPackageName.empty() ? (*currentDisplay)[selectedPackage].senderName : selectedPackageName);
    }
}

void SceneMain::RenderRemoteOpenButton() {
    openButton = LoadRenderTexture(360, 80);
    Texture2D buttonTemplate = LoadTexture(AssetLoader::ResolveResource("sprites/button.png").c_str());
    Font buttonFont = LoadFontEx("romfs:/fonts/ComicHelvetic_Heavy.otf", 42, 0, 381);

    BeginTextureMode(openButton);
    DrawTexture(buttonTemplate, 0, 0, WHITE);
    Vector2 size = MeasureTextEx(buttonFont, i18n::GetString("main.remote.open_button").c_str(), 35, 4);
    DrawTextOutlineEx(buttonFont, i18n::GetString("main.remote.open_button").c_str(), {180, 40}, {size.x/2, size.y/2}, 35, 2, BLACK, WHITE, 4);
    EndTextureMode();

    UnloadTexture(buttonTemplate);
    UnloadFont(buttonFont);
}

void SceneMain::SceneInit() {
    poststamp = LoadTexture(AssetLoader::ResolveResource("sprites/znaczek.png").c_str());
    package = LoadTexture(AssetLoader::ResolveResource("sprites/paczka.png").c_str());
    selectorCorner = LoadTexture(AssetLoader::ResolveResource("sprites/paczka_selector_corner.png").c_str());
    loadingCircle = LoadTexture(AssetLoader::ResolveResource("sprites/loading_circle.png").c_str());
    promptY = LoadTexture(AssetLoader::ResolveResource("sprites/prompts/Switch_Y.png").c_str());
    promptX = LoadTexture(AssetLoader::ResolveResource("sprites/prompts/Switch_X.png").c_str());
    promptPlus = LoadTexture(AssetLoader::ResolveResource("sprites/prompts/Switch_Plus.png").c_str());
    promptMinus = LoadTexture(AssetLoader::ResolveResource("sprites/prompts/Switch_Minus.png").c_str());
    reloadButton = LoadTexture(AssetLoader::ResolveResource("sprites/refresh.png").c_str());
    renameButton = LoadTexture(AssetLoader::ResolveResource("sprites/rename.png").c_str());
    archiveButton = LoadTexture(AssetLoader::ResolveResource("sprites/archive.png").c_str());
    settingsButton = LoadTexture(AssetLoader::ResolveResource("sprites/settings.png").c_str());
    delivered = LoadTexture(AssetLoader::ResolveResource("sprites/delivered.png").c_str());
    readyForPickup = LoadTexture(AssetLoader::ResolveResource("sprites/ready_for_pickup.png").c_str());
    mainFont = LoadFontEx("romfs:/fonts/Ubuntu-Regular.ttf", 42, 0, 381);
    packageDetails = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    RenderRemoteOpenButton();

    std::string voice = Config::openedFile.voice;
    if (voice != "none" && !voice.empty() && std::filesystem::exists(AssetLoader::ResolveResource("voice/" + voice + "/confirm_closed.ogg"))) {
        confirmClosed = LoadSound(AssetLoader::ResolveResource("voice/" + voice + "/confirm_closed.ogg").c_str());
    }

    if (voice != "none" && !voice.empty() && std::filesystem::exists(AssetLoader::ResolveResource("voice/" + voice + "/confirm_open.ogg"))) {
        confirmOpen = LoadSound(AssetLoader::ResolveResource("voice/" + voice + "/confirm_open.ogg").c_str());
    }

    currentDisplay = &InPostAPI::packages;

    // poststamp drop in anim
    poststampFade = tweeny::from((float)-poststamp.height).to(0).during(500).via(tweeny::easing::backOut);

    // packages on bottom of the screen rise in
    packagesFade = tweeny::from(820.0f).to(500.0f).during(500).via(tweeny::easing::backOut);

    // details fade in
    detailsFade = tweeny::from(0.0f).to(1.0f).during(400).via(tweeny::easing::sinusoidalInOut);

    // details scroll up from middle
    detailsScrollUp = tweeny::from(360.0f).to(0.0f).during(400).via(tweeny::easing::sinusoidalInOut);

    // nice selector pulsating anim
    selectorFadePulse = tweeny::from(0.0f).to(5.0f).during(1000).via(tweeny::easing::sinusoidalInOut);

    modeChangeAnim = tweeny::from(0.0f).to(-12.5f).during(100).via(tweeny::easing::sinusoidalInOut);

    poststampFade.seek(0);
    packagesFade.seek(0);
    detailsFade.seek(0);
    detailsScrollUp.seek(0);
    selectorFadePulse.seek(0);
    modeChangeAnim.seek(0);

    GenerateSenderNameRenderTexture();

    offlineMode = !IsConnected();
}

void SceneMain::SceneUpdate(float dt) {
    if (!loadingDone) {
        loadingDone = true;
        return;
    }

    spinnerRotation += 180 * dt;
    if (inDetails) {
        float drawOffset = 10;

        // --- render detail mode ---
        {
            BeginTextureMode(packageDetails);

            DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(),
                {255, 255, 255, 255}, {255, 255, 140, 255});
            if ((*currentDisplay)[selectedPackage].openable) {
                DrawTextPro(mainFont, i18n::GetString("main.history.code").c_str(), {20, scrollOffset + drawOffset}, {0, 0}, 0, 32, 0, GRAY);
                drawOffset += 37;
                DrawTextPro(mainFont, (*currentDisplay)[selectedPackage].pickupCode.c_str(), {20, scrollOffset + drawOffset}, {0, 0}, 0, 50, 0, BLACK);
                drawOffset += 55;
                DrawRectangle(20, scrollOffset + drawOffset, GetScreenWidth() - 40, 5, {255, 204, 0, 255});
                drawOffset += 20;

                if (!(*currentDisplay)[selectedPackage].courier) {
                    DrawTexturePro(promptY, {0, 0, 100, 100}, {(float)GetScreenWidth() - 180, scrollOffset + 10, 80, 80}, {0, 0}, 0, WHITE);
                    DrawRectangle(GetScreenWidth() - 90, scrollOffset + 10, 80, 80, {255, 170, 0, 255});
                    Vector2 textSize = MeasureTextEx(mainFont, "QR", 50, 0);
                    DrawTextPro(mainFont, "QR", {(float)GetScreenWidth() - 50, scrollOffset + 50}, {textSize.x / 2, textSize.y / 2}, 0, 50, 0, WHITE);
                }
            }
            drawOffset += 25;
            Vector2 textSize = MeasureTextEx(mainFont, i18n::GetString("main.history").c_str(), 50, 2);
            DrawTextPro(mainFont, i18n::GetString("main.history").c_str(), {(float)GetScreenWidth()/2, scrollOffset + drawOffset},
        {textSize.x/2, textSize.y/2}, 0, 50, 2, BLACK);
            drawOffset += 32;
            for (PackageEvent event : (*currentDisplay)[selectedPackage].events) {
                DrawTextPro(mainFont, event.date.c_str(), {20, scrollOffset + drawOffset}, {0, 0}, 0, 32, 0, GRAY);
                drawOffset += 37;
                DrawTextPro(mainFont, i18n::GetString(ToLowercase(event.name)).c_str(), {20, scrollOffset + drawOffset}, {0, 0}, 0, 32, 0, BLACK);
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

            if (!inQR && (*currentDisplay)[selectedPackage].openable &&
                IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) {
                qrCode = GenerateQrTexture((*currentDisplay)[selectedPackage].qrCode.c_str());
                inQR = qrCode.id != 0;
                }

            if (inQR && !inOpenPaczkomat && !inConfirmClosed && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                UnloadTexture(qrCode);
                inQR = false;
            }

            if (inQR && !inOpenPaczkomat && !inConfirmClosed && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP) && InPostAPI::getPaczkomatStatusBuffer.status == NotStarted && !offlineMode) {
                InPostAPI::GetPaczkomatStatus(
                        (*currentDisplay)[selectedPackage].number,
                        (*currentDisplay)[selectedPackage].pickupCode,
                        (*currentDisplay)[selectedPackage].phoneNumber,
                        (*currentDisplay)[selectedPackage].phonePrefix,
                        (*currentDisplay)[selectedPackage].lat,
                        (*currentDisplay)[selectedPackage].lon
                        );
            }
        }

        // --- handle touch ---
        {
            if (!inQR && (*currentDisplay)[selectedPackage].openable && GetTouchPointCount() > 0 && !screenTouched) {
                Vector2 touchPoint = GetTouchPosition(0);
                screenTouched = true;
                if (touchPoint.x >= GetScreenWidth() - 90 && touchPoint.y >= scrollOffset + 10 &&
                    touchPoint.x <= GetScreenWidth() - 90 + 80 && touchPoint.y <= scrollOffset + 10 + 80) {
                    qrCode = GenerateQrTexture((*currentDisplay)[selectedPackage].qrCode.c_str());
                    inQR = qrCode.id != 0;
                    }
            }

            // DrawTextureEx(openButton, {GetScreenWidth()/2 - openButton.width/2, 40 + qrCode.height}, 0, 1, WHITE);
            if (inQR && !inOpenPaczkomat && !inConfirmClosed && GetTouchPointCount() > 0 && !screenTouched) {
                Vector2 touchPoint = GetTouchPosition(0);
                screenTouched = true;
                if (touchPoint.x >= GetScreenWidth()/2 - openButton.texture.width/2 && touchPoint.y >= 40 + qrCode.height &&
                    touchPoint.x <= GetScreenWidth()/2 - openButton.texture.width/2 + openButton.texture.width && touchPoint.y <= 40 + qrCode.height + openButton.texture.height) {
                    InPostAPI::GetPaczkomatStatus(
                            (*currentDisplay)[selectedPackage].number,
                            (*currentDisplay)[selectedPackage].pickupCode,
                            (*currentDisplay)[selectedPackage].phoneNumber,
                            (*currentDisplay)[selectedPackage].phonePrefix,
                            (*currentDisplay)[selectedPackage].lat,
                            (*currentDisplay)[selectedPackage].lon
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
            if (inQR && inOpenPaczkomat && !inConfirmClosed && InPostAPI::openPaczkomatBuffer.status == NotStarted && !inputLock) {
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                    inOpenPaczkomat = false;
                    ResetRemoteLockerData();
                } else if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                    InPostAPI::OpenPaczkomat(sessionUuid);
                }
            }

            if (inQR && !inOpenPaczkomat && !inConfirmClosed && InPostAPI::getPaczkomatStatusBuffer.status == Done) {
                if (InPostAPI::getPaczkomatStatusBuffer.code == 200) {
                    std::string rawData = std::string(InPostAPI::getPaczkomatStatusBuffer.data.begin(), InPostAPI::getPaczkomatStatusBuffer.data.end());
                    if (nlohmann::json::accept(rawData)) {
                        nlohmann::json statusData = nlohmann::json::parse(rawData);
                        if (statusData.contains("sessionUuid") && !statusData["sessionUuid"].is_null()) {
                            sessionUuid = statusData["sessionUuid"].get<std::string>();
                            inOpenPaczkomat = true;
                            if (confirmOpen.frameCount > 0) {
                                PlaySound(confirmOpen);
                            }
                        } else {
                            SPDLOG_ERROR("failed to parse session data for parcel {}", (*currentDisplay)[selectedPackage].number);
                            ResetRemoteLockerData();
                        }
                    } else {
                        SPDLOG_ERROR("failed to start open session parcel {}", (*currentDisplay)[selectedPackage].number);
                        SPDLOG_ERROR("http code: {}", InPostAPI::getPaczkomatStatusBuffer.code);
                        ResetRemoteLockerData();
                    }
                }
            }

            if (inQR && inOpenPaczkomat && !inConfirmClosed && InPostAPI::openPaczkomatBuffer.status == Done) {
                if (InPostAPI::openPaczkomatBuffer.code == 200) {
                    inOpenPaczkomat = false;
                    inConfirmClosed = true;
                    if (confirmClosed.frameCount > 0) {
                        PlaySound(confirmClosed);
                    }
                } else {
                    SPDLOG_ERROR("failed to open locker for parcel {}", (*currentDisplay)[selectedPackage].number);
                    SPDLOG_ERROR("http code: {}", InPostAPI::openPaczkomatBuffer.code);
                    inOpenPaczkomat = false;
                    ResetRemoteLockerData();
                }
            }

            if (inQR && !inOpenPaczkomat && inConfirmClosed && InPostAPI::terminatePaczkaBuffer.status == NotStarted && !inputLock) {
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                    inConfirmClosed = false;
                    ResetRemoteLockerData();
                } else if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                    InPostAPI::TerminatePaczka(sessionUuid);
                }
            }

            if (inQR && !inOpenPaczkomat && inConfirmClosed && InPostAPI::terminatePaczkaBuffer.status == Done) {
                if (InPostAPI::terminatePaczkaBuffer.code != 200) {
                    SPDLOG_ERROR("failed to terminate session for parcel {}", (*currentDisplay)[selectedPackage].number);
                    SPDLOG_ERROR("http code: {}", InPostAPI::terminatePaczkaBuffer.code);
                }
                ReloadScene();
            }
        }
    }

    // --- handle camera ---
    {
        if (!useTouch) {
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
    }

    // --- handle input ---
    {
        if (!inDetails && GetTouchPointCount() > 0) {
            Vector2 currentTouch = GetTouchPosition(0); // where finger is

            if (!screenTouched) {
                touchStartPos = currentTouch; // where finger was at the very start to calculate if is outside the deadzone
                previousTouch = currentTouch; // where finger was previous frame
                isDragging = false;
                screenTouched = true;
                useTouch = true;
            }

            // if finger moves past threshold, it's a scroll, not a tap
            if (std::abs(currentTouch.x - touchStartPos.x) > dragThreshold ||
                std::abs(currentTouch.y - touchStartPos.y) > dragThreshold) {
                isDragging = true;

                Vector2 leftUpper = {0, packagesFade.peek()};
                Vector2 rightLower = { GetScreenWidth(), GetScreenHeight()};

                if (touchStartPos.x >= leftUpper.x && touchStartPos.y >= leftUpper.y &&
                    touchStartPos.x <= rightLower.x && touchStartPos.y <= rightLower.y) {
                    float scrollMaxOffset = 40.0f + ((float) package.width + 40) * (*currentDisplay).size() - GetScreenWidth();
                    if (scrollMaxOffset + GetScreenWidth() >= GetScreenWidth()) {
                        cameraOffset -= (currentTouch.x - previousTouch.x);
                        previousTouch = currentTouch;

                        if (cameraOffset > scrollMaxOffset) cameraOffset = scrollMaxOffset;
                        if (cameraOffset < 0) cameraOffset = 0;
                    }
                }
            }
        }
        // when the finger is lifted
        else if (screenTouched && GetTouchPointCount() == 0) {
            if (!isDragging) {
                for (int i = 0; i < (*currentDisplay).size(); i++) {
                    // use touchStartPos here so it selects what they originally tapped
                    Vector2 leftUpper = {40.0f + ((float) package.width + 40) * i - cameraOffset + 6, packagesFade.peek() + modeChangeAnim.peek() + 6};
                    Vector2 rightLower = { leftUpper.x + package.width, leftUpper.y + package.height };

                    if (touchStartPos.x >= leftUpper.x && touchStartPos.y >= leftUpper.y &&
                        touchStartPos.x <= rightLower.x && touchStartPos.y <= rightLower.y) {
                        if (selectedPackage == i) {
                            SoundManager::PlaySound(GoSound);
                            scrollOffset = 0;
                            detailsFade.forward();
                            detailsScrollUp.forward();
                            inDetails = true;
                        } else {
                            SoundManager::PlaySound(ChangeSound);
                            selectedPackage = i;
                            GenerateSenderNameRenderTexture();
                        }
                    }
                }
            }
            screenTouched = false;
            isDragging = false;
            previousTouch = {0, 0};
        }

        if (GetTouchPointCount() == 0) screenTouched = false;

        float currentStickValue = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);

        if (currentStickValue < -0.5f && !stickMoved &&
        selectedPackage > 0 && std::size((*currentDisplay)) && !inputLock && !inDetails) {
            stickMoved = true;
            selectorFadePulse.forward();
            selectorFadePulse.seek(0);
            selectedPackage--;
            SoundManager::PlaySound(ChangeSound);
            GenerateSenderNameRenderTexture();
            useTouch = false;
        }

        if (currentStickValue > 0.5f && !stickMoved &&
        selectedPackage < std::size((*currentDisplay)) - 1 && std::size((*currentDisplay)) && !inputLock && !inDetails) {
            stickMoved = true;
            selectorFadePulse.forward();
            selectorFadePulse.seek(0);
            selectedPackage++;
            SoundManager::PlaySound(ChangeSound);
            GenerateSenderNameRenderTexture();
            useTouch = false;
        }

        if (currentStickValue > -0.3f && currentStickValue < 0.3f) {
            stickMoved = 0;
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) &&
        selectedPackage < std::size((*currentDisplay)) - 1 && std::size((*currentDisplay))
        && !inDetails && !inputLock) {
            selectorFadePulse.forward();
            selectorFadePulse.seek(0);
            selectedPackage++;
            SoundManager::PlaySound(ChangeSound);
            GenerateSenderNameRenderTexture();
            useTouch = false;
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)
            && selectedPackage > 0 && std::size((*currentDisplay))
            && !inDetails && !inputLock) {
            selectorFadePulse.forward();
            selectorFadePulse.seek(0);
            selectedPackage--;
            SoundManager::PlaySound(ChangeSound);
            GenerateSenderNameRenderTexture();
            useTouch = false;
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)
        && !inDetails && !inputLock && std::size((*currentDisplay)) > 0) {
            SoundManager::PlaySound(GoSound);
            scrollOffset = 0;
            detailsFade.forward();
            detailsScrollUp.forward();
            inDetails = true;
        }

        if (askForParcelName) {
            swkbdCreate(&kbd, 0);
            swkbdConfigSetType(&kbd, SwkbdType_All);
            swkbdConfigSetStringLenMax(&kbd, 100);
            swkbdConfigSetStringLenMin(&kbd, 0);
            swkbdConfigSetHeaderText(&kbd, i18n::GetString("main.new_name.prompt").c_str());
            swkbdConfigSetGuideText(&kbd, "Cosplay");

            rc = swkbdShow(&kbd, parcelName, sizeof(parcelName));
            swkbdClose(&kbd);

            if (R_SUCCEEDED(rc)) {
                Config::openedFile.parcelNames.insert_or_assign((*currentDisplay)[selectedPackage].number, std::string(parcelName));
                if (std::size((*currentDisplay)) > 0) {
                    GenerateSenderNameRenderTexture();
                }
            }
            askForParcelName = false;
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP)
        && !inDetails && !inputLock && std::size((*currentDisplay)) > 0) {
            SoundManager::PlaySound(GoSound);
            askForParcelName = true;
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_LEFT) && !inDetails && !inputLock) {
            modeChangeAnim.forward();
            modeChangeAnim.seek(0);
            playModeChangeAnim = true;
            SoundManager::PlaySound(ChangeSound);
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT) && !inDetails && !inputLock && !offlineMode) {
            inputLock = true;
            SceneManager::ChangeScene(std::make_unique<SceneLoading>());
            return;
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT) && !inDetails && !inputLock) {
            inputLock = true;
            SoundManager::PlaySound(GoSound);
            SceneManager::ChangeScene(std::make_unique<SceneOptions>(true));
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

        if (playModeChangeAnim) {
            modeChangeAnim.step((int)(dt * 1000.0f));
        }
        if (modeChangeAnim.progress() >= 1.0f && modeChangeAnim.direction() == 1) {
            selectedPackage = 0;
            useTouch = false;
            if (currentDisplay == &InPostAPI::packages) {
                currentDisplay = &InPostAPI::packageArchive;
            } else if (currentDisplay == &InPostAPI::packageArchive) {
                currentDisplay = &InPostAPI::packages;
            } else {
                SPDLOG_CRITICAL("current display pointing at unknown location");
            }
            if (std::size((*currentDisplay)) > 0) {
                GenerateSenderNameRenderTexture();
            }
            modeChangeAnim.backward();
        } else if (modeChangeAnim.progress() <= 0.0f && modeChangeAnim.direction() == -1) {
            modeChangeAnim.forward();
            modeChangeAnim.seek(0);
            playModeChangeAnim = false;
        }

        if (!textScrollDirection) {
            if (textScrollAnim + textAreaWidth > senderName.texture.width) {
                textScrollAnimDelay -= dt;
                if (textScrollAnimDelay < 0) {
                    textScrollAnimDelay = 3.0f;
                    textScrollDirection = true;
                }
            } else {
                textScrollAnim += 35 * dt;
            }
        } else {
            if (textScrollAnim < 0) {
                textScrollAnimDelay -= dt;
                if (textScrollAnimDelay < 0) {
                    textScrollAnimDelay = 3.0f;
                    textScrollDirection = false;
                }
            } else {
                textScrollAnim -= 35 * dt;
            }
        }
    }
}

void SceneMain::SceneDraw() {
    if (!inQR) {
        if (currentDisplay == &InPostAPI::packageArchive) {
            DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 192});
            DrawTextOutlineEx(mainFont, i18n::GetString("main.archive").c_str(), {30, 30}, {0, 0}, 42, 0, {255, 204, 0, 255}, BLACK, 4);
        } else if (currentDisplay == &InPostAPI::packages) {
            DrawTextOutlineEx(mainFont, i18n::GetString("main.current").c_str(), {30, 30}, {0, 0}, 42, 0, {255, 204, 0, 255}, BLACK, 4);
        }

        if (offlineMode) {
            DrawTextOutlineEx(mainFont, i18n::GetString("main.offline").c_str(), {30, 77}, {0, 0}, 42, 0, {255, 0, 0, 255}, BLACK, 4);
        }

        Rectangle source = {0.0f, 0.0f, (float) poststamp.width, (float) poststamp.height};
        Vector2 screenPos = {(float) GetScreenWidth() / 2.0f, 0};
        Rectangle dest = {screenPos.x, poststampFade.peek() + modeChangeAnim.peek(), (float) poststamp.width * 2,
                          (float) poststamp.height * 2};
        Vector2 origin = {(float) poststamp.width, 0};
        DrawTexturePro(poststamp, source, {dest.x + 3, dest.y + 3, dest.width, dest.height}, origin, 0.0f,
                       {0, 0, 0, 100});
        DrawTexturePro(poststamp, source, dest, {origin.x + 3, 3}, 0.0f, WHITE);
        for (int i = 0; i < (*currentDisplay).size(); i++) {
            DrawTextureEx(package,
                          {40.0f + ((float) package.width + 40) * i - cameraOffset + 6, packagesFade.peek() + modeChangeAnim.peek() + 6}, 0,
                          1, {0, 0, 0, 100});
            DrawTextureEx(package, {40.0f + ((float) package.width + 40) * i - cameraOffset, packagesFade.peek() + modeChangeAnim.peek()},
                          0, 1, WHITE);

            if ((*currentDisplay)[i].openable) {
                DrawTextureEx(readyForPickup,
                          {40.0f + ((float) package.width + 40) * i - cameraOffset + 3, packagesFade.peek() + modeChangeAnim.peek() + 3}, 0,
                          1, {0, 0, 0, 100});
                DrawTextureEx(readyForPickup, {40.0f + ((float) package.width + 40) * i - cameraOffset, packagesFade.peek() + modeChangeAnim.peek()},
                              0, 1, WHITE);
            } else if ((*currentDisplay)[i].delivered) {
                DrawTextureEx(delivered,
                          {40.0f + ((float) package.width + 40) * i - cameraOffset + 3, packagesFade.peek() + modeChangeAnim.peek() + 3}, 0,
                          1, {0, 0, 0, 100});
                DrawTextureEx(delivered, {40.0f + ((float) package.width + 40) * i - cameraOffset, packagesFade.peek() + modeChangeAnim.peek()},
                              0, 1, WHITE);
            }

#ifdef DEBUG
            Vector2 leftUpper = {40.0f + ((float) package.width + 40) * i - cameraOffset + 6, packagesFade.peek() + modeChangeAnim.peek() + 6};

            DrawRectangle(leftUpper.x, leftUpper.y, package.width, package.height, {0, 255, 0, 128});
#endif
        }

        if ((*currentDisplay).size() > 0){
            //DrawTextureEx(selector, { 5.0f + ((float)package.width + 40) * selectedPackage - cameraOffset, packagesFade.peek() + modeChangeAnim.peek() - 30.0f}, 0, 1, WHITE);
            float x = 5.0f + ((float)package.width + 40) * selectedPackage - cameraOffset;
            float y = packagesFade.peek() + modeChangeAnim.peek() - 30.0f;

            DrawTextureEx(selectorCorner, { x - selectorFadePulse.peek(), y - selectorFadePulse.peek() }, 0.0f, 1.0f, WHITE);
            DrawTextureEx(selectorCorner, { x + 250.0f + selectorFadePulse.peek(), y - selectorFadePulse.peek() }, 90.0f, 1.0f, WHITE);
            DrawTextureEx(selectorCorner, { x + 250.0f + selectorFadePulse.peek(), y + 228.0f + selectorFadePulse.peek() }, 180.0f, 1.0f, WHITE);
            DrawTextureEx(selectorCorner, { x - selectorFadePulse.peek(), y + 228.0f + selectorFadePulse.peek() }, 270.0f, 1.0f, WHITE);
        }

        if (!offlineMode) {
            DrawTextureEx(promptY, {5.0f,  410.0f}, 0, 0.5f, WHITE);
            DrawTextureEx(reloadButton, {60.0f, 410.0f}, 0, 1, WHITE);
        }

        DrawTextureEx(promptX, {5.0f, 350.0f}, 0, 0.5f, WHITE);
        DrawTextureEx(renameButton, {60.0f, 350.0f}, 0, 1, WHITE);
        DrawTextureEx(promptMinus, {5.0f, 290.0f}, 0, 0.5f, WHITE);
        DrawTextureEx(archiveButton, {60.0f, 290.0f}, 0, 1, WHITE);
        DrawTextureEx(promptPlus, {5.0f, 230.0f}, 0, 0.5f, WHITE);
        DrawTextureEx(settingsButton, {60.0f, 230.0f}, 0, 1, WHITE);

        if ((*currentDisplay).size() == 0) {
            Vector2 textSize = MeasureTextEx(mainFont, i18n::GetString("main.no_parcels").c_str(), 42, 1);
            DrawTextOutlineEx(mainFont, i18n::GetString("main.no_parcels").c_str(), { (float)GetScreenWidth() / 2, poststampFade.peek() + modeChangeAnim.peek() + poststamp.height }, {textSize.x / 2, textSize.y / 2}, 42, 0, WHITE, BLACK, 4);
        } else {
            Vector2 textSize = MeasureTextEx(mainFont, i18n::GetString(ToLowercase((*currentDisplay)[selectedPackage].events[0].name)).c_str(), 32, 1);
            DrawTextOutlineEx(mainFont, i18n::GetString(ToLowercase((*currentDisplay)[selectedPackage].events[0].name)).c_str(), {(float)GetScreenWidth()/2, poststampFade.peek() + modeChangeAnim.peek() + 170}, {textSize.x/2, textSize.y/2}, 32, 0, WHITE, BLACK, 2);
            std::string paczkaCounter;
            paczkaCounter += std::to_string(selectedPackage + 1);
            paczkaCounter += " / ";
            paczkaCounter += std::to_string((*currentDisplay).size());
#ifdef DEBUG
            paczkaCounter += "\n";
            paczkaCounter += std::to_string(textScrollAnim);
            paczkaCounter += ", ";
            paczkaCounter += std::to_string(senderName.texture.width);
#endif
            textSize = MeasureTextEx(mainFont, paczkaCounter.c_str(), 28, 1);
            DrawTextOutlineEx(mainFont, paczkaCounter.c_str(), {(float)GetScreenWidth()/2 - 190, poststampFade.peek() + modeChangeAnim.peek() + 100}, {textSize.x/2, textSize.y/2}, 28, 1, WHITE, BLACK, 2);

            textSize = MeasureTextEx(mainFont, (*currentDisplay)[selectedPackage].events[0].date.c_str(), 32, 0);
            DrawTextOutlineEx(mainFont, (*currentDisplay)[selectedPackage].events[0].date.c_str(), {(float)GetScreenWidth()/2, poststampFade.peek() + modeChangeAnim.peek() + 120}, {textSize.x/2, textSize.y/2}, 32, 0, WHITE, BLACK, 2);

            if ((*currentDisplay)[selectedPackage].courier) {
                textSize = MeasureTextEx(mainFont, i18n::GetString("main.courier").c_str(), 40, 0);
                DrawTextOutlineEx(mainFont, i18n::GetString("main.courier").c_str(), {(float)GetScreenWidth()/2, poststampFade.peek() + modeChangeAnim.peek() + poststamp.height}, {textSize.x/2, textSize.y/2}, 40, 0, WHITE, BLACK, 2);
            } else {
                textSize = MeasureTextEx(mainFont, (*currentDisplay)[selectedPackage].pickupPointName.c_str(), 40, 0);
                DrawTextOutlineEx(mainFont, (*currentDisplay)[selectedPackage].pickupPointName.c_str(), {(float)GetScreenWidth()/2, poststampFade.peek() + modeChangeAnim.peek() + poststamp.height}, {textSize.x/2, textSize.y/2}, 40, 0, WHITE, BLACK, 2);

                textSize = MeasureTextEx(mainFont, std::string((*currentDisplay)[selectedPackage].street + ", " + (*currentDisplay)[selectedPackage].city).c_str(), 32, 0);
                DrawTextOutlineEx(mainFont, std::string((*currentDisplay)[selectedPackage].street + ", " + (*currentDisplay)[selectedPackage].city).c_str(), {(float)GetScreenWidth()/2, poststampFade.peek() + modeChangeAnim.peek() + poststamp.height + 30}, {textSize.x/2, textSize.y/2}, 32, 0, WHITE, BLACK, 2);
            }

            // DrawTextOutlineEx(mainFont, selectedPackageName.c_str(), {(float)GetScreenWidth()/2, poststampFade.peek() + modeChangeAnim.peek() + poststamp.height + 95}, {textSize.x/2, textSize.y/2}, 28, 0, WHITE, BLACK, 2);
            if (senderName.texture.width > textAreaWidth) {
                DrawTexturePro(senderName.texture, {textScrollAnim, 0, textAreaWidth, -senderName.texture.height},
                                {GetScreenWidth()/2, poststampFade.peek() + modeChangeAnim.peek() + poststamp.height + 95, textAreaWidth, senderName.texture.height},
                                {textAreaWidth/2, senderName.texture.height/2}, 0, WHITE);
            } else {
                DrawTexturePro(senderName.texture, {0, 0, senderName.texture.width, -senderName.texture.height},
                {GetScreenWidth()/2, poststampFade.peek() + modeChangeAnim.peek() + poststamp.height + 95, senderName.texture.width, senderName.texture.height},
                {senderName.texture.width/2, senderName.texture.height/2}, 0, WHITE);
            }

            if (askForParcelName) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 192});
            }

            DrawTexturePro(packageDetails.texture, {0, 0, (float)packageDetails.texture.width, (float)-packageDetails.texture.height},
               (Rectangle){0, detailsScrollUp.peek(), (float)GetScreenWidth(), (float)GetScreenHeight()},
               {0, 0}, 0, ColorAlpha(WHITE, detailsFade.peek()));
        }
    } else {
        DrawTexturePro(qrCode, {0, 0, (float)qrCode.width, (float)qrCode.height},
            {(float)GetScreenWidth()/2, 20, (float)qrCode.width, (float)qrCode.height},
            {(float)qrCode.width/2, 0}, 0, WHITE);

        if (!offlineMode) {
            /* DrawTexturePro(senderName.texture, {0, 0, senderName.texture.width, -senderName.texture.height},
            {GetScreenWidth()/2, poststampFade.peek() + modeChangeAnim.peek() + poststamp.height + 95, senderName.texture.width, senderName.texture.height},
            {senderName.texture.width/2, senderName.texture.height/2}, 0, WHITE); */
            DrawTexturePro(openButton.texture, {0, 0, 360,-80},
                {GetScreenWidth()/2 - openButton.texture.width/2, 40 + qrCode.height, 360, 80}, {0, 0}, 0, WHITE);
            DrawTextureEx(promptX, {GetScreenWidth()/2 - openButton.texture.width/2 - 10 - promptX.width, 35 + qrCode.height}, 0, 1, WHITE);
        }

        if (inOpenPaczkomat || inConfirmClosed) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 192});
        }

        if (inOpenPaczkomat) {
            Vector2 textSize = MeasureTextEx(mainFont, i18n::GetString("main.remote.open").c_str(), 32, 0);
            DrawRectangle(GetScreenWidth()/2 - textSize.x/2 - 50, GetScreenHeight()/2 - textSize.y/2 - 50, textSize.x + 100, textSize.y + 100, WHITE);
            DrawTextPro(mainFont, i18n::GetString("main.remote.open").c_str(),
                {GetScreenWidth()/2, GetScreenHeight()/2}, {textSize.x/2, textSize.y/2}, 0, 32, 0, BLACK);
        }

        if (inConfirmClosed) {
            Vector2 textSize = MeasureTextEx(mainFont, i18n::GetString("main.remote.picked_up").c_str(), 32, 0);
            DrawRectangle(GetScreenWidth()/2 - textSize.x/2 - 50, GetScreenHeight()/2 - textSize.y/2 - 50, textSize.x + 100, textSize.y + 100, WHITE);
            DrawTextPro(mainFont, i18n::GetString("main.remote.picked_up").c_str(),
                {GetScreenWidth()/2, GetScreenHeight()/2}, {textSize.x/2, textSize.y/2}, 0, 32, 0, BLACK);
        }

        if (InPostAPI::getPaczkomatStatusBuffer.status == InProgress ||
            InPostAPI::openPaczkomatBuffer.status == InProgress ||
            InPostAPI::terminatePaczkaBuffer.status == InProgress) {

            Rectangle source = { 0.0f, 0.0f, (float)loadingCircle.width, (float)loadingCircle.height };
            Rectangle dest = { 1197, 637, (float)loadingCircle.width/2, (float)loadingCircle.height/2};
            Vector2 origin = { (float)loadingCircle.width/4.0f, (float)loadingCircle.height/4.0f};
            DrawTexturePro(loadingCircle, source, { dest.x, dest.y, dest.width, dest.height}, origin, spinnerRotation, {255, 255, 255, 100});
        }
    }
}

void SceneMain::SceneExit() {
    StopSound(confirmClosed);
    UnloadSound(confirmClosed);
    StopSound(confirmOpen);
    UnloadSound(confirmOpen);
    UnloadRenderTexture(packageDetails);
    UnloadFont(mainFont);
    UnloadTexture(package);
    UnloadTexture(poststamp);
    UnloadTexture(selectorCorner);
    UnloadTexture(loadingCircle);
    UnloadTexture(promptY);
    UnloadTexture(promptX);
    UnloadTexture(promptPlus);
    UnloadTexture(promptMinus);
    UnloadRenderTexture(openButton);
    UnloadTexture(reloadButton);
    UnloadTexture(renameButton);
    UnloadTexture(settingsButton);
    UnloadTexture(readyForPickup);
    UnloadTexture(delivered);
}