#pragma once

class NewMath : public Statement
{
public:
    NewMath(std::vector<std::unique_ptr<Statement>> args);
    // Возвращает объект, содержащий значение типа MathInstance,
    // представляющее собой созданный экземпляр математической коллекции.
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

std::unique_ptr<Statement> CreateMath(std::vector<std::unique_ptr<Statement>> args);
