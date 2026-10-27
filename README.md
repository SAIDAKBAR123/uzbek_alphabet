<p align="center">
  <img src="icons/icon-128.png" width="96" height="96" alt="O'zbek harflari">
</p>

<h1 align="center">Oʻzbek harflari</h1>

<p align="center">
  Matn yozayotganda <b>oʻ → ö</b>, <b>gʻ → ğ</b>, <b>sh → ş</b>, <b>ch → ç</b> avtomatik almashtiriladigan Google Chrome kengaytmasi.
</p>

<p align="center">
  <a href="#english">English</a>
</p>

---

macOS va boshqa tizimlarda `ş`, `ç`, `ö`, `ğ` kabi harflarni yozish uchun tugmani bosib turib, harf variantlari menyusini kutish kerak. Bu kengaytma bilan esa odatdagidek yozasiz — harflar yozish jarayonida oʻzi almashadi.

<p align="center">
  <img src="docs/popup-light.png" width="320" alt="Kengaytma oynasi (yorugʻ mavzu)">
  &nbsp;&nbsp;
  <img src="docs/popup-dark.png" width="320" alt="Kengaytma oynasi (qorongʻi mavzu)">
</p>

## Qoidalar

| Yozasiz | Natija |
|---|---|
| `o'` &nbsp; `O'` | `ö` &nbsp; `Ö` |
| `g'` &nbsp; `G'` | `ğ` &nbsp; `Ğ` |
| `sh` &nbsp; `Sh` &nbsp; `SH` | `ş` &nbsp; `Ş` &nbsp; `Ş` |
| `ch` &nbsp; `Ch` &nbsp; `CH` | `ç` &nbsp; `Ç` &nbsp; `Ç` |

Apostrofning barcha turlari qoʻllab-quvvatlanadi: `'` `‘` `’` `ʻ` `ʼ` `` ` `` — macOS “aqlli qoʻshtirnoq”larni oʻzi almashtirsa ham ishlaydi.

**Masalan:** `Shahar o'zbek choy g'alaba` → `Şahar özbek çoy ğalaba`

## Imkoniyatlar

- **Barcha matn maydonlarida ishlaydi** — `input`, `textarea` va `contenteditable` (Gmail, Telegram Web, ijtimoiy tarmoqlar va h.k.).
- **Bekor qilish** — almashtirishdan keyin darhol <kbd>Backspace</kbd> bosilsa, asl harflar qaytadi. Masalan: `Is` + `h` → `Iş`, <kbd>Backspace</kbd> → `Ish`, keyin `oq` → `Ishoq`.
- **<kbd>Ctrl</kbd>/<kbd>Cmd</kbd>+<kbd>Z</kbd>** ham ishlaydi.
- **Tezkor tugma** — <kbd>Alt</kbd>+<kbd>Shift</kbd>+<kbd>U</kbd> (macOS: <kbd>⌥</kbd><kbd>⇧</kbd><kbd>U</kbd>) bilan yoqish/oʻchirish. Oʻchirilganda belgida `OFF` koʻrinadi. Tugmani `chrome://extensions/shortcuts` sahifasida oʻzgartirish mumkin.
- **Sayt boʻyicha oʻchirish** — masalan, inglizcha yozadigan saytlarda.
- **Har bir qoidani alohida** yoqish/oʻchirish.
- **“Sinab koʻring”** maydoni — kengaytma oynasining oʻzida tekshirib koʻrish uchun.
- Yorugʻ va qorongʻi mavzu (tizimga qarab).

## Oʻrnatish

Chrome Web Store’da chiqquncha qoʻlda oʻrnatiladi:

1. Repozitoriyani yuklab oling:
   ```bash
   git clone <repo-url>
   ```
   yoki **Code → Download ZIP** orqali yuklab, arxivdan chiqaring.
2. Chrome’da `chrome://extensions` sahifasini oching.
3. Yuqori oʻng burchakdagi **Developer mode** ni yoqing.
4. **Load unpacked** tugmasini bosib, loyiha papkasini tanlang.

Kengaytma Chrome, Edge, Brave, Opera, Arc va boshqa Chromium asosidagi brauzerlarda ishlaydi.

### Windows (barcha dasturlarda)

Word, Excel, Outlook, Teams, Telegram Desktop va boshqa dasturlarda ham yozish uchun — Windows dasturi. Bitta MSI fayl, tashkilotlarda Intune / Group Policy / SCCM orqali jim oʻrnatish va ADMX siyosatlari bilan. Batafsil: [windows/README.md](windows/README.md).

## Maxfiylik

- Kengaytma **hech qanday maʼlumot toʻplamaydi va hech qayerga yubormaydi** — kodda tarmoq soʻrovlari yoʻq.
- Faqat sozlamalar (yoqilgan/oʻchirilgan, oʻchirilgan saytlar, qoidalar) `chrome.storage.sync` da saqlanadi.
- Parol maydonlariga (`type="password"`) tegilmaydi.
- `<all_urls>` ruxsati kerak, chunki harflar istalgan saytda yozish jarayonida almashtiriladi. Kod ochiq — buni oʻzingiz tekshirishingiz mumkin.

## Cheklovlar

- **Google Docs** qoʻllab-quvvatlanmaydi — u oddiy matn maydonlaridan foydalanmaydi.
- Inglizcha soʻzlarda ham almashtirish boʻladi (`show` → `şow`). Bunday holatda <kbd>Backspace</kbd>, tezkor tugma yoki sayt boʻyicha oʻchirishdan foydalaning.
- Chrome ichki sahifalarida (`chrome://…`) va Chrome Web Store’da kengaytmalar ishlamaydi.

## Loyiha tuzilishi

```
├── manifest.json     Kengaytma tavsifi (Manifest V3)
├── rules.js          Almashtirish qoidalari va standart sozlamalar
├── content.js        Yozish jarayonida harflarni almashtirish
├── background.js     Tezkor tugma va belgi ustidagi “OFF”
├── popup.html        Kengaytma oynasi
├── popup.css
├── popup.js
├── icons/            Belgilar (icon.svg — manba)
├── docs/             README uchun skrinshotlar
└── windows/          Windows dasturi (windows/README.md)
```

### Yangi qoida qoʻshish

[`rules.js`](rules.js) dagi `UZ_RULES` roʻyxatiga bitta qator qoʻshing va `UZ_DEFAULTS.rules` ga uning `id` sini kiriting:

```js
{ id: "sh", from: "sh", base: "s", triggers: ["h", "H"], lower: "ş", upper: "Ş" }
```

| Maydon | Maʼnosi |
|---|---|
| `id` | Qoida identifikatori (sozlamalarda saqlanadi) |
| `from` | Oynada koʻrsatiladigan matn |
| `base` | Kursordan oldingi harf (kichik harfda) |
| `triggers` | Almashtirishni boshlaydigan belgilar |
| `lower` / `upper` | Natija — `base` kichik yoki katta harf boʻlishiga qarab |

### Qanday ishlaydi

`content.js` sahifadagi `beforeinput` hodisasini tinglaydi. Yangi belgi qoidaga mos kelsa (masalan, `s` dan keyin `h`), standart kiritish bekor qilinadi va kursordan oldingi harf `document.execCommand("insertText")` orqali almashtiriladi. Bu usul <kbd>Ctrl</kbd>+<kbd>Z</kbd> tarixini saqlaydi va React, Gmail kabi muharrirlar bilan toʻgʻri ishlaydi.

## Hissa qoʻshish

Xato topsangiz yoki gʻoyangiz boʻlsa — Issue oching yoki Pull Request yuboring.

---

<a id="english"></a>

## English

**Oʻzbek harflari** is a Chrome extension that converts Uzbek digraphs into single letters as you type: `o'` → `ö`, `g'` → `ğ`, `sh` → `ş`, `ch` → `ç`. Capitalisation is preserved (`Sh`/`SH` → `Ş`).

- Works in any `input`, `textarea` and `contenteditable` field (Gmail, Telegram Web, social networks, etc.).
- Press <kbd>Backspace</kbd> right after a conversion to restore the original letters; <kbd>Ctrl</kbd>/<kbd>Cmd</kbd>+<kbd>Z</kbd> also works.
- Toggle with <kbd>Alt</kbd>+<kbd>Shift</kbd>+<kbd>U</kbd>, disable per site, or turn individual rules on and off from the popup.
- Collects no data and makes no network requests; only settings are stored in `chrome.storage.sync`. Password fields are never touched.

**Install:** open `chrome://extensions`, enable **Developer mode**, click **Load unpacked** and select the project folder.

**Windows (all programs):** a native tray app applies the same rules in Word, Outlook, Teams and every other Windows program, and ships as an MSI with ADMX policies for Intune / Group Policy / SCCM rollout. See [windows/README.md](windows/README.md).

**Not supported:** Google Docs (it doesn't use regular text fields) and Chrome's internal pages.
