
/*

    Goldleaf - Multipurpose homebrew tool for Nintendo Switch
    Copyright (C) 2018-2021 XorTroll

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include <cfg/cfg_Settings.hpp>
#include <fs/fs_FileSystem.hpp>
#include <ui/ui_Utils.hpp>

namespace cfg {

    namespace {

        inline std::string ColorToHex(pu::ui::Color clr) {
            char str[0x20] = {0};
            sprintf(str, "#%02X%02X%02X%02X", clr.R, clr.G, clr.B, clr.A);
            return str;
        }

    }

    void Settings::Save() {
        auto json = JSON::object();
        
        if(this->has_custom_lang) {
            json["general"]["customLanguage"] = LanguageToString(this->custom_lang);
        }

        if(this->has_external_romfs) {
            json["general"]["externalRomFs"] = this->external_romfs;
        }
        
        if(this->has_menu_item_size) {
            json["ui"]["menuItemSize"] = this->menu_item_size;
        }

        if(this->has_custom_scheme) {
            json["ui"]["base"] = ColorToHex(this->custom_scheme.Base);
            json["ui"]["baseFocus"] = ColorToHex(this->custom_scheme.BaseFocus);
            json["ui"]["text"] = ColorToHex(this->custom_scheme.Text);
        }

        if(this->has_scrollbar_color) {
            json["ui"]["scrollBar"] = ColorToHex(this->scrollbar_color);
        }

        if(this->has_progressbar_color) {
            json["ui"]["progressBar"] = ColorToHex(this->progressbar_color);
        }

        json["installs"]["ignoreRequiredFwVersion"] = this->ignore_required_fw_ver;
        
        for(u32 i = 0; i < this->bookmarks.size(); i++) {
            const auto &bmk = this->bookmarks[i];
            json["web"]["bookmarks"][i]["name"] = bmk.name;
            json["web"]["bookmarks"][i]["url"] = bmk.url;
        }
        auto sd_exp = fs::GetSdCardExplorer();
        sd_exp->DeleteFile(consts::Settings);

        const auto settings_data = json.dump(4);
        sd_exp->WriteFile(consts::Settings, settings_data.c_str(), settings_data.length());
    }

    std::string Settings::PathForResource(const std::string &Path) {
        auto outres = "romfs:" + Path;
        if(this->has_external_romfs) {
            const auto &tmpres = this->external_romfs + "/" + Path;
            auto sd_exp = fs::GetSdCardExplorer();
            if(sd_exp->IsFile(tmpres)) {
                outres = tmpres;
            }
        }
        return outres;
    }

    void Settings::ApplyScrollBarColor(pu::ui::elm::Menu::Ref &menu) {
        if(this->has_scrollbar_color) {
            menu->SetScrollbarColor(this->scrollbar_color);
        }
    }

    void Settings::ApplyProgressBarColor(pu::ui::elm::ProgressBar::Ref &p_bar) {
        if(this->has_progressbar_color) {
            p_bar->SetProgressColor(this->progressbar_color);
        }
    }

    Settings ProcessSettings() {
        Settings settings = {};

        u64 lang_code = 0;
        auto lang = SetLanguage_ENUS;
        setGetSystemLanguage(&lang_code);
        setMakeLanguage(lang_code, &lang);
        
        switch(lang) {
            case SetLanguage_ENUS:
            case SetLanguage_ENGB: {
                settings.custom_lang = Language::English;
                break;
            }
            case SetLanguage_FR:
            case SetLanguage_FRCA: {
                settings.custom_lang = Language::French;
                break;
            }
            case SetLanguage_DE: {
                settings.custom_lang = Language::German;
                break;
            }
            case SetLanguage_IT: {
                settings.custom_lang = Language::Italian;
                break;
            }
            case SetLanguage_ES:
            case SetLanguage_ES419: {
                settings.custom_lang = Language::Spanish;
                break;
            }
            case SetLanguage_NL: {
                settings.custom_lang = Language::Dutch;
                break;
            }
            default: {
                settings.custom_lang = Language::English;
                break;
            }
        }

        settings.has_custom_lang = false;
        settings.has_external_romfs = false;
        settings.has_scrollbar_color = false;
        settings.has_progressbar_color = false;
        settings.has_custom_scheme = false;
        settings.has_menu_item_size = false;

        settings.menu_item_size = 80;
        settings.ignore_required_fw_ver = true;

        settings.custom_scheme = ui::GenerateRandomScheme();

        // TODO: read with sdexp?
        std::ifstream ifs("sdmc:/" + consts::Settings);
        if(ifs.good()) {
            const auto &settings_json = JSON::parse(ifs);
            if(settings_json.count("general")) {
                const auto &lang = settings_json["general"].value("customLanguage", "");
                if(!lang.empty()) {
                    auto clang = StringToLanguage(lang);
                    settings.has_custom_lang = true;
                    settings.custom_lang = clang;
                }

                const auto &extrom = settings_json["general"].value("externalRomFs", "");
                if(!extrom.empty()) {
                    settings.has_external_romfs = true;
                    if(extrom.substr(0, 6) == "sdmc:/") {
                        settings.external_romfs = extrom;
                    }
                    else {
                        settings.external_romfs = "sdmc:";
                        if(extrom[0] != '/') {
                            settings.external_romfs += "/";
                        }
                        settings.external_romfs += extrom;
                    }
                }
            }

            if(settings_json.count("ui")) {
                const auto itemsize = settings_json["ui"].value("menuItemSize", 0);
                if(itemsize > 0) {
                    settings.has_menu_item_size = true;
                    settings.menu_item_size = itemsize;
                }
                const auto &background_clr = settings_json["ui"].value("background", "");
                if(!background_clr.empty()) {
                    settings.has_custom_scheme = true;
                    settings.custom_scheme.Background = pu::ui::Color::FromHex(background_clr);
                }
                const auto &base_clr = settings_json["ui"].value("base", "");
                if(!base_clr.empty()) {
                    settings.has_custom_scheme = true;
                    settings.custom_scheme.Base = pu::ui::Color::FromHex(base_clr);
                }
                const auto &base_focus_clr = settings_json["ui"].value("baseFocus", "");
                if(!base_focus_clr.empty()) {
                    settings.has_custom_scheme = true;
                    settings.custom_scheme.BaseFocus = pu::ui::Color::FromHex(base_focus_clr);
                }
                const auto &text_clr = settings_json["ui"].value("text", "");
                if(!text_clr.empty()) {
                    settings.has_custom_scheme = true;
                    settings.custom_scheme.Text = pu::ui::Color::FromHex(text_clr);
                }
                const auto &scrollbar_clr = settings_json["ui"].value("scrollBar", "");
                if(!scrollbar_clr.empty()) {
                    settings.has_scrollbar_color = true;
                    settings.scrollbar_color = pu::ui::Color::FromHex(scrollbar_clr);
                }
                const auto &pbar_clr = settings_json["ui"].value("progressBar", "");
                if(!pbar_clr.empty()) {
                    settings.has_progressbar_color = true;
                    settings.progressbar_color = pu::ui::Color::FromHex(pbar_clr);
                }
            }
            if(settings_json.count("installs")) {
                settings.ignore_required_fw_ver = settings_json["installs"].value("ignoreRequiredFwVersion", true);
            }
            if(settings_json.count("web")) {
                if(settings_json["web"].count("bookmarks")) {
                    for(u32 i = 0; i < settings_json["web"]["bookmarks"].size(); i++) {
                        WebBookmark bmk = {};
                        bmk.name = settings_json["web"]["bookmarks"][i].value("name", "");
                        bmk.url = settings_json["web"]["bookmarks"][i].value("url", "");
                        if(!bmk.url.empty() && !bmk.name.empty()) {
                            settings.bookmarks.push_back(bmk);
                        }
                    }
                }
            }
            ifs.close();
        }
        return settings;
    }

    bool Exists() {
        auto sd_exp = fs::GetSdCardExplorer();
        return sd_exp->IsFile(consts::Settings);
    }
}
