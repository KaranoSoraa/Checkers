#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// methods for hands
class Hand // Объявление класса Hand, отвечающего за отслеживания действий игрока.
{
public:
    Hand(Board* board) : board(board)
    {
    }
    tuple<Response, POS_T, POS_T> get_cell() const // Используем tuple с стандартным для нашего поля типом данных POS_T, объявляем функцию get_call
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        int x = -1, y = -1;
        int xc = -1, yc = -1;
        // Бесконечный цикл, который, с помощью библиотеки SDL, ожидает клик.
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type) // Ветвление полученного ответа. Ниже мы присваевам ответу то, какое действие было совершено пользователем.
                {
                case SDL_QUIT:
                    resp = Response::QUIT; // Обработчик выхода
                    break;
                case SDL_MOUSEBUTTONDOWN: // Обработчик нажатия на клетку. 
                    x = windowEvent.motion.x;
                    y = windowEvent.motion.y;
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK; // Обработчик возврата к прошлой позиции.
                    }
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY; // Обработчик реплея.
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL; // Обработчик выбора игроком клетки.
                    }
                    else
                    {
                        xc = -1; // Если игрок не выбрал координаты, возвращает -1.
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT: // Обработчик изменения ширины экрана.
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size();
                        break;
                    }
                }
                if (resp != Response::OK) // Если был выбран другой вариант действий, то прекращает цикл.
                    break;
            }
        }
        // Возврат результата и координат.
        return { resp, xc, yc };
    }

    Response wait() const
    {   // функция ожидания после завершения игры.
        SDL_Event windowEvent;
        Response resp = Response::OK; // Стандартное состояние ответа.
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    resp = Response::QUIT; // Снова выход
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED: // стандартный размер поля. 
                    board->reset_window_size();
                    break;
                case SDL_MOUSEBUTTONDOWN: { // очередной обработчик команл нажатия мыши. 
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY; // Снова реплей. 
                }
                                        break;
                }
                if (resp != Response::OK) // Снова завершение цикла, если выбрано какое-либо значение.
                    break;
            }
        }
        return resp;
    }

private:
    Board* board;
};
