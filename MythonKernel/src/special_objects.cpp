
#include "statement.h"
#include "parse.h"
#include "throw_messages.h"
#include "error_classes.h"

#include <cassert>
#include <optional>
#include <sstream>

using namespace std;
using namespace runtime;

namespace ast
{
    NewArray::NewArray(std::vector<std::unique_ptr<Statement>> args) : args_(move(args))
    {
        if (!args_.size())
            throw ParseError(ThrowMessageNumber::THRM_ARRAY_MUST_HAVE_DIMS);
    }

    runtime::ObjectHolder NewArray::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        std::vector<int> elements_count;
        for (auto& cur_param_ptr : args_)
        {
            ObjectHolder cur_count_object = cur_param_ptr->Execute(closure, context);
            runtime::Number* cur_element_count_ptr = cur_count_object.TryAs<runtime::Number>();
            if (cur_element_count_ptr)
                elements_count.push_back(cur_element_count_ptr->GetIntValue());
            else
                ThrowRuntimeError(this, ThrowMessageNumber::THRM_ARRAY_SIZE_NOT_NUMERIC);
        }

        return ObjectHolder::Own(runtime::ArrayInstance(move(elements_count)));
    }

    NewMap::NewMap(std::vector<std::unique_ptr<Statement>> args)
    {
        if (args.size())
            throw ParseError(ThrowMessageNumber::THRM_MAP_CTOR_HAS_NO_PARAMS);
    }

    runtime::ObjectHolder NewMap::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        return ObjectHolder::Own(runtime::MapInstance());
    }

    NewTypeTraits::NewTypeTraits(std::vector<std::unique_ptr<Statement>> args) : args_(move(args))
    {
        if (args_.size() != 1)
            throw ParseError(ThrowMessageNumber::THRM_STR_HAS_ONE_PARAM);
    }

    // Возвращает объект, содержащий значение типа TypeTraits, представляющее собой характеристический тип для значения первого
    // аргумента args_[0], ранее переданного в конструктор данного класса.
    runtime::ObjectHolder NewTypeTraits::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        ObjectHolder traits_value = args_[0]->Execute(closure, context);
        return ObjectHolder::Own(runtime::TypeTraitsInstance(move(traits_value)));
    }

    unique_ptr<Statement> CreateArray(vector<unique_ptr<Statement>> args)
    {
        return make_unique<NewArray>(NewArray(move(args)));
    }

    unique_ptr<Statement> CreateMap(vector<unique_ptr<Statement>> args)
    {
        return make_unique<NewMap>(NewMap(move(args)));
    }

    unique_ptr<Statement> CreateTypeTraits(std::vector<std::unique_ptr<Statement>> args)
    {
        return make_unique<NewTypeTraits>(NewTypeTraits(move(args)));
    }
} // namespace ast

namespace runtime
{
    const unordered_map<string_view, ArrayInstance::ArrayCallMethod> ArrayInstance::array_method_table_
    {   
        {"get"sv, &ArrayInstance::MethodGet},
        {"Get"sv, &ArrayInstance::MethodGet},
        {"get_array_dimensions"sv, &ArrayInstance::MethodGetArrayDimensions},
        {"GetArrayDimensions"sv, &ArrayInstance::MethodGetArrayDimensions},
        {"get_dimension_count"sv, &ArrayInstance::MethodGetDimensionCount},
        {"GetDimensionCount"sv, &ArrayInstance::MethodGetDimensionCount},
        {"resize"sv, &ArrayInstance::MethodResize},
        {"Resize"sv, &ArrayInstance::MethodResize},
        {"clear"sv, &ArrayInstance::MethodClear},
        {"Clear"sv, &ArrayInstance::MethodClear},
        {"push_back"sv, &ArrayInstance::MethodPushBack},
        {"PushBack"sv, &ArrayInstance::MethodPushBack},
        {"back"sv, &ArrayInstance::MethodBack},
        {"Back"sv, &ArrayInstance::MethodBack},
        {"pop_back"sv, &ArrayInstance::MethodPopBack},
        {"PopBack"sv, &ArrayInstance::MethodPopBack}
    };

    const unordered_map<string_view, pair<size_t, size_t>> ArrayInstance::array_method_argument_count_
    {   
        {"get"sv, {0, UINT_MAX}},
        {"Get"sv, {0, UINT_MAX}},
        {"get_array_dimensions"sv, {0, 0}},
        {"GetArrayDimensions"sv, {0, 0}},
        {"get_dimension_count"sv, {1, 1}},
        {"GetDimensionCount"sv, {1, 1}},
        {"resize"sv, {0, UINT_MAX}},
        {"Resize"sv, {0, UINT_MAX}},
        {"clear"sv, {0, 0}},
        {"Clear"sv, {0, 0}},
        {"push_back"sv, {1, 1}},
        {"PushBack"sv, {1, 1}},
        {"back"sv, {0, 0}},
        {"Back"sv, {0, 0}},
        {"pop_back"sv, {0, 0}},
        {"PopBack"sv, {0, 0}}
    };

    const unordered_map<string_view, MapInstance::MapCallMethod> MapInstance::map_method_table_
    {
        {"insert"sv, &MapInstance::MethodInsert},
        {"Insert"sv, &MapInstance::MethodInsert},
        {"find"sv, &MapInstance::MethodFind},
        {"Find"sv, &MapInstance::MethodFind},
        {"erase"sv, &MapInstance::MethodErase},
        {"Erase"sv, &MapInstance::MethodErase},
        {"contains"sv, &MapInstance::MethodContains},
        {"Contains"sv, &MapInstance::MethodContains},
        {"clear"sv, &MapInstance::MethodClear},
        {"Clear"sv, &MapInstance::MethodClear},
        {"begin"sv, &MapInstance::MethodBegin},
        {"Begin"sv, &MapInstance::MethodBegin},
        {"previous"sv, &MapInstance::MethodPrevious},
        {"Previous"sv, &MapInstance::MethodPrevious},
        {"next"sv, &MapInstance::MethodNext},
        {"Next"sv, &MapInstance::MethodNext},
        {"key"sv, &MapInstance::MethodKey},
        {"Key"sv, &MapInstance::MethodKey},
        {"value"sv, &MapInstance::MethodValue},
        {"Value"sv, &MapInstance::MethodValue},
        {"is_cursor_begin"sv, &MapInstance::MethodIsCursorBegin},
        {"IsCursorBegin"sv, &MapInstance::MethodIsCursorBegin},
        {"is_cursor_end"sv, &MapInstance::MethodIsCursorEnd},
        {"IsCursorEnd"sv, &MapInstance::MethodIsCursorEnd},
        {"release"sv, &MapInstance::MethodRelease},
        {"Release"sv, &MapInstance::MethodRelease}
    };

    const unordered_map<string_view, pair<size_t, size_t>> MapInstance::map_method_argument_count_
    {
        {"insert"sv, {2, 2}},
        {"Insert"sv, {2, 2}},
        {"find"sv, {1, 1}},
        {"Find"sv, {1, 1}},
        {"erase"sv, {1, 1}},
        {"Erase"sv, {1, 1}},
        {"contains"sv, {1, 1}},
        {"Contains"sv, {1, 1}},
        {"clear"sv, {0, 0}},
        {"Clear"sv, {0, 0}},
        {"begin"sv, {0, 0}},
        {"Begin"sv, {0, 0}},
        {"previous"sv, {1, 1}},
        {"Previous"sv, {1, 1}},
        {"next"sv, {1, 1}},
        {"Next"sv, {1, 1}},
        {"key"sv, {1, 1}},
        {"Key"sv, {1, 1}},
        {"value"sv, {1, 1}},
        {"Value"sv, {1, 1}},
        {"is_cursor_begin"sv, {1, 1}},
        {"IsCursorBegin"sv, {1, 1}},
        {"is_cursor_end"sv, {1, 1}},
        {"IsCursorEnd"sv, {1, 1}},
        {"release"sv, {0, 0}},
        {"Release"sv, {0, 0}}
    };

    const unordered_map<string_view, CoroutineInstance::CoroutineCallMethod> CoroutineInstance::coroutine_method_table_
    {
        {"resume"sv, &CoroutineInstance::MethodResume},
        {"Resume"sv, &CoroutineInstance::MethodResume},
        {"is_started"sv, &CoroutineInstance::MethodIsStarted},
        {"IsStarted"sv, &CoroutineInstance::MethodIsStarted},
        {"is_awaiting"sv, &CoroutineInstance::MethodIsAwaiting},
        {"IsAwaiting"sv, &CoroutineInstance::MethodIsAwaiting},
        {"value"sv, &CoroutineInstance::MethodValue},
        {"Value"sv, &CoroutineInstance::MethodValue},
        {"get_awaitable"sv, &CoroutineInstance::MethodGetAwaitable},
        {"GetAwaitable"sv, &CoroutineInstance::MethodGetAwaitable},
        {"set_awaitable"sv, &CoroutineInstance::MethodSetAwaitable},
        {"SetAwaitable"sv, &CoroutineInstance::MethodSetAwaitable},
        {"suspend_type"sv, &CoroutineInstance::MethodSuspendType},
        {"SuspendType"sv, &CoroutineInstance::MethodSuspendType},
        {"is_free_function"sv, &CoroutineInstance::MethodIsFreeFunction},
        {"IsFreeFunction"sv, &CoroutineInstance::MethodIsFreeFunction}
    };

    const unordered_map<string_view, pair<size_t, size_t>> CoroutineInstance::coroutine_method_argument_count_
    {
        {"resume"sv, {0, 0}},
        {"Resume"sv, {0, 0}},
        {"is_started"sv, {0, 0}},
        {"IsStarted"sv, {0, 0}},
        {"is_awaiting"sv, {0, 0}},
        {"IsAwaiting"sv, {0, 0}},
        {"value"sv, {0, 0}},
        {"Value"sv, {0, 0}},
        {"get_awaitable"sv, {0, 0}},
        {"GetAwaitbale"sv, {0, 0}},
        {"set_awaitable"sv, {1, 1}},
        {"SetAwaitbale"sv, {1, 1}},
        {"suspend_type"sv, {0, 0}},
        {"SuspendType"sv, {0, 0}},
        {"is_free_function"sv, {0, 0}},
        {"IsFreeFunction"sv, {0, 0}}
    };

    int MapInstance::last_iterator_pack_serial_ = 0;

    void CheckMethodParams(Context& context, const string& method_name,
                           MethodParamCheckMode check_mode,
                           MethodParamType param_type, size_t required_params,
                           const vector<ObjectHolder>& actual_args)
    {
        static constexpr int PARAM_CHECK_QUANTITY_MASK = 3;
        string pattern_text = "%1"s + method_name + "%2"s + to_string(required_params) + "%3", err_mess;

        switch (check_mode & PARAM_CHECK_QUANTITY_MASK)
        {
        case MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL:
            if (actual_args.size() != required_params)
            {
                err_mess = ThrowMessages::ConstructThrowText(pattern_text,
                    {ThrowMessageNumber::THRM_METHOD, ThrowMessageNumber::THRM_DEMAND_EQUAL, ThrowMessageNumber::THRM_ARGUMENTS});
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, err_mess);
            }
            break;
        case MethodParamCheckMode::PARAM_CHECK_QUANTITY_LESS_EQ:
            if (actual_args.size() > required_params)
            {
                err_mess = ThrowMessages::ConstructThrowText(pattern_text,
                    {ThrowMessageNumber::THRM_METHOD, ThrowMessageNumber::THRM_DEMAND_LESS_OR_EQUAL, ThrowMessageNumber::THRM_ARGUMENTS});
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, err_mess);
            }
            break;
        case MethodParamCheckMode::PARAM_CHECK_QUANTITY_GREATER_EQ:
            if (actual_args.size() < required_params)
            {
                err_mess = ThrowMessages::ConstructThrowText(pattern_text,
                    {ThrowMessageNumber::THRM_METHOD, ThrowMessageNumber::THRM_DEMAND_GREATER_OR_EQUAL, ThrowMessageNumber::THRM_ARGUMENTS});
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, err_mess);
            }
            break;
        default:
            break;
        }

        if (check_mode & MethodParamCheckMode::PARAM_CHECK_TYPE)
        {
            bool is_throw_exception = false;
            // Если флаг PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS установлен, рассматриваем соответствие типа только для первых
            // required_params параметров. Иначе проверяем все имеющиеся.
            size_t i_max = check_mode & MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS ? required_params : actual_args.size();
            for (size_t i = 1; i < i_max; ++i)
            {
                auto& current_param = actual_args[i];
                if (current_param)
                {
                    if (current_param.TryAs<Number>() && !(param_type & MethodParamType::PARAM_TYPE_NUMERIC))
                        is_throw_exception = true;
                    if (current_param.TryAs<String>() && !(param_type & MethodParamType::PARAM_TYPE_STRING))
                        is_throw_exception = true;
                    if (current_param.TryAs<Bool>() && !(param_type & MethodParamType::PARAM_TYPE_LOGICAL))
                        is_throw_exception = true;
                }
                else
                {
                    is_throw_exception = !(param_type & MethodParamType::PARAM_TYPE_NONE);
                }

                if (is_throw_exception)
                {
                    err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_PARAMETER) + to_string(i) +
                        ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_OF_METHOD) + method_name +
                        ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_HAVE_INCOMPATIBLE_TYPE);
                    ThrowRuntimeError(context, ThrowMessageNumber::THRM_PARAMS_TYPE_INCONSISTENCY, err_mess);
                }
            }
        }
    }

    ArrayInstance::ArrayInstance(std::vector<int> elements_count) : elements_count_(move(elements_count))
    {
        int total_elements = 1;
        for (size_t i = 0; i < elements_count_.size(); ++i)
            total_elements *= elements_count_[i];

        data_storage_.resize(total_elements, ObjectHolder::None());
    }

    void ArrayInstance::Print(std::ostream& os, Context& context)
    {
        os << "Arr:" << elements_count_.size();
        for (size_t i = 0; i < elements_count_.size(); ++i)
            os << ':' << elements_count_[i];
    }

    ObjectHolder ArrayInstance::MethodGet(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                          Context& context)
    {
        CheckMethodParams(context, "Get"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
            MethodParamType::PARAM_TYPE_NUMERIC, elements_count_.size(), actual_args);

        int absolute_element_number = 0;
        for (size_t current_index_num = 0; current_index_num < elements_count_.size();
            ++current_index_num)
        {
            int current_index_value = actual_args[current_index_num].TryAs<runtime::Number>()->GetIntValue();

            if (current_index_value < 0 || current_index_value >= elements_count_[current_index_num])
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_ARRAY_INDEX);
            if (current_index_num > 0)
                absolute_element_number *= elements_count_[current_index_num - 1];
            absolute_element_number += current_index_value;
        }

        return ObjectHolder::Own(PointerObject(&data_storage_[absolute_element_number]));
    }

    ObjectHolder ArrayInstance::MethodGetArrayDimensions(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                                         Context& context)
    {
        CheckMethodParams(context, "GetArrayDimensions"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
            MethodParamType::PARAM_TYPE_ANY, 0, actual_args);

        return ObjectHolder::Own(runtime::Number(static_cast<int>(elements_count_.size())));
    }

    ObjectHolder ArrayInstance::MethodGetDimensionCount(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                                        Context& context)
    {
        CheckMethodParams(context, "GetDimensionCount"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
            MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);

        runtime::Number* dimension_number_ptr = actual_args[0].TryAs<runtime::Number>();
        int dimension_number = dimension_number_ptr->GetIntValue();
        if (dimension_number >= 1 && dimension_number <= static_cast<int>(elements_count_.size()))
            return ObjectHolder::Own(runtime::Number(elements_count_[dimension_number - 1]));
        else
            return ObjectHolder::Own(runtime::Number(-1));
    }

    ObjectHolder ArrayInstance::MethodResize(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                             Context& context)
    {
        CheckMethodParams(context, "Resize"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ,
            MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);

        size_t old_dimensions_count = elements_count_.size();
        elements_count_.clear();
        int total_elements = 1;
        for (const ObjectHolder& current_index_object : actual_args)
        {
            runtime::Number* current_index_ptr = current_index_object.TryAs<runtime::Number>();
            elements_count_.push_back(current_index_ptr->GetIntValue());
            total_elements *= elements_count_.back();
        }

        if (old_dimensions_count != 1 || elements_count_.size() != 1)
            data_storage_.clear();
        data_storage_.resize(total_elements, ObjectHolder::None());
        return ObjectHolder::None();
    }

    ObjectHolder ArrayInstance::MethodClear(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                            Context& context)
    {
        CheckMethodParams(context, "Clear"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
            MethodParamType::PARAM_TYPE_ANY, 0, actual_args);
            
        if (elements_count_.size() == 1)
        {
            data_storage_.clear();
            elements_count_[0] = 0;
        }
        else
        {
            for (ObjectHolder& current_object : data_storage_)
                current_object = ObjectHolder::None();
        }        
        return ObjectHolder::None();
    }

    ObjectHolder ArrayInstance::MethodPushBack(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                               Context& context)
    {
        CheckMethodParams(context, "PushBack"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
            MethodParamType::PARAM_TYPE_ANY, 1, actual_args);

        if (elements_count_.size() == 1)
        {
            data_storage_.push_back(actual_args[0]);
            ++elements_count_[0];
        }
        else
        {
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_PUSH_BACK_ONE_DIM_ONLY);
        }

        return ObjectHolder::None();
    }

    ObjectHolder ArrayInstance::MethodBack(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                           Context& context)
    {
        CheckMethodParams(context, "Back"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
            MethodParamType::PARAM_TYPE_ANY, 0, actual_args);    
    
        if (elements_count_.size() == 1)
        {
            if (!data_storage_.empty())
                return data_storage_.back();
            else
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_ARRAY_IS_EMPTY);
        }
        else
        {
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_BACK_ONE_DIM_ONLY);
        }
    }

    ObjectHolder ArrayInstance::MethodPopBack(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                              Context& context)
    {
        CheckMethodParams(context, "PopBack"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
            MethodParamType::PARAM_TYPE_ANY, 0, actual_args);

        if (elements_count_.size() == 1)
        {
            if (elements_count_[0])
            {
                data_storage_.pop_back();
                --elements_count_[0];
            }
        }
        else
        {
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_POP_BACK_ONE_DIM_ONLY);
        }

        return ObjectHolder::None();
    }

    ObjectHolder ArrayInstance::Call(const std::string& method_name, const std::vector<ObjectHolder>& actual_args,
                                     Context& context, const std::string& parent_name)
    {
        if (array_method_table_.count(method_name))
            return (this->*array_method_table_.at(method_name))(method_name, actual_args, context);
        else
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
    }

    bool ArrayInstance::HasMethod(const string& method_name, size_t argument_count, const std::string& parent_name) const
    {
        if (!parent_name.empty())
            return false;   // Метод не имеет предков, поэтому не существует методов с непустыми спецификаторами предшественников в иерархии наследования.
        if (array_method_argument_count_.count(method_name))
        {
            auto argument_org_count = array_method_argument_count_.at(method_name);
            return argument_count >= argument_org_count.first &&
                   argument_count <= argument_org_count.second;
        }
        else
        {
            return false;
        }
    }

    MapCursor::MapCursor(MapInstance & map_instance, map<string, ObjectHolder> & map_storage) :
        map_instance_ref_(map_instance), map_storage_ref_(map_storage), map_iterator_(map_storage.begin()),
        iterator_pack_serial_(map_instance.AllocIteratorPackSerial())
    {}

    bool MapCursor::Begin()
    {
        map_iterator_ = map_storage_ref_.begin();
        return map_iterator_ != map_storage_ref_.end();
    }

    bool MapCursor::CursorLowerBound(const string& map_key)
    {
        map_iterator_ = map_storage_ref_.lower_bound(map_key);
        return map_iterator_ != map_storage_ref_.end();
    }

    ObjectHolder MapCursor::CursorGetKey()
    {
        if (map_iterator_ != map_storage_ref_.end())
            return ObjectHolder::Own(String(map_iterator_->first));
        else
            return ObjectHolder::None();
    }

    ObjectHolder MapCursor::CursorGetValue()
    {
        if (map_iterator_ != map_storage_ref_.end())
            return ObjectHolder::Own(PointerObject(&(map_iterator_->second)));
        else
            return ObjectHolder::Own(PointerObject());
    }

    bool MapCursor::CursorNext()
    {
        if (map_iterator_ != map_storage_ref_.end())
            ++map_iterator_;
        return map_iterator_ != map_storage_ref_.end();
    }

    bool MapCursor::CursorPrevious()
    {
        if (map_iterator_ != map_storage_ref_.begin())
            --map_iterator_;
        return map_iterator_ != map_storage_ref_.begin();
    }

    bool MapCursor::IsCursorEnd()
    {
        return map_iterator_ == map_storage_ref_.end();
    }

    bool MapCursor::IsCursorBegin()
    {
        return map_iterator_ == map_storage_ref_.begin();
    }

    void MapCursor::Print(std::ostream& os, Context& context)
    {
        os << "MapIter:" << iterator_pack_serial_ << ' ' << boolalpha << IsCursorValid();
    }

    bool MapCursor::IsCursorValid()
    {
        return map_instance_ref_.GetIteratorModeFlag() &&
            map_instance_ref_.GetIteratorPackSerial() == iterator_pack_serial_;
    }

    void MapInstance::Print(std::ostream& os, Context& context)
    {
        os << "Map:" << map_storage_.size();
        os << ' ' << boolalpha << is_in_iterator_mode_ << ' ' << iterator_pack_serial_;
    }

    void CheckMapIteratorParam(Context& context, const string& method_name,
                               const vector<ObjectHolder>& actual_args)
    {
        string err_mess;

        if (actual_args.size() != 1)
        {
            err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_METHOD) + method_name +
                       ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_DEMAND_ONE_ARGUMENT);
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, err_mess);
        }

        MapCursor* map_cursor_ptr = actual_args[0].TryAs<MapCursor>();
        if (!map_cursor_ptr)
        {
            err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_FIRST_PARAM_OF_METHOD) +
                       method_name + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_MUST_BE_CURSOR);
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_PARAMS_TYPE_INCONSISTENCY, err_mess);
        }

        if (!map_cursor_ptr->IsCursorValid())
        {
            err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_IN_METHOD) +
                method_name + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_CURSOR_INVALID);
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, err_mess);
        }
    }

    string GetStringKey(ObjectHolder object_holder, Context& context)
    {
        string string_key;
        Object* obj_ptr = object_holder.Get();
        if (obj_ptr)
        {
            ostringstream ostr;
            obj_ptr->Print(ostr, context);
            string_key = ostr.str();
        }
        else
        {
            string_key = "__NONE__"s;
        }

        return string_key;
    }

    ObjectHolder MapInstance::MethodInsert(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                           Context& context)
    {
        CheckMethodParams(context, "Insert"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 2, actual_args);
        if (is_in_iterator_mode_)
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_CURSOR_IN_PROGRESS_INSERT);

        auto [map_iterator, inserted] = map_storage_.insert({ GetStringKey(actual_args[0], context), actual_args[1] });
        return ObjectHolder::Own(PointerObject(&(map_iterator->second)));
    }

    ObjectHolder MapInstance::MethodFind(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                         Context& context)
    {
        CheckMethodParams(context, "Find"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 1, actual_args);

        auto map_iterator = map_storage_.find(GetStringKey(actual_args[0], context));
        if (map_iterator != map_storage_.end())
            return ObjectHolder::Own(PointerObject(&(map_iterator->second)));
        else
            return ObjectHolder::Own(PointerObject());
    }

    ObjectHolder MapInstance::MethodErase(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                          Context& context)
    {
        CheckMethodParams(context, "Erase"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 1, actual_args);
        if (is_in_iterator_mode_)
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_CURSOR_IN_PROGRESS_ERASE);

        size_t items_deleted = map_storage_.erase(GetStringKey(actual_args[0], context));
        return ObjectHolder::Own(Number(static_cast<int>(items_deleted)));
    }

    ObjectHolder MapInstance::MethodContains(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                             Context& context)
    {
        CheckMethodParams(context, "Contains"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 1, actual_args);

        return ObjectHolder::Own(Bool(map_storage_.count(GetStringKey(actual_args[0], context))));
    }

    ObjectHolder MapInstance::MethodClear(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                          Context& context)
    {
        CheckMethodParams(context, "Clear"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);
            
        map_storage_.clear();
        return ObjectHolder::None();            
    }

    ObjectHolder MapInstance::MethodBegin(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                          Context& context)
    {
        CheckMethodParams(context, "Begin"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);

        return ObjectHolder::Own(MapCursor(*this, map_storage_));
    }

    ObjectHolder MapInstance::MethodPrevious(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                             Context& context)
    {
        CheckMapIteratorParam(context, "Previous"s, actual_args);

        return ObjectHolder::Own(Bool(actual_args[0].TryAs<MapCursor>()->CursorPrevious()));
    }

    ObjectHolder MapInstance::MethodNext(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                         Context& context)
    {
        CheckMapIteratorParam(context, "Next"s, actual_args);

        return ObjectHolder::Own(Bool(actual_args[0].TryAs<MapCursor>()->CursorNext()));
    }

    ObjectHolder MapInstance::MethodKey(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                        Context& context)
    {
        CheckMapIteratorParam(context, "Key"s, actual_args);

        return actual_args[0].TryAs<MapCursor>()->CursorGetKey();
    }

    ObjectHolder MapInstance::MethodValue(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                          Context& context)
    {
        CheckMapIteratorParam(context, "Value"s, actual_args);

        return actual_args[0].TryAs<MapCursor>()->CursorGetValue();
    }

    ObjectHolder MapInstance::MethodIsCursorBegin(const std::string& method,
                            const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMapIteratorParam(context, "IsIteratorBegin"s, actual_args);

        return ObjectHolder::Own(Bool(actual_args[0].TryAs<MapCursor>()->IsCursorBegin()));
    }

    ObjectHolder MapInstance::MethodIsCursorEnd(const std::string& method,
                            const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMapIteratorParam(context, "IsIteratorEnd"s, actual_args);

        return ObjectHolder::Own(Bool(actual_args[0].TryAs<MapCursor>()->IsCursorEnd()));
    }

    ObjectHolder MapInstance::MethodRelease(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                            Context& context)
    {
        CheckMethodParams(context, "Release"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);

        is_in_iterator_mode_ = false;
        return ObjectHolder::None();
    }

    ObjectHolder MapInstance::Call(const std::string& method_name,
                                   const std::vector<ObjectHolder>& actual_args, Context& context, const std::string& parent_name)
    {
        if (map_method_table_.count(method_name))
            return (this->*map_method_table_.at(method_name))(method_name, actual_args, context);
        else
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
    }

    bool MapInstance::HasMethod(const string& method_name, size_t argument_count, const std::string& parent_name) const
    {
        if (!parent_name.empty())
            return false;   // Метод не имеет предков, поэтому не существует методов с непустыми спецификаторами предшественников в иерархии наследования.
        if (map_method_argument_count_.count(method_name))
        {
            auto argument_org_count = map_method_argument_count_.at(method_name);
            return argument_count >= argument_org_count.first &&
                   argument_count <= argument_org_count.second;
        }
        else
        {
            return false;
        }
    }

    CoroutineInstance::CoroutineInstance(ClassInstance* class_instance, const runtime::Method* method, Closure& closure) :
        class_instance_(class_instance), method_(method), coro_closure_(closure), is_started_(false), is_awaiting_(true)
    {
        if (!method_->is_coroutine)
        {
            assert(false);
            throw runtime_error("Метод " + method_->name + " не сопрограмма");
        }
        // Подготовим к работе символьную таблицу coro_closure_ сопрограммы, добавив в нее ссылку (слабую, невладеющую) на
        // объект-дескриптор сопрограммы (то есть, на этот объект).
        coro_closure_[COROUTINE_STATUS_VAR] = ObjectHolder::Share(*this);
    }
    
    CoroutineInstance::CoroutineInstance(FreeFunction* free_function, Closure& closure) :
        free_function_(free_function), coro_closure_(closure), is_started_(false), is_awaiting_(true)
    {
        if (!free_function_->IsCoroutine())
        {
            assert(false);
            throw runtime_error("Функция " + free_function_->GetName() + " не сопрограмма");
        }
        // Подготовим к работе символьную таблицу coro_closure_ сопрограммы, добавив в нее ссылку (слабую, невладеющую) на
        // объект-дескриптор сопрограммы (то есть, на этот объект).
        coro_closure_[COROUTINE_STATUS_VAR] = ObjectHolder::Share(*this);
    }

    void CoroutineInstance::Print(std::ostream& os, Context& context)
    {
        if (class_instance_ &&  method_)
        {
            os << "Coroutine - Class:" << class_instance_->GetClassName() << " - Method:" << method_->name
               << " - Coroutine:" << std::boolalpha << method_->is_coroutine;
        }
        else if (free_function_)
        {
            os << "Coroutine - Function:" << free_function_->GetName()
               << " - Coroutine:" << std::boolalpha << free_function_->IsCoroutine();
        }
        else
        {
            os << "Сопрограмма невалидна";
        }
    }

    ObjectHolder CoroutineInstance::Call(const std::string& method_name,
                                         const std::vector<ObjectHolder>& actual_args, Context& context, const std::string& parent_name)
    {
        if (coroutine_method_table_.count(method_name))
            return (this->*coroutine_method_table_.at(method_name))(method_name, actual_args, context);
        else
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
    }

    bool CoroutineInstance::HasMethod(const string& method_name, size_t argument_count, const std::string& parent_name) const
    {
        if (!parent_name.empty())
            return false;   // Метод не имеет предков, поэтому не существует методов с непустыми спецификаторами предшественников в иерархии наследования.
        if (coroutine_method_argument_count_.count(method_name))
        {
            auto argument_org_count = coroutine_method_argument_count_.at(method_name);
            return argument_count >= argument_org_count.first &&
                argument_count <= argument_org_count.second;
        }
        else
        {
            return false;
        }
    }

    ObjectHolder CoroutineInstance::MethodResume(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        if (!is_awaiting_)
            return ret_value_;

        is_started_ = true;
        is_awaiting_ = false;
        // Обновим указатель на сам объект сопрограммы (то есть на this), так как между вызовами наш объект мог переместиться в памяти.
        coro_closure_[COROUTINE_STATUS_VAR] = ObjectHolder::Share(*this);
        // Восстановим хранилище стека потока выполнения к положению, запомненному при последней приостановке сопрограммы.
        workflow_ = move(last_workflow_);
        workflow_.SetIndex(-1);  // -1 - положение "перед началом" стека кадров положения потока управления.
        try
        {
            if (class_instance_)
                ret_value_ = method_->body->Execute(coro_closure_, context);
            else if (free_function_)
                ret_value_ = free_function_->ExecuteBody(coro_closure_, context);
        }
        catch (...)
        { // Любое исключение, распространившееся за пределы сопрограммы, окончательно её завершает.
            is_awaiting_ = false;
            throw;  // Передаём исключение далее по цепочке для возможных вышележащих обработчиков.
        }
        // А теперь вновь запомним состояние потока управления сопрограммы, но уже по позиции её новой приостановки.
        last_workflow_ = move(workflow_);

        return ret_value_;
    }
    
    ObjectHolder CoroutineInstance::MethodIsStarted
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return ObjectHolder::Own(Bool(is_started_));
    }
    
    ObjectHolder CoroutineInstance::MethodIsAwaiting
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return ObjectHolder::Own(Bool(is_awaiting_));
    }
    
    ObjectHolder CoroutineInstance::MethodValue
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return ret_value_;
    }

    ObjectHolder CoroutineInstance::MethodGetAwaitable
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return coro_awaitable_;
    }
    
    ObjectHolder CoroutineInstance::MethodSetAwaitable
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    { // Метод назначения ждуна этой сопрограмме. Ждун обязательно должен быть наследником встроенного прототипа Awaitable.
        CheckMethodParams(context, "SetAwaitable"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
            MethodParamType::PARAM_TYPE_ANY, 1, actual_args);

        ClassInstance* try_awaitable = actual_args[0].TryAs<ClassInstance>();
        if (try_awaitable && try_awaitable->IsSuccessorOf(AWAITABLE_CLASS_NAME))
        {
            ObjectHolder old_coro_awaitable = coro_awaitable_;
            coro_awaitable_ = actual_args[0];
            return old_coro_awaitable;
        }
        else
        {
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_OBJECT_NOT_AWAITABLE);
        }
    }

    ObjectHolder CoroutineInstance::MethodSuspendType(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return ObjectHolder::Own(Number(static_cast<int>(suspend_type_)));
    }

    // Предикат, возвращающий "ИСТИНУ", если сопрограмма построена на основе свободной функции.
    ObjectHolder CoroutineInstance::MethodIsFreeFunction(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return ObjectHolder::Own(Bool(free_function_));
    }

    // Определения статических полей класса TypeTraitsInstance.
    // Таблица внешних методов этого специального класса, доступного из МУФЛОН-программы.
    const std::unordered_map<std::string_view, TypeTraitsInstance::TypeTraitsCallMethod> TypeTraitsInstance::type_traits_method_table_
    {
        {"is_bool"sv, &TypeTraitsInstance::MethodIsBool},
        {"IsBool"sv, &TypeTraitsInstance::MethodIsBool},
        {"is_numeric"sv, &TypeTraitsInstance::MethodIsNumeric},
        {"IsNumeric"sv, &TypeTraitsInstance::MethodIsNumeric},
        {"is_string"sv, &TypeTraitsInstance::MethodIsString},
        {"IsString"sv, &TypeTraitsInstance::MethodIsString},
        {"is_none"sv, &TypeTraitsInstance::MethodIsNone},
        {"IsNone"sv, &TypeTraitsInstance::MethodIsNone},
        {"is_same_type"sv, &TypeTraitsInstance::MethodIsSameType},
        {"IsSameType"sv, &TypeTraitsInstance::MethodIsSameType},
        {"is_same_target"sv, &TypeTraitsInstance::MethodIsSameTarget},
        {"IsSameTarget"sv, &TypeTraitsInstance::MethodIsSameTarget},
        {"is_class"sv, &TypeTraitsInstance::MethodIsClass},
        {"IsClass"sv, &TypeTraitsInstance::MethodIsClass},
        {"is_successor_of"sv, &TypeTraitsInstance::MethodIsSuсcessorOf},
        {"IsSuccessorOf"sv, &TypeTraitsInstance::MethodIsSuсcessorOf},
        {"is_predecessor_of"sv, &TypeTraitsInstance::MethodIsPredecessorOf},
        {"IsPredecessorOf"sv, &TypeTraitsInstance::MethodIsPredecessorOf},
        {"is_successor_of_name"sv, &TypeTraitsInstance::MethodIsSuсcessorOfName},
        {"IsSuccessorOfName"sv, &TypeTraitsInstance::MethodIsSuсcessorOfName},
        {"is_predecessor_of_name"sv, &TypeTraitsInstance::MethodIsPredecessorOfName},
        {"IsPredecessorOfName"sv, &TypeTraitsInstance::MethodIsPredecessorOfName},
        {"id"sv, &TypeTraitsInstance::MethodId},
        {"Id"sv, &TypeTraitsInstance::MethodId},
        {"name"sv, &TypeTraitsInstance::MethodName},
        {"Name"sv, &TypeTraitsInstance::MethodName},
        {"has_method"sv, &TypeTraitsInstance::MethodHasMethod},
        {"HasMethod"sv, &TypeTraitsInstance::MethodHasMethod},
        {"has_field"sv, &TypeTraitsInstance::MethodHasField},
        {"HasField"sv, &TypeTraitsInstance::MethodHasField},
        {"get_field_value"sv, &TypeTraitsInstance::MethodGetFieldValue},
        {"GetFieldValue"sv, &TypeTraitsInstance::MethodGetFieldValue},
        {"set_field_value"sv, &TypeTraitsInstance::MethodSetFieldValue},
        {"SetFieldValue"sv, &TypeTraitsInstance::MethodSetFieldValue},
        {"call_method"sv, &TypeTraitsInstance::MethodCallMethod},
        {"CallMethod"sv, &TypeTraitsInstance::MethodCallMethod}
    };

    // Описание аргументов внешних методов этого класса.
    const std::unordered_map<std::string_view, std::pair<size_t, size_t>> TypeTraitsInstance::type_traits_method_argument_count_
    {
        {"is_bool"sv, {0, 0}},
        {"IsBool"sv, {0, 0}},
        {"is_numeric"sv, {0, 0}},
        {"IsNumeric"sv, {0, 0}},
        {"is_string"sv, {0, 0}},
        {"IsString"sv, {0, 0}},
        {"is_none"sv, {0, 0}},
        {"IsNone"sv, {0, 0}},
        {"is_same_type"sv, {1, 1}},
        {"IsSameType"sv, {1, 1}},
        {"is_same_target"sv, {1, 1}},
        {"IsSameTarget"sv, {1, 1}},
        {"is_class"sv, {1, 1}},
        {"IsClass"sv, {1, 1}},
        {"is_successor_of"sv, {1, 1}},
        {"IsSuccessorOf"sv, {1, 1}},
        {"is_predecessor_of"sv, {1, 1}},
        {"IsPredecessorOf"sv, {1, 1}},
        {"is_successor_of_name"sv, {1, 1}},
        {"IsSuccessorOfName"sv, {1, 1}},
        {"is_predecessor_of_name"sv, {1, 1}},
        {"IsPredecessorOfName"sv, {1, 1}},
        {"id"sv, {0, 0}},
        {"Id"sv, {0, 0}},
        {"name"sv, {0, 0}},
        {"Name"sv, {0, 0}},
        {"has_method"sv, {2, 2}},
        {"HasMethod"sv, {2, 2}},
        {"has_field"sv, {1, 1}},
        {"HasField"sv, {1, 1}},
        {"get_field_value"sv, {1, 1}},
        {"GetFieldValue"sv, {1, 1}},
        {"set_field_value"sv, {2, 2}},
        {"SetFieldValue"sv, {2, 2}},
        {"call_method"sv, {1, (std::numeric_limits<size_t>::max)()}},
        {"CallMethod"sv, {1, (std::numeric_limits<size_t>::max)()}}
    };

    // Словари, заполняемые при разборе и синтаксическом анализе МУФЛОН-программы.
    std::unordered_map<std::string, int> TypeTraitsInstance::internal_classes_ids_;
    std::unordered_map<std::string, ast::ClassDefinition*> TypeTraitsInstance::declared_classes_def_;

    // Определение методов класса TypeTraitsInstance.

    TypeTraitsInstance::TypeTraitsInstance(ObjectHolder traits_value) : traits_value_(move(traits_value))
    {}

    void TypeTraitsInstance::Print(std::ostream& os, Context& context)
    {
        os << "Типовая характеристика TypeTraits : ID - " << ObjectIdInternal(traits_value_)
           << " - Name - " << ObjectNameInternal(traits_value_);
    }

    ObjectHolder TypeTraitsInstance::Call(const std::string& method_name, const std::vector<ObjectHolder>& actual_args,
                                          Context& context, const std::string& parent_name)
    {
        if (type_traits_method_table_.count(method_name))
            return (this->*type_traits_method_table_.at(method_name))(method_name, actual_args, context);
        else
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
    }
    
    bool TypeTraitsInstance::HasMethod(const std::string& method_name, size_t argument_count, const std::string& parent_name) const
    {
        if (!parent_name.empty())
            return false;   // Метод не имеет предков, поэтому не существует методов с непустыми спецификаторами предшественников в иерархии наследования.
        if (type_traits_method_argument_count_.count(method_name))
        {
            auto argument_org_count = type_traits_method_argument_count_.at(method_name);
            return argument_count >= argument_org_count.first && argument_count <= argument_org_count.second;
        }
        else
        {
            return false;
        }
    }

    void TypeTraitsInstance::AppendDeclaredClassDef(const std::string& class_name, ast::ClassDefinition* class_def)
    {
        declared_classes_def_.emplace(std::pair{class_name, class_def});
    }

    int TypeTraitsInstance::ObjectIdInternal(const ObjectHolder& what_id)
    {
        if (!what_id)
            return NONE_IDENT;
        else if (what_id.TryAs<runtime::Bool>())
            return BOOL_IDENT;
        else if (what_id.TryAs<runtime::Number>())
            return NUMERIC_IDENT;
        else if (what_id.TryAs<runtime::String>())
            return STRING_IDENT;
        else if (runtime::ClassInstance* class_instance = what_id.TryAs<runtime::ClassInstance>())
            return class_instance->GetBaseClass().GetId();
        else if (runtime::CommonClassInstance* common_class_instance = what_id.TryAs<runtime::CommonClassInstance>())
        {
            auto classes_ids_it = internal_classes_ids_.find(common_class_instance->GetClassName());
            if (classes_ids_it != internal_classes_ids_.end())
                return classes_ids_it->second;
            else
                return INVALID_TYPE_IDENT;
        }
        else
            return INVALID_TYPE_IDENT;
    }

    std::string TypeTraitsInstance::ObjectNameInternal(const ObjectHolder& what_id)
    {
        if (!what_id)
            return "None"s;
        else if (what_id.TryAs<runtime::Bool>())
            return "Bool"s;
        else if (what_id.TryAs<runtime::Number>())
            return "Number"s;
        else if (what_id.TryAs<runtime::String>())
            return "String"s;
        else if (runtime::CommonClassInstance* common_class_instance = what_id.TryAs<runtime::CommonClassInstance>())
            return common_class_instance->GetClassName();
        else
            return {};
    }

    ObjectHolder TypeTraitsInstance::MethodIsBool(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "IsBool"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);
        return ObjectHolder::Own(Bool(traits_value_.TryAs<runtime::Bool>()));
    }
    
    ObjectHolder TypeTraitsInstance::MethodIsNumeric(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "IsNumeric"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);
        return ObjectHolder::Own(Bool(traits_value_.TryAs<runtime::Number>()));
    }
    
    ObjectHolder TypeTraitsInstance::MethodIsString(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "IsString"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);
        return ObjectHolder::Own(Bool(traits_value_.TryAs<runtime::String>()));
    }
    
    ObjectHolder TypeTraitsInstance::MethodIsNone(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "IsNone"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);
        return ObjectHolder::Own(Bool(!traits_value_));
    }
    
    ObjectHolder TypeTraitsInstance::MethodIsSameType(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "IsSameType"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 1, actual_args);

        int traits_value_id = ObjectIdInternal(traits_value_);
        int actual_arg_id = ObjectIdInternal(actual_args[0]);
        if (traits_value_id < 0 || actual_arg_id < 0)
            return ObjectHolder::Own(Bool(false));

        return ObjectHolder::Own(Bool(traits_value_id == actual_arg_id));
    }
    
    ObjectHolder TypeTraitsInstance::MethodIsSameTarget(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "IsSameTarget"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 1, actual_args);

        if (!traits_value_)
        { // Характеризуемый объект пуст.
            return ObjectHolder::Own(Bool(!actual_args[0]));
        }
        else
        { // Характеризуемый объект содержит какое-то значение.
            if (!actual_args[0])
                return ObjectHolder::Own(Bool(false));
            return ObjectHolder::Own(Bool(traits_value_.Get() == actual_args[0].Get()));
        }
    }

    // Проверка на совпадение истинного имени характеризуемого класса и строкового аргумента метода.
    ObjectHolder TypeTraitsInstance::MethodIsClass(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "IsClass"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 1, actual_args);

        std::string traits_name = ObjectNameInternal(traits_value_);
        String* test_class_name = actual_args[0].TryAs<String>();

        if (!test_class_name || traits_name.empty() || test_class_name->GetValue().empty())
            return ObjectHolder::Own(Bool(false));
        return ObjectHolder::Own(Bool(traits_name == test_class_name->GetValue()));
    }
    
    ObjectHolder TypeTraitsInstance::MethodIsSuсcessorOf(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "IsSuсcessorOf"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 1, actual_args);

        runtime::CommonClassInstance* traits_common_class_instance = traits_value_.TryAs<runtime::CommonClassInstance>();
        runtime::CommonClassInstance* test_common_class_instance = actual_args[0].TryAs<runtime::CommonClassInstance>();
        if (!traits_common_class_instance || !test_common_class_instance)
            return ObjectHolder::Own(Bool(false));  // Одно или оба из сравниваемых значений не класс, оно не участвует в наследственных отношениях.

        runtime::ClassInstance* test_class_instance = actual_args[0].TryAs<runtime::ClassInstance>();
        if (!test_class_instance)
            // Класс, для которого нужно выяснить, не являемся ли мы его потомком, является CommonClassInstance, но не ClassInstance.
            // В этом случае проверим их связь по происхождению, используя имя проверяемого класса test_common_class_instance.
            return ObjectHolder::Own(Bool(traits_common_class_instance->IsSuccessorOf(test_common_class_instance->GetClassName())));
        else
            // Оба сверяемых класса есть программно определяемые классы общего типа (ClassInstance). В этом случае проверим их родственную связь
            // прямо по классовым объектам (runtime::Class).
            return ObjectHolder::Own(Bool(traits_common_class_instance->IsSuccessorOf(test_class_instance->GetBaseClass())));
    }
    
    ObjectHolder TypeTraitsInstance::MethodIsPredecessorOf(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "IsPredecessorOf"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 1, actual_args);

        runtime::CommonClassInstance* traits_common_class_instance = traits_value_.TryAs<runtime::CommonClassInstance>();
        runtime::CommonClassInstance* test_common_class_instance = actual_args[0].TryAs<runtime::CommonClassInstance>();
        if (!traits_common_class_instance || !test_common_class_instance)
            // Одно или оба из сравниваемых значений не класс, оно(они) не участвует(ют) в наследственных отношениях.
            return ObjectHolder::Own(Bool(false));

        runtime::ClassInstance* traits_class_instance = traits_value_.TryAs<runtime::ClassInstance>();
        if (!traits_class_instance)
            // "Наш" класс есть CommonClassInstance, но не ClassInstance, то есть он есть какой-то встроенный фиксированный класс среды.
            // В этом случае проверим их родственное отношение по имени.
            return ObjectHolder::Own(Bool(test_common_class_instance->IsSuccessorOf(traits_common_class_instance->GetClassName())));
        else        
            // Оба класса (и характеризуемый, и проверяемый) являются ClassInstance. Проверим их родство непосредсвенно по классовым описателям.
            return ObjectHolder::Own(Bool(test_common_class_instance->IsSuccessorOf(traits_class_instance->GetBaseClass())));
    }
    
    // Проверка предположения, что мы являемся потомком (наследником) класса, имя которого является первым фактическим аргументом этого метода.
    ObjectHolder TypeTraitsInstance::MethodIsSuсcessorOfName(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        // Фактический параметр должен быть строковым и единственным - имя класса, предположительно, нашего предка.
        CheckMethodParams(context, "IsSuсcessorOfName"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 1, actual_args);

        String* test_our_predecessor_class_name = actual_args[0].TryAs<String>();
        runtime::CommonClassInstance* traits_common_class_instance = traits_value_.TryAs<runtime::CommonClassInstance>();
        if (!traits_common_class_instance)
            return ObjectHolder::Own(Bool(false));  // Наше значение не класс, оно не участвует в наследственных отношениях.

        return ObjectHolder::Own(Bool(traits_common_class_instance->IsSuccessorOf(test_our_predecessor_class_name->GetValue())));
    }
    
    // Проверка того, являемся ли мы предшественником класса (а он, соответственно, нашим потомком) с именем, заданным аргументом данного метода.
    ObjectHolder TypeTraitsInstance::MethodIsPredecessorOfName(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        // Фактический параметр должен быть строковым и единственным - имя класса, предположительно, нашего потомка.
        CheckMethodParams(context, "IsPredecessorOfName"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 1, actual_args);

        String* test_our_successor_class_ptr = actual_args[0].TryAs<String>();
        std::string test_our_successor_class_name = test_our_successor_class_ptr->GetValue();

        runtime::CommonClassInstance* traits_common_class_instance = traits_value_.TryAs<runtime::CommonClassInstance>();
        if (!traits_common_class_instance)
            return ObjectHolder::Own(Bool(false));  // Наше значение не класс, оно не участвует в наследственных отношениях.        
        
        auto test_internal_class_it = internal_classes_ids_.find(test_our_successor_class_name);
        if (test_internal_class_it != internal_classes_ids_.end())
            // Предположительный потомок является встроенным классом исполнительской среды. Отношения типа "предок-потомок" между такими классами
            // могут существовать только в виде их полной эквивалентности, что мы сейчас и проверим.
            return ObjectHolder::Own(Bool(traits_common_class_instance->GetClassName() == test_our_successor_class_name));

        // Дальнейшая проверка предусматривает только тот случай, если проверяемый класс (с именем test_our_successor_class_name) является классом
        // общего типа.
        auto test_declared_classes_it = declared_classes_def_.find(test_our_successor_class_name);
        if (test_declared_classes_it == declared_classes_def_.end())
            // Такого (с именем test_our_successor_class_name) общего класса не существует. Поэтому он не может быть чьим-то
            // потомком (включая нас).
            return ObjectHolder::Own(Bool(false));

        // Проверяемый класс - общий программно определяемый класс.
        runtime::ClassInstance* traits_class_instance = traits_value_.TryAs<runtime::ClassInstance>();
        runtime::Class* test_class = test_declared_classes_it->second->GetClass();
        if (traits_class_instance)
            // Характеризуемый класс - также класс общего типа. Проверим их родство прямо по классовым описателям.
            return ObjectHolder::Own(Bool(test_class->IsSuccessorOf(traits_class_instance->GetBaseClass())));
        else
            // Характеризуемый класс - не программно определяемый, но класс типа runtime::CommonClassInstance. Используем проверку родства по именам.
            return ObjectHolder::Own(Bool(test_class->IsSuccessorOf(traits_common_class_instance->GetClassName())));
    }
    
    ObjectHolder TypeTraitsInstance::MethodId(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "Id"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);
        return ObjectHolder::Own(Number(ObjectIdInternal(traits_value_)));
    }
    
    ObjectHolder TypeTraitsInstance::MethodName(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "Name"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);
        return ObjectHolder::Own(String(ObjectNameInternal(traits_value_)));
    }
    
    ObjectHolder TypeTraitsInstance::MethodHasMethod(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "HasMethod"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_NUMERIC_STRING, 2, actual_args);
        String* find_method_name = actual_args[0].TryAs<String>();
        Number* find_method_param_count = actual_args[1].TryAs<Number>();
        if (!find_method_name || !find_method_param_count)
            // Недопустимый тип фактических параметров данного метода.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_PARAMS_TYPE_INCONSISTENCY,
                              "Недопустимые параметры метода HasMethod() - должны быть (строка, число)");

        runtime::CommonClassInstance* common_class_instance = traits_value_.TryAs<runtime::CommonClassInstance>();
        if (!common_class_instance)
            return ObjectHolder::Own(Bool(false));  // Это не класс. Стандартные типы не имеют никаких методов.

        return ObjectHolder::Own(Bool(common_class_instance->HasMethod(find_method_name->GetValue(), find_method_param_count->GetIntValue())));
    }
    
    ObjectHolder TypeTraitsInstance::MethodHasField(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "HasField"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 1, actual_args);

        String* find_field_name = actual_args[0].TryAs<String>();
        runtime::ClassInstance* class_instance = traits_value_.TryAs<runtime::ClassInstance>();
        if (!class_instance)
            return ObjectHolder::Own(Bool(false));  // Это не класс общего типа. У прочих типов выражений полей нет вовсе.

        return ObjectHolder::Own(Bool(class_instance->Fields().contains(find_field_name->GetValue())));
    }

    // Функции-члены извлечения и установки значения некоторого поля объекта. Имя поля передаётся первым строковым аргументом.
    // Второй аргумент есть у функции-установщика и является значением, которое будет назначено указанному полю.
    ObjectHolder TypeTraitsInstance::MethodGetFieldValue(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "GetFieldValue"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 1, actual_args);

        String* find_field_name = actual_args[0].TryAs<String>();
        runtime::ClassInstance* class_instance = traits_value_.TryAs<runtime::ClassInstance>();
        if (!class_instance)
            // Это не класс общего типа. У прочих типов выражений полей нет вовсе.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_HAVE_INCOMPATIBLE_TYPE);

        Closure& class_closure = class_instance->Fields();
        if (auto closure_field_it = class_closure.find(find_field_name->GetValue()); closure_field_it != class_closure.end())
            return closure_field_it->second;
        else
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_FIELD_NOT_FOUND);
    }
    
    ObjectHolder TypeTraitsInstance::MethodSetFieldValue(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "SetFieldValue"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 2, actual_args);

        String* find_field_name = actual_args[0].TryAs<String>();
        if (!find_field_name) // Имя поля должно быть строкой.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE);
        runtime::ClassInstance* class_instance = traits_value_.TryAs<runtime::ClassInstance>();
        if (!class_instance)
            // Это не класс общего типа. У прочих типов выражений полей нет вовсе.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_HAVE_INCOMPATIBLE_TYPE);

        Closure& class_closure = class_instance->Fields();
        if (auto closure_field_it = class_closure.find(find_field_name->GetValue()); closure_field_it != class_closure.end())
        {
            ObjectHolder old_value = move(closure_field_it->second);
            closure_field_it->second = actual_args[1];
            return old_value;
        }
        else
        {
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_FIELD_NOT_FOUND);
        }
    }

    // Метод вызова метода того объекта, для которого создана данная характеристика, по его строковому имени. Первый аргумент - строковое имя
    // вызываемого метода, остальные аргументы передаются этому методу "как есть". При вызове выполняется проверка наличия требуемого метода
    // целевого класса по его имени и количеству параметров.
    ObjectHolder TypeTraitsInstance::MethodCallMethod(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "CallMethod"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_GREATER_EQ,
                          MethodParamType::PARAM_TYPE_ANY, 1, actual_args);

        String* find_field_name = actual_args[0].TryAs<String>();
        if (!find_field_name) // Имя метода должно быть строкой.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE);
        runtime::CommonClassInstance* common_class_instance = traits_value_.TryAs<runtime::CommonClassInstance>();
        if (!common_class_instance)
            // Это не какой-либо класс. У прочих простых типов выражений методов нет вовсе.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_HAVE_INCOMPATIBLE_TYPE);
        return common_class_instance->Call(find_field_name->GetValue(), {actual_args.begin() + 1, actual_args.end()}, context);
    }
} //namespace runtime
