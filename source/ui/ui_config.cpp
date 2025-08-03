#include <liarsdice/ui/ui_config.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>
#include <sstream>

namespace liarsdice::ui {

void UIConfig::load_from_file(const std::string& filepath) {
    boost::property_tree::ptree pt;
    boost::property_tree::read_info(filepath, pt);
    load_from_info(pt);
}

void UIConfig::load_from_info(const boost::property_tree::ptree& pt) {
    // Load menus
    if (auto menus_opt = pt.get_child_optional("menus")) {
        for (const auto& [menu_id, menu_pt] : menus_opt.get()) {
            Menu menu;
            menu.id = menu_id;
            menu.title = menu_pt.get<std::string>("title", "");
            menu.prompt = menu_pt.get<std::string>("prompt", "");
            if (auto back = menu_pt.get_optional<std::string>("back_option")) {
                menu.back_option = *back;
            }
            
            // Load menu items
            if (auto items_opt = menu_pt.get_child_optional("items")) {
                for (const auto& [item_id, item_pt] : items_opt.get()) {
                    MenuItem item;
                    item.id = item_id;
                    item.label = item_pt.get<std::string>("label", "");
                    item.shortcut = item_pt.get<std::string>("shortcut", "");
                    item.description = item_pt.get<std::string>("description", "");
                    menu.items.push_back(std::move(item));
                }
            }
            
            menus_[menu_id] = std::move(menu);
        }
    }
    
    // Load prompts
    if (auto prompts_opt = pt.get_child_optional("prompts")) {
        for (const auto& [prompt_id, prompt_pt] : prompts_opt.get()) {
            Prompt prompt;
            prompt.id = prompt_id;
            prompt.text = prompt_pt.get<std::string>("text", "");
            prompt.default_value = prompt_pt.get<std::string>("default_value", "");
            prompt.validation_pattern = prompt_pt.get<std::string>("validation_pattern", "");
            prompt.error_message = prompt_pt.get<std::string>("error_message", "");
            prompts_[prompt_id] = std::move(prompt);
        }
    }
    
    // Load messages
    if (auto messages_opt = pt.get_child_optional("messages")) {
        for (const auto& [msg_id, msg_text] : messages_opt.get()) {
            Message message;
            message.id = msg_id;
            message.template_text = msg_text.data();
            
            // Extract placeholders from template
            std::string temp = message.template_text;
            size_t start = 0;
            while ((start = temp.find('{', start)) != std::string::npos) {
                size_t end = temp.find('}', start);
                if (end != std::string::npos) {
                    message.placeholders.push_back(temp.substr(start + 1, end - start - 1));
                    start = end + 1;
                } else {
                    break;
                }
            }
            
            messages_[msg_id] = std::move(message);
        }
    }
    
    // Load simple texts
    if (auto texts_opt = pt.get_child_optional("texts")) {
        for (const auto& [text_id, text_value] : texts_opt.get()) {
            texts_[text_id] = text_value.data();
        }
    }
    
    // Load theme
    if (auto theme_opt = pt.get_child_optional("theme")) {
        theme_ = theme_opt.get();
    }
}

const UIConfig::Menu* UIConfig::get_menu(const std::string& menu_id) const {
    auto it = menus_.find(menu_id);
    return it != menus_.end() ? &it->second : nullptr;
}

const UIConfig::Prompt* UIConfig::get_prompt(const std::string& prompt_id) const {
    auto it = prompts_.find(prompt_id);
    return it != prompts_.end() ? &it->second : nullptr;
}

std::string UIConfig::get_message(const std::string& message_id, 
                                 const std::unordered_map<std::string, std::string>& params) const {
    auto it = messages_.find(message_id);
    if (it == messages_.end()) {
        return "";
    }
    
    return replace_placeholders(it->second.template_text, params);
}

std::string UIConfig::get_text(const std::string& text_id) const {
    auto it = texts_.find(text_id);
    return it != texts_.end() ? it->second : "";
}

std::string UIConfig::get_color(const std::string& element) const {
    return theme_.get<std::string>("colors." + element, "");
}

std::string UIConfig::get_style(const std::string& element) const {
    return theme_.get<std::string>("styles." + element, "");
}

std::string UIConfig::replace_placeholders(const std::string& template_text, 
                                          const std::unordered_map<std::string, std::string>& params) const {
    std::string result = template_text;
    
    for (const auto& [key, value] : params) {
        std::string placeholder = "{" + key + "}";
        boost::algorithm::replace_all(result, placeholder, value);
    }
    
    return result;
}

} // namespace liarsdice::ui