# SwitchPost

![logo.png](/.github/logo.png)

Unofficial InPost Mobile client for Nintendo Switch.

Based on [InPost3DS](https://github.com/TehFridge/InPost3DS) by TehFridge.

![screenshot.png](/.github/screenshot.png)

## New features compared to InPost3DS
- Resource pack support
- 10 different backgrounds
- Parel indicators (received or ready to pickup)
- parcel archive
- Ability to change parcel names

## Installation

Copy SwitchPost.nro to the SD card of a modded Nintendo Switch.

**App works only in title takeover mode (hold R when launching any Switch app from home menu, free app from eShop will work for this)**

## Compilation

devkitPro is required.

1. `(dkp-)pacman -Syu switch-dev switch-curl switch-zlib switch-mesa switch-libdrm_nouveau`
2. `mkdir build`
3. `cmake -B build`
4. `cd build`
5. `make`
   
This will build a debug SwitchPost.nro which outputs more logs than release.

## Legal note

App uses InPost Mobile API by InPost sp. z o.o.

I'm not associated with InPost sp. z o.o., and I'm not deriving any financial gain from this project.

Application created for educational purposes.

All trademarks are the property of their respective owners.

This application is provided 'as is', without warranty of any kind.

:3
