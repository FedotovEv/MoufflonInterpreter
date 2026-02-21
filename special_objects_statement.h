#pragma once

// Далее описаны классы-фабрики объектов-экземпляров различных специальных внутренних классов интерпретатора.
// По сути они являются аналогами фабричного класса NewInstance, который служит фабрикой объектов классов общего вида
// (программно определенных). При своём исполнении (вызове функции-члена Execute()) они, так же как и NewInstance::Execute(),
// возвращают новый экземпляр соответствующего класса, изготовлением которого они занимаются.

class NewArray : public Statement
{
public:
    NewArray(std::vector<std::unique_ptr<Statement>> args);
    // Возвращает объект, содержащий значение типа ArrayInstance,
    // представляющее собой созданный экземпляр массива.
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

private:
    std::vector<std::unique_ptr<Statement>> args_;
};

class NewMap : public Statement
{
public:
    NewMap(std::vector<std::unique_ptr<Statement>> args);
    // Возвращает объект, содержащий значение типа MapInstance,
    // представляющее собой созданный экземпляр ассоциативного массива (словаря).
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

std::unique_ptr<Statement> CreateArray(std::vector<std::unique_ptr<Statement>> args);
std::unique_ptr<Statement> CreateMap(std::vector<std::unique_ptr<Statement>> args);
