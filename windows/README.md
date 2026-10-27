<h1 align="center">Oʻzbek harflari — Windows</h1>

<p align="center">
  <b>oʻ → ö</b>, <b>gʻ → ğ</b>, <b>sh → ş</b>, <b>ch → ç</b> — Windows’dagi <b>barcha dasturlarda</b>: Word, Excel, Outlook, Teams, Telegram Desktop, 1C, brauzerlar va h.k.
</p>

<p align="center">
  <a href="#english">English</a>
</p>

---

Chrome kengaytmasi faqat brauzer ichida ishlaydi. Windows dasturi esa tizim darajasida ishlaydi: tray’da (soat yonida) turadi va istalgan dasturda yozilgan harflarni kengaytmadagi qoidalar boʻyicha almashtiradi. Bitta MSI fayl orqali tashkilotdagi barcha kompyuterlarga oʻrnatish mumkin.

## Oʻrnatish

### Bitta kompyuterga

1. [Releases](https://github.com/SAIDAKBAR123/uzbek_alphabet/releases) sahifasidan `UzbekHarflari-<versiya>-x64.msi` faylini yuklab oling.
2. Faylni ishga tushiring (administrator huquqi kerak).
3. Dastur har safar Windows’ga kirganda oʻzi ishga tushadi. Hozir ishga tushirish uchun: **Start → Oʻzbek harflari**.

> Fayl hali raqamli imzoga ega emas, shuning uchun Windows SmartScreen ogohlantirishi mumkin: **More info → Run anyway**.

### Tashkilotda (IT administratorlar uchun)

| Vazifa | Buyruq |
|---|---|
| Jim oʻrnatish | `msiexec /i UzbekHarflari-1.0.0-x64.msi /qn /norestart` |
| Oʻchirish | `msiexec /x UzbekHarflari-1.0.0-x64.msi /qn` |
| Yangilash | Yangi MSI’ni oʻrnating — eski versiya avtomatik oʻchiriladi |

- **Intune:** *Apps → Windows → Add → Line-of-business app* (MSI).
- **Group Policy:** *Computer Configuration → Policies → Software Settings → Software installation*.
- **SCCM / MECM:** odatiy MSI deployment type.

MSI dasturni `C:\Program Files\UzbekHarflari\` ga oʻrnatadi va barcha foydalanuvchilar uchun avtomatik ishga tushirishni yoqadi (`HKLM\…\Run`). Foydalanuvchi buni oʻzida *Settings → Apps → Startup* orqali oʻchira oladi.

### Guruh siyosatlari (Group Policy / Intune)

`policy/` papkasidagi shablonlarni nusxalang:

- `UzbekHarflari.admx` → `C:\Windows\PolicyDefinitions\` (yoki domen uchun `\\<domen>\SYSVOL\<domen>\Policies\PolicyDefinitions\`)
- `en-US\UzbekHarflari.adml` → `…\PolicyDefinitions\en-US\`

Shundan soʻng sozlamalar *Administrative Templates → Oʻzbek harflari* boʻlimida paydo boʻladi. Intune’da: *Devices → Configuration → Import ADMX*.

Siyosatlarni registr orqali ham berish mumkin — `HKLM\Software\Policies\UzbekHarflari` (yoki `HKCU\…`):

| Qiymat | Turi | Maʼnosi |
|---|---|---|
| `Enabled` | DWORD | `1` — doim yoqilgan, `0` — doim oʻchirilgan |
| `RuleO`, `RuleG`, `RuleSh`, `RuleCh` | DWORD | Har bir qoidani `1`/`0` |
| `ExcludedApps\` (boʻlim) | REG_SZ | Ishlamaydigan dasturlar: har bir qiymat — exe nomi |

```bat
reg add "HKLM\Software\Policies\UzbekHarflari\ExcludedApps" /v 1 /d code.exe /f
```

Siyosat bilan belgilangan sozlamani foydalanuvchi menyuda oʻzgartira olmaydi. Siyosat yangilanganda (`gpupdate`) dastur uni qayta oʻqiydi.

## Foydalanish

Tray’dagi **Ö** belgisini bosing:

- **Avtomatik almashtirish** — yoqish/oʻchirish. Tezkor tugma: <kbd>Alt</kbd>+<kbd>Shift</kbd>+<kbd>U</kbd>. Oʻchirilganda belgi kulrang boʻladi.
- **Qoidalar** — har birini alohida yoqish/oʻchirish.
- **Shu dasturda ishlaydi (…exe)** — oxirgi faol dasturda oʻchirish (masalan, kod muharriri yoki oʻyinda).
- **Chiqish**.

Kengaytmadagidek: almashtirishdan keyin darhol <kbd>Backspace</kbd> bosilsa, asl harflar qaytadi (`Is` + `h` → `Iş`, <kbd>Backspace</kbd> → `Ish`).

Parol maydonlarida almashtirish ishlamaydi.

Sozlamalar foydalanuvchining `HKCU\Software\UzbekHarflari` registr boʻlimida saqlanadi.

## Cheklovlar

- **Administrator nomidan ishlayotgan dasturlarda** almashtirish boʻlmaydi — Windows bunga ruxsat bermaydi (UIPI).
- Dastur kursor oldidagi matnni koʻrmaydi, faqat yozilgan harflarni eslab qoladi. Sichqoncha bosilsa, strelkalar yoki boshqa oyna tanlansa, xotira tozalanadi: masalan, `s` dan keyin sichqoncha bilan bosib `h` yozilsa, almashtirish boʻlmaydi.
- Avtomatik toʻldiriladigan maydonlarda (masalan, brauzer manzil qatori) <kbd>Backspace</kbd> taklifni oʻchirishi mumkin. Bunday holatda tezkor tugmadan yoki «Shu dasturda ishlaydi» dan foydalaning.
- <kbd>Ctrl</kbd>, <kbd>Alt</kbd>, <kbd>AltGr</kbd> bilan bosilgan tugmalar almashtirilmaydi.
- Inglizcha soʻzlarda ham almashtirish boʻladi (`show` → `şow`) — xuddi kengaytmadagidek.
- Brauzerda **Chrome kengaytmasi ham oʻrnatilgan boʻlsa**, ikkalasidan bittasini qoldiring (yoki brauzerni «Shu dasturda ishlaydi» orqali oʻchiring): aks holda <kbd>Backspace</kbd> bilan qaytarilgan `sh` ni kengaytma yana `ş` ga aylantiradi.
- Chrome/Edge’dagi parol maydonlarini aniqlash uchun Windows maxsus imkoniyatlar interfeysi (MSAA) ishlatiladi. Bunda Chromium oʻzining accessibility rejimini yoqadi, bu esa xotira sarfini biroz oshiradi.
- Imzosiz dastur klaviaturani kuzatgani uchun ayrim antiviruslar/EDR ogohlantirishi mumkin. Tashkilotda faylni oʻz sertifikatingiz bilan imzolash yoki ruxsat roʻyxatiga qoʻshish tavsiya etiladi.
- Windows 10 (1607+) va Windows 11, x64.

## Qanday ishlaydi

`src/main.c` tizim boʻyicha klaviatura hukini (`WH_KEYBOARD_LL`) oʻrnatadi va har bir tugma qaysi belgini berishini faol oynaning klaviatura tartibida aniqlaydi (`ToUnicodeEx`) — rus tartibida yozilganda hech narsa oʻzgarmaydi. Belgi qoidaga mos kelsa (masalan, `s` dan keyin `h`), tugma bosilishi bekor qilinadi va `SendInput` orqali <kbd>Backspace</kbd> + yangi harf yuboriladi.

Almashtirish mantigʻi `src/engine.c` da — Windows’ga bogʻliq emas va `tests/test_engine.c` bilan istalgan tizimda tekshiriladi. Qoidalar [`uzb_ext/rules.js`](../uzb_ext/rules.js) bilan bir xil — yangi qoida qoʻshganda ikkalasini ham oʻzgartiring.

## Yigʻish

Kerak: `make`, `python3`, mingw-w64 va `wixl` (msitools). Windows’ning oʻzi shart emas.

```bash
# Ubuntu / Debian
sudo apt install make python3 gcc-mingw-w64-x86-64 wixl
# macOS
brew install mingw-w64 msitools

cd windows
make test   # engine.c testlari
make        # build/UzbekHarflari.exe
make msi    # build/UzbekHarflari-<versiya>-x64.msi
```

Yoki Docker orqali:

```bash
docker run --rm -v "$PWD":/src -w /src/windows ubuntu:24.04 sh -c \
  "apt-get update && apt-get install -y make gcc python3 gcc-mingw-w64-x86-64 wixl && make test msi"
```

Har bir Pull Request’da GitHub Actions ([`windows.yml`](../.github/workflows/windows.yml)) testlarni ishga tushiradi va EXE/MSI’ni artefakt sifatida saqlaydi.

**Reliz:** `src/version.h` dagi versiyani oshiring va `windows-vX.Y.Z` tegini yuboring — CI MSI’ni Releases sahifasiga joylaydi.

```
windows/
├── src/
│   ├── engine.c, engine.h   Almashtirish mantigʻi (rules.js bilan bir xil)
│   ├── main.c               Klaviatura huki, tray, sozlamalar, siyosatlar
│   ├── app.rc, app.manifest Belgilar, versiya, DPI
│   └── version.h            Versiya (EXE va MSI uchun)
├── tests/test_engine.c
├── installer/UzbekHarflari.wxs  MSI
├── policy/                  ADMX/ADML shablonlari
├── tools/make_icons.py      uzb_ext/icons dan .ico yasaydi
└── Makefile
```

---

<a id="english"></a>

## English

**Oʻzbek harflari for Windows** brings the extension's conversion (`o'` → `ö`, `g'` → `ğ`, `sh` → `ş`, `ch` → `ç`, capitalisation preserved) to every Windows program — Office, Outlook, Teams, Telegram Desktop, 1C, browsers. It is a 75 KB tray app with no runtime dependencies.

**Install on one PC:** download `UzbekHarflari-<version>-x64.msi` from Releases and run it. It starts automatically at sign-in; start it now from the Start menu. The binary is not code-signed yet, so SmartScreen may warn.

**Deploy in an organisation:**

- Silent install: `msiexec /i UzbekHarflari-1.0.0-x64.msi /qn /norestart`; uninstall: `msiexec /x … /qn`. Newer MSIs upgrade older ones in place.
- Works as an Intune line-of-business app, a GPO software installation package or an SCCM MSI deployment type.
- Per-machine install to `C:\Program Files\UzbekHarflari\` with auto-start for all users (`HKLM\…\Run`); users can disable auto-start in *Settings → Apps → Startup*.

**Group Policy:** copy `policy/UzbekHarflari.admx` and `policy/en-US/UzbekHarflari.adml` into `PolicyDefinitions` (or import the ADMX into Intune). Policies live under `HKLM` or `HKCU\Software\Policies\UzbekHarflari`:

| Value | Type | Meaning |
|---|---|---|
| `Enabled` | DWORD | `1` force on, `0` force off |
| `RuleO`, `RuleG`, `RuleSh`, `RuleCh` | DWORD | Force a rule on (`1`) or off (`0`) |
| `ExcludedApps\` (subkey) | REG_SZ values | Executable names where conversion never happens |

Policy-controlled settings are greyed out in the tray menu and reloaded on `gpupdate`.

**Usage:** the tray menu toggles conversion (or <kbd>Alt</kbd>+<kbd>Shift</kbd>+<kbd>U</kbd>), individual rules, and the last active program. <kbd>Backspace</kbd> right after a conversion restores the original letters. Password fields are skipped (Win32/WinForms/RichEdit by window style; Chrome, Edge, Firefox, WPF via MSAA).

**Limitations:**

- No conversion in programs running as administrator (UIPI).
- The app only sees keystrokes, not the text in the field, so its memory resets on mouse clicks, arrow keys and window switches.
- In autocomplete fields (e.g. a browser address bar) Backspace may remove the suggestion instead of the letter.
- Keys pressed with Ctrl, Alt or AltGr are never converted.
- If the Chrome extension is installed too, use only one of them in the browser, or the extension re-converts the `sh` restored by Backspace.
- Detecting password fields in Chromium uses MSAA, which makes Chromium turn on its accessibility tree (slightly higher memory use).
- An unsigned binary with a global keyboard hook may be flagged by some EDR products — sign it with your organisation's certificate or allow-list it.
- Windows 10 1607+ and Windows 11, x64.

**Build** (Linux, macOS or CI — Windows is not required): install `make`, `python3`, mingw-w64 and `wixl`/msitools, then run `make test`, `make` and `make msi` in `windows/`. CI builds the EXE and MSI for every pull request; pushing a `windows-vX.Y.Z` tag that matches `src/version.h` publishes a release.
