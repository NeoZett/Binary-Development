#pragma once

#include <bin/binvm.hpp>
#include <todo_program/enums.hpp>
#include <todo_program/binary_types.hpp>
#include <todo_program/utilities.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

static void append(
    binvm::Program& program,
    binvm::Instruction instruction)
{
    program.add_instruction(instruction);
}

static binvm::Instruction read_todos(
    binvm::VarId storage_variable)
{
    binvm::Instruction instruction;
    instruction.opcode = opcode(AppOpcode::ReadTodos);
    instruction.var_id = storage_variable;
    return instruction;
}

static binvm::Instruction set_todo_info(
    binvm::VarId todo_id,
    TodoField field,
    const binvm::Operand& value)
{
    binvm::Instruction instruction;
    instruction.opcode = opcode(AppOpcode::SetTodoInfo);
    instruction.var_id = todo_id;
    instruction.operand1 = binvm::BinBytes::pack<TodoField>(field);
    instruction.operand2 = value.pack();
    return instruction;
}

static binvm::Instruction add_todo(
    binvm::VarId storage_variable,
    binvm::VarId todo_id)
{
    binvm::Instruction instruction;
    instruction.opcode = opcode(AppOpcode::AddTodo);
    instruction.var_id = storage_variable;
    instruction.operand1 = binvm::BinBytes::pack<binvm::VarId>(todo_id);
    return instruction;
}

static binvm::Instruction remove_todo(
    binvm::VarId storage_variable,
    const binvm::Operand& index)
{
    binvm::Instruction instruction;
    instruction.opcode = opcode(AppOpcode::RemoveTodo);
    instruction.var_id = storage_variable;
    instruction.operand1 = index.pack();
    return instruction;
}

static binvm::Instruction todo_list_size(
    binvm::VarId storage_variable,
    binvm::VarId output)
{
    binvm::Instruction instruction;
    instruction.opcode = opcode(AppOpcode::TodoListSize);
    instruction.var_id = storage_variable;
    instruction.operand1 = binvm::BinBytes::pack<binvm::VarId>(output);
    return instruction;
}

static binvm::Instruction extract_todo(
    binvm::VarId storage_variable,
    binvm::VarId todo_output,
    const binvm::Operand& index)
{
    binvm::Instruction instruction;
    instruction.opcode = opcode(AppOpcode::ExtractTodo);
    instruction.var_id = storage_variable;
    instruction.operand1 = binvm::BinBytes::pack<binvm::VarId>(todo_output);
    instruction.operand2 = index.pack();
    return instruction;
}

static binvm::Instruction get_todo_info(
    binvm::VarId todo_id,
    TodoField field,
    binvm::VarId output)
{
    binvm::Instruction instruction;
    instruction.opcode = opcode(AppOpcode::GetTodoInfo);
    instruction.var_id = todo_id;
    instruction.operand1 = binvm::BinBytes::pack<TodoField>(field);
    instruction.operand2 = binvm::BinBytes::pack<binvm::VarId>(output);
    return instruction;
}

static binvm::Instruction parse_command(
    binvm::VarId input,
    binvm::VarId command_output,
    binvm::VarId argument_output)
{
    binvm::Instruction instruction;
    instruction.opcode = opcode(AppOpcode::ParseCommand);
    instruction.var_id = input;
    instruction.operand1 = binvm::BinBytes::pack<binvm::VarId>(command_output);
    instruction.operand2 = binvm::BinBytes::pack<binvm::VarId>(argument_output);
    return instruction;
}

static binvm::Instruction print_todo_list(
    binvm::VarId storage_variable)
{
    binvm::Instruction instruction;
    instruction.opcode = opcode(AppOpcode::PrintTodoList);
    instruction.var_id = storage_variable;
    return instruction;
}

static binvm::Instruction validate_delete(
    binvm::VarId storage_variable,
    const binvm::Operand& argument,
    binvm::VarId output)
{
    binvm::Instruction instruction;
    instruction.opcode = opcode(AppOpcode::ValidateDelete);
    instruction.var_id = storage_variable;
    instruction.operand1 = argument.pack();
    instruction.operand2 = binvm::BinBytes::pack<binvm::VarId>(output);
    return instruction;
}

static void set_result(
    binvm::VirtualMachine& vm,
    DispatchResult result)
{
    vm.set_variable(vars::Result, binvm::BinBytes::pack<DispatchResult>(result));
}

static void register_todo_handlers(binvm::VirtualMachine& vm)
{
    const std::string savefile = "savefile.dat";

    vm.dispatcher().register_handler(
        opcode(AppOpcode::ReadTodos),
        [savefile](binvm::VirtualMachine& vm, const binvm::Instruction& instruction)
        {
            std::vector<Todo> records;

            if (std::filesystem::exists(savefile))
            {
                try
                {
                    bin::BinaryStream stream(savefile);
                    stream.read({ Todo::bin_schema });
                    for (bin::Record& record : stream.records())
                        records.push_back(Todo::from_record(record));
                }
                catch (const std::exception& e)
                {
                    std::cout << "Warning: Couldn't read file: " << e.what() << '\n';
                }
                catch (...)
                {
                    std::cout << "Warning: Couldn't read file due to an unknown error.\n";
                }
            }
            else
            {
                std::cout << "Warning: Save file does not exist yet.\n";
            }

            vm.set_variable(instruction.var_id, binvm::BinBytes::pack(records));
            set_result(vm, DispatchResult::Completed);
        }
    );

    vm.dispatcher().register_handler(
        opcode(AppOpcode::GetTimestamp),
        [](
            binvm::VirtualMachine& vm,
            const binvm::Instruction& instruction)
        {
            const std::string timestamp = current_timestamp();

            vm.set_variable(
                instruction.var_id,
                binvm::BinBytes::pack<binvm::BinString>(
                    binvm::BinString(timestamp.c_str())
                )
            );
        }
    );

    vm.dispatcher().register_handler(
        opcode(AppOpcode::SetTodoInfo),
        [](
            binvm::VirtualMachine& vm,
            const binvm::Instruction& instruction)
        {
            Todo todo;
            vm.variable(instruction.var_id).unpack(todo);
            TodoField field;
            instruction.operand1.unpack(field);
            const std::string value = vm.evaluate_operand_string(instruction.operand2);

            switch (field)
            {
            case TodoField::Name:
                todo.name = bin::FixedString<>(value.c_str());
                break;
            case TodoField::Description:
                todo.description = bin::FixedString<>(value.c_str());
                break;
            case TodoField::When:
                todo.when = bin::FixedString<>(value.c_str());
                break;
            default:
                throw std::runtime_error("Invalid TodoField.");
            }

            vm.set_variable(
                instruction.var_id,
                binvm::BinBytes::pack<Todo>(todo)
            );
        }
    );

    vm.dispatcher().register_handler(
        opcode(AppOpcode::AddTodo),
        [savefile](
            binvm::VirtualMachine& vm,
            const binvm::Instruction& instruction)
        {
            std::vector<Todo> storage;
            vm.variable(instruction.var_id).unpack(storage);

            binvm::VarId todo_id;
            instruction.operand1.unpack(todo_id);
            Todo todo;
            vm.variable(todo_id).unpack(todo);

            if (todo.when.c_str()[0] == '\0')
            {
                todo.when = bin::FixedString<>(current_timestamp().c_str());
            }

            storage.push_back(todo);

            bin::BinaryStream stream(savefile);
            for (const Todo& todo : storage)
                stream.push_back(todo.to_record());
            stream.write();

            vm.set_variable(
                instruction.var_id,
                binvm::BinBytes::pack(storage));
            set_result(vm, DispatchResult::Completed);
        }
    );

    vm.dispatcher().register_handler(
        opcode(AppOpcode::RemoveTodo),
        [savefile](
            binvm::VirtualMachine& vm,
            const binvm::Instruction& instruction)
        {
            std::vector<Todo> storage;
            vm.variable(instruction.var_id).unpack(storage);

            const long index = vm.evaluate_operand_long(instruction.operand1);
            const std::size_t count = storage.size();

            if (index < 1 || static_cast<std::size_t>(index) > count)
            {
                set_result(vm, DispatchResult::NotFound);
                return;
            }

            storage.erase(storage.begin() + (index - 1));

            if (!storage.empty())
            {
                bin::BinaryStream stream(savefile);
                for (const Todo& todo : storage)
                    stream.push_back(todo.to_record());
                stream.write();

                vm.set_variable(
                    instruction.var_id,
                    binvm::BinBytes::pack(storage));
            }

            set_result(vm, DispatchResult::Completed);
        }
    );

    vm.dispatcher().register_handler(
        opcode(AppOpcode::TodoListSize),
        [](
            binvm::VirtualMachine& vm,
            const binvm::Instruction& instruction)
        {
            std::vector<Todo> storage;
            vm.variable(instruction.var_id).unpack(storage);

            binvm::VarId output;
            instruction.operand1.unpack(output);
            const std::int32_t size = static_cast<std::int32_t>(storage.size());
            const std::string size_str = std::to_string(size);

            vm.set_variable(
                output,
                binvm::BinBytes::pack<binvm::BinString>(
                    binvm::BinString(size_str.c_str())
                )
            );
        }
    );

    vm.dispatcher().register_handler(
        opcode(AppOpcode::ExtractTodo),
        [](
            binvm::VirtualMachine& vm,
            const binvm::Instruction& instruction)
        {
            std::vector<Todo> storage;
            vm.variable(instruction.var_id).unpack(storage);

            binvm::VarId output;
            instruction.operand1.unpack(output);
            const long index = vm.evaluate_operand_long(instruction.operand2);

            if (index < 1 || static_cast<std::size_t>(index) > storage.size())
            {
                set_result(vm, DispatchResult::NotFound);
                return;
            }

            const Todo todo = storage.at(index - 1);

            vm.set_variable(
                output,
                binvm::BinBytes::pack<Todo>(todo)
            );

            set_result(vm, DispatchResult::Completed);
        }
    );

    vm.dispatcher().register_handler(
        opcode(AppOpcode::GetTodoInfo),
        [](
            binvm::VirtualMachine& vm,
            const binvm::Instruction& instruction)
        {
            Todo todo;
            vm.variable(instruction.var_id).unpack(todo);
            TodoField field;
            instruction.operand1.unpack(field);
            binvm::VarId output;
            instruction.operand2.unpack(output);

            binvm::BinString value;

            switch (field)
            {
            case TodoField::Name:
                value = todo.name.c_str();
                break;
            case TodoField::Description:
                value = todo.description.c_str();
                break;
            case TodoField::When:
                value = todo.when.c_str();
                break;
            default:
                throw std::runtime_error("Invalid TodoField.");
            }

            vm.set_variable(
                output,
                binvm::BinBytes::pack<binvm::BinString>(value)
            );
        }
    );

    vm.dispatcher().register_handler(
        opcode(AppOpcode::ParseCommand),
        [](
            binvm::VirtualMachine& vm,
            const binvm::Instruction& instruction)
        {
            binvm::BinString input_string;
            vm.variable(instruction.var_id).unpack(input_string);
            std::string input = input_string.c_str();

            binvm::VarId command_output;
            instruction.operand1.unpack(command_output);
            binvm::VarId argument_output;
            instruction.operand2.unpack(argument_output);

            std::istringstream stream(input);
            std::string command;
            std::string argument;

            stream >> command;
            stream >> argument;

            vm.set_variable(
                command_output,
                binvm::BinBytes::pack<binvm::BinString>(
                    binvm::BinString(command.c_str())
                )
            );

            vm.set_variable(
                argument_output,
                binvm::BinBytes::pack<binvm::BinString>(
                    binvm::BinString(argument.c_str())
                )
            );
        }
    );

    vm.dispatcher().register_handler(
        opcode(AppOpcode::PrintTodoList),
        [](
            binvm::VirtualMachine& vm,
            const binvm::Instruction& instruction)
        {
            std::vector<Todo> storage;
            vm.variable(instruction.var_id).unpack(storage);

            const std::size_t count = storage.size();

            for (std::size_t i = 0; i < count; ++i)
            {
                const Todo todo = storage.at(i);

                vm.output_stream()
                    << "[" << (i + 1) << "] "
                    << todo.name.c_str()
                    << " (" << todo.when.c_str() << ")\n";

                if (std::strlen(todo.description.c_str()) > 0)
                {
                    vm.output_stream()
                        << "    Details: "
                        << todo.description.c_str()
                        << "\n";
                }
            }
        }
    );

    vm.dispatcher().register_handler(
        opcode(AppOpcode::ValidateDelete),
        [](
            binvm::VirtualMachine& vm,
            const binvm::Instruction& instruction)
        {
            std::vector<Todo> storage;
            vm.variable(instruction.var_id).unpack(storage);

            const std::string argument =
                vm.evaluate_operand_string(instruction.operand1);

            binvm::VarId output;
            instruction.operand2.unpack(output);

            long index = 0;

            try
            {
                std::size_t consumed = 0;
                index = std::stol(argument, &consumed);

                if (consumed != argument.length())
                {
                    index = 0;
                }
            }
            catch (...)
            {
                index = 0;
            }

            if (index < 1 || static_cast<std::size_t>(index) > storage.size())
            {
                vm.set_variable(
                    output,
                    binvm::BinBytes::pack<binvm::BinString>(
                        binvm::BinString("0")
                    )
                );

                vm.output_stream() << "Invalid task ID.\n";
                return;
            }

            const std::string index_str = std::to_string(index);

            vm.set_variable(
                output,
                binvm::BinBytes::pack<binvm::BinString>(
                    binvm::BinString(index_str.c_str())
                )
            );
        }
    );
}