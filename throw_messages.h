#pragma once

#include "declares.h"
#include <string>
#include <unordered_map>

namespace runtime
{
    enum class ThrowMessageNumber
    {
        THRM_UNKNOWN = 0,
        THRM_NOT_SUPPORT_FREE_FUNCTION,
        THRM_ARRAY_MUST_HAVE_DIMS,
        THRM_MAP_CTOR_HAS_NO_PARAMS,
        THRM_STR_HAS_ONE_PARAM,
        THRM_UNKNOWN_METHOD_CALL,
        THRM_POINTER_RET_TO_VAL_DENIED,
        THRM_POINTER_RET_TOL_LOCAL_VAR_DENIED,
        THRM_BASE_CLASS,
        THRM_NOT_FOUND_FOR_CLASS,
        THRM_CLASS,
        THRM_ALREADY_EXISTS,
        THRM_METHOD_NOT_FOUND,
        THRM_INDIRECT_ASSIGN_ERROR,
        THRM_VARIABLE_NOT_FOUND,
        THRM_IMPOSSIBLE_ADDITION,
        THRM_IMPOSSIBLE_SUBTRACTION,
        THRM_IMPOSSIBLE_MULTIPLICATION,
        THRM_IMPOSSIBLE_DIVISION,
        THRM_IMPOSSIBLE_COMPARE_EQUAL,
        THRM_IMPOSSIBLE_COMPARE_LESS,
        THRM_DIVISION_BY_ZERO,
        THRM_IMPOSSIBLE_MODULO_DIVISION,
        THRM_MODULO_DIVISION_BY_ZERO,
        THRM_NOT_DIGIT_SIZES,
        THRM_INVALID_ARRAY_INDEX,
        THRM_PUSH_BACK_ONE_DIM_ONLY,
        THRM_BACK_ONE_DIM_ONLY,
        THRM_POP_BACK_ONE_DIM_ONLY,
        THRM_ARRAY_IS_EMPTY,
        THRM_ITERATOR_IN_PROGRESS_INSERT,
        THRM_ITERATOR_IN_PROGRESS_ERASE,
        THRM_METHOD,
        THRM_ARGUMENTS,
        THRM_DEMAND_EQUAL,
        THRM_DEMAND_LESS_OR_EQUAL,
        THRM_DEMAND_GREATER_OR_EQUAL,
        THRM_PARAMETER,
        THRM_OF_METHOD,
        THRM_HAVE_INCOMPATIBLE_TYPE,
        THRM_DEMAND_ONE_ARGUMENT,
        THRM_FIRST_PARAM_OF_METHOD,
        THRM_MUST_BE_ITERATOR,
        THRM_IN_METHOD,
        THRM_ITERATOR_INVALID,
        THRM_MATH_CTOR_HAS_NO_PARAMS,
        THRM_INCORRECT_TOKEN_LIST,
        THRM_INVALID_IMPORT_FILENAME,
        THRM_DYNAMIC_LIBRARY_NOT_LOADED,
        THRM_CREATE_PLUGIN_NOT_FOUND,
        THRM_LOAD_PLUGIN_LIST_NOT_FOUND,
        THRM_INCLUDE_INVALID_PARAMS,
        THRM_NODE_NOT_ROOT,
        THRM_URGENT_TERMINATE
    };

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
