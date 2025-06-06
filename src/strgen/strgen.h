/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file strgen.h Structures related to strgen. */

#ifndef STRGEN_H
#define STRGEN_H

#include "../core/string_consumer.hpp"
#include "../language.h"
#include "../string_type.h"
#include "../3rdparty/fmt/format.h"

#include <unordered_map>
#include <array>

/** Container for the different cases of a string. */
struct Case {
	uint8_t caseidx;       ///< The index of the case.
	std::string string; ///< The translation of the case.

	Case(uint8_t caseidx, std::string_view string);
};

/** Information about a single string. */
struct LangString {
	std::string name;       ///< Name of the string.
	std::string english;    ///< English text.
	std::string translated; ///< Translated text.
	size_t index;           ///< The index in the language file.
	size_t line;            ///< Line of string in source-file.
	std::vector<Case> translated_cases; ///< Cases of the translation.

	LangString(std::string_view name, std::string_view english, size_t index, size_t line);
	void FreeTranslation();
};

/** Information about the currently known strings. */
struct StringData {
	std::vector<std::shared_ptr<LangString>> strings; ///< List of all known strings.
	std::unordered_map<std::string, std::shared_ptr<LangString>, StringHash, std::equal_to<>> name_to_string; ///< Lookup table for the strings.
	size_t tabs;          ///< The number of 'tabs' of strings.
	size_t max_strings;   ///< The maximum number of strings.
	size_t next_string_id;///< The next string ID to allocate.

	StringData(size_t tabs);
	void FreeTranslation();
	void Add(std::shared_ptr<LangString> ls);
	LangString *Find(std::string_view s);
	uint32_t Version() const;
	size_t CountInUse(size_t tab) const;
};

/** Helper for reading strings. */
struct StringReader {
	StringData &data; ///< The data to fill during reading.
	const std::string file; ///< The file we are reading.
	bool master;      ///< Are we reading the master file?
	bool translation; ///< Are we reading a translation, implies !master. However, the base translation will have this false.

	StringReader(StringData &data, const std::string &file, bool master, bool translation);
	virtual ~StringReader() = default;
	void HandleString(std::string_view str);

	/**
	 * Read a single line from the source of strings.
	 * @return The line, or std::nullopt if at the end of the file.
	 */
	virtual std::optional<std::string> ReadLine() = 0;

	/**
	 * Handle the pragma of the file.
	 * @param str    The pragma string to parse.
	 */
	virtual void HandlePragma(std::string_view str, LanguagePackHeader &lang);

	/**
	 * Start parsing the file.
	 */
	virtual void ParseFile();
};

/** Base class for writing the header, i.e. the STR_XXX to numeric value. */
struct HeaderWriter {
	/**
	 * Write the string ID.
	 * @param name     The name of the string.
	 * @param stringid The ID of the string.
	 */
	virtual void WriteStringID(const std::string &name, size_t stringid) = 0;

	/**
	 * Finalise writing the file.
	 * @param data The data about the string.
	 */
	virtual void Finalise(const StringData &data) = 0;

	/** Especially destroy the subclasses. */
	virtual ~HeaderWriter() = default;

	void WriteHeader(const StringData &data);
};

/** Base class for all language writers. */
struct LanguageWriter {
	/**
	 * Write the header metadata. The multi-byte integers are already converted to
	 * the little endian format.
	 * @param header The header to write.
	 */
	virtual void WriteHeader(const LanguagePackHeader *header) = 0;

	/**
	 * Write a number of bytes.
	 * @param buffer The buffer to write.
	 */
	virtual void Write(std::string_view buffer) = 0;

	/**
	 * Finalise writing the file.
	 */
	virtual void Finalise() = 0;

	/** Especially destroy the subclasses. */
	virtual ~LanguageWriter() = default;

	virtual void WriteLength(size_t length);
	virtual void WriteLang(const StringData &data);
};

struct CmdStruct;

struct CmdPair {
	const CmdStruct *cmd;
	std::string param;
};

struct ParsedCommandStruct {
	std::vector<CmdPair> non_consuming_commands;
	std::array<const CmdStruct*, 32> consuming_commands{ nullptr }; // ordered by param #
};

const CmdStruct *TranslateCmdForCompare(const CmdStruct *a);
ParsedCommandStruct ExtractCommandString(std::string_view s, bool warnings);

void StrgenWarningI(const std::string &msg);
void StrgenErrorI(const std::string &msg);
[[noreturn]] void StrgenFatalI(const std::string &msg);
#define StrgenWarning(format_string, ...) StrgenWarningI(fmt::format(FMT_STRING(format_string) __VA_OPT__(,) __VA_ARGS__))
#define StrgenError(format_string, ...) StrgenErrorI(fmt::format(FMT_STRING(format_string) __VA_OPT__(,) __VA_ARGS__))
#define StrgenFatal(format_string, ...) StrgenFatalI(fmt::format(FMT_STRING(format_string) __VA_OPT__(,) __VA_ARGS__))
std::optional<std::string_view> ParseWord(StringConsumer &consumer);

/** Global state shared between strgen.cpp, game_text.cpp and strgen_base.cpp */
struct StrgenState {
	std::string file = "(unknown file)"; ///< The filename of the input, so we can refer to it in errors/warnings
	size_t cur_line = 0; ///< The current line we're parsing in the input file
	size_t errors = 0;
	size_t warnings = 0;
	bool show_warnings = false;
	bool annotate_todos = false;
	bool translation = false; ///< Is the current file actually a translation or not
	LanguagePackHeader lang; ///< Header information about a language.
};
extern StrgenState _strgen;

#endif /* STRGEN_H */
