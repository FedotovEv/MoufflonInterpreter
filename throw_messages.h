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
        static const std::string& GetThrowText(ThrowMessageNumber thow_message_number)
        {
            if (throw_messages_.count(thow_message_number))
                return throw_messages_.at(thow_message_number);
            else
                return throw_messages_.at(ThrowMessageNumber::THRM_UNKNOWN);
        }

    private:
        static const std::unordered_map<ThrowMessageNumber, std::string> throw_messages_;
    };
} // namespace runtime
