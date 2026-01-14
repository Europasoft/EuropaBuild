// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <cstdint>

namespace JSONTextUtils
{
	typedef std::string str_t;
	typedef std::string_view str_view;
	typedef char char_t;

	constexpr auto STC_SBR_L = '[';
	constexpr auto STC_SBR_R = ']';
	constexpr auto STC_CBR_L = '{';
	constexpr auto STC_CBR_R = '}';
	constexpr auto STC_CL = ':';
	constexpr auto STC_CM = ',';
	constexpr auto STR_DELIM = '"';

	uint8_t ctu8(char_t c);

	uint32_t numBytesChar(char_t c);

	bool isNumerical(char_t c);
	bool isWhitespaceChar(char_t c);
	bool isStructuralChar(char_t c);

	bool isUnicodeChar(char_t c);
	bool isControlChar(char_t c);

	int32_t digitFromChar(char_t c);
	bool isDigitValid(int32_t d);

	bool isliteralBooleanStr(size_t i, str_view text);
	str_view literalBooleanValue(size_t& i, str_view text);

	bool isLiteralNullStr(size_t i, str_view text);
	str_view literalNullValue(size_t& i);

	// converts a single UTF-8 codepoint to UTF-32BE
	uint32_t utf8to32be(str_view fullString, size_t& startIndexInOut);
	// swaps the endianness of a 32-bit number (e.g. a UTF-32 codepoint)
	uint32_t swapEndian32(uint32_t u);
	// converts a whole string from UTF-8 to UTF-32
	std::u32string utf8to32str(str_view s, bool littleEndian = false);
	void test_utf8to32();

}

namespace JSON 
{
	using str_view = JSONTextUtils::str_view;
	using str_t = JSONTextUtils::str_t;
	class Object;
	using ObjectPtr = std::shared_ptr<Object>;
	using SubobjectIterator = std::vector<ObjectPtr>::const_iterator;
	using std::uint32_t;

	enum class ObjectType { Undefined, Object, Array, String, Number, Boolean, Null };

	class Object
	{
	public:
		Object() { reset(); }
		Object(JSONTextUtils::str_view name, JSONTextUtils::str_view value) : name{ name }, value{ value }, type{ ObjectType::Undefined } {};
		Object(ObjectType t) : type{ t } {};
		Object(ObjectType t, JSONTextUtils::str_view n) : type{ t }, name{ n } {};
		Object(ObjectType t, JSONTextUtils::str_view n, JSONTextUtils::str_view v) : type{ t }, name{ n }, value{ v } {};
		void reset();

		bool isNamed() const noexcept;
		str_view getName() const noexcept;
		bool isValue() const noexcept;
		bool isContainer() const noexcept;
		bool isArray() const noexcept;
		bool hasNamedSubobject(str_view name) const noexcept;
		JSONTextUtils::str_view getValue() const noexcept;
		ObjectType getType() const noexcept;
		size_t size() const noexcept;
		void set(ObjectType t, JSONTextUtils::str_view v);

		JSONTextUtils::str_t toString(bool readable = true) const noexcept;
		
		Object& operator[](size_t i);
		const Object& operator[](size_t i) const;
		const ObjectPtr getNamedSubobject(str_view name) const noexcept;
		ObjectPtr getNamedSubobject(str_view name) noexcept;
		const Object& operator[](str_view name) const;
		Object& operator[](str_view name);
		std::map<std::string, ObjectPtr> map() const;
		std::vector<std::string> vector() const;
		
	private:
		JSONTextUtils::str_t getSubobjectsAsStringInternal(size_t& depth, bool readable) const;
		SubobjectIterator getNamedSubobjectIteratorInternal(str_view name) const noexcept;

	protected:
		JSONTextUtils::str_t name;
		ObjectType type;
		std::vector<ObjectPtr> subobjects;
		JSONTextUtils::str_t value;
	public:
		SubobjectIterator begin() const noexcept { return subobjects.begin(); }
		SubobjectIterator end() const noexcept { return subobjects.end(); }
		void push_back(ObjectPtr subobject);
	};

	enum class Result : uint32_t
	{
		OK = 0,
		Error_Lexer_InvalidEncoding				= 101,	// unsupported character encoding
		Error_Lexer_IllegalToken				= 102,	// disallowed ASCII in structural text
		Error_Lexer_IllegalTokenMultiByte		= 103,	// multi-byte character in structural text
		Error_Lexer_IncompleteUnicodeInString	= 104,	// incomplete or corrupted multi-byte codepoint
		Error_Parser_NoTokens					= 201,	// lexer succeeded, but no tokens were passed to the parser
		Error_Parser_UndefinedToken				= 202,	// token has undefined type
		Error_Parser_EmptyToken					= 203,	// token has no content
		Error_Parser_InvalidRoot				= 204,	// text root must be a lone value, an unnamed object, or an unnamed array
		Error_Parser_IllegalTokenAtStart		= 205,	// incorrect structural character at start
		Error_Parser_IllegalClosingToken		= 206,	// incorrect token at end of container
		Error_Parser_ExpectedTokenAfterValue	= 207,	// expected additional tokens following string
		Error_Parser_NamedValueInArray			= 208,	// key-value pair inside array
		Error_Parser_LoneValue					= 209,	// unnamed value not allowed outside arrays
		Error_Parser_InvalidKeyValuePair		= 210,	// invalid name or value
		Error_Parser_MissingSeparator			= 211,	// expected comma before token
		Error_File								= 301,	// failure to read file
	};
	#define RETURN_ERROR(err) return JSON::Result::err
	#define RETURN_ERROR_IF(condition, err) if (condition) { RETURN_ERROR(err); }

	Result load(JSONTextUtils::str_view text, Object& objectOut);
	Result loadFromFile(JSONTextUtils::str_view filePath, Object& objectOut);
	void testLexer(JSONTextUtils::str_view filePath);

}

namespace JSON::Internals
{
	enum class TokenType { Undefined, Structural, String, Number, Boolean, Null };
	enum class StructuralTokenType { NotStructural, ObjectBegin, ObjectEnd, ArrayBegin, ArrayEnd, KeyValueDelim, MemberDelim };
	class Token 
	{ 
	public:
		TokenType type; 
		str_t data;
		Token() { reset(); }
		Token(TokenType t, str_view d) : type{ t }, data{ d } {};
		void reset() { data.clear(); type = TokenType::Undefined; }
	};

	JSON::Result lex(str_view text, std::vector<Token>& tokens);
	JSON::Result parse(const std::vector<Token>& tokens, JSON::Object& obj);
	
	bool getFileSize(str_view filePath, size_t& fileSizeOut);
	bool openFile(str_view filePath, std::ifstream& fileStreamOut, size_t& fileSizeOut);

	str_view tokenTypeToString(TokenType t);
	JSON::ObjectType valueTokenToObjType(const Token& token);
}

namespace XMLTextUtils
{
	using str_t = JSONTextUtils::str_t;
	using str_view = JSONTextUtils::str_view;
	using char_t = JSONTextUtils::char_t;

	constexpr auto STC_CHEVRON_L = '<';
	constexpr auto STC_CHEVRON_R = '>';
	constexpr auto STC_SLASH = '/';
	constexpr auto STC_EQUALS = '=';
	constexpr auto STR_DELIM = '"';
	constexpr auto STC_QUESTION = '?';


}

namespace XML
{

}

namespace XML::Internals
{

}




