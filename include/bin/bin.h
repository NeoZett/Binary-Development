// Copyright (c) 2026 Erik-Neo Östlund-Zetterberg
// See the license in the accompanying LICENSE.md file at the github repository:
// https://github.com/NeoZett/Binary-Development

#ifndef BIN_H
#define BIN_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

/* Export / Import visibility macros */
#if defined(_MSC_VER) && defined(BIN_EXPORTS)
#define BIN_EXPORT __declspec(dllexport)
#elif defined(_MSC_VER) && defined(BIN_DLL)
#define BIN_EXPORT __declspec(dllimport)
#elif defined(BIN_EXPORTS)
#define BIN_EXPORT __attribute__((visibility("default")))
#else
#define BIN_EXPORT
#endif

/* C / C++ Interoperability & Inline semantics */
#ifdef __cplusplus
#define BIN_API extern "C"
#define BIN_INLINE inline
#else
#define BIN_API
#define BIN_INLINE static inline
#endif

/* Debug breakpoints */
#if defined(_MSC_VER)
#define BIN_BREAKPOINT __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define BIN_BREAKPOINT __builtin_trap()
#else
#define BIN_BREAKPOINT assert(0)
#endif

/* No-throw annotations across compilers */
#if defined(__cplusplus)
#define BIN_NOEXCEPT noexcept
#else
/* A trailing __attribute__((nothrow)) is only valid on a function *declaration* in C;
 * GCC/Clang reject it in this position on a function *definition* (every function in
 * this header is BIN_INLINE, i.e. defined right here), so it is intentionally omitted
 * for C rather than only being usable from bin.hpp's C++ wrapper. */
#define BIN_NOEXCEPT
#endif

 /* Primitive Type Aliases */
typedef uint8_t bin_byte_t;
typedef int32_t bin_id_t;
typedef uint32_t bin_size_t;

/* Cross-platform strdup abstraction */
BIN_API BIN_INLINE char* bin_strdup(const char* s) BIN_NOEXCEPT
{
	if (!s) return NULL;
	size_t len = strlen(s) + 1;
	char* dup = (char*)malloc(len);
	if (dup) memcpy(dup, s, len);
	return dup;
}

/* Low-Level Stream I/O Helpers */

BIN_API BIN_INLINE bool bin_write_bytes(FILE* file, const void* data, size_t size) BIN_NOEXCEPT
{
	if (!file || !data || size == 0) return false;
	return fwrite(data, 1, size, file) == size;
}

BIN_API BIN_INLINE bool bin_read_bytes(FILE* file, void* data, size_t size) BIN_NOEXCEPT
{
	if (!file || !data || size == 0) return false;
	return fread(data, 1, size, file) == size;
}

/* Record & Schema Structures */

typedef struct bin_record
{
	bin_id_t id;
	size_t field_count;
	char** field_names;
	bin_byte_t** field_values;
	size_t* field_lengths;
} bin_record_t, bin_schema_t;

BIN_API BIN_INLINE void bin_record_init(bin_record_t* self, bin_id_t id, const char* const* fields, size_t count) BIN_NOEXCEPT
{
	if (!self) return;
	memset(self, 0, sizeof(bin_record_t));
	self->id = id;
	self->field_count = count;
	if (count > 0)
	{
		self->field_names = (char**)calloc(count, sizeof(char*));
		self->field_values = (bin_byte_t**)calloc(count, sizeof(bin_byte_t*));
		self->field_lengths = (size_t*)calloc(count, sizeof(size_t));
		if (!self->field_names || !self->field_values || !self->field_lengths) return;
		for (size_t i = 0; i < count; ++i) {
			if (fields && fields[i]) {
				self->field_names[i] = bin_strdup(fields[i]);
			}
		}
	}
}

BIN_API BIN_INLINE void bin_record_free(bin_record_t* record) BIN_NOEXCEPT
{
	if (!record) return;
	if (record->field_names)
	{
		for (size_t i = 0; i < record->field_count; i++)
		{
			if (record->field_names[i])
			{
				free(record->field_names[i]);
				record->field_names[i] = NULL;
			}
		}
		free(record->field_names);
		record->field_names = NULL;
	}
	if (record->field_values)
	{
		for (size_t i = 0; i < record->field_count; i++)
		{
			if (record->field_values[i])
			{
				free(record->field_values[i]);
				record->field_values[i] = NULL;
			}
		}
		free(record->field_values);
		record->field_values = NULL;
	}
	if (record->field_lengths)
	{
		free(record->field_lengths);
		record->field_lengths = NULL;
	}
	record->field_count = 0;
	memset(record, 0, sizeof(bin_record_t));
}

BIN_API BIN_INLINE void bin_record_move(bin_record_t* dest,bin_record_t* src) BIN_NOEXCEPT
{
	if (!dest || !src || dest == src) return;
	*dest = *src;
	memset(src, 0, sizeof(bin_record_t));
}

BIN_API BIN_INLINE bool bin_record_copy(bin_record_t* dest, const bin_record_t* src) BIN_NOEXCEPT
{
	if (!dest || !src) return false;
	memset(dest, 0, sizeof(bin_record_t));
	dest->id = src->id;
	dest->field_count = src->field_count;
	if (src->field_count == 0) return true;

	dest->field_names = (char**)calloc(src->field_count, sizeof(char*));
	dest->field_values = (bin_byte_t**)calloc(src->field_count, sizeof(bin_byte_t*));
	dest->field_lengths = (size_t*)calloc(src->field_count, sizeof(size_t));

	if (!dest->field_names || !dest->field_values || !dest->field_lengths)
	{
		bin_record_free(dest);
		return false;
	}

	for (size_t i = 0; i < src->field_count; ++i)
	{
		if (src->field_names && src->field_names[i])
			dest->field_names[i] = bin_strdup(src->field_names[i]);
		if (src->field_lengths)
			dest->field_lengths[i] = src->field_lengths[i];
		if (src->field_values && src->field_values[i] && dest->field_lengths[i] > 0)
		{
			dest->field_values[i] = (bin_byte_t*)malloc(dest->field_lengths[i]);
			if (dest->field_values[i])
				memcpy(dest->field_values[i], src->field_values[i], dest->field_lengths[i]);
		}
	}
	return true;
}

BIN_API BIN_INLINE bool bin_record_set_field_at(bin_record_t* self, size_t index, const bin_byte_t* value, size_t len) BIN_NOEXCEPT
{
	if (!self || index >= self->field_count) return false;
	if (self->field_values[index])
	{
		free(self->field_values[index]);
		self->field_values[index] = NULL;
	}
	if (value && len > 0)
	{
		self->field_values[index] = (bin_byte_t*)malloc(len);
		if (!self->field_values[index]) return false;
		memcpy(self->field_values[index], value, len);
		self->field_lengths[index] = len;
	}
	else
	{
		self->field_values[index] = NULL;
		self->field_lengths[index] = 0;
	}
	return true;
}

BIN_API BIN_INLINE bool bin_record_set_field(bin_record_t* self, const char* name, const bin_byte_t* value, size_t len) BIN_NOEXCEPT
{
	if (!self || !name) return false;
	for (size_t i = 0; i < self->field_count; ++i)
	{
		if (self->field_names[i] && strcmp(self->field_names[i], name) == 0)
			return bin_record_set_field_at(self, i, value, len);
	}
	return false;
}

BIN_API BIN_INLINE bool bin_record_get_field_at(const bin_record_t* self, size_t index, bin_byte_t* out_buffer) BIN_NOEXCEPT
{
	if (!self || !out_buffer || index >= self->field_count || !self->field_values[index]) return false;
	memcpy(out_buffer, self->field_values[index], self->field_lengths[index]);
	return true;
}

BIN_API BIN_INLINE bool bin_record_get_field(const bin_record_t* self, const char* name, bin_byte_t* out_buffer) BIN_NOEXCEPT
{
	if (!self || !name) return false;
	for (size_t i = 0; i < self->field_count; ++i)
	{
		if (self->field_names[i] && strcmp(self->field_names[i], name) == 0)
			return bin_record_get_field_at(self, i, out_buffer);
	}
	return false;
}

/* Record Serialization */

BIN_API BIN_INLINE bool bin_record_write(FILE* file, const bin_record_t* record) BIN_NOEXCEPT
{
	if (!file || !record) return false;
	if (!bin_write_bytes(file, &record->id, sizeof(bin_id_t))) return false;
	for (size_t i = 0; i < record->field_count; ++i)
	{
		bin_size_t size = (bin_size_t)record->field_lengths[i];
		if (!bin_write_bytes(file, &size, sizeof(bin_size_t))) return false;
		if (size > 0 && !bin_write_bytes(file, record->field_values[i], size)) return false;
	}
	return true;
}

BIN_API BIN_INLINE bool bin_record_read(FILE* file, bin_record_t* record) BIN_NOEXCEPT
{
	if (!file || !record) return false;
	for (size_t i = 0; i < record->field_count; ++i)
	{
		bin_size_t size;
		if (!bin_read_bytes(file, &size, sizeof(bin_size_t))) return false;
		record->field_lengths[i] = size;

		if (record->field_values[i]) {
			free(record->field_values[i]);
			record->field_values[i] = NULL;
		}

		if (size > 0)
		{
			record->field_values[i] = (bin_byte_t*)malloc(size);
			if (!record->field_values[i]) return false;
			if (!bin_read_bytes(file, record->field_values[i], size)) return false;
		}
	}
	return true;
}

/* Stream Abstraction, Reader & Writer */

typedef struct bin_stream
{
	const char* path;
	FILE* file;
} bin_stream_t;

BIN_API BIN_INLINE bool bin_stream_open(bin_stream_t* self, const char* path, const char* mode) BIN_NOEXCEPT
{
	if (!self || !path || !mode) return false;
	self->path = path;
#if defined(_MSC_VER)
	return (fopen_s(&self->file, path, mode) == 0);
#else
	self->file = fopen(path, mode);
	return (self->file != NULL);
#endif
}

BIN_API BIN_INLINE bool bin_stream_close(bin_stream_t* self) BIN_NOEXCEPT
{
	if (self && self->file != NULL)
	{
		fclose(self->file);
		self->file = NULL;
		return true;
	}
	return false;
}

typedef struct bin_catalog
{
	bin_schema_t* schemas;
	size_t schema_count;
} bin_catalog_t;

typedef struct bin_reader
{
	bin_stream_t stream;
	bin_catalog_t catalog;
	bin_record_t* records;
	size_t record_count;
	size_t record_capacity;
	bool has_completed;
} bin_reader_t;

BIN_API BIN_INLINE bool bin_reader_init(bin_reader_t* self, const char* path, const bin_catalog_t* catalog) BIN_NOEXCEPT
{
	if (!self || !path || !catalog) return false;
	memset(self, 0, sizeof(bin_reader_t));
	self->catalog = *catalog;
	if (!bin_stream_open(&self->stream, path, "rb")) return false;
	self->has_completed = false;
	return true;
}

BIN_API BIN_INLINE bool bin_reader_close(bin_reader_t* self) BIN_NOEXCEPT
{
	if (!self) return false;
	return bin_stream_close(&self->stream);
}

BIN_API BIN_INLINE bool bin_reader_execute(bin_reader_t* self) BIN_NOEXCEPT
{
	if (!self || !self->stream.file) return false;
	if (self->has_completed) return true;

	bin_id_t id;
	while (bin_read_bytes(self->stream.file, &id, sizeof(bin_id_t)))
	{
		bool matched = false;
		for (size_t i = 0; i < self->catalog.schema_count; ++i)
		{
			if (self->catalog.schemas[i].id == id)
			{
				bin_record_t record;
				if (!bin_record_copy(&record, &self->catalog.schemas[i]))
				{
					bin_reader_close(self);
					return false;
				}

				if (!bin_record_read(self->stream.file, &record))
				{
					bin_record_free(&record);
					bin_reader_close(self);
					return false;
				}

				if (self->record_count >= self->record_capacity)
				{
					size_t new_cap = self->record_capacity == 0 ? 8 : self->record_capacity * 2;
					bin_record_t* new_recs = (bin_record_t*)realloc(self->records, new_cap * sizeof(bin_record_t));
					if (!new_recs)
					{
						bin_record_free(&record);
						bin_reader_close(self);
						return false;
					}
					self->records = new_recs;
					self->record_capacity = new_cap;
				}

				if (self->record_count < self->record_capacity)
				{
					bin_record_move(
						&self->records[self->record_count++],
						&record
					);
				}
				else
				{
					bin_record_free(&record);
					bin_reader_close(self);
					return false;
				}

				matched = true;
				break;
			}
		}
		if (!matched)
		{
			bin_reader_close(self);
			return false;
		}
	}
	self->has_completed = true;
	bin_reader_close(self);
	return true;
}

BIN_API BIN_INLINE void bin_reader_deinit(bin_reader_t* self) BIN_NOEXCEPT
{
	if (self)
	{
		if (self->records)
		{
			for (size_t i = 0; i < self->record_count; ++i)
			{
				bin_record_free(&self->records[i]);
			}
			free(self->records);
		}
		bin_stream_close(&self->stream);
		memset(self, 0, sizeof(bin_reader_t));
	}
}

typedef struct bin_writer
{
	bin_stream_t stream;
	bin_record_t* records;
	size_t record_count;
	bool has_completed;
} bin_writer_t;

BIN_API BIN_INLINE void bin_writer_deinit(bin_writer_t* self) BIN_NOEXCEPT;

BIN_API BIN_INLINE bool bin_writer_init(bin_writer_t* self, const char* path, const bin_record_t* records, size_t record_count) BIN_NOEXCEPT
{
	if (!self || !path || (!records && record_count > 0)) return false;
	memset(self, 0, sizeof(bin_writer_t));
	if (record_count > 0)
	{
		self->records = (bin_record_t*)calloc(record_count, sizeof(bin_record_t));
		if (!self->records) return false;
		for (size_t i = 0; i < record_count; ++i)
		{
			if (!bin_record_copy(&self->records[i], &records[i]))
			{
				bin_writer_deinit(self);
				return false;
			}
		}
	}
	self->record_count = record_count;
	if (!bin_stream_open(&self->stream, path, "wb")) return false;
	self->has_completed = false;
	return true;
}

BIN_API BIN_INLINE bool bin_writer_close(bin_writer_t* self) BIN_NOEXCEPT
{
	if (!self) return false;
	return bin_stream_close(&self->stream);
}

BIN_API BIN_INLINE bool bin_writer_execute(bin_writer_t* self) BIN_NOEXCEPT
{
	if (!self || !self->stream.file) return false;
	if (self->has_completed) return true;
	for (size_t i = 0; i < self->record_count; ++i)
	{
		if (!bin_record_write(self->stream.file, &self->records[i]))
		{
			bin_writer_close(self);
			return false;
		}
	}
	self->has_completed = true;
	bin_writer_close(self);
	return true;
}

BIN_API BIN_INLINE void bin_writer_deinit(bin_writer_t* self) BIN_NOEXCEPT
{
	if (!self) return;
	if (self->records)
	{
		for (size_t i = 0; i < self->record_count; ++i)
		{
			bin_record_free(&self->records[i]);
		}
		free(self->records);
	}
	bin_stream_close(&self->stream);
	memset(self, 0, sizeof(bin_writer_t));
}

#endif /* BIN_H */