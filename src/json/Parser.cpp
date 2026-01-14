// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "Parser.h"

#include <limits.h>
#include <iostream>
#include <format>
#include <cassert>
#include <iomanip>
#include <stack>
#include <exception>
#include <algorithm>
#include <utility>


namespace JSONTextUtils
{
	uint8_t ctu8(char_t c)
	{
		return static_cast<uint8_t>(c); // (signed) char to unsigned int
	}

	uint32_t numBytesChar(char_t c)
	{
		const auto uc = ctu8(c);
		if (uc <= 127) { return 1; } // U+0000 - U+007F (ASCII)
		else if ((uc & 0xE0) == 0xC0) { return 2; } // U+0080 - U+07FF
		else if ((uc & 0xF0) == 0xE0) { return 3; } // U+0800 - U+FFFF
		else if ((uc & 0xF8) == 0xF0) { return 4; } // U+10000 - U+10FFFF
		else { return 0; } // unknown encoding
	}

	bool isNumerical(char_t c)
	{
		return isDigitValid(digitFromChar(c)) || // base 10 digits
			ctu8(c) == 69 || ctu8(c) == 101 || // E, e
			ctu8(c) == 43 || ctu8(c) == 45 || ctu8(c) == 46; // +, -, .
	}

	bool isWhitespaceChar(char_t c)
	{
		const auto uc = ctu8(c);
		return uc == 0x20 || uc == 0x09 || uc == 0x0A || uc == 0x0D; // space, tab, newline, carriage return
	}

	bool isStructuralChar(char_t c)
	{
		return c == STC_SBR_L || c == STC_SBR_R || c == STC_CBR_L || c == STC_CBR_R ||
			c == STC_CL || c == STC_CM;
	}

	bool isUnicodeChar(char_t c)
	{
		return numBytesChar(c) > 1;
	}

	bool isControlChar(char_t c)
	{
		return ctu8(c) <= 31;
	}

	int32_t digitFromChar(char_t c)
	{
		return static_cast<int32_t>(ctu8(c)) - 48;
	}

	bool isDigitValid(int32_t d)
	{
		return d >= 0 && d <= 9;
	}

	bool isliteralBooleanStr(size_t i, str_view text)
	{
		const auto len = text.length() - i;
		return (len > 3 && text.substr(i, 4) == "true") || (len > 4 && text.substr(i, 5) == "false");
	}

	str_view literalBooleanValue(size_t& i, str_view text)
	{
		auto len = text.length() - i;
		assert(len >= 4);
		len = (ctu8(text[i]) == 0x74) ? 4 : 5;
		auto strBool = text.substr(i, len);
		i += len - 1;
		return strBool;
	}

	bool isLiteralNullStr(size_t i, str_view text)
	{
		return (text.length() - i > 3) && text.substr(i, 4) == "null";
	}

	str_view literalNullValue(size_t& i)
	{
		i += 3;
		return "null";
	}

	uint32_t utf8to32be(str_view fullString, size_t& startIndexInOut)
	{
		const auto numBytes = numBytesChar(ctu8(fullString[startIndexInOut]));
		if (numBytes == 1)
		{
			return ctu8(fullString[startIndexInOut++]);
		}
		assert(numBytes > 0 && numBytes < 5 && "Invalid UTF-8 start byte");
		assert(startIndexInOut + numBytes <= fullString.size() && "Invalid or incomplete UTF-8 codepoint");

		const auto sf = 24 + numBytes;
		uint32_t value = ((ctu8(fullString[startIndexInOut]) << sf) >> sf);
		value = (value << (6 * (numBytes - 1)));
		for (uint32_t i = 1; i < numBytes; i++)
		{
			value |= (ctu8(fullString[startIndexInOut + i]) & 0x7F) << (6 * (numBytes - (i + 1)));
		}
		startIndexInOut += numBytes;
		return value;
	}

	uint32_t swapEndian32(uint32_t u)
	{
		return ((u & 0xFF) << 24) | ((u & 0xFF00) << 8) | ((u & 0xFF0000) >> 8) | ((u & 0xFF000000) >> 24);
	}

	std::u32string utf8to32str(str_view s, bool littleEndian)
	{
		std::u32string s32;
		size_t i = 0;
		if (!littleEndian)
		{
			while (i < s.size()) { s32 += utf8to32be(s, i); }
		}
		else
		{
			while (i < s.size()) { s32 += swapEndian32(utf8to32be(s, i)); }
		}
		return s32;
	}

	/*void test_utf8to32()
	{
		str_t vec("\xE0\xA4\xB9");
		std::cout << "\nUTF-8: ";
		for (auto c : vec)
		{
			std::cout << std::format("{:08b}", ctu8(c)) << ", ";
		}
		std::cout << "\n";
		size_t i = 0;
		while (i < vec.size())
		{
			uint32_t b = utf8to32be(vec, i);
			std::cout << "\nUTF-32BE: 0x" << std::hex << b << " (" << std::format("{:032b}", b) << ")";
		}
		std::cout << "\n";
		i = 0;
		while (i < vec.size())
		{
			uint32_t l = utf8to32le(vec, i);
			std::cout << "\nUTF-32LE: 0x" << std::hex << l << " (" << std::format("{:032b}", l) << ")";
		}
	}*/

	void test_utf8to32()
	{
		str_t s("\xE0\xA4\xB9");
		std::cout << "\nUTF-8: ";
		for (auto c : s)
		{
			std::cout << std::format("{:08b}", ctu8(c)) << ", ";
		}

		std::u32string s32be = utf8to32str(s);
		std::u32string s32le = utf8to32str(s, true);
		std::cout << "\nUTF-32BE:" << std::hex;
		for (auto c : s32be)
		{
			std::cout << " 0x" << static_cast<int32_t>(c);

		}
		std::cout << "\nUTF-32LE:";
		for (auto c : s32le)
		{
			std::cout << " 0x" << static_cast<int32_t>(c);
		}
	}
	
}

namespace JSON 
{
	using namespace JSON::Internals;

	Result load(str_view text, Object& objectOut)
	{
		std::vector<Token> tokens;
		Result result = lex(text, tokens);
		if (result != Result::OK) { return result; }
		return parse(tokens, objectOut);
	}

	Result loadFromFile(str_view filePath, Object& objectOut)
	{
		std::ifstream fs;
		size_t fileSize;
		if (!openFile(filePath, fs, fileSize)) { return Result::Error_File; }
		str_t file;
		file.resize(fileSize);
		fs.read(&file[0], file.length());

		return load(file, objectOut);
	}
	
		
	void testLexer(str_view filePath)
	{
		std::ifstream fs;
		size_t fileSize;
		if (!openFile(filePath, fs, fileSize)) { return; }
		str_t file;
		file.resize(fileSize);
		fs.read(&file[0], file.length());

		std::vector<Token> tokens;
		Result r = lex(file, tokens);
		for (auto& token : tokens) 
		{
			if (token.type == TokenType::String)
			{
				std::cout << " \"" << token.data << "\" ";
			}
			else if (token.type == TokenType::Structural)
			{
				std::cout << token.data << " \n ";
			}
			else
			{
				std::cout << "  " << token.data;
			}
		}
		std::cout << "\n\n";

		for (auto& token : tokens) { std::cout << "  " << token.data << "=" << tokenTypeToString(token.type); }
	}

	bool Object::isNamed() const noexcept
	{
		return !name.empty();
	}

	str_view Object::getName() const noexcept
	{
		return str_view(name);
	}

	bool Object::isValue() const noexcept
	{
		return !isContainer() && type != JSON::ObjectType::Undefined;
	}

	bool Object::isContainer() const noexcept
	{
		return type == JSON::ObjectType::Array || type == JSON::ObjectType::Object;
	}

	bool Object::isArray() const noexcept
	{
		return type == JSON::ObjectType::Array;
	}

	

	str_view Object::getValue() const noexcept
	{
		return value;
	}

	ObjectType Object::getType() const noexcept
	{
		return type;
	}

	size_t Object::size() const noexcept
	{
		if (isContainer())
		{
			return subobjects.size();
		}
		return getValue().size();
	}

	void Object::set(ObjectType t, str_view v)
	{
		type = t;
		value = v;
	}

	void Object::reset()
	{
		type = ObjectType::Undefined;
		subobjects.clear();
		name = value = str_t();
	}

	str_t Object::toString(bool readable) const noexcept
	{
		if (subobjects.empty()) { return str_t(); }
		size_t recursionDepth = -1;
		return subobjects[0]->getSubobjectsAsStringInternal(recursionDepth, readable);
	}

	Object& Object::operator[](size_t i)
	{
		return *subobjects[i].get();
	}
	const Object& Object::operator[](size_t i) const
	{
		return *subobjects[i].get();
	}

	SubobjectIterator Object::getNamedSubobjectIteratorInternal(str_view name) const noexcept
	{
		SubobjectIterator it = std::find_if(subobjects.begin(), subobjects.end(),
			[name](const JSON::ObjectPtr& obj)
			{
				return obj->name == name;
			});
		return it;
	}

	void Object::push_back(ObjectPtr subobject)
	{
		subobjects.push_back(subobject);
	}
	
	bool Object::hasNamedSubobject(str_view name) const noexcept
	{
		const SubobjectIterator it = getNamedSubobjectIteratorInternal(name);
		return (it != subobjects.end());
	}

	const ObjectPtr Object::getNamedSubobject(str_view name) const noexcept
	{
		const SubobjectIterator it = getNamedSubobjectIteratorInternal(name);
		return (it != subobjects.end()) ? *it : ObjectPtr(nullptr);
	}

	ObjectPtr Object::getNamedSubobject(str_view name) noexcept
	{
		SubobjectIterator it = getNamedSubobjectIteratorInternal(name);
		return (it != subobjects.end()) ? *it : ObjectPtr(nullptr);
	}

	const Object& Object::operator[](str_view name) const
	{
		return *getNamedSubobject(name);
	}

	Object& Object::operator[](str_view name)
	{
		return *getNamedSubobject(name);
	}

	std::vector<std::string> Object::vector() const
	{
		std::vector<std::string> vec;
		for (const ObjectPtr& sub : subobjects)
		{
			if (sub->isContainer() or sub->isNamed())
			{
				vec.push_back(sub->toString(true));
			}
			else if (sub->size() > 0)
			{
				vec.push_back(std::string(sub->getValue()));
			}
		}
		return vec;
	}

	std::map<std::string, ObjectPtr> Object::map() const
	{
		std::map<str_t, ObjectPtr> outMap;
		for (const ObjectPtr& sub : subobjects)
		{
			if (sub->isNamed())
				outMap[str_t(sub->getName())] = sub;
		}
		return outMap;
	}

    str_t Object::getSubobjectsAsStringInternal(size_t& depth, bool readable) const
    {
		const str_t nl = (readable) ? "\n" : "";

		depth++;
		str_t indent;
		if (readable) { for (size_t i = 0; i < depth; i++) { indent += "\t"; } }
		str_t s = indent;

		if (!name.empty()) { s += nl + indent + "\""+name+"\"" + ":"; }

		if (type == ObjectType::Object)
			s += nl + indent + "{";
		else if (type == ObjectType::Array)
			s += nl + indent + "[";

		for (size_t i = 0; i < subobjects.size(); i++)
		{
			s += indent + subobjects[i]->getSubobjectsAsStringInternal(depth, readable);
			if (i < subobjects.size()-1) s += ",";
		}

		if (type == ObjectType::Object)
			s += nl + indent + "}";
		else if (type == ObjectType::Array)
			s += nl + indent + "]";

		else if (type == ObjectType::String)
		{
			s += name.empty() ? nl + indent + "\"" + value + "\""  :  "\"" + value + "\"";
			//if (!name.empty())
			//	s += "\"" + value + "\"";
			//else
			//	s += nl + indent + "\"" + value + "\"";
		}
		else
			s += name.empty() ? nl + indent + value  :  value;
			

		depth--;
        return s;
    }

}

namespace JSON::Internals
{
	using namespace JSONTextUtils;

	JSON::Result lex(str_view text, std::vector<Token>& tokens)
	{
		static_assert(CHAR_BIT == 8);
		Token token;
		bool inString = false;
		bool inNumber = false;
		bool inLiteral = false;

		for (size_t i = 0; i < text.length(); i++)
		{
			const auto c = ctu8(text[i]);
			auto numBytes = numBytesChar(c);
			if (!numBytes) { return JSON::Result::Error_Lexer_InvalidEncoding; }

			if (!inString) // structural
			{
				if (inNumber && !isNumerical(c))
				{
					tokens.push_back(token);
					token.reset();
					inNumber = false;
				}
				if (isNumerical(c))
				{
					if (inNumber) { token.data.push_back(c); } // digit or symbol (found next digit)
					else { token.data.push_back(c); token.type = TokenType::Number; inNumber = true; } // start of number
				} 
				else if (isWhitespaceChar(c)) { continue; }
				else if (isStructuralChar(c)) { tokens.push_back(Token(TokenType::Structural, str_t(1, c))); }
				else if (c == STR_DELIM) { token.type = TokenType::String; inString = true; }

				else if (isliteralBooleanStr(i, text)) { tokens.push_back(Token(TokenType::Boolean, literalBooleanValue(i, text))); }
				else if (isLiteralNullStr(i, text)) { tokens.push_back(Token(TokenType::Null, literalNullValue(i))); }

				else if (numBytes > 1) { return JSON::Result::Error_Lexer_IllegalTokenMultiByte; }
				else { return JSON::Result::Error_Lexer_IllegalToken; }
			}
			else // string	
			{
				if (numBytes == 1)
				{
					if (c != STR_DELIM) { token.data.push_back(c); } // string constituent ASCII
					else // end of string
					{
						tokens.push_back(token);
						token.reset();
						inString = false;
					}
				}
				else
				{
					// start of multi-byte UTF-8 codepoint
				}
			}
		}
		return JSON::Result::OK;
	}


	bool getFileSize(str_view filePath, size_t& fileSizeOut)
	{
		if (!std::filesystem::exists(filePath)) { return false; }
		fileSizeOut = static_cast<size_t>(std::filesystem::file_size(filePath));
		return true;
	}

	bool openFile(str_view filePath, std::ifstream& fileStreamOut, size_t& fileSizeOut)
	{
		if (!getFileSize(filePath, fileSizeOut)) { return false; }
		fileStreamOut.open(filePath, std::ios_base::binary);
		return static_cast<bool>(fileStreamOut);
	}

	str_view tokenTypeToString(TokenType t)
	{
		switch (t)
		{
		case TokenType::Undefined: return str_view("Undefined");
		case TokenType::Structural: return str_view("Structural");
		case TokenType::String: return str_view("String");
		case TokenType::Number: return str_view("Number");
		case TokenType::Boolean: return str_view("Boolean");
		case TokenType::Null: return str_view("Null");
		default: return str_view("UnknownTokenType");
		}
	}

	StructuralTokenType structuralTokenToObjType(const Token& token)
	{
		if (token.type != TokenType::Structural) { return StructuralTokenType::NotStructural; }
		switch (token.data[0])
		{
			case STC_CBR_L: return StructuralTokenType::ObjectBegin;
			case STC_CBR_R: return StructuralTokenType::ObjectEnd;
			case STC_SBR_L: return StructuralTokenType::ArrayBegin;
			case STC_SBR_R: return StructuralTokenType::ArrayEnd;
			case STC_CL:	return StructuralTokenType::KeyValueDelim;
			case STC_CM:	return StructuralTokenType::MemberDelim;
			default:		return StructuralTokenType::NotStructural;
		}
	}

	JSON::ObjectType valueTokenToObjType(const Token& token)
	{
		switch (token.type)
		{
			case TokenType::String: return JSON::ObjectType::String;
			case TokenType::Number: return JSON::ObjectType::Number;
			case TokenType::Boolean: return JSON::ObjectType::Boolean;
			case TokenType::Null: return JSON::ObjectType::Null;
			default: return JSON::ObjectType::Undefined;
		}
	}
	
	JSON::Result parse(const std::vector<Token>& tokens, JSON::Object& objectOut)
	{
		objectOut.reset();
		auto numTokens = tokens.size();
		RETURN_ERROR_IF(!numTokens, Error_Parser_NoTokens); // fail: no tokens passed to parser

		if (valueTokenToObjType(tokens[0]) != JSON::ObjectType::Undefined)
		{
			// string, number, bool, or null at start
			RETURN_ERROR_IF(numTokens > 1, Error_Parser_InvalidRoot); // fail: text root must be a lone value, an unnamed object, or an unnamed array
			objectOut.set(valueTokenToObjType(tokens[0]), tokens[0].data);
			return JSON::Result::OK; // lone value is ok
		}
		RETURN_ERROR_IF(tokens[0].data[0] != STC_SBR_L && tokens[0].data[0] != STC_CBR_L, Error_Parser_IllegalTokenAtStart); // fail: incorrect structural character at start

		std::stack<JSON::Object> objects;
		objects.push(JSON::Object(JSON::ObjectType::Object, "root"));
		str_view name;
		const Token* lastToken = nullptr;

		for (size_t i = 0; i < numTokens; i++)
		{
			const TokenType type = tokens[i].type;
			const str_view data = tokens[i].data;

			if (i > 0) { lastToken = &tokens[i-1]; }
			RETURN_ERROR_IF(type == TokenType::Undefined, Error_Parser_UndefinedToken); // fail: token with undefined type passed to parser
			RETURN_ERROR_IF(data.empty() and type != TokenType::String, Error_Parser_EmptyToken); // fail: empty token passed to parser

			const JSON::ObjectType valueType = valueTokenToObjType(tokens[i]);
			const StructuralTokenType strucType = structuralTokenToObjType(tokens[i]);
			const bool isArrayToken = (strucType == StructuralTokenType::ArrayBegin || strucType == StructuralTokenType::ArrayEnd);
			const bool isInArray = objects.top().isArray();

			if (strucType != StructuralTokenType::NotStructural)
			{
				// structural  [ ] { } , :
				if (strucType == StructuralTokenType::ObjectBegin || strucType == StructuralTokenType::ArrayBegin)
				{
					// begin container
					RETURN_ERROR_IF(isInArray && !name.empty(), Error_Parser_NamedValueInArray); // fail: named object inside array
					RETURN_ERROR_IF(!isInArray && name.empty() && i != 0, Error_Parser_LoneValue); // fail: unnamed object not allowed outside arrays
					if (i > 0)
					{
						const StructuralTokenType prevStrucType = structuralTokenToObjType(tokens[name.empty() ? i - 1 : i - 3]);
						RETURN_ERROR_IF(prevStrucType != StructuralTokenType::MemberDelim &&
										prevStrucType != StructuralTokenType::ObjectBegin &&
										prevStrucType != StructuralTokenType::ArrayBegin, 
										Error_Parser_MissingSeparator); // fail: unexpected token  
					}
					objects.push(JSON::Object(isArrayToken ? JSON::ObjectType::Array : JSON::ObjectType::Object, name));
					name = str_view(); // clear name
				}
				else if (strucType == StructuralTokenType::ObjectEnd || strucType == StructuralTokenType::ArrayEnd)
				{
					// end container
					JSON::Object top = objects.top();
					RETURN_ERROR_IF((top.isArray()) != isArrayToken, Error_Parser_IllegalClosingToken); // fail: incorrect token at end of container
					objects.pop();
					objects.top().push_back(std::make_shared<JSON::Object>(top));
				}
				else if (strucType == StructuralTokenType::KeyValueDelim)
				{
					// fail: delimiter not following a string is illegal (pairs are properly handled when a name string is encountered)
					RETURN_ERROR(Error_Parser_InvalidKeyValuePair);
				}
				// commas are ignored at first, handled in the next iteration
				else if (strucType != StructuralTokenType::MemberDelim)
				{
					RETURN_ERROR(Error_Parser_UndefinedToken);
				}
			}
			
			else if (valueType != JSON::ObjectType::Undefined)
			{
				// string, number, bool, or null
				if (i + 1 >= numTokens) { RETURN_ERROR(Error_Parser_ExpectedTokenAfterValue); } // fail: expected additional tokens following value
				
				if (valueType == JSON::ObjectType::String && structuralTokenToObjType(tokens[i+1]) == StructuralTokenType::KeyValueDelim)
				{
					// key-value pair (handled in two iterations)
					const auto nextStrucType = structuralTokenToObjType(tokens[i+2]);
					if (valueTokenToObjType(tokens[i+2]) == JSON::ObjectType::Undefined && 
						nextStrucType != StructuralTokenType::ArrayBegin && 
						nextStrucType != StructuralTokenType::ObjectBegin) 
					{
						RETURN_ERROR(Error_Parser_InvalidKeyValuePair); // fail: expected a named value or object
					} 
					if (isInArray) { RETURN_ERROR(Error_Parser_NamedValueInArray); } // fail: named value inside array
					name = data;
					i++; // skip the ":"
					continue;
				}
				else if (name.empty() && (not objects.top().isArray())) { RETURN_ERROR(Error_Parser_LoneValue); } // fail: unnamed value not allowed outside arrays

				objects.top().push_back(std::make_shared<JSON::Object>(valueType, name, data));
				name = str_view(); // clear name
			}

		}
		objectOut = objects.top();
		return JSON::Result::OK;
	}
}

namespace XMLTextUtils
{

}

namespace XML
{
	using namespace XML::Internals;

}

namespace XML::Internals
{
	using namespace XMLTextUtils;

}

