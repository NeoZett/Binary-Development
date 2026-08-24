#pragma once

#include <bin/binvm.hpp>

/* ---- Todo ---- */

struct Todo
{
	BIN_ID(101);
	BIN_DECLARE_SCHEMA;

	/* A written binary struct uses the serializable
	 * bytes or string directly because it doesn't take
	 * any unnecessary space. An object that is
	 * serialized using the serializable bytes, however,
	 * require the binary struct to use a fixed-size
	 * string or bytes instead if it is stored inside
	 * a vector before it is saved.
	 */

	bin::FixedString<> name;
	bin::FixedString<> description;
	bin::FixedString<> when;

	BIN_FACILITY_DECLARATIONS(Todo);
};

BIN_STRUCT(Todo, obj.name, obj.description, obj.when);
BIN_FACILITY_DEFINITIONS(Todo);
BIN_DEFINE_SCHEMA(Todo);