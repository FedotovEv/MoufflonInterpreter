#pragma once

#include <string>
#include <vector>
#include <optional>

extern const std::vector<std::pair<char, char>> empty_upcase_table;
extern const std::string empty_collate, std_collate;
extern const std::vector<uint32_t> std_to_utf8;
extern std::vector<SingleByteEncodingDesc> encodings_data;

struct CompareCollateMode
{
	// Параметры однобайтовой кодировки, существенные для сравнения.
	// Парность символов верхнего и нижнего регистра. Первый член - символ верхнего регистра, второй - нижнего.
	const std::vector<std::pair<char, char>>& upcase_table = empty_upcase_table;
	const std::string& collate = empty_collate;		// Список сравнительных весов символов.
	// Флаги (способа) режима сравнения.
    // Семантика флагов полность аналогична структуре parse:token_type:String.
    bool is_use_collate = true;				// Применение "весов" символов при сравнительных действиях над строками.
    bool is_equal_collate = true;			// Способ применения весов при сравнении строк на точное равенство.
    bool is_case_indep_compare = false;     // Регистронезависимое сравнение.
};

enum class UTF8ErrorCode
{
	UTF8_NO_ERROR = 0,
	UTF8_STRING_TERMINATED,			// Строка обрывается преждевременно.
	UTF8_TO_UTF_TABLE_TOO_SHORT,	// Таблица кодирования в UTF-8 представление слишком коротка.
	UTF8_CONTINUE_WITHOUT_START,	// Встретился байт продолжения при отсутствии стартового байта кодовой точки.
	UTF8_UNICODE_TOO_LONG,			// Длина Юникода больше максимально допустимой или не помещается в тип uint32_t.
	UTF8_SYMBOL_NOT_DEFINED			// Символ с таким Юникодом не определён для данной однобайтовой кодировки.
};

struct UTF8Error
{
	UTF8ErrorCode code = UTF8ErrorCode::UTF8_NO_ERROR;
	size_t pos = std::string::npos;
};

using TranscodeResult = std::pair<std::string, UTF8Error>;

// Поиск зарегистрированной кодировки с именем encoding_name. Возвращает идент (числовой идентификатор) найденной кодировки либо
// NON_INDEXED_ENCODING_ID, если кодировка с таким именем не найдена.
int FindEncoding(const std::string& encoding_name);
// Получение указателя на запись внутренней базы данных, описывающей свойства определённой кодировки с идентом encoding_id.
const SingleByteEncodingDesc* GetEncoding(int encoding_id);
// Поиск в таблице регистрового спаривания upcase_table записи (пары) для символа scan_c верхнего (при scan_for_up == true) или нижнего
// (при scan_for_up == false) регистров.
std::optional<std::pair<char, char>> FindRegisterPair(char scan_c, bool scan_for_up, const std::vector<std::pair<char, char>>& upcase_table);
// Многорежимная функция сравнения однобайтовых строк op_str_1 и op_str_2 с возможностью игнорирования регистра символов и
// применения взвешивающей строки.
int CompareCollate(const std::string& op_str_1, const std::string& op_str_2, const CompareCollateMode& compare_mode = {});
// Функция сравнения подстрок UTF-8 кодированных строк. Начальные позиции подстрок и их размеры являются байтовыми.
int CompareUTF8Substr(const std::string& op_str_1, size_t start_op_pos_1, size_t op_size_1,
					  const std::string& op_str_2, size_t start_op_pos_2, size_t op_size_2);
// Функция сравнения полных UTF-8 кодированных строк.
int CompareUTF8(const std::string& op_str_1, const std::string& op_str_2);
// Преобразование символа с UNCODE-кодом unicode_symb в набор байт в UTF-8 представлении.
std::string ConvSymbToUTF8(uint32_t unicode_symb);
// Извлечение UTF-8 символа, начиная с позиции symb_pos строки src_utf8_string. Его UNICODE-код возвращается в первом члене итоговой пары,
// а длина его UTF-8 представления в составе строки - во втором её члене. При ошибке декодирования второй член пары устанавливается в нуль,
// а первый будет в этом случае равен коду ошибки - одному из членов перечисления UTF8ErrorCode.
std::pair<uint32_t, size_t> ConvSymbFromUTF8(const std::string& src_utf8_string, size_t symb_pos);
// Две похожие (различающиеся только типом возвращаемого результата) функции перекодирования из однобайтовой кодировки в кодировку UTF-8.
// Второй вариант этой функции попутно составляет и возвращает карту расположения Юникодов исходника в итоговой UTF-8-строке.
TranscodeResult TranscodeToUTF8(const std::string& unibyte_source_str, const std::vector<uint32_t>& to_utf8);
std::tuple<std::string, UTF8Map, UTF8Error> TranscodeToUTF8Ex(const std::string& unibyte_source_str, const std::vector<uint32_t>& to_utf8);
// Перекодирование из UTF-8 в некоторую однобайтовую кодировку, определяемую массивом to_utf8.
TranscodeResult TranscodeFromUTF8(const std::string& utf8_source_str, const std::vector<uint32_t>& to_utf8);
// Транскодирование между двумя однобайтовыми кодировками, определяемыми массивами src_to_utf8 (исходная) и dest_to_utf8 (целевая).
TranscodeResult TranscodeBetweenUnibytes
	(const std::string& unibyte_source_str, const std::vector<uint32_t>& src_to_utf8, const std::vector<uint32_t>& dest_to_utf8);
// Генерация карты размещения многобайтовых UTF-8-кодов в пределах однобайтовой строки (потока байтов) parse_str.
// Также возвращает юникод последнего символа, полученного в процессе работы функции.
std::pair<UTF8Map, uint32_t> BuildUTF8Map(const std::string& parse_str, size_t max_elem_count = (std::numeric_limits<size_t>::max)());
