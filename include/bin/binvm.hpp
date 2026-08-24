// Copyright (c) 2026 Erik-Neo Östlund-Zetterberg
// See the license in the accompanying LICENSE.md file at the github repository:
// https://github.com/NeoZett/Binary-Development

#ifndef BINVM_HPP
#define BINVM_HPP

#include <bin/bin.hpp>
#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace binvm
{
	enum class Opcode : bin_id_t
	{
		Nop = 0,

		Set = 1,
		Copy = 2,
		Erase = 3,

		Print = 10,
		PrintVar = 11,
		Input = 12,

		Add = 20,
		Sub = 21,
		Mul = 22,
		Div = 23,
		Mod = 24,
		Neg = 25,

		Equal = 30,
		NotEqual = 31,
		Less = 32,
		LessEqual = 33,
		Greater = 34,
		GreaterEqual = 35,

		Jump = 40,
		JumpIf = 41,
		JumpIfNot = 42,

		Halt = 50
	};

	using BinString = bin::SerializableString;
	using BinBytes = bin::SerializableBytes;

	using VarId = bin_id_t;
	using LabelId = bin_id_t;

	struct Instruction
	{
		BIN_ID(1000);
		BIN_DECLARE_SCHEMA;

		Opcode opcode = Opcode::Nop;

		VarId var_id = 0;

		BinBytes operand1{};
		BinBytes operand2{};

		LabelId label_id = 0;

		BIN_FACILITY_DECLARATIONS(Instruction);
	};
}

BIN_STRUCT(binvm::Instruction, obj.opcode, obj.var_id, obj.operand1, obj.operand2);
BIN_FACILITY_DEFINITIONS(binvm::Instruction);
BIN_DEFINE_SCHEMA(binvm::Instruction);

namespace binvm
{
	class Operand
	{
	public:
		constexpr Operand(VarId var_id) BIN_NOEXCEPT
			: m_is_variable(true), m_var_id(var_id)
		{
		}

		Operand(std::string_view literal)
			: m_is_variable(false), m_literal(literal)
		{
		}

		Operand(const char* literal)
			: m_is_variable(false), m_literal(literal ? literal : "")
		{
		}

		Operand(const std::string& literal)
			: m_is_variable(false), m_literal(literal)
		{
		}

		BIN_NODISCARD bool is_variable() const BIN_NOEXCEPT { return m_is_variable; }
		BIN_NODISCARD VarId var_id() const BIN_NOEXCEPT { return m_var_id; }
		BIN_NODISCARD const std::string& literal() const BIN_NOEXCEPT { return m_literal; }

		BIN_NODISCARD BinBytes pack() const
		{
			// Packed as three uniform fields (rather than one OperandPayload struct) so that
			// FieldSerializer only ever has to handle primitives and BinString individually -
			// see FieldSerializer<SerializableString> and the SerializerImpl<T,true> path.
			const std::uint8_t is_variable = m_is_variable ? 1 : 0;
			const VarId var_id = m_is_variable ? m_var_id : 0;
			const BinString literal = m_is_variable ? BinString() : BinString(m_literal);
			return BinBytes::pack<std::uint8_t, VarId, BinString>(is_variable, var_id, literal);
		}

	private:
		bool m_is_variable{ false };
		VarId m_var_id{ 0 };
		std::string m_literal{};
	};

	struct Label
	{
		LabelId id{ 0 };
		size_t instruction_index{ 0 };
	};

	class Program
	{
	public:
		Program() = default;

		Program& set(VarId id, const BinBytes& value)
		{
			Instruction instruction;
			instruction.opcode = Opcode::Set;
			instruction.var_id = id;
			instruction.operand1 = value;
			m_instructions.push_back(std::move(instruction));
			return *this;
		}

		Program& set(VarId id, std::string_view value)
		{
			return set(id, BinBytes::pack<BinString>(BinString(value.data())));
		}

		Program& copy(VarId destination, VarId source)
		{
			Instruction instruction;
			instruction.opcode = Opcode::Copy;
			instruction.var_id = source;
			instruction.operand1 = BinBytes::pack<VarId>(destination);
			m_instructions.push_back(std::move(instruction));
			return *this;
		}

		Program& erase(VarId id)
		{
			Instruction instruction;
			instruction.opcode = Opcode::Erase;
			instruction.var_id = id;
			m_instructions.push_back(std::move(instruction));
			return *this;
		}

		Program& print(const Operand& operand)
		{
			Instruction instruction;
			instruction.opcode = Opcode::Print;
			instruction.operand1 = operand.pack();
			m_instructions.push_back(std::move(instruction));
			return *this;
		}

		Program& print_variable(VarId id)
		{
			Instruction instruction;
			instruction.opcode = Opcode::PrintVar;
			instruction.var_id = id;
			m_instructions.push_back(std::move(instruction));
			return *this;
		}

		Program& input(VarId id)
		{
			Instruction instruction;
			instruction.opcode = Opcode::Input;
			instruction.var_id = id;
			m_instructions.push_back(std::move(instruction));
			return *this;
		}

		Program& add(VarId id, const Operand& left, const Operand& right)
		{
			return binary_op(Opcode::Add, id, left, right);
		}

		Program& sub(VarId id, const Operand& left, const Operand& right)
		{
			return binary_op(Opcode::Sub, id, left, right);
		}

		Program& mul(VarId id, const Operand& left, const Operand& right)
		{
			return binary_op(Opcode::Mul, id, left, right);
		}

		Program& div(VarId id, const Operand& left, const Operand& right)
		{
			return binary_op(Opcode::Div, id, left, right);
		}

		Program& mod(VarId id, const Operand& left, const Operand& right)
		{
			return binary_op(Opcode::Mod, id, left, right);
		}

		Program& neg(VarId destination, const Operand& source)
		{
			Instruction instruction;
			instruction.opcode = Opcode::Neg;
			instruction.var_id = destination;
			instruction.operand1 = source.pack();
			m_instructions.push_back(std::move(instruction));
			return *this;
		}

		Program& equal(VarId id, const Operand& left, const Operand& right)
		{
			return binary_op(Opcode::Equal, id, left, right);
		}

		Program& not_equal(VarId id, const Operand& left, const Operand& right)
		{
			return binary_op(Opcode::NotEqual, id, left, right);
		}

		Program& less(VarId id, const Operand& left, const Operand& right)
		{
			return binary_op(Opcode::Less, id, left, right);
		}

		Program& less_equal(VarId id, const Operand& left, const Operand& right)
		{
			return binary_op(Opcode::LessEqual, id, left, right);
		}

		Program& greater(VarId id, const Operand& left, const Operand& right)
		{
			return binary_op(Opcode::Greater, id, left, right);
		}

		Program& greater_equal(VarId id, const Operand& left, const Operand& right)
		{
			return binary_op(Opcode::GreaterEqual, id, left, right);
		}

		Program& label(LabelId id)
		{
			if (m_labels.contains(id))
			{
				throw std::runtime_error("Duplicate label: " + std::to_string(id));
			}

			m_labels[id] = m_instructions.size();
			return *this;
		}

		Program& jump(LabelId id)
		{
			Instruction instruction;
			instruction.opcode = Opcode::Jump;
			instruction.label_id = id;
			m_instructions.push_back(std::move(instruction));
			return *this;
		}

		Program& jump_if_true(VarId var_id, LabelId label_id)
		{
			Instruction instruction;
			instruction.opcode = Opcode::JumpIf;
			instruction.var_id = var_id;
			instruction.label_id = label_id;
			m_instructions.push_back(std::move(instruction));
			return *this;
		}

		Program& jump_if_false(VarId var_id, LabelId label_id)
		{
			Instruction instruction;
			instruction.opcode = Opcode::JumpIfNot;
			instruction.var_id = var_id;
			instruction.label_id = label_id;
			m_instructions.push_back(std::move(instruction));
			return *this;
		}

		Program& halt()
		{
			Instruction instruction;
			instruction.opcode = Opcode::Halt;
			m_instructions.push_back(std::move(instruction));
			return *this;
		}

		Program& add_instruction(Instruction& instruction)
		{
			m_instructions.push_back(std::move(instruction));
			return *this;
		}

		BIN_NODISCARD const std::vector<Instruction>& instructions() const BIN_NOEXCEPT
		{
			return m_instructions;
		}

		BIN_NODISCARD size_t size() const BIN_NOEXCEPT
		{
			return m_instructions.size();
		}

		void update_labels()
		{
			for (Instruction& instruction : m_instructions)
			{
				if ((instruction.opcode == Opcode::Jump ||
					instruction.opcode == Opcode::JumpIf ||
					instruction.opcode == Opcode::JumpIfNot) &&
					instruction.operand2.size() == 0)
				{
					instruction.operand2 = BinBytes::pack<size_t>(resolve_label(instruction.label_id));
				}
			}
		}

		BIN_NODISCARD std::vector<bin::Record> generate_records()
		{
			std::vector<bin::Record> result;
			result.reserve(m_instructions.size());

			update_labels();

			for (Instruction& instruction : m_instructions)
			{
				result.push_back(instruction.to_record());
			}

			return result;
		}

		void write(const std::string& path)
		{
			bin::BinaryStream stream(path);

			for (const bin::Record& record : generate_records())
			{
				stream.push_back(record);
			}

			if (!stream.write())
			{
				throw std::runtime_error("Failed to write BIN program.");
			}
		}

		static Program read(const std::string& path)
		{
			bin::SchemaCatalog catalog;
			catalog.add(Instruction::bin_schema);

			bin::BinaryStream stream(path);
			if (!stream.read(catalog))
			{
				throw std::runtime_error("Failed to read BIN program.");
			}

			Program program;
			for (const auto& record : stream.records())
			{
				if (record.id() != Instruction::bin_id)
				{
					throw std::runtime_error("Unknown BIN record in program.");
				}

				Instruction instruction = Instruction::from_record(record);
				program.m_instructions.push_back(std::move(instruction));
			}

			return program;
		}

	private:
		Program& binary_op(Opcode opcode, VarId id, const Operand& left, const Operand& right)
		{
			Instruction instruction;
			instruction.opcode = opcode;
			instruction.var_id = id;
			instruction.operand1 = left.pack();
			instruction.operand2 = right.pack();
			m_instructions.push_back(std::move(instruction));
			return *this;
		}

		BIN_NODISCARD size_t resolve_label(LabelId label) const
		{
			const auto it = m_labels.find(label);

			if (it == m_labels.end())
			{
				throw std::runtime_error("Unknown label: " + std::to_string(label));
			}

			return it->second;
		}

		std::vector<Instruction> m_instructions;
		std::unordered_map<LabelId, size_t> m_labels;
	};

	class VirtualMachine;

	using InstructionHandler = std::function<void(VirtualMachine&, const Instruction&)>;

	class OpcodeDispatcher
	{
	public:
		void register_handler(Opcode opcode, InstructionHandler handler)
		{
			m_handlers[opcode] = std::move(handler);
		}

		BIN_NODISCARD bool contains(Opcode opcode) const
		{
			return m_handlers.contains(opcode);
		}

		void execute(Opcode opcode, VirtualMachine& vm, const Instruction& instruction) const
		{
			const auto it = m_handlers.find(opcode);
			if (it == m_handlers.end())
			{
				throw std::runtime_error("Unknown opcode: " + std::to_string(static_cast<bin_id_t>(opcode)));
			}

			it->second(vm, instruction);
		}

	private:
		std::unordered_map<Opcode, InstructionHandler> m_handlers;
	};

	class VirtualMachine
	{
	public:
		VirtualMachine()
		{
			register_builtin_handlers();
		}

		explicit VirtualMachine(OpcodeDispatcher dispatcher)
			: m_dispatcher(std::move(dispatcher))
		{
		}

		void load(Program program)
		{
			program.update_labels();
			m_program = std::move(program);
			m_pc = 0;
			m_halted = false;
		}

		void load(const std::string& path)
		{
			load(Program::read(path));
		}

		void execute()
		{
			m_halted = false;
			m_pc = 0;

			while (!m_halted && m_pc < m_program.size())
			{
				step();
			}
		}

		bool step()
		{
			if (m_halted)
				return false;

			if (m_pc >= m_program.size())
			{
				m_halted = true;
				return false;
			}

			const size_t current_pc = m_pc;
			const Instruction& instruction = m_program.instructions()[current_pc];

			m_next_pc = current_pc + 1;
			m_dispatcher.execute(instruction.opcode, *this, instruction);
			m_pc = m_next_pc;

			return !m_halted;
		}

		BIN_NODISCARD bool has_variable(VarId id) const
		{
			return m_variables.contains(id);
		}

		BIN_NODISCARD const BinBytes& variable(VarId id) const
		{
			const auto it = m_variables.find(id);
			if (it == m_variables.end())
			{
				throw std::runtime_error("Unknown variable ID: " + std::to_string(id));
			}
			return it->second;
		}

		void set_variable(VarId id, BinBytes value)
		{
			m_variables[id] = std::move(value);
		}

		void erase_variable(VarId id)
		{
			m_variables.erase(id);
		}

		BIN_NODISCARD const std::unordered_map<VarId, BinBytes>& variables() const BIN_NOEXCEPT
		{
			return m_variables;
		}

		BIN_NODISCARD size_t program_counter() const BIN_NOEXCEPT
		{
			return m_pc;
		}

		void jump_to(size_t target)
		{
			if (target >= m_program.size())
			{
				throw std::runtime_error("Jump target outside program bounds.");
			}
			m_next_pc = target;
		}

		void halt() BIN_NOEXCEPT
		{
			m_halted = true;
		}

		BIN_NODISCARD bool is_halted() const BIN_NOEXCEPT
		{
			return m_halted;
		}

		BIN_NODISCARD OpcodeDispatcher& dispatcher() BIN_NOEXCEPT
		{
			return m_dispatcher;
		}

		BIN_NODISCARD const OpcodeDispatcher& dispatcher() const BIN_NOEXCEPT
		{
			return m_dispatcher;
		}

		BIN_NODISCARD std::istream& input_stream() BIN_NOEXCEPT
		{
			return *m_input;
		}

		BIN_NODISCARD std::ostream& output_stream() BIN_NOEXCEPT
		{
			return *m_output;
		}

		void set_input_stream(std::istream& input) BIN_NOEXCEPT
		{
			m_input = &input;
		}

		void set_output_stream(std::ostream& output) BIN_NOEXCEPT
		{
			m_output = &output;
		}

		BIN_NODISCARD std::string evaluate_operand_string(const BinBytes& bytes) const
		{
			std::uint8_t is_variable = 0;
			VarId var_id = 0;
			BinString literal;
			bytes.unpack(is_variable, var_id, literal);

			if (is_variable != 0)
			{
				BinString string;
				variable(var_id).unpack(string);
				return string.c_str();
			}
			return literal.c_str();
		}

		BIN_NODISCARD double evaluate_operand_double(const BinBytes& bytes) const
		{
			return std::stod(evaluate_operand_string(bytes));
		}

		BIN_NODISCARD long evaluate_operand_long(const BinBytes& bytes) const
		{
			return std::stol(evaluate_operand_string(bytes));
		}

		void binary_arithmetic(const Instruction& instruction, char op)
		{
			const double left = evaluate_operand_double(instruction.operand1);
			const double right = evaluate_operand_double(instruction.operand2);
			double result = 0.0;

			switch (op)
			{
			case '+': result = left + right; break;
			case '-': result = left - right; break;
			case '*': result = left * right; break;
			case '/':
				if (right == 0.0) throw std::runtime_error("Division by zero.");
				result = left / right;
				break;
			default:
				throw std::runtime_error("Invalid arithmetic operator.");
			}

			const std::string res_str = std::to_string(result);
			set_variable(instruction.var_id, BinBytes::pack<BinString>(BinString(res_str.c_str())));
		}

		void compare(const Instruction& instruction, const std::function<bool(std::string_view, std::string_view)>& comp)
		{
			const std::string left = evaluate_operand_string(instruction.operand1);
			const std::string right = evaluate_operand_string(instruction.operand2);
			const bool res = comp(left, right);

			set_variable(instruction.var_id, BinBytes::pack<int32_t>(res ? 1 : 0));
		}

		void numeric_compare(const Instruction& instruction, const std::function<bool(double, double)>& comp)
		{
			const double left = evaluate_operand_double(instruction.operand1);
			const double right = evaluate_operand_double(instruction.operand2);
			const bool res = comp(left, right);

			set_variable(instruction.var_id, BinBytes::pack<int32_t>(res ? 1 : 0));
		}

		void reset_variables() BIN_NOEXCEPT
		{
			m_variables.clear();
		}

	private:
		void register_builtin_handlers()
		{
			m_dispatcher.register_handler(Opcode::Nop, [](VirtualMachine&, const Instruction&) {});

			m_dispatcher.register_handler(Opcode::Set, [](VirtualMachine& vm, const Instruction& inst) {
				vm.set_variable(inst.var_id, inst.operand1);
				});

			m_dispatcher.register_handler(Opcode::Copy, [](VirtualMachine& vm, const Instruction& inst) {
				VarId var_id;
				inst.operand1.unpack(var_id);
				vm.set_variable(var_id, vm.variable(inst.var_id));
				});

			m_dispatcher.register_handler(Opcode::Erase, [](VirtualMachine& vm, const Instruction& inst) {
				vm.erase_variable(inst.var_id);
				});

			m_dispatcher.register_handler(Opcode::Print, [](VirtualMachine& vm, const Instruction& inst) {
				vm.output_stream() << vm.evaluate_operand_string(inst.operand1);
				});

			m_dispatcher.register_handler(Opcode::PrintVar, [](VirtualMachine& vm, const Instruction& inst) {
				BinString text;
				vm.variable(inst.var_id).unpack(text);
				vm.output_stream() << text.c_str();
				});

			m_dispatcher.register_handler(Opcode::Input, [](VirtualMachine& vm, const Instruction& inst) {
				std::string line;
				std::getline(vm.input_stream(), line);
				vm.set_variable(inst.var_id, BinBytes::pack<BinString>(BinString(line.c_str())));
				});

			m_dispatcher.register_handler(Opcode::Add, [](VirtualMachine& vm, const Instruction& inst) { vm.binary_arithmetic(inst, '+'); });
			m_dispatcher.register_handler(Opcode::Sub, [](VirtualMachine& vm, const Instruction& inst) { vm.binary_arithmetic(inst, '-'); });
			m_dispatcher.register_handler(Opcode::Mul, [](VirtualMachine& vm, const Instruction& inst) { vm.binary_arithmetic(inst, '*'); });
			m_dispatcher.register_handler(Opcode::Div, [](VirtualMachine& vm, const Instruction& inst) { vm.binary_arithmetic(inst, '/'); });

			m_dispatcher.register_handler(Opcode::Mod, [](VirtualMachine& vm, const Instruction& inst) {
				const long left = vm.evaluate_operand_long(inst.operand1);
				const long right = vm.evaluate_operand_long(inst.operand2);
				if (right == 0) throw std::runtime_error("Modulo by zero.");

				const std::string res = std::to_string(left % right);
				vm.set_variable(inst.var_id, BinBytes::pack<BinString>(BinString(res.c_str())));
				});

			m_dispatcher.register_handler(Opcode::Neg, [](VirtualMachine& vm, const Instruction& inst) {
				const double val = vm.evaluate_operand_double(inst.operand1);
				const std::string res = std::to_string(-val);
				vm.set_variable(inst.var_id, BinBytes::pack<BinString>(BinString(res.c_str())));
				});

			m_dispatcher.register_handler(Opcode::Equal, [](VirtualMachine& vm, const Instruction& inst) {
				vm.compare(inst, [](std::string_view a, std::string_view b) { return a == b; });
				});

			m_dispatcher.register_handler(Opcode::NotEqual, [](VirtualMachine& vm, const Instruction& inst) {
				vm.compare(inst, [](std::string_view a, std::string_view b) { return a != b; });
				});

			m_dispatcher.register_handler(Opcode::Less, [](VirtualMachine& vm, const Instruction& inst) {
				vm.numeric_compare(inst, [](double a, double b) { return a < b; });
				});

			m_dispatcher.register_handler(Opcode::LessEqual, [](VirtualMachine& vm, const Instruction& inst) {
				vm.numeric_compare(inst, [](double a, double b) { return a <= b; });
				});

			m_dispatcher.register_handler(Opcode::Greater, [](VirtualMachine& vm, const Instruction& inst) {
				vm.numeric_compare(inst, [](double a, double b) { return a > b; });
				});

			m_dispatcher.register_handler(Opcode::GreaterEqual, [](VirtualMachine& vm, const Instruction& inst) {
				vm.numeric_compare(inst, [](double a, double b) { return a >= b; });
				});

			m_dispatcher.register_handler(Opcode::Jump, [](VirtualMachine& vm, const Instruction& inst) {
				size_t index;
				inst.operand2.unpack(index);
				vm.jump_to(index);
				});

			m_dispatcher.register_handler(Opcode::JumpIf, [](VirtualMachine& vm, const Instruction& inst) {
				int32_t conditional;
				vm.variable(inst.var_id).unpack(conditional);
				if (conditional != 0)
				{
					size_t index;
					inst.operand2.unpack(index);
					vm.jump_to(index);
				}
				});

			m_dispatcher.register_handler(Opcode::JumpIfNot, [](VirtualMachine& vm, const Instruction& inst) {
				int32_t conditional;
				vm.variable(inst.var_id).unpack(conditional);
				if (conditional == 0)
				{
					size_t index;
					inst.operand2.unpack(index);
					vm.jump_to(index);
				}
				});

			m_dispatcher.register_handler(Opcode::Halt, [](VirtualMachine& vm, const Instruction&) {
				vm.halt();
				});
		}

		Program m_program{};
		size_t m_pc{ 0 };
		size_t m_next_pc{ 0 };
		bool m_halted{ false };

		OpcodeDispatcher m_dispatcher{};
		std::unordered_map<VarId, BinBytes> m_variables{};

		std::istream* m_input{ &std::cin };
		std::ostream* m_output{ &std::cout };
	};
}

#endif /* BINVM_HPP */