#pragma once

class NewArray : public Statement
{
public:
    NewArray(std::vector<std::unique_ptr<Statement>> args);
    // ���������� ������, ���������� �������� ���� ArrayInstance,
    // �������������� ����� ��������� ��������� �������.
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
private:

    std::vector<std::unique_ptr<Statement>> args_;
};

class NewMap : public Statement
{
public:
    NewMap(std::vector<std::unique_ptr<Statement>> args);
    // ���������� ������, ���������� �������� ���� MapInstance,
    // �������������� ����� ��������� ��������� �������������� ������� (�������).
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

std::unique_ptr<Statement> CreateArray(std::vector<std::unique_ptr<Statement>> args);
std::unique_ptr<Statement> CreateMap(std::vector<std::unique_ptr<Statement>> args);
