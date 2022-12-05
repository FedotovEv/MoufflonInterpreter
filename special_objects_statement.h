#pragma once

class NewArray : public Statement
{
public:
    NewArray(std::vector<std::unique_ptr<Statement>> args);
    // ¬озвращает объект, содержащий значение типа ArrayInstance,
    // представл€ющее собой созданный экземпл€р массива.
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
private:

    std::vector<std::unique_ptr<Statement>> args_;
};

class NewMap : public Statement
{
public:
    NewMap(std::vector<std::unique_ptr<Statement>> args);
    // ¬озвращает объект, содержащий значение типа MapInstance,
    // представл€ющее собой созданный экземпл€р ассоциативного массива (словар€).
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

std::unique_ptr<Statement> CreateArray(std::vector<std::unique_ptr<Statement>> args);
std::unique_ptr<Statement> CreateMap(std::vector<std::unique_ptr<Statement>> args);
