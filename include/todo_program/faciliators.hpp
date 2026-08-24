#pragma once

#include <todo_program/dispatcher.hpp>

static binvm::Program build_todo_program()
{
    binvm::Program program;

    constexpr binvm::LabelId LOOP = 100;
    constexpr binvm::LabelId LIST = 110;
    constexpr binvm::LabelId LIST_EMPTY = 111;
    constexpr binvm::LabelId LIST_DONE = 112;
    constexpr binvm::LabelId ADD = 120;
    constexpr binvm::LabelId ADD_DONE = 121;
    constexpr binvm::LabelId DELETE = 130;
    constexpr binvm::LabelId DELETE_INVALID = 131;
    constexpr binvm::LabelId DELETE_DONE = 132;
    constexpr binvm::LabelId EXIT = 133;
    constexpr binvm::LabelId UNKNOWN = 140;

    program
        .print(binvm::Operand("=== BSER Todo CLI ===\n"))
        .print(binvm::Operand("Commands: list | add | delete <id> | exit\n\n"));

    append(program, read_todos(vars::TodoStorage));

    program
        .label(LOOP)
        .print(binvm::Operand("todo > "))
        .input(vars::Command);

    append(program, parse_command(vars::Command, vars::Command, vars::CommandArgument));

    program.equal(vars::IsList, binvm::Operand(vars::Command), binvm::Operand("list"));
    program.jump_if_true(vars::IsList, LIST);

    program.equal(vars::IsAdd, binvm::Operand(vars::Command), binvm::Operand("add"));
    program.jump_if_true(vars::IsAdd, ADD);

    program.equal(vars::IsDelete, binvm::Operand(vars::Command), binvm::Operand("delete"));
    program.jump_if_true(vars::IsDelete, DELETE);

    program.equal(vars::IsExit, binvm::Operand(vars::Command), binvm::Operand("exit"));
    program.jump_if_true(vars::IsExit, EXIT);

    program.jump(UNKNOWN);

    program.label(LIST);
    append(program, todo_list_size(vars::TodoStorage, vars::Size));
    program.equal(vars::IsList, binvm::Operand(vars::Size), binvm::Operand("0"));
    program.jump_if_true(vars::IsList, LIST_EMPTY);
    append(program, print_todo_list(vars::TodoStorage));
    program.jump(LIST_DONE);

    program.label(LIST_EMPTY);
    program.print(binvm::Operand("No tasks found.\n"));

    program.label(LIST_DONE);
    program.jump(LOOP);

    program.label(ADD);
    program.set(vars::Todo, binvm::BinBytes::pack<Todo>(Todo{}));
    program.print(binvm::Operand("  Title: "));
    program.input(vars::NameInput);
    append(program, set_todo_info(vars::Todo, TodoField::Name, binvm::Operand(vars::NameInput)));

    program.print(binvm::Operand("  Description: "));
    program.input(vars::DescriptionInput);
    append(program, set_todo_info(vars::Todo, TodoField::Description, binvm::Operand(vars::DescriptionInput)));

    append(program, add_todo(vars::TodoStorage, vars::Todo));
    program.print(binvm::Operand("Saved successfully!\n"));

    program.label(ADD_DONE);
    program.jump(LOOP);

    program.label(DELETE);
    append(program, validate_delete(vars::TodoStorage, binvm::Operand(vars::CommandArgument), vars::DeleteIndex));
    program.equal(vars::IsDelete, binvm::Operand(vars::DeleteIndex), binvm::Operand("0"));
    program.jump_if_true(vars::IsDelete, DELETE_INVALID);

    append(program, remove_todo(vars::TodoStorage, binvm::Operand(vars::DeleteIndex)));
    program.print(binvm::Operand("Task #"));
    program.print_variable(vars::CommandArgument);
    program.print(binvm::Operand(" removed.\n"));
    program.jump(DELETE_DONE);

    program.label(DELETE_INVALID);
    program.label(DELETE_DONE);
    program.jump(LOOP);

    program.label(UNKNOWN);
    program.print(binvm::Operand("Unknown command. Options: list, add, delete <id>, exit\n"));
    program.jump(LOOP);

    program.label(EXIT);
    program.halt();

    return program;
}

static binvm::Program build_todo_program_compact()
{
    binvm::Program program;

    constexpr binvm::LabelId LOOP = 100, LIST = 110, LIST_EMPTY = 111, LIST_DONE = 112;
    constexpr binvm::LabelId ADD = 120, DELETE = 130, DELETE_INVALID = 131, DELETE_DONE = 132;
    constexpr binvm::LabelId EXIT = 133, UNKNOWN = 140;

    program.print(binvm::Operand("Use: list | add | delete <id> | exit\n"));
    append(program, read_todos(vars::TodoStorage));

    program.label(LOOP)
        .print(binvm::Operand("\n> "))
        .input(vars::Command);

    append(program, parse_command(vars::Command, vars::Command, vars::CommandArgument));

    program.equal(vars::IsList, binvm::Operand(vars::Command), binvm::Operand("list"));
    program.jump_if_true(vars::IsList, LIST);

    program.equal(vars::IsAdd, binvm::Operand(vars::Command), binvm::Operand("add"));
    program.jump_if_true(vars::IsAdd, ADD);

    program.equal(vars::IsDelete, binvm::Operand(vars::Command), binvm::Operand("delete"));
    program.jump_if_true(vars::IsDelete, DELETE);

    program.equal(vars::IsExit, binvm::Operand(vars::Command), binvm::Operand("exit"));
    program.jump_if_true(vars::IsExit, EXIT);

    program.jump(UNKNOWN);

    program.label(LIST);
    append(program, todo_list_size(vars::TodoStorage, vars::Size));
    program.equal(vars::IsList, binvm::Operand(vars::Size), binvm::Operand("0"));
    program.jump_if_true(vars::IsList, LIST_EMPTY);
    append(program, print_todo_list(vars::TodoStorage));
    program.jump(LIST_DONE);

    program.label(LIST_EMPTY);
    program.print(binvm::Operand("[empty]\n"));
    program.label(LIST_DONE);
    program.jump(LOOP);

    program.label(ADD);
    program.set(vars::Todo, binvm::BinBytes::pack<Todo>(Todo{}));
    program.print(binvm::Operand("Title: "));
    program.input(vars::NameInput);
    append(program, set_todo_info(vars::Todo, TodoField::Name, binvm::Operand(vars::NameInput)));

    program.print(binvm::Operand("Details: "));
    program.input(vars::DescriptionInput);
    append(program, set_todo_info(vars::Todo, TodoField::Description, binvm::Operand(vars::DescriptionInput)));

    append(program, add_todo(vars::TodoStorage, vars::Todo));
    program.print(binvm::Operand("+ Added.\n"));
    program.jump(LOOP);

    program.label(DELETE);
    append(program, validate_delete(vars::TodoStorage, binvm::Operand(vars::CommandArgument), vars::DeleteIndex));
    program.equal(vars::IsDelete, binvm::Operand(vars::DeleteIndex), binvm::Operand("0"));
    program.jump_if_true(vars::IsDelete, DELETE_INVALID);

    append(program, remove_todo(vars::TodoStorage, binvm::Operand(vars::DeleteIndex)));
    program.print(binvm::Operand("- Removed task "));
    program.print_variable(vars::CommandArgument);
    program.print(binvm::Operand("\n"));
    program.jump(DELETE_DONE);

    program.label(DELETE_INVALID);
    program.label(DELETE_DONE);
    program.jump(LOOP);

    program.label(UNKNOWN);
    program.print(binvm::Operand("? Invalid command.\n"));
    program.jump(LOOP);

    program.label(EXIT);
    program.halt();

    return program;
}

static binvm::Program build_todo_program_wizard()
{
    binvm::Program program;

    constexpr binvm::LabelId LOOP = 100, LIST = 110, LIST_EMPTY = 111, LIST_DONE = 112;
    constexpr binvm::LabelId ADD = 120, DELETE = 130, DELETE_INVALID = 131, DELETE_DONE = 132;
    constexpr binvm::LabelId EXIT = 133, UNKNOWN = 140;

    program.print(binvm::Operand("Welcome to your Personal Task Assistant!\n"))
        .print(binvm::Operand("Type 'list' to view tasks, 'add' to make a new one, 'delete <id>' to remove, or 'exit'.\n\n"));
    append(program, read_todos(vars::TodoStorage));

    program.label(LOOP)
        .print(binvm::Operand("What would you like to do next? "))
        .input(vars::Command);

    append(program, parse_command(vars::Command, vars::Command, vars::CommandArgument));

    program.equal(vars::IsList, binvm::Operand(vars::Command), binvm::Operand("list"));
    program.jump_if_true(vars::IsList, LIST);

    program.equal(vars::IsAdd, binvm::Operand(vars::Command), binvm::Operand("add"));
    program.jump_if_true(vars::IsAdd, ADD);

    program.equal(vars::IsDelete, binvm::Operand(vars::Command), binvm::Operand("delete"));
    program.jump_if_true(vars::IsDelete, DELETE);

    program.equal(vars::IsExit, binvm::Operand(vars::Command), binvm::Operand("exit"));
    program.jump_if_true(vars::IsExit, EXIT);

    program.jump(UNKNOWN);

    program.label(LIST);
    program.print(binvm::Operand("\n--- Current To-Do List ---\n"));
    append(program, todo_list_size(vars::TodoStorage, vars::Size));
    program.equal(vars::IsList, binvm::Operand(vars::Size), binvm::Operand("0"));
    program.jump_if_true(vars::IsList, LIST_EMPTY);
    append(program, print_todo_list(vars::TodoStorage));
    program.jump(LIST_DONE);

    program.label(LIST_EMPTY);
    program.print(binvm::Operand("You don't have any tasks scheduled right now!\n"));
    program.label(LIST_DONE);
    program.print(binvm::Operand("--------------------------\n\n"));
    program.jump(LOOP);

    program.label(ADD);
    program.print(binvm::Operand("\n[ Creating a New Task ]\n"));
    program.set(vars::Todo, binvm::BinBytes::pack<Todo>(Todo{}));

    program.print(binvm::Operand("Step 1: Enter task title: "));
    program.input(vars::NameInput);
    append(program, set_todo_info(vars::Todo, TodoField::Name, binvm::Operand(vars::NameInput)));

    program.print(binvm::Operand("Step 2: Enter description (optional): "));
    program.input(vars::DescriptionInput);
    append(program, set_todo_info(vars::Todo, TodoField::Description, binvm::Operand(vars::DescriptionInput)));

    append(program, add_todo(vars::TodoStorage, vars::Todo));
    program.print(binvm::Operand("Done! Task saved successfully.\n\n"));
    program.jump(LOOP);

    program.label(DELETE);
    append(program, validate_delete(vars::TodoStorage, binvm::Operand(vars::CommandArgument), vars::DeleteIndex));
    program.equal(vars::IsDelete, binvm::Operand(vars::DeleteIndex), binvm::Operand("0"));
    program.jump_if_true(vars::IsDelete, DELETE_INVALID);

    append(program, remove_todo(vars::TodoStorage, binvm::Operand(vars::DeleteIndex)));
    program.print(binvm::Operand("Success: Task #"));
    program.print_variable(vars::CommandArgument);
    program.print(binvm::Operand(" was deleted.\n\n"));
    program.jump(DELETE_DONE);

    program.label(DELETE_INVALID);
    program.print(binvm::Operand("\n"));
    program.label(DELETE_DONE);
    program.jump(LOOP);

    program.label(UNKNOWN);
    program.print(binvm::Operand("I didn't understand that command."))
        .print(binvm::Operand("Try 'list', 'add', 'delete <id>', or 'exit'.\n\n"));
    program.jump(LOOP);

    program.label(EXIT);
    program.print(binvm::Operand("Goodbye!\n"));
    program.halt();

    return program;
}

static binvm::Program build_todo_program_shell()
{
    binvm::Program program;

    constexpr binvm::LabelId LOOP = 100, LIST = 110, LIST_EMPTY = 111, LIST_DONE = 112;
    constexpr binvm::LabelId ADD = 120, DELETE = 130, DELETE_INVALID = 131, DELETE_DONE = 132;
    constexpr binvm::LabelId EXIT = 133, UNKNOWN = 140;

    program.print(binvm::Operand("[SYS] BSER Task Workspace v1.0\n"))
        .print(binvm::Operand("[SYS] Commands: list | add | delete <id> | exit\n\n"));
    append(program, read_todos(vars::TodoStorage));

    program.label(LOOP)
        .print(binvm::Operand("[cmd]# "))
        .input(vars::Command);

    append(program, parse_command(vars::Command, vars::Command, vars::CommandArgument));

    program.equal(vars::IsList, binvm::Operand(vars::Command), binvm::Operand("list"));
    program.jump_if_true(vars::IsList, LIST);

    program.equal(vars::IsAdd, binvm::Operand(vars::Command), binvm::Operand("add"));
    program.jump_if_true(vars::IsAdd, ADD);

    program.equal(vars::IsDelete, binvm::Operand(vars::Command), binvm::Operand("delete"));
    program.jump_if_true(vars::IsDelete, DELETE);

    program.equal(vars::IsExit, binvm::Operand(vars::Command), binvm::Operand("exit"));
    program.jump_if_true(vars::IsExit, EXIT);

    program.jump(UNKNOWN);

    program.label(LIST);
    program.print(binvm::Operand("[...] Querying storage...\n"));
    append(program, todo_list_size(vars::TodoStorage, vars::Size));
    program.equal(vars::IsList, binvm::Operand(vars::Size), binvm::Operand("0"));
    program.jump_if_true(vars::IsList, LIST_EMPTY);
    append(program, print_todo_list(vars::TodoStorage));
    program.jump(LIST_DONE);

    program.label(LIST_EMPTY);
    program.print(binvm::Operand("[!] Storage contains 0 items.\n"));
    program.label(LIST_DONE);
    program.jump(LOOP);

    program.label(ADD);
    program.set(vars::Todo, binvm::BinBytes::pack<Todo>(Todo{}));

    program.print(binvm::Operand("  [+] title        : "));
    program.input(vars::NameInput);
    append(program, set_todo_info(vars::Todo, TodoField::Name, binvm::Operand(vars::NameInput)));

    program.print(binvm::Operand("  [+] description : "));
    program.input(vars::DescriptionInput);
    append(program, set_todo_info(vars::Todo, TodoField::Description, binvm::Operand(vars::DescriptionInput)));

    append(program, add_todo(vars::TodoStorage, vars::Todo));
    program.print(binvm::Operand("[OK] Record committed.\n"));
    program.jump(LOOP);

    program.label(DELETE);
    append(program, validate_delete(vars::TodoStorage, binvm::Operand(vars::CommandArgument), vars::DeleteIndex));
    program.equal(vars::IsDelete, binvm::Operand(vars::DeleteIndex), binvm::Operand("0"));
    program.jump_if_true(vars::IsDelete, DELETE_INVALID);

    append(program, remove_todo(vars::TodoStorage, binvm::Operand(vars::DeleteIndex)));
    program.print(binvm::Operand("[-] Record #"));
    program.print_variable(vars::CommandArgument);
    program.print(binvm::Operand(" purged.\n"));
    program.jump(DELETE_DONE);

    program.label(DELETE_INVALID);
    program.label(DELETE_DONE);
    program.jump(LOOP);

    program.label(UNKNOWN);
    program.print(binvm::Operand("[ERR] Unknown command string.\n"));
    program.jump(LOOP);

    program.label(EXIT);
    program.print(binvm::Operand("[SYS] Session closed.\n"));
    program.halt();

    return program;
}