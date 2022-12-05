
#include "statement.h"
#include "parse.h"
#include "throw_messages.h"

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
        PrepareExecute(this, context);
        std::vector<int> elements_count;
        for (auto& cur_param_ptr : args_)
        {
            ObjectHolder cur_count_object = cur_param_ptr->Execute(closure, context);
            runtime::Number* cur_element_count_ptr = cur_count_object.TryAs<runtime::Number>();
            if (cur_element_count_ptr)
                elements_count.push_back(cur_element_count_ptr->GetIntValue());
            else
                ThrowRuntimeError(this, ThrowMessageNumber::THRM_NOT_DIGIT_SIZES);
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
        PrepareExecute(this, context);
        return ObjectHolder::Own(runtime::MapInstance());
    }

    unique_ptr<Statement> CreateArray(vector<unique_ptr<Statement>> args)
    {
        return make_unique<NewArray>(NewArray(move(args)));
    }

    unique_ptr<Statement> CreateMap(vector<unique_ptr<Statement>> args)
    {
        return make_unique<NewMap>(NewMap(move(args)));
    }
} // namespace ast

namespace runtime
{
    const unordered_map<string_view, ArrayInstance::ArrayCallMethod> ArrayInstance::array_method_table_
    { {"get"sv, &ArrayInstance::MethodGet},
        {"Get"sv, &ArrayInstance::MethodGet},
        {"get_array_dimensions"sv, &ArrayInstance::MethodGetArrayDimensions},
        {"GetArrayDimensions"sv, &ArrayInstance::MethodGetArrayDimensions},
        {"get_dimension_count"sv, &ArrayInstance::MethodGetDimensionCount},
        {"GetDimensionCount"sv, &ArrayInstance::MethodGetDimensionCount},
        {"resize"sv, &ArrayInstance::MethodResize},
        {"Resize"sv, &ArrayInstance::MethodResize},
        {"push_back"sv, &ArrayInstance::MethodPushBack},
        {"PushBack"sv, &ArrayInstance::MethodPushBack},
        {"back"sv, &ArrayInstance::MethodBack},
        {"Back"sv, &ArrayInstance::MethodBack},
        {"pop_back"sv, &ArrayInstance::MethodPopBack},
        {"PopBack"sv, &ArrayInstance::MethodPopBack}
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
        {"is_iterator_begin"sv, &MapInstance::MethodIsIteratorBegin},
        {"IsIteratorBegin"sv, &MapInstance::MethodIsIteratorBegin},
        {"is_iterator_end"sv, &MapInstance::MethodIsIteratorEnd},
        {"IsIteratorEnd"sv, &MapInstance::MethodIsIteratorEnd},
        {"release"sv, &MapInstance::MethodRelease},
        {"Release"sv, &MapInstance::MethodRelease}
    };

    int MapInstance::last_iterator_pack_serial_ = 0;

    void CheckMethodParams(Context& context, const string& method_name,
                           MethodParamCheckMode check_mode,
                           MethodParamType param_type, size_t required_params,
                           const vector<ObjectHolder>& actual_args)
    {
        static constexpr int PARAM_CHECK_QUANTITY_MASK = 3;
        string err_mess;

        switch (check_mode & PARAM_CHECK_QUANTITY_MASK)
        {
        case MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL:
            if (actual_args.size() != required_params)
            {
                err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_METHOD) + method_name +
                           ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_DEMAND_EQUAL) +
                           to_string(required_params) + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_ARGUMENTS);
                ThrowRuntimeError(context, err_mess);
            }
        case MethodParamCheckMode::PARAM_CHECK_QUANTITY_LESS_EQ:
            if (actual_args.size() > required_params)
            {
                err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_METHOD) + method_name +
                    ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_DEMAND_LESS_OR_EQUAL) +
                    to_string(required_params) + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_ARGUMENTS);
                ThrowRuntimeError(context, err_mess);
            }
        case MethodParamCheckMode::PARAM_CHECK_QUANTITY_GREATER_EQ:
            if (actual_args.size() < required_params)
            {
                err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_METHOD) + method_name +
                    ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_DEMAND_GREATER_OR_EQUAL) +
                    to_string(required_params) + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_ARGUMENTS);
                ThrowRuntimeError(context, err_mess);
            }
        default:
            break;
        }

        if (check_mode & MethodParamCheckMode::PARAM_CHECK_TYPE)
        {
            size_t i = 1;
            bool is_throw_exception = false;
            for (auto& current_param : actual_args)
            {
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
                    ThrowRuntimeError(context, err_mess);
                }
                ++i;
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

    ObjectHolder ArrayInstance::Call(const std::string& method_name,
                        const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        if (array_method_table_.count(method_name))
            return (this->*array_method_table_.at(method_name))(method_name, actual_args, context);
        else
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
    }

    MapIterator::MapIterator(MapInstance & map_instance, map<string, ObjectHolder> & map_storage) :
        map_instance_ref_(map_instance), map_storage_ref_(map_storage), map_iterator_(map_storage.begin()),
        iterator_pack_serial_(map_instance.AllocIteratorPackSerial())
    {}

    bool MapIterator::Begin()
    {
        map_iterator_ = map_storage_ref_.begin();
        return map_iterator_ != map_storage_ref_.end();
    }

    bool MapIterator::IteratorLowerBound(const string& map_key)
    {
        map_iterator_ = map_storage_ref_.lower_bound(map_key);
        return map_iterator_ != map_storage_ref_.end();
    }

    ObjectHolder MapIterator::IteratorGetKey()
    {
        if (map_iterator_ != map_storage_ref_.end())
            return ObjectHolder::Own(String(map_iterator_->first));
        else
            return ObjectHolder::None();
    }

    ObjectHolder MapIterator::IteratorGetValue()
    {
        if (map_iterator_ != map_storage_ref_.end())
            return ObjectHolder::Own(PointerObject(&(map_iterator_->second)));
        else
            return ObjectHolder::Own(PointerObject());
    }

    bool MapIterator::IteratorNext()
    {
        if (map_iterator_ != map_storage_ref_.end())
            ++map_iterator_;
        return map_iterator_ != map_storage_ref_.end();
    }

    bool MapIterator::IteratorPrevious()
    {
        if (map_iterator_ != map_storage_ref_.begin())
            --map_iterator_;
        return map_iterator_ != map_storage_ref_.begin();
    }

    bool MapIterator::IsIteratorEnd()
    {
        return map_iterator_ == map_storage_ref_.end();
    }

    bool MapIterator::IsIteratorBegin()
    {
        return map_iterator_ == map_storage_ref_.begin();
    }

    void MapIterator::Print(std::ostream& os, Context& context)
    {
        os << "MapIter:" << iterator_pack_serial_ << ' ' << boolalpha << IsIteratorValid();
    }

    bool MapIterator::IsIteratorValid()
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
            ThrowRuntimeError(context, err_mess);
        }

        MapIterator * map_iter_ptr = actual_args[0].TryAs<MapIterator>();
        if (!map_iter_ptr)
        {
            err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_FIRST_PARAM_OF_METHOD) +
                       method_name + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_MUST_BE_ITERATOR);
            ThrowRuntimeError(context, err_mess);
        }

        if (!map_iter_ptr->IsIteratorValid())
        {
            err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_IN_METHOD) +
                method_name + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_ITERATOR_INVALID);
            ThrowRuntimeError(context, err_mess);
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
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_ITERATOR_IN_PROGRESS_INSERT);

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
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_ITERATOR_IN_PROGRESS_ERASE);

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

    ObjectHolder MapInstance::MethodBegin(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                          Context& context)
    {
        CheckMethodParams(context, "Begin"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
            MethodParamType::PARAM_TYPE_ANY, 0, actual_args);

        return ObjectHolder::Own(MapIterator(*this, map_storage_));
    }

    ObjectHolder MapInstance::MethodPrevious(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                             Context& context)
    {
        CheckMapIteratorParam(context, "Previous"s, actual_args);

        return ObjectHolder::Own(Bool(actual_args[0].TryAs<MapIterator>()->IteratorPrevious()));
    }

    ObjectHolder MapInstance::MethodNext(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                         Context& context)
    {
        CheckMapIteratorParam(context, "Next"s, actual_args);

        return ObjectHolder::Own(Bool(actual_args[0].TryAs<MapIterator>()->IteratorNext()));
    }

    ObjectHolder MapInstance::MethodKey(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                        Context& context)
    {
        CheckMapIteratorParam(context, "Key"s, actual_args);

        return actual_args[0].TryAs<MapIterator>()->IteratorGetKey();
    }

    ObjectHolder MapInstance::MethodValue(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                          Context& context)
    {
        CheckMapIteratorParam(context, "Value"s, actual_args);

        return actual_args[0].TryAs<MapIterator>()->IteratorGetValue();
    }

    ObjectHolder MapInstance::MethodIsIteratorBegin(const std::string& method,
                            const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMapIteratorParam(context, "IsIteratorBegin"s, actual_args);

        return ObjectHolder::Own(Bool(actual_args[0].TryAs<MapIterator>()->IsIteratorBegin()));
    }

    ObjectHolder MapInstance::MethodIsIteratorEnd(const std::string& method,
                            const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMapIteratorParam(context, "IsIteratorEnd"s, actual_args);

        return ObjectHolder::Own(Bool(actual_args[0].TryAs<MapIterator>()->IsIteratorEnd()));
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
                            const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        if (map_method_table_.count(method_name))
            return (this->*map_method_table_.at(method_name))(method_name, actual_args, context);
        else
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
    }
} //namespace runtime
