
#include "declares.h"
#include "encodings.h"
#include <optional>

using namespace std::literals;

static constexpr size_t MAX_BITS_IN_START_BYTE = 5;		// Максимальное количество значащих бит в стартовом байте UTF-8-кода.
static constexpr size_t BITS_PER_CONTINUE_BYTE = 6;		// Число значащих бит в "байте продолжения" UTF-8-кода.
static constexpr char CONTINUE_BYTE_DATAMASK = 0x3f;	// Маска области данных (значащей области) "байта продолжения".

const std::vector<std::pair<char, char>> empty_upcase_table;
const std::string empty_collate;

// "Стандартная" таблица классификации, которая соответствует минимальной C-локали библиотеки периода исполнения среды msvc.
const std::vector<EncodingCharClasses> std_char_classifier
{

};

// Тривиальная таблица сравнительных весов символов, в которой каждый символ имеет вес, равный его коду.
const std::string std_collate
{
	"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
	"\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f"
	"\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f"
	"\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x7e\x7f"
	"\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f"
	"\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf"
	"\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf"
	"\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"s
};

// Таблица преобразования в Юникод для стандартной полной 8-битовой таблицы ASCII. Будет использоваться и в том случае, если кодировка
// строки не определена, но такая таблица необходима.
const std::vector<uint32_t> std_to_utf8
{
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
	0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7, 0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
	0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9, 0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7,	0x0192,
	0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba, 0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b,	0x2510,
	0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
	0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,
	0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4, 0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229,
	0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248, 0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0
};

const SingleByteEncodingDesc CP437_ENC =
{
	.name = "CP437",
	.upcase_table
	{
		// Регистровая парность для латинских букв.
		{0x41, 0x61},
		{0x42, 0x62},
		{0x43, 0x63},
		{0x44, 0x64},
		{0x45, 0x65},
		{0x46, 0x66},
		{0x47, 0x67},
		{0x48, 0x68},
		{0x49, 0x69},
		{0x4A, 0x6A},
		{0x4B, 0x6B},
		{0x4C, 0x6C},
		{0x4D, 0x6D},
		{0x4E, 0x6E},
		{0x4F, 0x6F},
		{0x50, 0x70},
		{0x51, 0x71},
		{0x52, 0x72},
		{0x53, 0x73},
		{0x54, 0x74},
		{0x55, 0x75},
		{0x56, 0x76},
		{0x57, 0x77},
		{0x58, 0x78},
		{0x59, 0x79},
		{0x5A, 0x7A},
		// Регистровые пары для прочих символов второй половины таблицы ASCII.
		{0x80, 0x87}, // LATIN CAPITAL LETTER C WITH CEDILLA - LATIN SMALL LETTER C WITH CEDILLA
		{0x9a, 0x81}, // LATIN CAPITAL LETTER U WITH DIAERESIS - LATIN SMALL LETTER U WITH DIAERESIS
		{0x90, 0x82}, // LATIN CAPITAL LETTER E WITH ACUTE - LATIN SMALL LETTER E WITH ACUTE
		{0x8e, 0x84}, // LATIN CAPITAL LETTER A WITH DIAERESIS - LATIN SMALL LETTER A WITH DIAERESIS
		{0x8f, 0x86}, // LATIN CAPITAL LETTER A WITH RING ABOVE - LATIN SMALL LETTER A WITH RING ABOVE
		{0x92, 0x91}, // LATIN CAPITAL LIGATURE AE - LATIN SMALL LIGATURE AE
		{0x99, 0x94}, // LATIN CAPITAL LETTER O WITH DIAERESIS - LATIN SMALL LETTER O WITH DIAERESIS
		{0xa5, 0xa4}, // LATIN CAPITAL LETTER N WITH TILDE - LATIN SMALL LETTER N WITH TILDE
		{0xe4, 0xe5}, // GREEK CAPITAL LETTER SIGMA - GREEK SMALL LETTER SIGMA
		{0xe8, 0xed}  // GREEK CAPITAL LETTER PHI - GREEK SMALL LETTER PHI
	},
	.to_utf8 = std_to_utf8
};

const SingleByteEncodingDesc CP866_ENC =
{
	.name = "CP866",
	.upcase_table
	{
		// Регистровая парность для латинских букв.
		{0x41, 0x61},
		{0x42, 0x62},
		{0x43, 0x63},
		{0x44, 0x64},
		{0x45, 0x65},
		{0x46, 0x66},
		{0x47, 0x67},
		{0x48, 0x68},
		{0x49, 0x69},
		{0x4A, 0x6A},
		{0x4B, 0x6B},
		{0x4C, 0x6C},
		{0x4D, 0x6D},
		{0x4E, 0x6E},
		{0x4F, 0x6F},
		{0x50, 0x70},
		{0x51, 0x71},
		{0x52, 0x72},
		{0x53, 0x73},
		{0x54, 0x74},
		{0x55, 0x75},
		{0x56, 0x76},
		{0x57, 0x77},
		{0x58, 0x78},
		{0x59, 0x79},
		{0x5A, 0x7A},
		// Регистровые пары для кириллицы.
		{0x80, 0xA0},
		{0x81, 0xA1},
		{0x82, 0xA2},
		{0x83, 0xA3},
		{0x84, 0xA4},
		{0x85, 0xA5},
		{0x86, 0xA6},
		{0x87, 0xA7},
		{0x88, 0xA8},
		{0x89, 0xA9},
		{0x8A, 0xAA},
		{0x8B, 0xAB},
		{0x8C, 0xAC},
		{0x8D, 0xAD},
		{0x8E, 0xAE},
		{0x8F, 0xAF},
		{0x90, 0xE0},
		{0x91, 0xE1},
		{0x92, 0xE2},
		{0x93, 0xE3},
		{0x94, 0xE4},
		{0x95, 0xE5},
		{0x96, 0xE6},
		{0x97, 0xE7},
		{0x98, 0xE8},
		{0x99, 0xE9},
		{0x9A, 0xEA},
		{0x9B, 0xEB},
		{0x9C, 0xEC},
		{0x9D, 0xED},
		{0x9E, 0xEE},
		{0x9F, 0xEF},
		{0xF0, 0xF1},	// "Ё" и "ё"
	},
	.to_utf8
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
		32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
		64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
		80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
		96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
		1040, 1041, 1042, 1043, 1044, 1045, 1046, 1047, 1048, 1049, 1050, 1051, 1052, 1053, 1054, 1055,
		1056, 1057, 1058, 1059, 1060, 1061, 1062, 1063, 1064, 1065, 1066, 1067, 1068, 1069, 1070, 1071,
		1072, 1073, 1074, 1075, 1076, 1077, 1078, 1079, 1080, 1081, 1082, 1083, 1084, 1085, 1086, 1087,
		9617, 9618, 9619, 9474, 9508, 9569, 9570, 9558, 9557, 9571, 9553, 9559, 9565, 9564, 9563, 9488,
		9492, 9524, 9516, 9500, 9472, 9532, 9566, 9567, 9562, 9556, 9577, 9574, 9568, 9552, 9580, 9575,
		9576, 9572, 9573, 9561, 9560, 9554, 9555, 9579, 9578, 9496, 9484, 9608, 9604, 9612, 9616, 9600,
		1088, 1089, 1090, 1091, 1092, 1093, 1094, 1095, 1096, 1097, 1098, 1099, 1100, 1101, 1102, 1103,
		1025, 1105, 1028, 1108, 1031, 1111, 1038, 1118, 176, 8729, 183, 8730, 8470, 164, 9632, 160
	}
};

const SingleByteEncodingDesc CP1251_ENC =
{
	.name = "CP1251",
	.upcase_table
	{
		// Регистровая парность для латинских букв.
		{0x41, 0x61},
		{0x42, 0x62},
		{0x43, 0x63},
		{0x44, 0x64},
		{0x45, 0x65},
		{0x46, 0x66},
		{0x47, 0x67},
		{0x48, 0x68},
		{0x49, 0x69},
		{0x4A, 0x6A},
		{0x4B, 0x6B},
		{0x4C, 0x6C},
		{0x4D, 0x6D},
		{0x4E, 0x6E},
		{0x4F, 0x6F},
		{0x50, 0x70},
		{0x51, 0x71},
		{0x52, 0x72},
		{0x53, 0x73},
		{0x54, 0x74},
		{0x55, 0x75},
		{0x56, 0x76},
		{0x57, 0x77},
		{0x58, 0x78},
		{0x59, 0x79},
		{0x5A, 0x7A},
		// Регистровые пары для кириллицы.
		{0x80, 0x90},	// "Ђ" и "ђ"
		{0x81, 0x83},	// "Ѓ" и "ѓ"
		{0x8A, 0x9A},	// "Љ" и "љ"
		{0x8C, 0x9C},	// "Њ" и "њ"
		{0x8D, 0x9D},	// "Ќ" и "ќ"
		{0x8E, 0x9E},	// "Ћ" и "ћ"
		{0x8F, 0x9F},	// "Џ" и "џ"
		{0xA1, 0xA2},	// "Ў" и "ў"
		{0xA3, 0xBC},	// "Ј" и "ј"
		{0xA5, 0xB4},	// "Ґ" и "ґ"
		{0xA8, 0xB8},	// "Ё" и "ё"
		{0xAA, 0xBA},	// "Є" и "є"
		{0xAF, 0xBF},	// "Ї" и "ї"
		{0xB2, 0xB3},	// "І" и "і"
		{0xBD, 0xBE},	// "Ѕ" и "ѕ"
		{0xC0, 0xE0},
		{0xC1, 0xE1},
		{0xC2, 0xE2},
		{0xC3, 0xE3},
		{0xC4, 0xE4},
		{0xC5, 0xE5},
		{0xC6, 0xE6},
		{0xC7, 0xE7},
		{0xC8, 0xE8},
		{0xC9, 0xE9},
		{0xCA, 0xEA},
		{0xCB, 0xEB},
		{0xCC, 0xEC},
		{0xCD, 0xED},
		{0xCE, 0xEE},
		{0xCF, 0xEF},
		{0xD0, 0xF0},
		{0xD1, 0xF1},
		{0xD2, 0xF2},
		{0xD3, 0xF3},
		{0xD4, 0xF4},
		{0xD5, 0xF5},
		{0xD6, 0xF6},
		{0xD7, 0xF7},
		{0xD8, 0xF8},
		{0xD9, 0xF9},
		{0xDA, 0xFA},
		{0xDB, 0xFB},
		{0xDC, 0xFC},
		{0xDD, 0xFD},
		{0xDE, 0xFE},
		{0xDF, 0xFF},
	},
	.to_utf8
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
		32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
		64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
		80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
		96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
		0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021, 0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
		0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 32, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
		0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7, 0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
		0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7, 0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
		0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
		0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
		0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
		0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F
	}
};

// Глобальные данные о кодировках, зарегистрированных в данном объекте. Ключ - имя кодировки, значение - её описание.
std::vector<SingleByteEncodingDesc> encodings_data
{
	CP437_ENC,
	CP866_ENC,
	CP1251_ENC
};

// Определение методов структуры SingleByteEncodingDesc, которые не определены несопредственно при её описании.
// Классификация однобайтового символа test_char относительно класса букв.
bool SingleByteEncodingDesc::IsAlpha(int test_char) const
{
	if (IsClassifierValid())
	{
		if (test_char < 0 || test_char >= static_cast<int>(char_classifier.size()))
			return false;
		else
			return char_classifier[test_char] & EncodingCharClasses::CHAR_CLASS_LETTER;
	}
	else
	{
		return isalpha(static_cast<unsigned char>(test_char));
	}
}

// Проверка однобайтовго символа test_char на принадлежность классам букв или цифр.
bool SingleByteEncodingDesc::IsAlNum(int test_char) const
{
	if (IsClassifierValid())
	{
		if (test_char < 0 || test_char >= static_cast<int>(char_classifier.size()))
			return false;
		else
			return (char_classifier[test_char] & EncodingCharClasses::CHAR_CLASS_LETTER) ||
			       (char_classifier[test_char] & EncodingCharClasses::CHAR_CLASS_DIGIT);
	}
	else
	{
		return isalnum(static_cast<unsigned char>(test_char));
	}
}

// Проверка однобайтовго символа test_char на принадлежность классу цифр.
bool SingleByteEncodingDesc::IsDigit(int test_char) const
{
	if (IsClassifierValid())
	{
		if (test_char < 0 || test_char >= static_cast<int>(char_classifier.size()))
			return false;
		else
			return char_classifier[test_char] & EncodingCharClasses::CHAR_CLASS_DIGIT;
	}
	else
	{
		return isdigit(static_cast<unsigned char>(test_char));
	}
}

// Является ли одноюайтовый символ test_char шестнадцатиричной цифрой.
bool SingleByteEncodingDesc::IsXDigit(int test_char) const
{
	if (IsClassifierValid())
	{
		if (test_char < 0 || test_char >= static_cast<int>(char_classifier.size()))
			return false;
		else
			return char_classifier[test_char] & EncodingCharClasses::CHAR_CLASS_XDIGIT;
	}
	else
	{
		return isxdigit(static_cast<unsigned char>(test_char));
	}
}

// Проверка на принадлежность символа test_char к буквам нижнего регистра.
bool SingleByteEncodingDesc::IsLower(int test_char) const
{
	if (IsClassifierValid())
	{ 
		if (test_char < 0 || test_char >= static_cast<int>(char_classifier.size()))
			return false;
		else if (!(char_classifier[test_char] & EncodingCharClasses::CHAR_CLASS_LETTER))
			// Будем считать, что различие в регистре присуще только буквам. Если анализируемый символ не буква,
			// то он не может быть "малой" или "большой".
			return false;
		return FindRegisterPair(static_cast<char>(test_char), false, upcase_table).has_value();
	}
	else
	{
		return islower(static_cast<unsigned char>(test_char));
	}
}

// Проверка на принадлежность символа test_char к буквам верхнего регистра.
bool SingleByteEncodingDesc::IsUpper(int test_char) const
{
	if (IsClassifierValid())
	{
		if (test_char < 0 || test_char >= static_cast<int>(char_classifier.size()))
			return false;
		else if (!(char_classifier[test_char] & EncodingCharClasses::CHAR_CLASS_LETTER))
			// Будем считать, что различие в регистре присуще только буквам. Если анализируемый символ не буква,
			// то он не может быть "малой" или "большой".
			return false;
		return FindRegisterPair(static_cast<char>(test_char), true, upcase_table).has_value();
	}
	else
	{
		return isupper(static_cast<unsigned char>(test_char));
	}
}

//
bool SingleByteEncodingDesc::IsCntrl(int test_char) const
{
	if (IsClassifierValid())
	{
		if (test_char == EOF)
			return true;	// EOF будем относить к управляющим символам.
		else if (test_char < 0 || test_char >= static_cast<int>(char_classifier.size()))
			return false;
		else
			return char_classifier[test_char] & EncodingCharClasses::CHAR_CLASS_CONTROL;
	}
	else
	{
		return iscntrl(static_cast<unsigned char>(test_char));
	}
}

//
bool SingleByteEncodingDesc::IsGraph(int test_char) const
{
	if (IsClassifierValid())
	{
		if (test_char < 0 || test_char >= static_cast<int>(char_classifier.size()))
			return false;
		else
			return char_classifier[test_char] & EncodingCharClasses::CHAR_CLASS_GRAPHIC;
	}
	else
	{
		return isgraph(static_cast<unsigned char>(test_char));
	}
}

//
bool SingleByteEncodingDesc::IsSpace(int test_char) const
{
	if (IsClassifierValid())
	{
		if (test_char < 0 || test_char >= static_cast<int>(char_classifier.size()))
			return false;
		else
			return char_classifier[test_char] & EncodingCharClasses::CHAR_CLASS_SPACE;
	}
	else
	{
		return isspace(static_cast<unsigned char>(test_char));
	}
}

//
bool SingleByteEncodingDesc::IsBlank(int test_char) const
{
	if (IsClassifierValid())
	{
		if (test_char < 0 || test_char >= static_cast<int>(char_classifier.size()))
			return false;
		else
			return char_classifier[test_char] & EncodingCharClasses::CHAR_CLASS_BLANK;
	}
	else
	{
		return isblank(static_cast<unsigned char>(test_char));
	}
}

//
bool SingleByteEncodingDesc::IsPrint(int test_char) const
{
	if (IsClassifierValid())
	{
		if (test_char < 0 || test_char >= static_cast<int>(char_classifier.size()))
			return false;
		else
			return char_classifier[test_char] & EncodingCharClasses::CHAR_CLASS_PRINT;
	}
	else
	{
		return isprint(static_cast<unsigned char>(test_char));
	}
}

//
bool SingleByteEncodingDesc::IsPunct(int test_char) const
{
	if (IsClassifierValid())
	{
		if (test_char < 0 || test_char >= static_cast<int>(char_classifier.size()))
			return false;
		else
			return char_classifier[test_char] & EncodingCharClasses::CHAR_CLASS_PUNCT;
	}
	else
	{
		return ispunct(static_cast<unsigned char>(test_char));
	}
}

// Определения методов типа UTF8Map.
// Функция-член возвращает байтовую позицию сразу за концом корректной UTF-8-строки.
size_t UTF8Map::BytePosAfterEnd() const
{
	if (begin_map.empty())
		return 0;
	else
		return begin_map.back() + last_symbol_size;
}

// Возвращает байтовую позицию символа с индексом symb_index.
size_t UTF8Map::SymbolBytePos(size_t symb_index) const
{
	if (symb_index < begin_map.size())
		return begin_map[symb_index];
	else if (symb_index == begin_map.size())
		return BytePosAfterEnd();
	else
		return std::string::npos;
}

// Расчёт байтовой длины (длины в байтах) кода символа с индексом symb_index.
size_t UTF8Map::SymbolByteSize(size_t symb_index) const
{
	if ((symb_index + 1) < begin_map.size())  // Существуют символы symb_index и следующий за ним.
		return begin_map[symb_index + 1] - begin_map[symb_index];
	else if (symb_index < begin_map.size()) // Существует только символ symb_index.
		return BytePosAfterEnd() - begin_map[symb_index];
	else
		return 0;
}

// Поиск существующей кодировки по её имени.
int FindEncoding(const std::string& encoding_name)
{
	if (encoding_name == NO_ENCODING_NAME)
		return NO_ENCODING_ID;
	if (encoding_name == UTF_8_ENCODING_NAME)
		return UTF_8_ENCODING_ID;
	// Имя искомой кодировки не относится к стандартным.
	for (auto enc_data_it = encodings_data.begin(); enc_data_it != encodings_data.end(); ++enc_data_it)
	{
		if (enc_data_it->name == encoding_name)
			return static_cast<int>(enc_data_it - encodings_data.begin() + 1);
	}
	// Кодировка с именем encoding_name среди зарегистрированных не значится.
	return NON_INDEXED_ENCODING_ID;
}

// Получение указателя на запись с описанием кодировки с идентом encoding_id.
const SingleByteEncodingDesc* GetEncoding(int encoding_id)
{
	switch (encoding_id)
	{
	case NON_INDEXED_ENCODING_ID:
		[[fallthrough]];
	case NO_ENCODING_ID:
		return NO_ENCODING;
	case UTF_8_ENCODING_ID:
		return UTF_8_ENCODING;
	default:
		--encoding_id;
		if (encoding_id < 0 || encoding_id >= static_cast<int>(encodings_data.size()))
			return NO_ENCODING;
		return &encodings_data[encoding_id];
	}
	return nullptr;
}

// Поиск в таблице регистрового спаривания upcase_table записи (пары) для символа scan_c верхнего (при scan_for_up == true) или нижнего
// (при scan_for_up == false) регистров.
std::optional<std::pair<char, char>> FindRegisterPair(char scan_c, bool scan_for_up, const std::vector<std::pair<char, char>>& upcase_table)
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
		return {};	// Запись для требуемого символа в таблице парности не найдена.
	else
		return *upcase_table_it;
}

char ConvSymbToUpper(char inp_c, const std::vector<std::pair<char, char>>& upcase_table)
{
	if (std::optional<std::pair<char, char>> inp_reg_pair = FindRegisterPair(inp_c, false, upcase_table); inp_reg_pair)
		// Символ inp_c принадлежит в нижнему регистру. Преобразуем его к верхнему.
		return inp_reg_pair->first;
	else
		return inp_c;
}

char ConvSymbToLower(char inp_c, const std::vector<std::pair<char, char>>& upcase_table)
{
	if (std::optional<std::pair<char, char>> inp_reg_pair = FindRegisterPair(inp_c, true, upcase_table); inp_reg_pair)
		// Символ inp_c принадлежит в верхнему регистру. Преобразуем его к нижнему.
		return inp_reg_pair->second;
	else
		return inp_c;
}

// Многорежимная функция сравнения однобайтовых строк op_str_1 и op_str_2 с возможностью игнорирования регистра символов и
// применения взвешивающей строки.
int CompareCollate(const std::string& op_str_1, const std::string& op_str_2, const CompareCollateMode& compare_mode)
{
	for (size_t i = 0; i < min(op_str_1.size(), op_str_2.size()); ++i)
	{
		char opc_1 = op_str_1[i],
			 opc_2 = op_str_2[i];

		if (compare_mode.is_case_indep_compare)
		{ // Перед сравнением преобразуем оба символа в нижний регистр.
			opc_1 = ConvSymbToLower(opc_1, compare_mode.upcase_table);
			opc_2 = ConvSymbToLower(opc_2, compare_mode.upcase_table);
		}
		// Сначала проверим символы opc_1 и opc_2 на равенство. Если они равны, продолжаем просмотр строк.
		if (compare_mode.is_use_collate && compare_mode.is_equal_collate && compare_mode.collate.size() == COLLATE_SIZE)
		{ // Заменяем символы их весами.
			if (compare_mode.collate[opc_1] == compare_mode.collate[opc_2])
				continue;
		}
		else
		{ // Сравниваем непосредственно коды символов.
			if (opc_1 == opc_2)
				continue;
		}

		// Очередные символы не равны, осталось определить их соотношение по величине и вернуть результат всего сравнения.
		bool is_first_lesser;
		if (compare_mode.is_use_collate && compare_mode.collate.size() == COLLATE_SIZE)
			is_first_lesser = compare_mode.collate[opc_1] <= compare_mode.collate[opc_2]; // Сравниваем символы по весам.
		else
			is_first_lesser = opc_1 <= opc_2; // Выполняем сравнение по величинам кодов.

		if (is_first_lesser)
			return -static_cast<int>(i + 1);
		else
			return static_cast<int>(i + 1);
	}
	// Все символы кратчайшего аргумента обследованы и расхождений не выявлено. В таких условиях меньшей считаем более короткую строку.
	if (op_str_1.size() == op_str_2.size())
		return 0;	// Аргументы равны по длине и содержимому.
	else if (op_str_1.size() < op_str_2.size())
		return -static_cast<int>(op_str_1.size() + 1);	// Первый аргумент короче и, следовательно, меньше второго.
	else
		return static_cast<int>(op_str_2.size() + 1);	// Второй аргумент короче и, следовательно, меньше первого.
}

// Функция сравнения подстрок UTF-8 кодированных строк. Начальные позиции подстрок и их размеры являются байтовыми.
int CompareUTF8Substr(const std::string& op_str_1, size_t start_op_pos_1, size_t op_size_1,
					  const std::string& op_str_2, size_t start_op_pos_2, size_t op_size_2)
{
	size_t op_pos_1 = start_op_pos_1, op_pos_2 = start_op_pos_2;
	size_t op_use_size_1 = min(op_str_1.size(), op_size_1);
	size_t op_use_size_2 = min(op_str_2.size(), op_size_2);

	while (op_pos_1 < op_use_size_1 && op_pos_2 < op_use_size_2)
	{
		std::pair<uint32_t, size_t> current_symb_1 = ConvSymbFromUTF8(op_str_1, op_pos_1);
		std::pair<uint32_t, size_t> current_symb_2 = ConvSymbFromUTF8(op_str_2, op_pos_2);
		if (current_symb_1.second == 0)
			op_pos_1 = (std::numeric_limits<size_t>::max)();
		if (current_symb_2.second == 0)
			op_pos_2 = (std::numeric_limits<size_t>::max)();
		if (op_pos_1 >= op_use_size_1 || op_pos_2 >= op_use_size_2)
			break;
		// Очередные легитимные Юникоды выделены из обоих строк.
		if (current_symb_1.first < current_symb_2.first) // Первая строка op_str_1 "меньше" второй строки op_str_2.
			return -static_cast<int>(op_pos_1 + 1);
		else if (current_symb_1.first > current_symb_2.first) // Первая строка op_str_1 "больше" второй строки op_str_2.
			return static_cast<int>(op_pos_1 + 1);

		op_pos_1 += current_symb_1.second;
		op_pos_2 += current_symb_2.second;
	}
	// Выясняем и возвращаем результат сравнения для строк с равными префиксами, так как он не был выяснен ранее, внутри цикла.
	if (op_pos_1 < op_use_size_1)			// Строка op_str_1 длиннее (и, следовательно, будет считаться "больше") строки op_str_2.
		return static_cast<int>(op_pos_1 + 1);
	else if (op_pos_2 < op_use_size_2)		// Строка op_str_2 длиннее (и, следовательно, будет считаться "больше") строки op_str_1.
		return -static_cast<int>(op_use_size_1 + 1);
	else									// Обе строки одинаковы.
		return 0;
}

// Функция сравнения полных UTF-8-кодированных строк.
int CompareUTF8(const std::string& op_str_1, const std::string& op_str_2)
{
	return CompareUTF8Substr(op_str_1, 0, op_str_1.size(), op_str_2, 0, op_str_2.size());
}

// Преобразование символа с UNCODE-кодом unicode_symb в набор байт в UTF-8 представлении.
std::string ConvSymbToUTF8(uint32_t unicode_symb)
{
	static constexpr char CONTINUE_BYTE_SIGNATURE = (char)0x80;	// Сигнатура "байта продолжения" (его формат - 0b10xxxxxx, два старших его бита равны 10).

	// Посчитаем количество значащих битов во входном коде (незначащим является участок старших нулевых битов).
	size_t bits_count = 0;
	for (uint32_t cnt_unicode_symb = unicode_symb; cnt_unicode_symb; cnt_unicode_symb >>= 1, ++bits_count);
	if (bits_count < 8)
		return std::string(1, unicode_symb);	// Однобайтовые коды ASCII-7.

	// Подсчёт длины итогового UTF-8 кода (! - здесь bits_count всегда <= 32 < (7 * 5) == 35).
	size_t utf8_code_length = 1;
	for (size_t bits_in_start_byte = MAX_BITS_IN_START_BYTE; bits_count > bits_in_start_byte;
		 ++utf8_code_length, bits_count -= min(BITS_PER_CONTINUE_BYTE, bits_count), --bits_in_start_byte);
	
	std::string result_str(utf8_code_length, 0);	// Накопитель, в котором будет формироваться результирующий UTF-8_код.
	// Вычисляем сигнатуру (набор старших бит) ведущего байта UTF-8-кода.
	char start_byte_signature = 0;
	for (size_t one_bits_count = utf8_code_length; one_bits_count; --one_bits_count)
	{
		start_byte_signature >>= 1;
		start_byte_signature |= 0x80;
	}

	// Длина результирующего кода подсчитана, место для него выделено. Теперь заполним его данными кода исходного символа.	
	// Заполняем справа по BITS_PER_CONTINUE_BYTE младших бит данных на байт кода. Исключение - самый старший байт, туда помещается всё,
	// что останется от заполнения "байтов продолжения".
	for (char* set_byte = result_str.data() + utf8_code_length - 1; utf8_code_length; --utf8_code_length, --set_byte)
	{
		if (utf8_code_length == 1)	// Это стартовый байт.
			*set_byte = start_byte_signature;
		else  // Это "байты продолжения".
			*set_byte = CONTINUE_BYTE_SIGNATURE;

		*set_byte |= unicode_symb & CONTINUE_BYTE_DATAMASK;
		unicode_symb >>= BITS_PER_CONTINUE_BYTE;
	}

	return result_str;
}

// Извлечение UTF-8 символа, начиная с позиции symb_pos строки src_utf8_string. Его UNICODE-код возвращается в первом члене итоговой пары,
// а длина его UTF-8 представления в составе строки - во втором её члене. При ошибке декодирования второй член пары устанавливается в нуль,
// а первый будет в этом случае равен коду ошибки - одному из членов перечисления UTF8ErrorCode.
std::pair<uint32_t, size_t> ConvSymbFromUTF8(const std::string& src_utf8_string, size_t symb_pos)
{
	if (symb_pos >= src_utf8_string.size())
		// Начальная позиция кода за пределами входной строки.
		return {static_cast<uint32_t>(UTF8ErrorCode::UTF8_STRING_TERMINATED), 0};
	char leading_byte = src_utf8_string[symb_pos];
	if (!(leading_byte & 0x80))
		return {static_cast<uint32_t>(leading_byte), 1};	// Прямой ASCII-7 код.
	
	// Все иные случаи требуют декодирования. Рассчитаем полную длину UTF-8-кода и количество значащих бит в его стартовом байте.
	// head_one_bits_count - число старших единичных битов в стартовом байте, bits_in_start_byte - количество битов данных в нём же.
	size_t head_one_bits_count = 0, bits_in_start_byte = MAX_BITS_IN_START_BYTE + 2;
	char start_byte_datamask = 0x7f;	// Маска данных ведущего байта, выделяющая его биты, содержащие действительные данные.
	for (char test_leading_byte = leading_byte; test_leading_byte & 0x80;
		 test_leading_byte <<= 1, ++head_one_bits_count, --bits_in_start_byte, start_byte_datamask >>= 1);

	// Правильный стартовый байт многобайтового UTF-8-кода должен иметь не менее 2 и не более MAX_UNICODE_LENGTH старших единичных бит.
	if (head_one_bits_count < 2)
		return {static_cast<uint32_t>(UTF8ErrorCode::UTF8_CONTINUE_WITHOUT_START), 0};
	if (head_one_bits_count > MAX_UNICODE_LENGTH)
		return {static_cast<uint32_t>(UTF8ErrorCode::UTF8_UNICODE_TOO_LONG), 0};
	// Полная длина UTF-8-кода, таким образом, head_one_bits_count байт (включая ведущий), а маска данных стартового байта - leading_byte_mask.
	size_t current_utf8_byte_pos = symb_pos + head_one_bits_count - 1;
	if (current_utf8_byte_pos >= src_utf8_string.size())
		// Преждевременное окончание входной строки.
		return {static_cast<uint32_t>(UTF8ErrorCode::UTF8_STRING_TERMINATED), 0};

	uint32_t symb_unicode = 0;
	size_t symb_unicode_bitpos = 0,	// Текущий номер бита, который будет заполняться следующей порцией данных UTF-8-кода.
		   real_symb_length = 0;	// Реальная длина считанного Юникода в битах (то есть номер его самого старшего единичного бита + 1).
	while (true)
	{
		char utf8_code_char = src_utf8_string[current_utf8_byte_pos];
		uint32_t next_data_portion;
		size_t data_portion_length;
		if (current_utf8_byte_pos > symb_pos)
		{ // Это один из "байтов продолжения".
			next_data_portion = utf8_code_char & CONTINUE_BYTE_DATAMASK;
			data_portion_length = BITS_PER_CONTINUE_BYTE;
		}
		else
		{ // Это стартовый байт UTF-8-кода.
			next_data_portion = utf8_code_char & start_byte_datamask;
			data_portion_length = bits_in_start_byte;
		}
		// Подсчитаем реальную длину текущего значения извлекаемого Юникода.
		if (next_data_portion)
		{
			size_t real_data_portion_length = 0;
			for (uint32_t test_data_portion = next_data_portion; test_data_portion; ++real_data_portion_length, test_data_portion >>= 1);
			if (real_data_portion_length)
				real_symb_length = real_data_portion_length + symb_unicode_bitpos;
		}
		// Заполняем данными next_data_portion длиной data_portion_length битов битовый фрагмент переменной symb_unicode,
		// начиная с бита symb_unicode_bitpos.
		if (symb_unicode_bitpos < sizeof(next_data_portion) * 8)
			next_data_portion <<= symb_unicode_bitpos;
		else
			next_data_portion = 0;
		symb_unicode |= next_data_portion;
		symb_unicode_bitpos += data_portion_length;

		if (current_utf8_byte_pos == 0 || current_utf8_byte_pos <= symb_pos)
			break;
		--current_utf8_byte_pos;
	}
	if (real_symb_length > sizeof(symb_unicode) * 8)
		// Значение Юникода, хранящееся в src_utf8_string, не помещается в выходную переменную symb_unicode.
		return {static_cast<uint32_t>(UTF8ErrorCode::UTF8_UNICODE_TOO_LONG), 0};

	return {symb_unicode, head_one_bits_count};
}

// Перекодирование из однобайтовой кодировки в UTF-8 с попутным составлением карты размещения UTF-8-кодов в пределах сгенерированной UTF-8-строки.
std::tuple<std::string, UTF8Map, UTF8Error> TranscodeToUTF8Ex(const std::string& unibyte_source_str, const std::vector<uint32_t>& to_utf8)
{
	if (to_utf8.size() < 256)
		// Некорректная таблица перекодировки. В ней должны быть определены Юникод-коды для всех возможных однобайтовых символов.
		return {{}, {},  {.code = UTF8ErrorCode::UTF8_TO_UTF_TABLE_TOO_SHORT}};

	std::string result_str;
	UTF8Map transcode_pos_map;
	for (size_t unibyte_symb_pos = 0; unibyte_symb_pos < unibyte_source_str.size(); ++unibyte_symb_pos)
	{
		std::string next_utf8_symbol = ConvSymbToUTF8(to_utf8[static_cast<size_t>(unibyte_source_str[unibyte_symb_pos])]);
		if (next_utf8_symbol.size() > MAX_UNICODE_LENGTH)
			// Длина UTF-8 представления для исходного символа в позиции unibyte_symb_pos выше максимально допустимой.
			return {std::move(result_str), std::move(transcode_pos_map), {.code = UTF8ErrorCode::UTF8_UNICODE_TOO_LONG, .pos = unibyte_symb_pos}};

		transcode_pos_map.begin_map.push_back(result_str.size());
		transcode_pos_map.last_symbol_size = next_utf8_symbol.size();
		result_str += std::move(next_utf8_symbol);
	}

	return {std::move(result_str), std::move(transcode_pos_map), {}};
}

// Перекодирование из однобайтовой кодировки в UTF-8.
TranscodeResult TranscodeToUTF8(const std::string& unibyte_source_str, const std::vector<uint32_t>& to_utf8)
{
	std::tuple<std::string, UTF8Map, UTF8Error> transcode_ex_result = TranscodeToUTF8Ex(unibyte_source_str, to_utf8);
	return {std::move(std::get<0>(transcode_ex_result)), std::get<2>(transcode_ex_result)};
}

// Перекодирование из UTF-8 в однобайтовую кодировку. Возвращает сформированную строку, а также информацию о возможной ошибке декодирования
// в виде пары, первый член которой - код ошибки, а второй - позиция её возникновения.
TranscodeResult TranscodeFromUTF8(const std::string& utf8_source_str, const std::vector<uint32_t>& to_utf8)
{
	UTF8Error result_error;
	std::string result_str;
	size_t source_str_pos = 0;
	while (source_str_pos < utf8_source_str.size())
	{
		std::pair<uint32_t, size_t> utf8_symb_result = ConvSymbFromUTF8(utf8_source_str, source_str_pos);
		if (utf8_symb_result.second == 0)
			// Декодирование очередного символа не удалось и завершилось ошибкой.
			return {std::move(result_str), {static_cast<UTF8ErrorCode>(utf8_symb_result.first), source_str_pos}};
		
		if (auto to_utf8_it = std::find(to_utf8.begin(), to_utf8.end(), utf8_symb_result.first); to_utf8_it != to_utf8.end())
		{
			result_str += static_cast<char>(to_utf8_it - to_utf8.begin());
		}
		else
		{ // Встретился Юникодный код, не определённый в данной однобайтовой кодировке.
		  // Будем заменять такие символы на нулевые, а также сообщим о проблеме через код ошибки.
			result_error = {UTF8ErrorCode::UTF8_SYMBOL_NOT_DEFINED, source_str_pos};
			result_str += '\0';
		}

		source_str_pos += utf8_symb_result.second;
	}
	return {std::move(result_str), result_error};
}

// Транскодирование между двумя однобайтовыми кодировками. Операция выполняется через промежуточное преобразование исходной
// строки в UTF-8 представление.
TranscodeResult TranscodeBetweenUnibytes
	(const std::string& unibyte_source_str, const std::vector<uint32_t>& src_to_utf8, const std::vector<uint32_t>& dest_to_utf8)
{
	std::tuple<std::string, UTF8Map, UTF8Error> to_utf8_result = TranscodeToUTF8Ex(unibyte_source_str, src_to_utf8);
	UTF8Error to_utf8_error = std::get<2>(to_utf8_result);
	if (to_utf8_error.code != UTF8ErrorCode::UTF8_NO_ERROR)
		return {{}, to_utf8_error};

	TranscodeResult dest_cnv_result = TranscodeFromUTF8(std::get<0>(to_utf8_result), dest_to_utf8);
	if (dest_cnv_result.second.code == UTF8ErrorCode::UTF8_NO_ERROR || dest_cnv_result.second.pos == std::string::npos)
		return dest_cnv_result;
	// Обратное перекодирование из UTF-8 представления завершилось ошибкой, и поле pos описания ошибки содержит значащую величину.
	// Так как dest_cnv_result.second.pos в данном случае указывает на сбойное положение в промежуточном UTF-8 представлении, то
	// теперь нужно вычислить соответствующее ему положение в исходной строке unibyte_source_str.
	size_t original_pos = 0;
	const UTF8Map& orig_symb_pos_map = std::get<1>(to_utf8_result);
	for (size_t utf8_code_pos_index = 0; utf8_code_pos_index < orig_symb_pos_map.begin_map.size();
		 ++utf8_code_pos_index, ++original_pos)
	{
		// Получим начальные положения Юникодов проверяемого исходного символа (символа с индексом original_pos в исходнике
		// unibyte_source_str) в промежуточной UTF-8-строке, а также исходного символа, следующего за ним (то есть с индексом
		// original_pos + 1 там же).
		size_t utf8_code_pos_current = orig_symb_pos_map.begin_map[utf8_code_pos_index];
		size_t utf8_code_pos_next = utf8_code_pos_index == orig_symb_pos_map.begin_map.size() - 1 ?
			utf8_code_pos_current + orig_symb_pos_map.last_symbol_size : // Текущий проверяемый символ последний в промежуточной UTF-8-строке.
			orig_symb_pos_map.begin_map[utf8_code_pos_index + 1];		 // Текущий символ original_pos не является последним в ней.

		if (dest_cnv_result.second.pos >= utf8_code_pos_current && dest_cnv_result.second.pos < utf8_code_pos_next)
			break;
	}
	return {std::move(dest_cnv_result.first), {.code = dest_cnv_result.second.code, .pos = original_pos}};
}

// Генерация карты размещения многобайтовых UTF-8-кодов в пределах однобайтовой строки (потока байтов) parse_str.
std::pair<UTF8Map, uint32_t> BuildUTF8Map(const std::string& parse_str, size_t max_elem_count)
{
	uint32_t last_unicode = 0;
	UTF8Map result_map;
	size_t symb_pos = 0;
	while (symb_pos < parse_str.size())
	{
		result_map.begin_map.push_back(symb_pos);
		std::pair<uint32_t, size_t> conv_result = ConvSymbFromUTF8(parse_str, symb_pos);
		last_unicode = conv_result.first;
		result_map.last_symbol_size = conv_result.second;

		if (!conv_result.second || result_map.begin_map.size() >= max_elem_count)
			break;  // Ошибка при выделении очередного UTF-8-кода или нужный символ при построении ограниченной карты достигнут.

		symb_pos += conv_result.second;
	}

	return {std::move(result_map), last_unicode};
}
