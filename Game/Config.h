#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"
// Класс для обработки настроек. 
class Config
{
public:
    // Объявление конструктора класса с функцией reload для настроек. 
    Config()
    {
        reload();
    }

    void reload()
    {   // Загрузка настроек из нашего файла. 
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }

    auto operator()(const string& setting_dir, const string& setting_name) const
    {
        return config[setting_dir][setting_name];
    }

private:
    // настройки загружаются в конфиг. 
    json config;
};
