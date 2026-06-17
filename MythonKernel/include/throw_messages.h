#pragma once

#include "declares.h"
#include <string>
#include <unordered_map>

#undef MYTHLON_PLUGIN
#include "plugin_helpers.h"

namespace runtime
{
    class ThrowMessages
    {
    public:
        ThrowMessages() = delete;
        ThrowMessages(const ThrowMessages&) = delete;
        ThrowMessages& operator=(const ThrowMessages&) = delete;
        // Возврат сообщения об ошибке, соответствующего коду throw_message_number.
        static const std::string& GetThrowText(ThrowMessageNumber throw_message_number);
        // Трафаретная генерация сложного сообщения об ошибке.
        static std::string ConstructThrowText(const std::string& text_pattern, std::vector<ThrowMessageNumber> throw_messages);

    private:
        static const std::unordered_map<ThrowMessageNumber, std::string> throw_messages_;
    };
} // namespace runtime
