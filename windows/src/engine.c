#include "engine.h"

#include <string.h>

#define UZ_APOSTROPHES L"'‘’ʻʼ`" // ' ‘ ’ ʻ ʼ `

const UzRule UZ_RULES[UZ_RULE_COUNT] = {
  { "o", L"oʻ", L'o', UZ_APOSTROPHES, L'ö', L'Ö' }, // oʻ → ö Ö
  { "g", L"gʻ", L'g', UZ_APOSTROPHES, L'ğ', L'Ğ' }, // gʻ → ğ Ğ
  { "sh", L"sh", L's', L"hH", L'ş', L'Ş' },              // sh → ş Ş
  { "ch", L"ch", L'c', L"hH", L'ç', L'Ç' },              // ch → ç Ç
};

void uz_init(UzEngine *e) {
  memset(e, 0, sizeof *e);
  for (int i = 0; i < UZ_RULE_COUNT; i++) e->rules[i] = true;
}

void uz_reset(UzEngine *e) {
  e->len = 0;
  e->canUndo = false;
}

static void push(UzEngine *e, wchar_t ch) {
  if (e->len == UZ_HISTORY) {
    memmove(e->history, e->history + 1, (UZ_HISTORY - 1) * sizeof(wchar_t));
    e->len--;
  }
  e->history[e->len++] = ch;
}

static wchar_t ascii_lower(wchar_t ch) {
  return ch >= L'A' && ch <= L'Z' ? ch - L'A' + L'a' : ch;
}

static const UzRule *find_rule(const UzEngine *e, wchar_t prev, wchar_t typed) {
  for (int i = 0; i < UZ_RULE_COUNT; i++) {
    const UzRule *r = &UZ_RULES[i];
    if (e->rules[i] && r->base == ascii_lower(prev) && wcschr(r->triggers, typed)) return r;
  }
  return NULL;
}

bool uz_type(UzEngine *e, wchar_t ch, wchar_t replacement[3]) {
  e->canUndo = false;
  const UzRule *r = e->len > 0 && ch ? find_rule(e, e->history[e->len - 1], ch) : NULL;
  if (!r) {
    push(e, ch);
    return false;
  }

  wchar_t prev = e->history[e->len - 1];
  replacement[0] = prev == r->base ? r->lower : r->upper;
  replacement[1] = 0;
  e->history[e->len - 1] = replacement[0];
  e->undo[0] = prev;
  e->undo[1] = ch;
  e->undo[2] = 0;
  e->canUndo = true;
  return true;
}

bool uz_backspace(UzEngine *e, wchar_t replacement[3]) {
  bool undo = e->canUndo;
  e->canUndo = false;
  if (!undo) {
    if (e->len > 0) e->len--;
    return false;
  }

  wcscpy(replacement, e->undo);
  e->len--;
  push(e, e->undo[0]);
  push(e, e->undo[1]);
  return true;
}
