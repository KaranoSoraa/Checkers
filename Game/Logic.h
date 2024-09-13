#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
public:
    Logic(Board* board, Config* config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine(
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        optimization = (*config)("Bot", "Optimization");
    }

    //vector<move_pos> find_best_turns(const bool color) // Функция для нахождения лучших ходов.
    //{
    //    next_best_state.clear();
    //    next_move.clear();

    //    find_first_best_turn(board->get_board(), color, -1, -1, 0); // Вызов функции по поиску лучшего первого хода

    //    int cur_state = 0;
    //    vector<move_pos> res;
    //    do
    //    {
    //        res.push_back(next_move[cur_state]);
    //        cur_state = next_best_state[cur_state];
    //    } while (cur_state != -1 && next_move[cur_state].x != -1);
    //    return res;
    //}
    vector<move_pos> find_best_turns(const bool color) {
        // Очищаем вектора движения и состояния.
        next_move.clear();
        next_best_state.clear();
        // Передаем матрицу(доску), цвет игрока, позицию(ничего не выбрано), состояние, Альфа-отсечение. 
        find_first_best_turn(board->get_board(), color, -1, -1, 0, -1); 

        vector<move_pos> res;
        int state = 0; 
        do {
            res.push_back(next_move[state]); // Добавляет в результат ход, который мы сделаем из текущего состояния. 
            state = next_best_state[state];
        } while (state != -1 && next_move[state].x != -1);
            return res;
    }
private:
    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
        double alpha = -1) {
        // Пока нет ходов, вычисляем далее.
        next_move.emplace_back(-1, -1, -1, -1);
        next_best_state.push_back(-1);
        // считаем все возможные ходы, если state != 0
        if (state != 0) {
            find_turns(x, y, mtx);
        }
        // создаем копии класса turns, так как при выполнении программы прошлые данные будут затераться. Так же с beats.
        auto now_turns = turns;
        auto now_have_beats = have_beats;

        //Если нечего бить и некуда ходить, то запускаем рекурсивный поиск для другого игрока.
        if (!now_have_beats && state != 0) {
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);
        }
        // Лучший результат на данный момент
        double best_score = -1; // Стандартное значение.
        // Проходим по всем ходам.
        for (auto turn : now_turns) {
            // Создаем новое состояние.
            size_t new_state = next_move.size();
            double score;
            if (now_have_beats) { // Если есть, что бить, то вызываем рекурсивную функцию для текущего игрока.
                score = find_best_turns_rec(make_turn(mtx, turn), color, turn.x2, turn.y2, new_state, best_score);
            }
            else { // Если бить нечего, вызываем от другого игрока.
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
            }
            if (score > best_score) { // Если score больше - обновляем значения классов. 
                best_score = score; // Данный ход стал лучшим, перезапись лучшего хода.
                next_move[state] = turn;
                next_best_state[state] = (now_have_beats ? new_state: -1);
            }
        }
        return best_score;

    }

    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
        double beta = INF + 1, const POS_T x = -1, const POS_T y = -1) {
        if (depth == Max_depth) { // При достижении максимальной глубины, делаем ретерн.
            return calc_score(mtx, (depth % 2 == color));
        }
        //Если есть серия побитий - ищем ходы.
        if (x != -1) {
            find_turns(x, y, mtx);
        }
        else {
            find_turns(color, mtx); // Иначе ищем все возможные ходы, доступные данному игроку.
        }
        // Сохраняем значения, по аналогии выше.
        auto now_turns = turns;
        auto now_have_beats = have_beats;
        // То же, что и сверху. 
        if (!now_have_beats && x != 0) {
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }
        //Проверяем, есть ли ходы.
        if (turns.empty()) {
            // Если есть, то делает return, вычисляя, победил или проиграл текущий игрок.
            return(depth % 2 ? 0 : INF); // Победа или поражение.
        }
        double min_score = INF + 1;
        double max_score = -1;
        for (auto turn : now_turns) {
            double score;
            //Если есть побития, продолжаем серию рекурсивным запуском. 
            if (now_have_beats) {
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }
            else {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }
            // Обновляет минимум и максимум.
            min_score = min(min_score, score);
            max_score = max(max_score, score);
            // Делаем Альфа-Бета отсечения.
            if (depth % 2) { // Если наш ход, то мы максимизатор.
                alpha = max(alpha, max_score);
            }
            else {
                beta = min(beta, min_score);
            }
            if (optimization != "O0" && alpha > beta) { // Если альфа больше беты, то делаем остановку цикла.
                break;
            }
            if (optimization != "O2" && alpha == beta) { // Есть вероятность недосчета начений, меняем возвращаемые параметры.
                return(depth % 2 ? max_score + 1: min_score - 1);
            }
        }
        return(depth % 2 ? max_score : min_score);
    }

    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        // Если задана начальная позиция фигуры, устанавливаем её как пустую (0).
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0;

        // Если фигура является пешкой и достигла конца доски, превращаем её в короля (1 -> 3, 2 -> 4).
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2;

        // Перемещаем фигуру на новую позицию и освобождаем старую позицию.
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y];
        mtx[turn.x][turn.y] = 0;

        // Возвращаем обновлённую матрицу.
        return mtx;
    }
        double calc_score(const vector<vector<POS_T>>& mtx, const bool first_bot_color) const
    {
        // Инициализация переменных для подсчета очков по фигурам разного типа.
        double w = 0, wq = 0, b = 0, bq = 0;

        // Перебор всех клеток доски.
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                // Подсчет количества белых и черных фигур, а также их королев.
                w += (mtx[i][j] == 1);
                wq += (mtx[i][j] == 3);
                b += (mtx[i][j] == 2);
                bq += (mtx[i][j] == 4);
            }
        }

        // Переключение переменных местами, если бот играет за черных.
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }

        // Проверка на нулевое количество фигур одного из цветов.
        if (w + wq == 0)
            return INF; // Если нет белых фигур, возвращаем бесконечность.
        if (b + bq == 0)
            return 0;   // Если нет черных фигур, возвращаем ноль.

        // Коэффициент для оценки королевских фигур.
        int q_coef = 4;

        // Расчет и возврат среднего коэффициента для оценки позиции.
        return (b + bq * q_coef) / (w + wq * q_coef);
    }

    //double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
    //    double alpha = -1)
    //{
    //    // Инициализация структур данных для следующего лучшего хода.
    //    next_best_state.push_back(-1);
    //    next_move.emplace_back(-1, -1, -1, -1);

    //    // Инициализация переменных для лучшего счета и текущих ходов.
    //    double best_score = -1;
    //    if (state != 0)
    //        find_turns(x, y, mtx); // Находим возможные ходы из текущей позиции.
    //    auto turns_now = turns;
    //    bool have_beats_now = have_beats;

    //    // Если нет возможности бить и это не начальное состояние, рекурсивно ищем лучший ход противника.
    //    if (!have_beats_now && state != 0)
    //    {
    //        return find_best_turns_rec(mtx, 1 - color, 0, alpha); 
    //    }

    //    // Векторы для лучших ходов и состояний.
    //    vector<move_pos> best_moves;
    //    vector<int> best_states;

    //    // Цикл по всем возможным ходам
    //    for (auto turn : turns_now)
    //    {
    //        size_t next_state = next_move.size();
    //        double score;
    //        // Выбор рекурсивной функции в зависимости от наличия возможности бить.
    //        if (have_beats_now)
    //        {
    //            score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, next_state, best_score);
    //        }
    //        else
    //        {
    //            score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
    //        }
    //        // Обновление лучшего счета и сохранение лучшего следующего хода и состояния.
    //        if (score > best_score)
    //        {
    //            best_score = score;
    //            next_best_state[state] = (have_beats_now ? int(next_state) : -1);
    //            next_move[state] = turn;
    //        }
    //    }
    //    // Возвращаем лучший найденный счет.
    //    return best_score;
    //}



    //double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
    //    double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
    //{
    //    // Базовый случай: достигнута максимальная глубина рекурсии
    //    if (depth == Max_depth)
    //    {
    //        return calc_score(mtx, (depth % 2 == color));
    //    }

    //    // Если заданы начальные координаты, находим возможные ходы из этой позиции
    //    if (x != -1)
    //    {
    //        find_turns(x, y, mtx);
    //    }
    //    else
    //    {
    //        find_turns(color, mtx);
    //    }
    //    auto turns_now = turns;
    //    bool have_beats_now = have_beats;

    //    // Если нет возможности бить и начальные координаты заданы, рекурсивно вызываем функцию для противника
    //    if (!have_beats_now && x != -1)
    //    {
    //        return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
    //    }

    //    // Если нет доступных ходов, возвращаем соответствующую оценку
    //    if (turns.empty())
    //    {
    //        return (depth % 2 ? 0 : INF);
    //    }

    //    // Инициализация переменных для оценок
    //    double min_score = INF + 1;
    //    double max_score = -1;

    //    // Цикл по всем возможным ходам
    //    for (auto turn : turns_now)
    //    {
    //        double score = 0.0;
    //        // Выбор рекурсивной функции в зависимости от наличия возможности бить
    //        if (!have_beats_now && x == -1)
    //        {
    //            score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
    //        }
    //        else
    //        {
    //            score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
    //        }

    //        // Обновление минимальной и максимальной оценок
    //        min_score = min(min_score, score);
    //        max_score = max(max_score, score);

    //        // Применение альфа-бета отсечения
    //        if (depth % 2)
    //        {
    //            alpha = max(alpha, max_score);
    //        }
    //        else
    //        {
    //            beta = min(beta, min_score);
    //        }

    //        // Проверка на отсечение в случае достаточной оптимизации
    //        if (optimization != "O0" && alpha >= beta)
    //        {
    //            return (depth % 2 ? max_score + 1 : min_score - 1);
    //        }
    //    }

    //    // Возвращаем лучшую оценку в зависимости от четности глубины
    //    return (depth % 2 ? max_score : min_score);
    //}

public:
    // Объявление перегруженной функции find_turns. В одном случае она принимает только цвет игрока, в другой - позицию. Её цель - нахождение возможных ходов.
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }

    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

private:
    // Приватный аналог find_turns. Так же ищет возможные ходы. Принимает в себя цвет и матрицу поля.
    void find_turns(const bool color, const vector<vector<POS_T>>& mtx)
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        for (POS_T i = 0; i < 8; ++i) // Проходим по полю. Если клетка совпадает с цветом, то идет новый поиск возможных ходов, но уже от другой клетки. 
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    find_turns(i, j, mtx);
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        shuffle(turns.begin(), turns.end(), rand_eng);
        have_beats = have_beats_before;
    }

    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>>& mtx) // Крайний поиск ходов от клетки. 
    {
        turns.clear();
        have_beats = false;
        POS_T type = mtx[x][y]; 
        // check beats
        switch (type) // Ветвление на определение, чем является тип данной фигуры. 
        {
        case 1:
        case 2:
            // check pieces - итак написано, что пешка. Ниже - королева. Всё это - возможность побития.
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }
        // check other turns
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
        {   // Все остальные варианты возможностей хода.
            POS_T i = ((type % 2) ? x - 1 : x + 1);
            for (POS_T j = y - 1; j <= y + 1; j += 2)
            {
                if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                    continue;
                turns.emplace_back(x, y, i, j);
            }
            break;
        }
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break;
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }
public:
    vector<move_pos> turns; // Вектор ходов, содержащий последовательность ходов.
    bool have_beats; // Флаг, указывающий на возможность бить.
    int Max_depth; // Максимальная глубина поиска в дереве принятия решений.

private:
    default_random_engine rand_eng; // Генератор случайных чисел по умолчанию.
    string optimization; // Строка для хранения информации об оптимизации.
    vector<move_pos> next_move; // Вектор следующего возможного хода.
    vector<int> next_best_state; // Вектор для хранения следующего лучшего состояния.
    Board* board; // Указатель на объект класса доски.
    Config* config; // Указатель на объект класса конфигурации.
};
