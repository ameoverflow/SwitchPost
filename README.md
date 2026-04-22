# SwitchPost

![logo.png](/.github/logo.png)

Nieoficjalny klient InPost Mobile na Nintendo Switch.

Bazowany na [Inpost3DS](https://github.com/TehFridge/InPost3DS) autorstwa TehFridge.

## Nowości w porównaniu do InPost3DS
- Wsparcie paczek zasobów
- Wsparcie własnych głosów aplikacji
- 6 różnych teł do wyboru
- Wskaźniki paczek (czy jest gotowa do odbioru lub została odebrana)

## Instalacja

Przekopiuj plik SwitchPost.nro do folderu switch na karcie SD zmodowanej konsoli.

**Aplikacja musi zostać uruchomiona w trybie title takeover (przytrzymaj R przy uruchamianiu dowolnej aplikacji z home menu, 
wystarczy jakaś darmowa z eShopa)**

## Kompilacja

Wymagany jest zainstalowany toolchain devkitPro.

1. `(dkp-)pacman -Syu switch-dev switch-curl switch-zlib switch-mesa switch-libdrm_nouveau`
2. `mkdir build`
3. `cmake -B build`
4. `cd build`
5. `make`
   
Wyjdzie z tego plik SwitchPost.nro w wersji debugowej, który wypluwa do pliku więcej logów niż wersja release

## Notka

Aplikacja korzysta z API InPost Mobile autorstwa InPost sp. z o.o.

Nie jestem powiązana z InPost sp. z o.o., ani nie czerpię korzyści finansowych za ten projekt.

Aplikacja stworzona w celach edukacyjnych.

Wszystkie znaki towarowe należą do ich właścicieli.

Użytkownicy korzystają z aplikacji na własne ryzyko.

:3
