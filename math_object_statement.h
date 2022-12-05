#pragma once

class NewMath : public Statement
{
public:
    NewMath(std::vector<std::unique_ptr<Statement>> args);
    // ���������� ������, ���������� �������� ���� MathInstance,
    // �������������� ����� ��������� ��������� �������������� ���������.
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

std::unique_ptr<Statement> CreateMath(std::vector<std::unique_ptr<Statement>> args);
