// Тесты engine.c — собираются обычным cc на любой ОС: make test
#include "../src/engine.h"

#include <locale.h>
#include <stdio.h>

static int failures;

// Имитирует набор в текстовом поле: '\b' — Backspace, '\x1b' — сброс (клик мышью, стрелки, смена окна).
static void type(UzEngine *e, const wchar_t *keys, wchar_t *doc) {
  size_t n = wcslen(doc);
  for (; *keys; keys++) {
    wchar_t rep[3];
    if (*keys == L'\x1b') {
      uz_reset(e);
    } else if (*keys == L'\b') {
      if (uz_backspace(e, rep)) {
        n--;
        for (wchar_t *p = rep; *p; p++) doc[n++] = *p;
      } else if (n > 0) {
        n--;
      }
    } else if (uz_type(e, *keys, rep)) {
      n--;
      for (wchar_t *p = rep; *p; p++) doc[n++] = *p;
    } else {
      doc[n++] = *keys;
    }
    doc[n] = 0;
  }
}

static void check(UzEngine *e, const wchar_t *keys, const wchar_t *want) {
  wchar_t doc[256] = L"";
  type(e, keys, doc);
  if (wcscmp(doc, want) != 0) {
    printf("FAIL: %ls\n  want: %ls\n  got:  %ls\n", keys, want, doc);
    failures++;
  }
}

static void expect(const wchar_t *keys, const wchar_t *want) {
  UzEngine e;
  uz_init(&e);
  check(&e, keys, want);
}

int main(void) {
  setlocale(LC_ALL, "C.UTF-8");

  // Примеры из README.
  expect(L"Shahar o'zbek choy g'alaba", L"Şahar özbek çoy ğalaba");
  expect(L"Ish\boq", L"Ishoq");

  // Регистр — по первой букве, как в content.js.
  expect(L"sh Sh SH sH", L"ş Ş Ş ş");
  expect(L"ch Ch CH cH", L"ç Ç Ç ç");
  expect(L"o' O' g' G'", L"ö Ö ğ Ğ");

  // Все виды апострофа.
  expect(L"o' o‘ o’ oʻ oʼ o`", L"ö ö ö ö ö ö");

  // Подряд и без повторной замены.
  expect(L"shshsh", L"şşş");
  expect(L"shh", L"şh");
  expect(L"o''", L"ö'");
  expect(L"sch", L"sç");

  // Чужие буквы не трогаем.
  expect(L"th ph xh o\" ых", L"th ph xh o\" ых");

  // Backspace откатывает только сразу после замены.
  expect(L"sh\b", L"sh");
  expect(L"sh\b\b", L"s");
  expect(L"sh\b\bh", L"ş");
  expect(L"sha\b", L"ş");
  expect(L"sha\b\b", L"");
  expect(L"o'\b", L"o'");

  // Обычный Backspace возвращает предыдущий символ в историю.
  expect(L"sx\bh", L"ş");
  expect(L"s\b\bh", L"h");

  // После сброса символ перед кареткой неизвестен.
  expect(L"s\x1bh", L"sh");
  expect(L"sh\x1b\b", L"");

  // Длинный ввод не переполняет историю.
  expect(L"abcdefghijklmnopqrstuvwxyz0123456789 abcdefghijklmnopqrstuvwxyz0123456789 sh",
         L"abcdefghijklmnopqrstuvwxyz0123456789 abcdefghijklmnopqrstuvwxyz0123456789 ş");

  // Отключённое правило.
  UzEngine e;
  uz_init(&e);
  e.rules[2] = false; // sh
  check(&e, L"shahar choy", L"shahar çoy");

  if (failures) {
    printf("%d test(s) failed\n", failures);
    return 1;
  }
  printf("engine: all tests passed\n");
  return 0;
}
