#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
public:
    // Конструктор. Формируется поле, его настройки и элементы интерфейса.
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }


    int play()
    {
        // Используется библиотека хроно для отчета начала и конца раунда.
        auto start = chrono::steady_clock::now();
        if (is_replay)
        {
            logic = Logic(&board, &config);
            config.reload();
            board.redraw();
        }
        else
        {
            board.start_draw();
        }
        is_replay = false;

        int turn_num = -1;
        bool is_quit = false;
        // Получение максимального количества ходов из настроек.
        const int Max_turns = config("Game", "MaxNumTurns");
        // Цикл игры. Действует, пока количество ходов меньше максимума.
        while (++turn_num < Max_turns)
        {
            beat_series = 0;
            logic.find_turns(turn_num % 2);
            if (logic.turns.empty())
                break;
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));
            // Определяем, чей ход на данный момент путем обращения к данным из настроект, получаем цвет игрока.
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                // Логическое ветвление для ходов человека.
                auto resp = player_turn(turn_num % 2); // Вызов функции хода игрока и записываем результат в переменную.
                if (resp == Response::QUIT) // Обработчик команды завершения.
                {
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY) // Обработчик команды реплея.
                {
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK) // Обработчик команды возврата на ход.
                {
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback();
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback();
                    --turn_num;
                    beat_series = 0;
                }
            }
            else
                // Вызов функции хода бота.
                bot_turn(turn_num % 2);
        }
        // конец периода.
        auto end = chrono::steady_clock::now();
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();

        if (is_replay)
            return play();
        if (is_quit)
            return 0;
        int res = 2;
        if (turn_num == Max_turns)
        {
            res = 0;
        }
        else if (turn_num % 2)
        {
            res = 1;
        }
        board.show_final(res);
        auto resp = hand.wait();
        if (resp == Response::REPLAY)
        {
            is_replay = true;
            return play();
        }
        return res;
    }

private:
    // функция работы бота
    void bot_turn(const bool color)
    {
        // Отчет начала и конца хода бота.
        auto start = chrono::steady_clock::now();

        auto delay_ms = config("Bot", "BotDelayMS");
        // new thread for equal delay for each turn
        thread th(SDL_Delay, delay_ms); // Используется отдельный поток для того, чтобы ожидание было синхронным с игроком.
        auto turns = logic.find_best_turns(color); // Вызов функции для поиска лучшего хода. Возвращает непечень ходов. 
        th.join();
        bool is_first = true;
        // making moves
        for (auto turn : turns) // Выполняем все вернувшиеся из функции хода.
        {
            if (!is_first)
            {
                SDL_Delay(delay_ms);
            }
            is_first = false;
            beat_series += (turn.xb != -1);
            board.move_piece(turn, beat_series); // Передвижение фигур.
        }

        auto end = chrono::steady_clock::now();
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();
    }
    // Объект класса для получения действий игрока путем обращения к перечислению. 
    Response player_turn(const bool color) // Получаем цвет игрока.
    {
        // return 1 if quit
        // выполняется подсветка клеток на поле.
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns) // Проходим по всем ходам.
        {
            cells.emplace_back(turn.x, turn.y);
        }
        board.highlight_cells(cells);
        move_pos pos = { -1, -1, -1, -1 };
        POS_T x = -1, y = -1;
        // trying to make first move
        // Цикл обработки хода.
        while (true)
        {   // обработка первого хода.
            auto resp = hand.get_cell(); // Получаем результат действий игрока из класса Hand.
            if (get<0>(resp) != Response::CELL)
                return get<0>(resp);
            pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };

            bool is_correct = false;
            // Дальше условия проверки правильности хода.
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                if (turn == move_pos{ x, y, cell.first, cell.second })
                {
                    pos = turn;
                    break;
                }
            }
            if (pos.x != -1)
                break;
            if (!is_correct) // Вариант некорректного указания хода.
            {
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;
                y = -1;
                continue;
            }
            x = cell.first;
            y = cell.second;
            board.clear_highlight();
            board.set_active(x, y);
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            board.highlight_cells(cells2);
        }
        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1);
        if (pos.xb == -1)
            return Response::OK; // Если нет побитых, то делаем возврат, заканчивая ход игрока.
        // continue beating while can
        beat_series = 1; // Серия побитых ходов. 
        while (true) // цикл обработки на случай, если игрок смог побить вражескую единицу.
        {
            logic.find_turns(pos.x2, pos.y2); // Снова вызываем find_turns, но теперь от позиций фишки.
            if (!logic.have_beats)
                break; // Если ходов нет, заканчиваем цикл.

            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2);
            // trying to make move
            while (true)
            {
                auto resp = hand.get_cell(); // Снова ждем действий игрока. 
                if (get<0>(resp) != Response::CELL)
                    return get<0>(resp);
                pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };

                bool is_correct = false;
                for (auto turn : logic.turns) // Обработчик корректного нажатия. 
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }
                if (!is_correct) // Устал писать комменты... тут, если ход был совершен не верно, он просто перезапускает цикл. Точнее, пропускает данную его итерацию.
                    continue;
                // Конечные настройки поля и увеличение серии побития. 
                board.clear_highlight();
                board.clear_active();
                beat_series += 1;
                board.move_piece(pos, beat_series);
                break;
            }
        }

        return Response::OK;
    }
    // Объявление защищенных переменных.
private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
