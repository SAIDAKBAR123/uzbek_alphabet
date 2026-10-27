// Логика замены без зависимостей от Windows — тестируется на любой ОС (tests/test_engine.c).
// Правила повторяют uzb_ext/rules.js, поведение Backspace — uzb_ext/content.js. Меняйте вместе.
#ifndef UZ_ENGINE_H
#define UZ_ENGINE_H

#include <stdbool.h>
#include <wchar.h>

#define UZ_RULE_COUNT 4
#define UZ_HISTORY 32

typedef struct {
  const char *id;
  const wchar_t *from;     // как показывать в меню
  wchar_t base;            // буква перед кареткой (в нижнем регистре)
  const wchar_t *triggers; // символы, запускающие замену
  wchar_t lower, upper;    // результат — по регистру base
} UzRule;

extern const UzRule UZ_RULES[UZ_RULE_COUNT];

typedef struct {
  bool rules[UZ_RULE_COUNT];
  // Символы, набранные с последнего сброса, — то, что сейчас стоит перед кареткой.
  // Системный хук не видит текст программы, поэтому после клика, стрелок и т.п. история сбрасывается.
  wchar_t history[UZ_HISTORY];
  int len;
  // Последний ввод был заменой — Backspace сразу после неё вернёт исходные символы ("sh" вместо "ş").
  bool canUndo;
  wchar_t undo[3];
} UzEngine;

void uz_init(UzEngine *e);
void uz_reset(UzEngine *e);

// Набран символ ch. true — нажатие погасить, а символ перед кареткой заменить на replacement.
bool uz_type(UzEngine *e, wchar_t ch, wchar_t replacement[3]);

// Нажат Backspace. true — нажатие погасить, а символ перед кареткой заменить на replacement.
bool uz_backspace(UzEngine *e, wchar_t replacement[3]);

#endif
