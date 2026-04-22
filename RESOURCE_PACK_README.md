# Tworzenie własnych resource packów

stwórz folder w `/config/switchpost` na karcie sd i nazwij go np. `moja-paczka`
a w nim plik `pack.spmeta` o treści (zastępując wartości własnymi)
np.
```
{
    "name": "Moja paczka",
    "author": "ameOverflow",
    "version": 1
}
```

- `name`: nazwa paczki
- `author`: twój nick
- `version`: musi być na 1

przykładowy resource pack znajduje się w folderze `example-pack`

paczka zasobów powinna pojawić się w aplikacji w ustawieniach

## Pliki których nie da się zastąpić
- `fonts/*`
- `text/test_data.json`

## Głos

utwórz folder `voice`, a w nim podfolder z nazwą głosu np. ivona
w nim umieść pliki dźwiękowe z nagraniami do otworzenia aplikacji,
nazwanymi tak samo jak pliki w `assets/voice/<female, male>`

w przypadku braku jakiegoś dźwięku, switchpost nie odtworzy nic

## Tutorial

aka shitty visual novel engine

w folderze swojej paczki utwórz folder `tutorial`, a w nim
- folder `backgrounds`, w nim pliki png teł do tutoriala
- folder z nazwą swojego tutoriala np. `moj-glos` a w nim
  - folder `sprites` z plikami png obrazków postaci
  - folder `voice` z plikami audio głosów postaci
  - plik `data.json`

### plik `data.json`
ten plik to tablica json, w której każdy element to obiekt z właściwościami:
```
{
  "speakingSprite": "speaking.png",
  "idleSprite": "idle.png",
  "voiceClip": "0.wav",
  "background": "bg.png",
  "text": "Dzień dobry!"
}
```

- `speakingSprite`: sprite jak postać gada 
- `idleSprite`: jak postać nie gada
- `voiceClip`: głos odtwarzany w aplikacji
- `background`: (opcjonalne) obrazek w tle
- `text`: zawartość textboxa