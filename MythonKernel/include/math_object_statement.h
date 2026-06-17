#pragma once

class NewMath : public Statement
{
public:
    NewMath(std::vector<std::unique_ptr<Statement>> args);
    // Возвращает объект, содержащий значение типа MathInstance,
    // представляющее собой созданный экземпляр математической коллекции.
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class NewStringOps : public Statement
{
public:
    NewStringOps(std::vector<std::unique_ptr<Statement>> args);
    // Возвращает объект, содержащий значение типа StringOpsInstance,
    // представляющее собой созданный экземпляр класса строковых преобразований.
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

std::unique_ptr<Statement> CreateMath(std::vector<std::unique_ptr<Statement>> args);
std::unique_ptr<Statement> CreateStringOps(std::vector<std::unique_ptr<Statement>> args);
