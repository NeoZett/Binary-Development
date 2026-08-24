#pragma once

#include <bin/binvm.hpp>

enum class AppOpcode : bin_id_t
{
    ReadTodos = 100,
    SetTodoInfo = 101,
    AddTodo = 102,
    RemoveTodo = 103,
    TodoListSize = 104,
    ExtractTodo = 105,
    GetTodoInfo = 106,
    GetTimestamp = 107,
    ParseCommand = 108,
    PrintTodoList = 109,
    ValidateDelete = 110
};

enum class TodoField : bin_id_t
{
    Name = 1,
    Description = 2,
    When = 3
};

enum class DispatchResult : bin_id_t
{
    Completed = 1,
    NotFound = 2
};

constexpr binvm::Opcode opcode(AppOpcode value)
{
    return static_cast<binvm::Opcode>(static_cast<bin_id_t>(value));
}

namespace vars
{
    constexpr binvm::VarId TodoStorage = 1;

    constexpr binvm::VarId Todo = 2;
    constexpr binvm::VarId Result = 3;
    constexpr binvm::VarId Size = 4;
    constexpr binvm::VarId ExtractedTodo = 5;
    constexpr binvm::VarId Info = 6;

    constexpr binvm::VarId Command = 10;
    constexpr binvm::VarId CommandArgument = 11;

    constexpr binvm::VarId IsList = 20;
    constexpr binvm::VarId IsAdd = 21;
    constexpr binvm::VarId IsDelete = 22;
    constexpr binvm::VarId IsExit = 23;

    constexpr binvm::VarId DeleteIndex = 30;

    constexpr binvm::VarId NameInput = 40;
    constexpr binvm::VarId DescriptionInput = 41;
}