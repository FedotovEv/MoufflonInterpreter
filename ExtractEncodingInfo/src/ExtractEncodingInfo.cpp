
// Программа извлечения информации о наборе некоторых однобайтовых кодировок, поддержмваемых библиотекой периода исполнения языка
// C и доступных через setlocale(). Набор получаемой информации (при успешной работе всех кодировочных механизмов) состоит из данных
// символьной классификации, регистрового соответствия и таблицы преобразования в Юникод.

#include "declares.h"
#include "getopt.h"
#include <iostream>
#include <iomanip>
#include <locale>
#include <fstream>
#include <set>

static const std::string C_LOCALE_NAME = "C_locale";

struct OutEncodingConfig
{
    int indent_step = 4;        // Величина шага (единицы) отступа.
    int to_utf8_per_str = 16;   // Количество элементов на строку при выводе перекодирующей таблицы.
    int classifier_per_str = 4; // Количество элементов на строку при выводе символьного классификатора.
    int collate_per_str = 16;   // Количество элементов на строку при выводе весовой строки.
};

OutEncodingConfig out_enc_config;

EncodingCharClasses operator|=(EncodingCharClasses& lhs, EncodingCharClasses rhs)
{
    lhs = static_cast<EncodingCharClasses>(lhs | rhs);
    return lhs;
}

size_t FindRegisterPairPos(char scan_c, bool scan_for_up, const std::vector<std::pair<char, char>>& upcase_table)
{
    auto upcase_table_it = std::find_if(upcase_table.begin(), upcase_table.end(),
        [scan_c, scan_for_up](const std::pair<char, char>& scan_pair) -> bool
        {
            if (scan_for_up)
                return scan_pair.first == scan_c;
            else
                return scan_pair.second == scan_c;
        });

    if (upcase_table_it == upcase_table.end())
        return std::string::npos;	// Запись для требуемого символа в таблице парности не найдена.
    else
        return upcase_table_it - upcase_table.begin();
}

void FillIndent(std::ostream& ostr, int indent_level)
{
    ostr << std::string(indent_level * out_enc_config.indent_step, ' ');
}

void OutEncodingInfo(std::ostream& ostr, const SingleByteEncodingDesc& encoding_info, const std::string& var_name)
{
    setlocale(LC_CTYPE, "ru_RU.UTF-8"); // Для вывода итоговой информации вновь переключим отображение текста в консоль в UTF-8 режим.

    ostr << "const SingleByteEncodingDesc " << var_name << "\n{\n";
    FillIndent(ostr, 1);
    ostr << ".name = \"" << encoding_info.name << "\"," << std::endl;
    // Вывод содержимого строкового классификатора.
    FillIndent(ostr, 1);
    ostr << ".char_classifier\n";
    FillIndent(ostr, 1);
    ostr << "{\n";

    for (int start_c = 0; start_c <= 0xff; start_c += out_enc_config.classifier_per_str)
    {
        FillIndent(ostr, 2);
        int max_c = min(start_c + out_enc_config.classifier_per_str - 1, 0xff);
        bool is_last_row = max_c >= 0xff;
        for (int current_c = start_c; current_c <= max_c; ++current_c)
        {
            ostr << "(EncodingCharClasses)0x" << std::hex << std::setfill('0') << std::setw(4) << encoding_info.char_classifier[current_c];
            if (current_c < max_c)
                ostr << ", ";
            else
                if (!is_last_row)
                    ostr << ",\n";
                else
                    ostr << std::endl;
        }
    }
    FillIndent(ostr, 1);
    ostr << "},\n";
        
    // Вывод таблицы перекодировки в Юникод.
    FillIndent(ostr, 1);
    ostr << ".to_utf8\n";
    FillIndent(ostr, 1);
    ostr << "{\n";
    for (int start_c = 0; start_c <= 0xff; start_c += out_enc_config.to_utf8_per_str)
    {
        FillIndent(ostr, 2);
        int max_c = min(start_c + out_enc_config.to_utf8_per_str - 1, 0xff);
        bool is_last_row = max_c >= 0xff;
        for (int current_c = start_c; current_c <= max_c; ++current_c)
        {
            ostr << "0x" << std::hex << std::setfill('0') << std::setw(sizeof(wchar_t) * 2) << encoding_info.to_utf8[current_c];
            if (current_c < max_c)
                ostr << ", ";
            else
                if (!is_last_row)
                    ostr << ",\n";
                else
                    ostr << std::endl;
        }
    }
    FillIndent(ostr, 1);
    ostr << "},\n";

    // Вывод таблицы парного соответствия регистров.
    FillIndent(ostr, 1);
    ostr << ".upcase_table\n";
    FillIndent(ostr, 1);
    ostr << "{\n";
    for (size_t i = 0; i  < encoding_info.upcase_table.size(); ++i)
    {
        std::pair<char, char> reg_symb_pair = encoding_info.upcase_table[i];
        FillIndent(ostr, 2);
        ostr << "{0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>((unsigned char)reg_symb_pair.first)
             << ", 0x" << std::setw(2) << static_cast<int>((unsigned char)reg_symb_pair.second) << "}";
        if ((i + 1) < encoding_info.upcase_table.size())
            ostr << ',';
        ostr << '\n';
    }

    // Вывод относительных сортировочных весов символов, если такие данные есть.
    if (encoding_info.collate.size() == COLLATE_SIZE)
    { // Данные о сортировочных весах существуют.
        FillIndent(ostr, 1);
        ostr << "},\n";
        FillIndent(ostr, 1);
        ostr << ".collate\n";
        FillIndent(ostr, 1);
        ostr << "{\n";
        for (int start_c = 0; start_c <= 0xff; start_c += out_enc_config.collate_per_str)
        {
            FillIndent(ostr, 2);
            ostr << "\"";   // Открывающая кавычка очередного блока весовой строки.
            int max_c = min(start_c + out_enc_config.collate_per_str - 1, 0xff);
            bool is_last_row = max_c >= 0xff;
            for (int current_c = start_c; current_c <= max_c; ++current_c)
            {
                ostr << "\\0x" << std::hex << std::setfill('0')
                     << std::setw(2) << static_cast<int>((unsigned char)encoding_info.collate[current_c]);
                if (current_c >= max_c)
                {
                    if (!is_last_row)
                        ostr << "\"" << std::endl;   // Закрывающая кавычка очередного блока весовой строки.
                    else
                        ostr << "\"s" << std::endl;  // Закрывающая кавычка последнего блока весовой строки.
                }
            }
        }
    }

    FillIndent(ostr, 1);
    ostr << "}\n";
    // Закрываем всю запись со всеми добытыми сведениями об очередной кодировке.
    ostr << "};\n";
}

int ExtractLocaleData(std::ostream& ostr, const std::string& locale_name, const std::string& var_name)
{
    std::string use_locale_name, msg_locale_name;
    if (!locale_name.empty())
    {
        msg_locale_name = use_locale_name = locale_name;
    }
    else
    {
        use_locale_name = C_LOCALE_NAME;
        msg_locale_name = "по умолчанию";
    }

    std::cerr << "Извлекаем информацию из локали " << msg_locale_name << std::endl;
    char* real_locale_name = setlocale(LC_CTYPE, locale_name.c_str());
    if (!real_locale_name)
    {
        std::cerr << "Ошибка при установке целевой локали " << msg_locale_name << std::endl;
        return EXIT_FAILURE;
    }

    std::cerr << "Внутреннее имя локали " << real_locale_name << std::endl;
    wchar_t unicode_char;
    SingleByteEncodingDesc new_encoding;
    new_encoding.name = strlen(real_locale_name) ? real_locale_name : locale_name;
    new_encoding.char_classifier.resize(COLLATE_SIZE, EncodingCharClasses::CHAR_CLASS_NOTHING);
    new_encoding.to_utf8.resize(COLLATE_SIZE, 0);
    new_encoding.to_utf8[0] = 0;

    for (int test_c = 0; test_c <= 0xff; ++test_c)
    {
        // Заполним для символа test_c классификационные признаки - флаги принадлежности к различным символьным классам.
        if (isalpha(test_c))
            new_encoding.char_classifier[test_c] |= EncodingCharClasses::CHAR_CLASS_LETTER;
        if (isdigit(test_c))
            new_encoding.char_classifier[test_c] |= EncodingCharClasses::CHAR_CLASS_DIGIT;
        if (isspace(test_c))
            new_encoding.char_classifier[test_c] |= EncodingCharClasses::CHAR_CLASS_SPACE;
        if (isblank(test_c))
            new_encoding.char_classifier[test_c] |= EncodingCharClasses::CHAR_CLASS_BLANK;
        if (iscntrl(test_c))
            new_encoding.char_classifier[test_c] |= EncodingCharClasses::CHAR_CLASS_CONTROL;
        if (ispunct(test_c))
            new_encoding.char_classifier[test_c] |= EncodingCharClasses::CHAR_CLASS_PUNCT;
        if (isxdigit(test_c))
            new_encoding.char_classifier[test_c] |= EncodingCharClasses::CHAR_CLASS_XDIGIT;
        if (isprint(test_c))
            new_encoding.char_classifier[test_c] |= EncodingCharClasses::CHAR_CLASS_PRINT;
        if (isgraph(test_c))
            new_encoding.char_classifier[test_c] |= EncodingCharClasses::CHAR_CLASS_GRAPHIC;
        // Сформируем для проверяемого символа test_c записи в таблице соответствия верхнего и нижнего символьных регистров.
        if (isupper(test_c))
        {
            // Проверим, нет ли уже записи для символа верхнего регистра test_c в составе таблицы регистрового парования.
            if (FindRegisterPairPos(test_c, true, new_encoding.upcase_table) == std::string::npos)
            {
                int low_test_c = tolower(test_c);
                if (low_test_c >= 0 && low_test_c <= 0xff)
                    new_encoding.upcase_table.push_back({test_c, low_test_c});
            }
        }
        if (islower(test_c))
        {
            // Проверим, нет ли уже записи для символа нижнего регистра test_c в составе таблицы регистрового парования.
            if (FindRegisterPairPos(test_c, false, new_encoding.upcase_table) == std::string::npos)
            {
                int up_test_c = toupper(test_c);
                if (up_test_c >= 0 && up_test_c <= 0xff)
                    new_encoding.upcase_table.push_back({up_test_c, test_c});
            }
        }
        // Получим и сохраним Юникод для однобайтового символа test_c.
        if (test_c != 0)
        {
            int unicode_byte_size = mbtowc(&unicode_char, reinterpret_cast<const char*>(&test_c), 1);
            if (unicode_byte_size >= 1 && unicode_byte_size <= 4)
            {
                if (sizeof(unicode_char) == 2)
                    new_encoding.to_utf8[test_c] = *reinterpret_cast<uint16_t*>(&unicode_char);
                else if (sizeof(unicode_char) == 4)
                    new_encoding.to_utf8[test_c] = *reinterpret_cast<uint32_t*>(&unicode_char);
            }
        }
    }

    // Далее попытаемся составить таблицу относительных сравнительных весов различных символов для данной кодировки.
    if (setlocale(LC_COLLATE, locale_name.c_str()))
    {
        std::vector<uint32_t> test_collate(COLLATE_SIZE, 0);
        char low_compare_str[2];
        char high_compare_str[2];
        low_compare_str[1] = 0;
        high_compare_str[1] = 0;
        bool continue_testing = true;
        uint32_t max_collate_value = 0, collate_test_steps = 0;

        while (continue_testing)
        {
            continue_testing = false;
            ++collate_test_steps;
            for (int low_compare_c = 0; low_compare_c <= 0xff; ++low_compare_c)
            {
                for (int high_compare_c = 0; high_compare_c <= 0xff; ++high_compare_c)
                {
                    low_compare_str[0] = static_cast<unsigned char>(low_compare_c);
                    high_compare_str[0] = static_cast<unsigned char>(high_compare_c);
                    int collate_result = strcoll(low_compare_str, high_compare_str);
                    if (collate_result < 0)
                    { // Символ low_compare_c меньше символа high_compare_c.
                        if (test_collate[low_compare_c] >= test_collate[high_compare_c])
                        { // А их веса указывают на обратное. Приведём состояние в соответствие с истинными показаниями
                          // сравнения символов в данной кодировке.
                            test_collate[high_compare_c] = test_collate[low_compare_c] + 1;
                            max_collate_value = max(max_collate_value, test_collate[high_compare_c]);
                            continue_testing = true;
                        }
                    }
                    else if (collate_result == 0)
                    { // Символы low_compare_c и high_compare_c равны.
                        if (test_collate[low_compare_c] != test_collate[high_compare_c])
                            continue_testing = true;
                        // Если веса при этом не равны, то сделаем их равными, выбрав для этого наибольший.
                        if (test_collate[low_compare_c] > test_collate[high_compare_c])
                            test_collate[high_compare_c] = test_collate[low_compare_c];
                        else if (test_collate[low_compare_c] < test_collate[high_compare_c])
                            test_collate[low_compare_c] = test_collate[high_compare_c];
                    }
                    else
                    { // Символ low_compare_c больше символа high_compare_c.
                        if (test_collate[low_compare_c] <= test_collate[high_compare_c])
                        { // А их веса указывают на обратное. Приведём состояние в соответствие с истинными показаниями
                          // сравнения символов в данной кодировке.
                            test_collate[low_compare_c] = test_collate[high_compare_c] + 1;
                            max_collate_value = max(max_collate_value, test_collate[low_compare_c]);
                            continue_testing = true;
                        }
                    }
                }
            }
            if (max_collate_value > (UINT32_MAX / 2))
                break;
        }
        // Первоначальная версия таблицы сравнительных весов готова. Далее её нужно нормализовать - привести к диапазону значений от 0 до 255.
        if (max_collate_value <= (UINT32_MAX / 2))
        {
            new_encoding.collate.resize(COLLATE_SIZE, 0);
            std::set<uint32_t> weight_values(test_collate.begin(), test_collate.end());
            for (int weighted_c = 0; weighted_c <= 0xff; ++weighted_c)
            {
                size_t normalized_weight = std::distance(weight_values.begin(), weight_values.find(test_collate[weighted_c]));
                new_encoding.collate[weighted_c] = static_cast<unsigned char>(normalized_weight);
            }
        }
        else
        {
            std::cerr << "Переполнение величин оценочных сравнительных весов для локали " << locale_name << std::endl;
        }
    }
    else
    {
        std::cerr << "Ошибка при установке целевой локали " << msg_locale_name << " для целей сортировки" << std::endl;
        return EXIT_FAILURE;
    }

    OutEncodingInfo(ostr, new_encoding, var_name);
    return EXIT_SUCCESS;
}

// Декодирование целого значения из текстового его представления. Корректный завершающий символ может быть как некоторой буквой
// (если is_letter_accepted == true), так и нулевым (во всех случаях).
std::pair<int, const char*> DecodeIntValue(const char* int_value_str, bool is_letter_accepted = false)
{
    char* end_symb_ptr;
    errno = 0;
    unsigned long int_result = strtoul(int_value_str, &end_symb_ptr, 10);
    if (errno != 0 || end_symb_ptr == int_value_str)
        return {int_result, nullptr};

    if (*end_symb_ptr == 0 || (is_letter_accepted && isalpha(*end_symb_ptr)))
        return {int_result, end_symb_ptr};
    else
        return {int_result, nullptr};
}

// Функция декодирования описания формата вывода итоговой информации.
bool DecodeFormatTerm(const char* term_body)
{
    while (*term_body != 0)
    {
        const char* next_term_symb = term_body + 1;
        std::pair<int, const char*> decode_result{0, nullptr};
        if (*next_term_symb != 0)
            decode_result = DecodeIntValue(next_term_symb, true);

        switch (*term_body)
        {
        case 'i':   // Величина шага выравнивания.
            if (decode_result.second)
                out_enc_config.indent_step = decode_result.first;
            else
                std::cerr << "Недопустимое значение шага выравнивания" << std::endl;
            break;
        case 'c':   // Количество значений в одной строке при формировании массива символьного классификатора.
            if (decode_result.second)
                out_enc_config.classifier_per_str = decode_result.first;
            else
                std::cerr << "Недопустимое значение количества элементов в строке классификатора" << std::endl;
            break;
        case 'u':   // Количество значений в одной строке при выводе таблицы перекодирования в UTF-8.
            if (decode_result.second)
                out_enc_config.to_utf8_per_str = decode_result.first;
            else
                std::cerr << "Некорректная величина количества элементов в строке массива перекодирования" << std::endl;
            break;
        case 'l':   // Количество символов (байт) в одной порции взвешивающей строки (строка collate).
            if (decode_result.second)
                out_enc_config.collate_per_str = decode_result.first;
            else
                std::cerr << "Невалидное значение числа символов в порции взвешивающей строки" << std::endl;
            break;
        default:
            std::cerr << "Неизвестный тип форматного раздела" << std::endl;
            return false;
        }
        if (!decode_result.second)
            return false;

        term_body = decode_result.second;
    }
    return true;
}

int main(int argc, char* argv[])
{
    setlocale(LC_CTYPE, "ru_RU.UTF-8"); // Изначально переключим отображение текста в консоль в UTF-8 режим.

    opterr = 0;
    bool is_help_required = false,
         is_error_occurred = false;
    std::string out_filename, var_name_prefix;
    std::vector<std::string> locale_names;

    int optr;
    while ((optr = getopt(argc, argv, "hl::o:v:f:")) != -1)
    {
        switch (optr)
        {
        case 'l':
            if (optarg)
                locale_names.push_back(optarg);
            else
                locale_names.push_back({});
            break;
        case 'o':
            out_filename = optarg;
            break;
        case 'v':
            var_name_prefix = optarg;
            break;
        case 'f':   // Формат вывода - количество элементов на строку при формировании различных выходных массивов.
            if (!DecodeFormatTerm(optarg))
                is_error_occurred = true;
            break;
        case 'h':
            is_help_required = true;
            break;
        default:
            std::cerr << "Ошибка в командной строке вызова программы" << std::endl;
            is_help_required = true;
            is_error_occurred = true;
            break;
        }
    }

    if (is_help_required || locale_names.empty())
    {
        std::cerr << "Формат вызова команды:\n ExtractEncodingInfo (-h) | ((-l:locale_name)+ (-o:file_name)? (-v:var_name_prefix)? (-f:format_term)?)\n"
                  << "\t -h - вывод данной помощи и завершение работы." << std::endl
                  << "\t locale_name - имя C - локали, которая будет установлена для сбора информации." << std::endl
                  << "\t file_name - имя выходного файла, в который будет сохранена собранная информация." << std::endl
                  << "\t var_name_prefix - имя переменной (или его префикс) типа SingleByteEncodingDesc, создаваемой для каждой локали." << std::endl
                  << "\t format_term - запись с описанием формата (некоторых его особенностей) сохраняемых данных." << std::endl;
        return is_error_occurred ? EXIT_FAILURE : EXIT_SUCCESS;
    }
    if (is_error_occurred)
        return EXIT_FAILURE;

    std::ostream* use_ostr;
    std::ofstream ofile_str;
    if (out_filename.empty())
    {
        use_ostr = &std::cout;
    }
    else
    {
        ofile_str.open(out_filename);
        if (!ofile_str)
        {
            std::cerr << "Не удалось создать выходной файл " << out_filename << std::endl;
            return EXIT_FAILURE;
        }
        use_ostr = &ofile_str;
    }

    std::unordered_map<std::string, int> locale_var_storage;
    for (const std::string& use_locale : locale_names)
    {
        std::string nonempty_locale_name = !use_locale.empty() ? use_locale : C_LOCALE_NAME;
        std::string locale_var_name;    // Имя переменной, которая будет содержать информацию о текущей исследуемой локали use_locale.

        if (var_name_prefix.empty())
            locale_var_name = nonempty_locale_name;
        else if (locale_names.size() > 1)
            locale_var_name = var_name_prefix + '_' + nonempty_locale_name;
        else
            locale_var_name = var_name_prefix;

        if (auto locale_var_storage_it = locale_var_storage.find(locale_var_name); locale_var_storage_it != locale_var_storage.end())
        { // Ранее переменная с таким именем уже встречалась. Обеспечим её уникальность приписыванием к хвосту её порядкового номера.
            locale_var_name += '_' + std::to_string(locale_var_storage_it->second);
            ++(locale_var_storage_it->second);
        }
        else
        { // Такой переменной ранее не встречалось. Для первого экземпляра используем её имя как есть.
            locale_var_storage.emplace(locale_var_name, 1);
        }

        ExtractLocaleData(*use_ostr, use_locale, locale_var_name);
    }
    if (!(*use_ostr))
        std::cerr << "Ошибка при записи в выходной файл" << std::endl;

    return EXIT_SUCCESS;
}
