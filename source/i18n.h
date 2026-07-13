//
// Created by void on 08/06/2026.
//

#ifndef SWITCHPOST_I18N_H
#define SWITCHPOST_I18N_H
#include <string>

namespace i18n {
    bool SetLanguage(std::string language);
    std::string GetLanguage();
    std::string GetString(std::string key);
}

#endif //SWITCHPOST_I18N_H
