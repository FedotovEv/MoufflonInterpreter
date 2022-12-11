#pragma once

#include "declares.h"
#include <memory>
#include <stdexcept>

#include "throw_messages.h"

namespace parse
{
    class Lexer;
}

namespace runtime
{
    class Executable;
}

struct ParseError : std::runtime_error
{
    using std::runtime_error::runtime_error;
    ParseError(runtime::ThrowMessageNumber throw_message_number);
};

// Ќа данный момент поддерживаютс€ два способа загрузки внешних двоичных расширений ("втыкал") -
// - из файла стандартной раздел€емой библиотеки (.DLL или .SO, в зависимости от операционной системы),
// а также непосредственно из пам€ти, если весь нужный объектно-процедурный комплекс уже сформирован
// внешней системой, котора€ подключила и использует этот интерпретатор.
// ≈сли LoadLibraryDefine содержит string - втыкало загружаетс€ из раздел€емой библиотеки, им€ которой
// совпадает с этой строкой. Ёто им€ без дальнейшей обработки будет передано службе динамической загрузки
// операционной системы (LoadLibraryW дл€ виндовс или dlopen дл€ линукса).
// ≈сли же LoadLibraryDefine содержит тип InternalObjectCreator - втыкало подключаетс€, использу€
// "фабричный метод", указатель на который и есть значение LoadLibraryDefine.
// ¬ случае, когда LoadLibraryDefine содержит monostate, втыкало не загружаетс€.

using FuncInternalObjectCreator = std::unique_ptr<runtime::Executable>
                        (std::vector<std::unique_ptr<runtime::Executable>>);
using InternalObjectCreator = std::function<FuncInternalObjectCreator>;
using LoadLibraryDefine = std::variant<std::monostate, std::string, InternalObjectCreator>;

class ParseContext
{
public:
    virtual LoadLibraryDefine GetLoadLibraryDesc(const std::string& library_name) const = 0;
};

std::unique_ptr<runtime::Executable> ParseProgram(parse::Lexer& lexer);