#pragma once

#include <string>
#include <vector>
#include <functional>
#include <variant>

namespace runtime
{
    enum class LinkCallReason
    {
        CALL_REASON_UNKNOWN = 0,
        CALL_REASON_READ_FIELD,
        CALL_REASON_WRITE_FIELD,
        CALL_REASON_CALL_METHOD
    };

    using LinkageReturn = std::variant<int, std::string>;
    using LinkageFunction = std::function<LinkageReturn(LinkCallReason, const std::string&,
        const std::vector<std::string>&)>;

    struct ProgramCommandDescriptor
    {
        int module_id = 0;
        int module_string_number = 0;
    };
} // namespace return
