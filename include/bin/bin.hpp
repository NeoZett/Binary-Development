// Copyright (c) 2026 Erik-Neo Östlund-Zetterberg
// See the license in the accompanying LICENSE.md file at the github repository:
// https://github.com/NeoZett/Binary-Development

#ifndef BIN_HPP
#define BIN_HPP

#ifndef __cplusplus
#error "bin.hpp is a C++ wrapper and requires a C++ compiler. Use bin.h for C."
#endif

#if defined(_MSVC_LANG)
#if _MSVC_LANG < 201103L
#error "bin.hpp requires C++11 or higher."
#endif
#elif __cplusplus < 201103L
#error "bin.hpp requires C++11 or higher."
#endif

#include <initializer_list>
#include <stdexcept>
#include <vector>
#include <string>
#include <cstring>
#include <cctype>
#include <type_traits>
#include <utility>
#include <array>
#include <memory>
#include <bin/bin.h>

#ifndef BIN_NOEXCEPT
#define BIN_NOEXCEPT noexcept
#endif

#ifndef BIN_NODISCARD
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard) >= 201603L
#define BIN_NODISCARD [[nodiscard]]
#else
#define BIN_NODISCARD
#endif
#endif

namespace bin
{
	class Record
	{
	public:
		using IndexType = size_t;
		using IdType = bin_id_t;
		using ByteType = bin_byte_t;

		Record() BIN_NOEXCEPT : m_handle{} {}

		Record(bin_id_t id, std::initializer_list<const char*> fields) : m_handle{}
		{
			bin_record_init(&m_handle, id, fields.begin(), fields.size());
		}

		Record(bin_id_t id, const char* const* fields, size_t field_count) : m_handle{}
		{
			bin_record_init(&m_handle, id, fields, field_count);
		}

		explicit Record(const bin_record_t& native_record)
			: m_handle{}
		{
			if (!bin_record_copy(&m_handle, &native_record))
				throw std::runtime_error("Failed to copy bin_record_t");
		}

		Record(const Record& other) : m_handle{}
		{
			if (!bin_record_copy(&m_handle, &other.m_handle))
				throw std::runtime_error("Failed to copy bin_record_t");
		}

		Record& operator=(const Record& other)
		{
			if (this != &other)
			{
				free_record();
				if (!bin_record_copy(&m_handle, &other.m_handle))
					throw std::runtime_error("Failed to copy bin_record_t");
			}
			return *this;
		}

		Record(Record&& other) BIN_NOEXCEPT : m_handle(other.m_handle)
		{
			std::memset(&other.m_handle, 0, sizeof(bin_record_t));
		}

		Record& operator=(Record&& other) BIN_NOEXCEPT
		{
			if (this != &other)
			{
				free_record();
				m_handle = other.m_handle;
				std::memset(&other.m_handle, 0, sizeof(bin_record_t));
			}
			return *this;
		}

		~Record() BIN_NOEXCEPT
		{
			free_record();
		}

		void free_record() BIN_NOEXCEPT
		{
			bin_record_free(&m_handle);
		}

		Record& set(IndexType index, const ByteType* value, size_t len)
		{
			static const ByteType empty_dummy = 0;
			const ByteType* safe_value = (value && len > 0) ? value : &empty_dummy;
			size_t safe_len = (value && len > 0) ? len : 0;
			if (!bin_record_set_field_at(&m_handle, index, safe_value, safe_len))
				throw std::runtime_error("Failed to set field at index " + std::to_string(index));
			return *this;
		}

		template <typename StringType, typename = typename std::enable_if<
			std::is_same<typename std::decay<StringType>::type, const char*>::value ||
			std::is_same<typename std::decay<StringType>::type, char*>::value
		>::type>
		Record& set(StringType name, const ByteType* value, size_t len)
		{
			if (!bin_record_set_field(&m_handle, name, value, len))
				throw std::runtime_error("Failed to set field with name: " + std::string(name));
			return *this;
		}

		template <typename T, typename = typename std::enable_if<!std::is_pointer<typename std::decay<T>::type>::value>::type>
		Record& set(IndexType index, const T& value)
		{
			static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
			return set(index, reinterpret_cast<const ByteType*>(&value), sizeof(T));
		}

		template <typename T, typename = typename std::enable_if<!std::is_pointer<typename std::decay<T>::type>::value>::type>
		Record& set(const char* name, const T& value)
		{
			static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
			return set(name, reinterpret_cast<const ByteType*>(&value), sizeof(T));
		}

		BIN_NODISCARD ByteType* get(IndexType index) const
		{
			if (index >= m_handle.field_count)
				throw std::runtime_error("Failed to get field at index " + std::to_string(index));
			return m_handle.field_values[index];
		}

		BIN_NODISCARD ByteType* get(const char* name) const
		{
			if (!name)
				throw std::runtime_error("Failed to get field with null name");
			for (size_t i = 0; i < m_handle.field_count; ++i)
			{
				if (m_handle.field_names[i] && std::strcmp(m_handle.field_names[i], name) == 0)
					return m_handle.field_values[i];
			}
			throw std::runtime_error("Failed to get field with name: " + std::string(name));
		}

		template <typename T>
		BIN_NODISCARD typename std::enable_if<!std::is_pointer<T>::value, T>::type
			get(IndexType index) const
		{
			static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
			T value{};
			if (!bin_record_get_field_at(&m_handle, index, reinterpret_cast<ByteType*>(&value)))
				throw std::runtime_error("Failed to extract field value at index " + std::to_string(index));
			return value;
		}

		template <typename T>
		BIN_NODISCARD typename std::enable_if<!std::is_pointer<T>::value, T>::type
			get(const char* name) const
		{
			static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
			T value{};
			if (!bin_record_get_field(&m_handle, name, reinterpret_cast<ByteType*>(&value)))
				throw std::runtime_error("Failed to extract field value for name: " + std::string(name));
			return value;
		}

		BIN_NODISCARD size_t size() const BIN_NOEXCEPT
		{
			return m_handle.field_count;
		}

		BIN_NODISCARD IdType id() const BIN_NOEXCEPT
		{
			return m_handle.id;
		}

		BIN_NODISCARD const bin_record_t* native_handle() const BIN_NOEXCEPT
		{
			return &m_handle;
		}

	private:
		bin_record_t m_handle;
	};

	using Schema = Record;

	class SchemaCatalog
	{
	public:
		SchemaCatalog() BIN_NOEXCEPT : m_handle{} {}

		SchemaCatalog(std::initializer_list<Schema> schemas) : m_handle{}
		{
			for (const Schema& schema : schemas)
				add(schema);
		}

		SchemaCatalog(const Schema* schemas, size_t schema_count) : m_handle{}
		{
			for (size_t i = 0; i < schema_count; ++i)
				add(schemas[i]);
		}

		SchemaCatalog(const SchemaCatalog& other) : m_handle{}
		{
			m_storage.reserve(other.m_storage.size());
			for (const bin_schema_t& schema : other.m_storage)
				add_native(schema);
		}

		SchemaCatalog& operator=(const SchemaCatalog& other)
		{
			if (this != &other)
			{
				clear();
				m_storage.reserve(other.m_storage.size());
				for (const bin_schema_t& schema : other.m_storage)
					add_native(schema);
			}
			return *this;
		}

		SchemaCatalog(SchemaCatalog&& other) BIN_NOEXCEPT
			: m_handle(other.m_handle), m_storage(std::move(other.m_storage))
		{
			other.m_handle = bin_catalog_t{};
		}

		SchemaCatalog& operator=(SchemaCatalog&& other) BIN_NOEXCEPT
		{
			if (this != &other)
			{
				clear();
				m_handle = other.m_handle;
				m_storage = std::move(other.m_storage);
				other.m_handle = bin_catalog_t{};
			}
			return *this;
		}

		~SchemaCatalog() BIN_NOEXCEPT
		{
			clear();
		}

		void add(const Schema& schema)
		{
			add_native(*(schema.native_handle()));
		}

		void clear() BIN_NOEXCEPT
		{
			for (bin_schema_t& schema : m_storage)
				bin_record_free(&schema);
			m_storage.clear();
			m_handle.schemas = nullptr;
			m_handle.schema_count = 0;
		}

		BIN_NODISCARD size_t size() const BIN_NOEXCEPT
		{
			return static_cast<size_t>(m_handle.schema_count);
		}

		BIN_NODISCARD bool empty() const BIN_NOEXCEPT
		{
			return m_handle.schema_count == 0;
		}

		BIN_NODISCARD const bin_catalog_t* native_handle() const BIN_NOEXCEPT
		{
			return &m_handle;
		}

	private:
		void add_native(const bin_schema_t& native_schema)
		{
			bin_schema_t copy{};
			if (!bin_record_copy(&copy, &native_schema))
				throw std::runtime_error("Failed to add schema to catalog");
			m_storage.push_back(copy);
			// m_storage may have just reallocated, so every pointer into it must be refreshed.
			m_handle.schemas = m_storage.data();
			m_handle.schema_count = m_storage.size();
		}

		bin_catalog_t m_handle;
		std::vector<bin_schema_t> m_storage;
	};

	class FileStream
	{
	public:
		FileStream(const char* path, const char* mode) : m_handle{}
		{
			if (!bin_stream_open(&m_handle, path, mode))
				throw std::runtime_error("Failed to open file stream at specified path");
		}

		FileStream(const FileStream&) = delete;
		FileStream& operator=(const FileStream&) = delete;

		FileStream(FileStream&& other) BIN_NOEXCEPT : m_handle(other.m_handle)
		{
			std::memset(&other.m_handle, 0, sizeof(bin_stream_t));
		}

		FileStream& operator=(FileStream&& other) BIN_NOEXCEPT
		{
			if (this != &other)
			{
				close();
				m_handle = other.m_handle;
				std::memset(&other.m_handle, 0, sizeof(bin_stream_t));
			}
			return *this;
		}

		~FileStream() BIN_NOEXCEPT
		{
			close();
		}

		bool close() BIN_NOEXCEPT
		{
			return bin_stream_close(&m_handle);
		}

		BIN_NODISCARD FILE* file() const BIN_NOEXCEPT
		{
			return m_handle.file;
		}

		BIN_NODISCARD const bin_stream_t* native_handle() const BIN_NOEXCEPT
		{
			return &m_handle;
		}

	private:
		bin_stream_t m_handle;
	};

	class BinaryReader
	{
	public:
		BinaryReader(const char* path, const SchemaCatalog& catalog) : m_handle{}
		{
			if (!path || !bin_reader_init(&m_handle, path, catalog.native_handle()))
				throw std::runtime_error("Failed to initialize binary reader");
		}

		BinaryReader(const std::string& path, const SchemaCatalog& catalog)
			: BinaryReader(path.c_str(), catalog)
		{
		}

		BinaryReader(const BinaryReader&) = delete;
		BinaryReader& operator=(const BinaryReader&) = delete;

		BinaryReader(BinaryReader&& other) BIN_NOEXCEPT : m_handle(other.m_handle)
		{
			std::memset(&other.m_handle, 0, sizeof(bin_reader_t));
		}

		BinaryReader& operator=(BinaryReader&& other) BIN_NOEXCEPT
		{
			if (this != &other)
			{
				bin_reader_deinit(&m_handle);
				m_handle = other.m_handle;
				std::memset(&other.m_handle, 0, sizeof(bin_reader_t));
			}
			return *this;
		}

		~BinaryReader() BIN_NOEXCEPT
		{
			bin_reader_deinit(&m_handle);
		}

		bool close() BIN_NOEXCEPT
		{
			return bin_reader_close(&m_handle);
		}

		bool execute() BIN_NOEXCEPT
		{
			return bin_reader_execute(&m_handle);
		}

		BIN_NODISCARD std::vector<Record> records() const
		{
			std::vector<Record> record_list;
			record_list.reserve(m_handle.record_count);

			for (size_t i = 0; i < m_handle.record_count; ++i)
			{
				record_list.emplace_back(m_handle.records[i]);
			}

			return record_list;
		}

		BIN_NODISCARD bool has_completed() const BIN_NOEXCEPT
		{
			return m_handle.has_completed;
		}

		BIN_NODISCARD const bin_reader_t* native_handle() const BIN_NOEXCEPT
		{
			return &m_handle;
		}

	private:
		bin_reader_t m_handle;
	};

	class BinaryWriter
	{
	public:
		BinaryWriter(const char* path, const Record* records, size_t record_count) : m_handle{}
		{
			init(path, records, record_count);
		}

		BinaryWriter(const std::string& path, const Record* records, size_t record_count) : m_handle{}
		{
			init(path.c_str(), records, record_count);
		}

		BinaryWriter(const char* path, std::initializer_list<Record> records)
			: BinaryWriter(path, records.begin(), records.size())
		{
		}

		BinaryWriter(const std::string& path, std::initializer_list<Record> records)
			: BinaryWriter(path.c_str(), records.begin(), records.size())
		{
		}

		BinaryWriter(const BinaryWriter&) = delete;
		BinaryWriter& operator=(const BinaryWriter&) = delete;

		BinaryWriter(BinaryWriter&& other) BIN_NOEXCEPT : m_handle(other.m_handle)
		{
			std::memset(&other.m_handle, 0, sizeof(bin_writer_t));
		}

		BinaryWriter& operator=(BinaryWriter&& other) BIN_NOEXCEPT
		{
			if (this != &other)
			{
				bin_writer_deinit(&m_handle);
				m_handle = other.m_handle;
				std::memset(&other.m_handle, 0, sizeof(bin_writer_t));
			}
			return *this;
		}

		~BinaryWriter() BIN_NOEXCEPT
		{
			bin_writer_deinit(&m_handle);
		}

		bool close() BIN_NOEXCEPT
		{
			return bin_writer_close(&m_handle);
		}

		bool execute() BIN_NOEXCEPT
		{
			return bin_writer_execute(&m_handle);
		}

		BIN_NODISCARD bool has_completed() const BIN_NOEXCEPT
		{
			return m_handle.has_completed;
		}

		BIN_NODISCARD const bin_writer_t* native_handle() const BIN_NOEXCEPT
		{
			return &m_handle;
		}

	private:
		void init(const char* path, const Record* records, size_t record_count)
		{
			if (!path)
				throw std::invalid_argument("File path cannot be null");

			std::vector<bin_record_t> native_records;
			native_records.reserve(record_count);

			for (size_t i = 0; i < record_count; ++i)
				native_records.push_back(*records[i].native_handle());

			if (!bin_writer_init(&m_handle, path, native_records.data(), native_records.size()))
				throw std::runtime_error("Failed to initialize binary writer");
		}

		bin_writer_t m_handle;
	};

	class BinaryStream
	{
	public:
		explicit BinaryStream(std::string path)
			: m_path(std::move(path))
		{
		}

		explicit BinaryStream(const char* path)
			: m_path(path ? path : "")
		{
		}

		void push_back(Record record)
		{
			m_records.push_back(std::move(record));
		}

		void clear() BIN_NOEXCEPT
		{
			m_records.clear();
		}

		bool read(const SchemaCatalog& catalog)
		{
			BinaryReader reader(m_path.c_str(), catalog);

			if (!reader.execute())
				throw std::runtime_error("An error occurred when reading binary stream");

			m_records = reader.records();
			return reader.has_completed();
		}

		bool write()
		{
			BinaryWriter writer(m_path.c_str(), m_records.data(), m_records.size());

			if (!writer.execute())
				throw std::runtime_error("An error occurred when writing binary stream");

			return writer.has_completed();
		}

		BIN_NODISCARD const std::vector<Record>& records() const BIN_NOEXCEPT
		{
			return m_records;
		}

		BIN_NODISCARD std::vector<Record>& records() BIN_NOEXCEPT
		{
			return m_records;
		}

		BIN_NODISCARD const std::string& path() const BIN_NOEXCEPT
		{
			return m_path;
		}

	private:
		std::string m_path;
		std::vector<Record> m_records;
	};

	template <typename T>
	struct StructTraits;

	namespace detail
	{
		template <typename T, bool IsTriviallyCopyable = std::is_trivially_copyable<T>::value>
		struct SerializerImpl
		{
			static void set(Record& rec, size_t idx, const T& val)
			{
				rec.set(idx, val);
			}

			template <typename Arg>
			static void get(const Record& rec, size_t idx, Arg& val)
			{
				using TargetType = typename std::decay<Arg>::type;
				val = rec.get<TargetType>(idx);
			}
		};

		template <typename T>
		struct SerializerImpl<T, false>
		{
			// This raw-bytes fallback is only safe for non-trivially-copyable types that are still
			// layout-compatible with a byte copy (e.g. POD payloads with alignment padding, or a type
			// with a user-provided-but-trivial-effect constructor). If T owns a resource (heap memory,
			// a file handle, etc.) - anything with a non-trivial destructor - a memcpy round-trip will
			// duplicate the underlying pointer/handle without duplicating what it points to, corrupting
			// or double-freeing it. Such types must go through BIN_STRUCT and per-field serialization instead.
			static_assert(std::is_trivially_destructible<T>::value,
				"Type T is not safe for raw byte serialization (it owns a resource / has a non-trivial "
				"destructor, e.g. contains a std::string or std::vector). Use BIN_STRUCT to serialize it "
				"field-by-field instead of packing/copying the whole struct as bytes.");

			static void set(Record& rec, size_t idx, const T& val)
			{
				rec.set(idx, reinterpret_cast<const bin_byte_t*>(&val), sizeof(T));
			}

			template <typename Arg>
			static void get(const Record& rec, size_t idx, Arg& val)
			{
				const bin_byte_t* ptr = rec.get(idx);
				if (ptr)
				{
					using TargetType = typename std::decay<Arg>::type;
					size_t copy_size = sizeof(TargetType) < sizeof(T) ? sizeof(TargetType) : sizeof(T);
					std::memcpy(const_cast<void*>(static_cast<const void*>(&val)), ptr, copy_size);
				}
			}
		};

		template <typename T>
		struct FieldSerializer
		{
			using RawType = typename std::decay<T>::type;

			static void set(Record& rec, size_t idx, const RawType& val)
			{
				SerializerImpl<RawType>::set(rec, idx, val);
			}

			template <typename Arg>
			static void get(const Record& rec, size_t idx, Arg& val)
			{
				SerializerImpl<RawType>::get(rec, idx, val);
			}
		};

		template <>
		struct FieldSerializer<std::string> {
			static void set(Record& rec, size_t idx, const std::string& val)
			{
				rec.set(idx, reinterpret_cast<const bin_byte_t*>(val.c_str()), val.size() + 1);
			}

			template <typename Arg>
			static void get(const Record& rec, size_t idx, Arg& val)
			{
				const bin_byte_t* ptr = rec.get(idx);
				if (ptr)
				{
					val = reinterpret_cast<const char*>(ptr);
				}
				else
				{
					val = "";
				}
			}
		};

		template <typename U>
		struct FieldSerializer<std::vector<U>> {
			static_assert(std::is_trivially_copyable<U>::value,
				"std::vector<U> is serialized via raw memcpy; U must be trivially copyable. "
				"bin::Record (and other RAII types) must not be used here.");

			static void set(Record& rec, size_t idx, const std::vector<U>& val)
			{
				if (!val.empty()) {
					rec.set(idx, reinterpret_cast<const bin_byte_t*>(val.data()), val.size() * sizeof(U));
				}
				else {
					static const bin_byte_t dummy = 0;
					rec.set(idx, &dummy, 0);
				}
			}

			template <typename Arg>
			static void get(const Record& rec, size_t idx, Arg& val)
			{
				const bin_byte_t* ptr = rec.get(idx);
				size_t len = rec.native_handle()->field_lengths[idx];
				if (ptr && len > 0) {
					val.resize(len / sizeof(U));
					std::memcpy(val.data(), ptr, len);
				}
				else {
					val.clear();
				}
			}
		};

		inline std::string clean_field_name(const std::string& name)
		{
			size_t dot_pos = name.rfind('.');
			if (dot_pos != std::string::npos)
				return name.substr(dot_pos + 1);
			size_t arrow_pos = name.rfind("->");
			if (arrow_pos != std::string::npos)
				return name.substr(arrow_pos + 2);
			return name;
		}

		inline std::vector<std::string> parse_field_names(const char* names_str)
		{
			std::vector<std::string> fields;
			if (!names_str) return fields;

			const char* p = names_str;
			while (*p)
			{
				while (*p && std::isspace(static_cast<unsigned char>(*p))) ++p;
				const char* start = p;
				while (*p && *p != ',') ++p;
				const char* end = p;
				while (end > start && std::isspace(static_cast<unsigned char>(*(end - 1)))) --end;

				if (end > start)
				{
					std::string raw_name(start, end - start);
					fields.push_back(clean_field_name(raw_name));
				}
				if (*p == ',') ++p;
			}
			return fields;
		}

		inline std::vector<const char*> to_char_ptrs(const std::vector<std::string>& strs)
		{
			std::vector<const char*> ptrs;
			ptrs.reserve(strs.size());
			for (const std::string& s : strs)
				ptrs.push_back(s.c_str());
			return ptrs;
		}

		inline void set_field_at_idx(Record&, size_t&) {}

		template <typename Head, typename... Tail>
		inline void set_field_at_idx(Record& rec, size_t& idx, const Head& head, const Tail&... tail)
		{
			if (idx < rec.size())
			{
				FieldSerializer<typename std::decay<Head>::type>::set(rec, idx, head);
				idx++;
				set_field_at_idx(rec, idx, tail...);
			}
		}

		inline void get_field_at_idx(const Record&, size_t&) {}

		template <typename Head, typename... Tail>
		inline void get_field_at_idx(const Record& rec, size_t& idx, Head& head, Tail&... tail)
		{
			if (idx < rec.size())
			{
				FieldSerializer<typename std::decay<Head>::type>::get(rec, idx, head);
				idx++;
				get_field_at_idx(rec, idx, tail...);
			}
		}
	}

	struct SerializableString
	{
		std::string value;

		SerializableString() = default;
		SerializableString(std::string s) : value(std::move(s)) {}
		SerializableString(const char* s) : value(s ? s : "") {}

		SerializableString& operator=(std::string s) { value = std::move(s); return *this; }
		SerializableString& operator=(const char* s) { value = s ? s : ""; return *this; }

		BIN_NODISCARD const char* c_str() const BIN_NOEXCEPT { return value.c_str(); }
		BIN_NODISCARD size_t size() const BIN_NOEXCEPT { return value.size(); }
		operator const char* () const BIN_NOEXCEPT { return value.c_str(); }
	};

	struct SerializableBytes
	{
		std::vector<bin_byte_t> data;

		SerializableBytes() = default;
		SerializableBytes(std::vector<bin_byte_t> bytes) : data(std::move(bytes)) {}
		SerializableBytes(const bin_byte_t* src, size_t len) : data(src, src + len) {}

		BIN_NODISCARD const bin_byte_t* data_ptr() const BIN_NOEXCEPT { return data.data(); }
		BIN_NODISCARD size_t size() const BIN_NOEXCEPT { return data.size(); }

		template <typename... Args>
		static SerializableBytes pack(const Args&... args)
		{
			const size_t field_count = sizeof...(Args);
			Record rec(0, nullptr, field_count);

			size_t idx = 0;
			detail::set_field_at_idx(rec, idx, args...);

			SerializableBytes result;
			const bin_record_t* native = rec.native_handle();
			for (size_t i = 0; i < native->field_count; ++i)
			{
				const size_t len = native->field_lengths[i];
				const bin_byte_t* ptr = native->field_values[i];
				const bin_size_t len32 = static_cast<bin_size_t>(len);
				const bin_byte_t* len_bytes = reinterpret_cast<const bin_byte_t*>(&len32);
				result.data.insert(result.data.end(), len_bytes, len_bytes + sizeof(bin_size_t));
				if (ptr && len > 0)
				{
					result.data.insert(result.data.end(), ptr, ptr + len);
				}
			}
			return result;
		}

		template <typename... Args>
		void unpack(Args&... args) const
		{
			const size_t field_count = sizeof...(Args);
			Record rec(0, nullptr, field_count);

			size_t offset = 0;
			for (size_t i = 0; i < field_count; ++i)
			{
				if (offset + sizeof(bin_size_t) > data.size())
					throw std::runtime_error("Corrupt packed data: truncated field length at field " + std::to_string(i));

				bin_size_t len = 0;
				std::memcpy(&len, data.data() + offset, sizeof(bin_size_t));
				offset += sizeof(bin_size_t);

				if (offset + len > data.size())
					throw std::runtime_error("Corrupt packed data: truncated field payload at field " + std::to_string(i));

				rec.set(static_cast<Record::IndexType>(i), data.data() + offset, len);
				offset += len;
			}

			size_t idx = 0;
			detail::get_field_at_idx(rec, idx, args...);
		}

		operator const bin_byte_t* () const BIN_NOEXCEPT { return data.data(); }
	};

	namespace detail
	{
		template <>
		struct FieldSerializer<SerializableString> {
			static void set(Record& rec, size_t idx, const SerializableString& val)
			{
				rec.set(idx, reinterpret_cast<const bin_byte_t*>(val.c_str()), val.size() + 1);
			}

			static void get(const Record& rec, size_t idx, SerializableString& val)
			{
				const bin_byte_t* ptr = rec.get(idx);
				val.value = ptr ? reinterpret_cast<const char*>(ptr) : "";
			}
		};

		template <>
		struct FieldSerializer<SerializableBytes> {
			static void set(Record& rec, size_t idx, const SerializableBytes& val)
			{
				rec.set(idx, val.data_ptr(), val.size());
			}

			static void get(const Record& rec, size_t idx, SerializableBytes& val)
			{
				const bin_byte_t* ptr = rec.get(idx);
				size_t len = rec.native_handle()->field_lengths[idx];
				val.data.assign(ptr, ptr + len);
			}
		};
	}

	template <typename T>
	inline Record& set_object(Record& rec, const char* field_name, const T& obj)
	{
		static_assert(std::is_trivially_copyable<T>::value, "Type T must be trivially copyable to serialize directly.");
		return rec.set(field_name, reinterpret_cast<const bin_byte_t*>(&obj), sizeof(T));
	}

	template <typename T>
	inline T get_object(const Record& rec, const char* field_name)
	{
		static_assert(std::is_trivially_copyable<T>::value, "Type T must be trivially copyable to serialize directly.");
		return rec.get<T>(field_name);
	}

	using BinString = SerializableString;
	using BinBytes = SerializableBytes;

	template <int size = 128>
	struct FixedString {
		char data[size]{};

		FixedString() = default;
		FixedString(const char* s) {
			strncpy_s(data, sizeof(data), s, _TRUNCATE);
			data[sizeof(data) - 1] = '\0';
		}
		const char* c_str() const { return data; }
	};

	template <int size = 128>
	struct FixedBytes {
		bin_byte_t data[size]{};

		FixedBytes() = default;
		FixedBytes(const bin_byte_t* s) {
			memcpy_s(data, sizeof(data), s, _TRUNCATE);
			data[sizeof(data) - 1] = '\0';
		}
	};
}

#define BIN_STRUCT(Type, ...)                                                   \
namespace bin                                                                   \
{                                                                               \
	template<>                                                                  \
	struct StructTraits<Type>                                                   \
	{                                                                           \
		static Record to_record(bin_id_t id, const Type& obj)                   \
		{                                                                       \
			auto field_strs = ::bin::detail::parse_field_names(#__VA_ARGS__);  \
			auto field_ptrs = ::bin::detail::to_char_ptrs(field_strs);          \
			Record rec(id, field_ptrs.data(), field_ptrs.size());               \
			apply_to_record(rec, obj);                                          \
			return rec;                                                         \
		}                                                                       \
		static Type from_record(const Record& rec)                              \
		{                                                                       \
			Type obj{};                                                         \
			apply_from_record(rec, obj);                                        \
			return obj;                                                         \
		}                                                                       \
	private:                                                                    \
		static void apply_to_record(Record& rec, const Type& obj)               \
		{                                                                       \
			size_t idx = 0;                                                     \
			::bin::detail::set_field_at_idx(rec, idx, __VA_ARGS__);             \
		}                                                                       \
		static void apply_from_record(const Record& rec, Type& obj)             \
		{                                                                       \
			size_t idx = 0;                                                     \
			::bin::detail::get_field_at_idx(rec, idx, __VA_ARGS__);             \
		}                                                                       \
	};                                                                          \
}

#define BIN_ID(obj_id) static constexpr bin_id_t bin_id = obj_id;
#define BIN_DECLARE_SCHEMA static const ::bin::Schema bin_schema;

#define BIN_FACILITY_DECLARATIONS(Type)                 \
::bin::Record to_record() const;                        \
static Type from_record(const ::bin::Record& record);

#define BIN_FACILITY_DEFINITIONS(Type)                          \
::bin::Record Type::to_record() const                           \
{                                                               \
	return ::bin::StructTraits<Type>::to_record(bin_id, *this); \
}                                                               \
Type Type::from_record(const ::bin::Record& record)             \
{                                                               \
	return ::bin::StructTraits<Type>::from_record(record);      \
}

#define BIN_DEFINE_SCHEMA(Type) const ::bin::Schema Type::bin_schema = Type{}.to_record();

#endif /* BIN_HPP */