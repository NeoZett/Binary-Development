#include <todo_program/faciliators.hpp>

#include <chrono>
#include <iostream>
#include <string>

int main()
{
    try
    {
        using clock = std::chrono::steady_clock;

        std::cout << "=== BSER Todo VM ===\n"
            << "1. Create and serialize program\n"
            << "2. Load serialized program\n\n"
            << "Select an option: ";

        std::string choice;
        std::getline(std::cin, choice);

        bool create_program = false;

        if (choice == "1")
        {
            create_program = true;
        }
        else if (choice == "2")
        {
            create_program = false;
        }
        else
        {
            std::cerr << "Invalid option. Please enter 1 or 2.\n";
            return 1;
        }

        binvm::Program program;

        if (create_program)
        {
            std::cout << "\nSelect CLI Style:\n"
                << "1. Default / Classic CLI\n"
                << "2. Compact Single-Line CLI\n"
                << "3. Conversational Wizard Style\n"
                << "4. Bracketed Shell Style\n\n"
                << "Choice [1-4]: ";

            std::string style_choice;
            std::getline(std::cin, style_choice);

            std::cout << "\nBuilding Todo program...\n";

            if (style_choice == "2")
            {
                program = build_todo_program_compact();
            }
            else if (style_choice == "3")
            {
                program = build_todo_program_wizard();
            }
            else if (style_choice == "4")
            {
                program = build_todo_program_shell();
            }
            else
            {
                program = build_todo_program();
            }

            const auto start = clock::now();
            program.write("todo_program.bin");
            const auto end = clock::now();

            const std::chrono::duration<double, std::milli> elapsed = end - start;

            std::cout << "Program written to \"todo_program.bin\".\n"
                << "Write time: " << elapsed.count() << " ms\n\n";
        }
        else
        {
            std::cout << "\nReading program from \"todo_program.bin\"...\n";

            const auto start = clock::now();
            program = binvm::Program::read("todo_program.bin");
            const auto end = clock::now();

            const std::chrono::duration<double, std::milli> elapsed = end - start;

            std::cout << "Program loaded successfully.\n"
                << "Read time: " << elapsed.count() << " ms\n\n";
        }

        binvm::VirtualMachine vm;
        vm.set_input_stream(std::cin);
        vm.set_output_stream(std::cout);

        register_todo_handlers(vm);

        vm.load(program);
        vm.execute();

        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Fatal error: " << exception.what() << '\n';
        return 1;
    }
}