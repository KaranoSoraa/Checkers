#pragma once
#include <stdlib.h>
// Объявление POS_T как 8-ми битного инта.
typedef int8_t POS_T;
// Структура для хранения данных о передвижениях.
struct move_pos
{   // Позиции: откуда, куда и побила ли фишка.
    POS_T x, y;             // from
    POS_T x2, y2;           // to
    POS_T xb = -1, yb = -1; // beaten
    // Далее объявлено два конструктора для данной структуры, необходимые для удобства дальнейшей эксплуатации. 
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    } // Делаем перегрузку операторов. Помогает в сравнении объектов данной структуры. 
    // Определение оператора сравнения ==
    bool operator==(const move_pos& other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }
    // Определение оператора сравнения !=
    bool operator!=(const move_pos& other) const
    {
        return !(*this == other);
    }
};
