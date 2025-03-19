#include "tokenizer.h"
#include "token.h"
#include <string>
#include <cctype>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <cstdarg>

namespace {
	static const char EndOfFileChar = std::char_traits<char>::to_char_type(std::char_traits<char>::eof());
}

//--------------------------------------------------------------------------------------------------
Tokenizer::Tokenizer() :
	input_(nullptr),
	inputLength_(0),
	cursorPos_(0),
	cursorLine_(0),
	error_(),
	m_macrosEnabled(true)
{

}

//--------------------------------------------------------------------------------------------------
Tokenizer::~Tokenizer()
{

}

//--------------------------------------------------------------------------------------------------
void Tokenizer::Reset(const char* input, size_t size)
{
	input_ = input;
	inputLength_ = size;
	cursorPos_ = 0;
	cursorLine_ = 1;
}

//--------------------------------------------------------------------------------------------------
char Tokenizer::GetChar(bool setPrevious)
{
	if (setPrevious) {
		prevCursorPos_ = cursorPos_;
		prevCursorLine_ = cursorLine_;
	}


	if(is_eof())
	{
		++cursorPos_;	// Do continue so UngetChar does what you think it does
		return EndOfFileChar;
	}
	
	char c = input_[cursorPos_];

	if (c == '\r') {
		++cursorPos_;
		return GetChar(false);
	}

	// New line moves the cursor to the new line
	if(c == '\n')
		cursorLine_++;

	cursorPos_++;
	return c;
}

//--------------------------------------------------------------------------------------------------
void Tokenizer::UngetChar()
{
	cursorLine_ = prevCursorLine_;
	cursorPos_ = prevCursorPos_;
}

//--------------------------------------------------------------------------------------------------
char Tokenizer::peek() const
{
	return !is_eof() ?
						input_[cursorPos_] :
						EndOfFileChar;
}

//--------------------------------------------------------------------------------------------------
char Tokenizer::GetLeadingChar()
{
	if (!comment_.text.empty())
		lastComment_ = comment_;

	comment_.text = "";
	comment_.startLine = cursorLine_;
	comment_.endLine = cursorLine_;

	char c;
	for(c = GetChar(); c != EndOfFileChar; c = GetChar())
	{
		// If this is a whitespace character skip it
		std::char_traits<char>::int_type intc = std::char_traits<char>::to_int_type(c);

		// In case of a new line
		if (c == '\n')
		{
			if (!comment_.text.empty())
				comment_.text += "\n";
			continue;
		}

		if(std::isspace(intc) || std::iscntrl(intc))
			continue;

		// If this is a single line comment
		char next = peek();
		if(c == '/' && next == '/')
		{
			std::vector<std::string> lines;

			size_t indentationLastLine = 0;
			while (!is_eof() && c == '/' && next == '/')
			{
				// Search for the end of the line
				std::string line;
				for (c = GetChar();
					c != EndOfFileChar && c != '\n';
					c = GetChar())
				{
					line += c;
				}
				
				// Store the line
				size_t lastSlashIndex = line.find_first_not_of("/");
				if (lastSlashIndex == std::string::npos)
					line = "";
				else
					line = line.substr(lastSlashIndex);

				size_t firstCharIndex = line.find_first_not_of(" \t");
				if (firstCharIndex == std::string::npos)
					line = "";
				else
					line = line.substr(firstCharIndex);

				if (firstCharIndex > indentationLastLine && !lines.empty())
					lines.back() += std::string(" ") + line;
				else
				{
					lines.emplace_back(std::move(line));
					indentationLastLine = firstCharIndex;
				}

				// Check the next line
				while (!is_eof() && std::isspace(c = GetChar()));

				if (!is_eof())
					next = peek();
			}

			// Unget previously get char
			if (!is_eof())
				UngetChar();

			// Build comment string
			std::stringstream ss;
			for (size_t i = 0; i < lines.size(); ++i)
			{
				if (i > 0)
					ss << "\n";
				ss << lines[i];
			}

			comment_.text = ss.str();
			comment_.endLine = cursorLine_;

			// Go to the next
			continue;
		}

		// If this is a block comment
		if(c == '/' && next == '*')
		{
			// Search for the end of the block comment
			std::vector<std::string> lines;
			std::string line;
			for (c = GetChar(), next = peek();
				c != EndOfFileChar && (c != '*' || next != '/');
				c = GetChar(), next = peek())
			{
				if (c == '\n')
				{
					if (!lines.empty() || !line.empty())
						lines.emplace_back(line);
					line.clear();
				}
				else
				{
					if (!line.empty() || !(std::isspace(c) || c == '*'))
						line += c;
				}
			}

			// Skip past the slash
			if(c != EndOfFileChar)
				GetChar();

			// Skip past new lines and spaces
			while (!is_eof() && std::isspace(c = GetChar()));
			if (!is_eof())
				UngetChar();

			// Remove empty lines from the back
			while (!lines.empty() && lines.back().empty())
				lines.pop_back();

			// Build comment string
			std::stringstream ss;
			for (size_t i = 0; i < lines.size(); ++i)
			{
				if (i > 0)
					ss << "\n";
				ss << lines[i]; 
			}

			comment_.text = ss.str();
			comment_.endLine = cursorLine_;

			// Move to the next character
			continue;
		}

		break;
	}
	
	return c;
}

bool Tokenizer::AddMacro(const std::string_view& macro)
{
	return m_macros.emplace(macro).second;
}

bool Tokenizer::ParseMacro(Token& token)
{
	if (token.tokenType != TokenType::kMacro) {
		return false;
	}

	//writer_.beginMacro(token.token);
	if (MatchSymbol("(")) {
		if (!MatchSymbol(")")) {
			do
			{
				// Parse key value
				Token keyToken;
				if (!GetIdentifier(keyToken))
					return Error("Expected identifier in macro sequence");

				if (!ParseMacro(keyToken)) {
					//writer_.macroArgument(keyToken.token);
				}
			} while (MatchSymbol(","));

			if (!MatchSymbol(")")) {
				return Error("Expected ')'");
			}
		}
	}

	//writer_.endMacro(token.token);

	return true;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::GetToken(Token &token, bool angleBracketsForStrings, bool seperateBraces)
{
	// Get the next character
	char c = GetLeadingChar();
	char p = peek();
	std::char_traits<char>::int_type intc = std::char_traits<char>::to_int_type(c);
	std::char_traits<char>::int_type intp = std::char_traits<char>::to_int_type(p);

	if(c == EndOfFileChar)
	{
		UngetChar();
		return false;
	}

	// Record the start of the token position
	token.startPos = prevCursorPos_;
	token.startLine = prevCursorLine_;
	token.token = std::string_view();
	token.tokenType = TokenType::kNone;

	// Alphanumeric token
	if(std::isalpha(intc) || c == '_')
	{
		// Read the rest of the alphanumeric characters
		do
		{
			c = GetChar();
			intc = std::char_traits<char>::to_int_type(c);
		} while(std::isalnum(intc) || c == '_');

		// Put back the last read character since it's not part of the identifier
		UngetChar();
		
		token.token = std::string_view(input_ + token.startPos, cursorPos_ - token.startPos);

		// Set the type of the token
		token.tokenType = TokenType::kIdentifier;

		if(token.token == "true")
		{
			token.tokenType = TokenType::kConst;
			token.constType = ConstType::kBoolean;
			token.boolConst = true;
		}
		else if(token.token == "false")
		{
			token.tokenType = TokenType::kConst;
			token.constType = ConstType::kBoolean;
			token.boolConst = false;
		}
		else if (m_macrosEnabled && m_macros.find(std::string(token.token)) != m_macros.cend())
		{
			token.tokenType = TokenType::kMacro;
			if (!ParseMacro(token)) {
				return Error("Invalid syntax");
			}

			GetToken(token);
		}

		return true;
	}
	// Constant
	else if(std::isdigit(intc) || ((c == '-' || c == '+') && std::isdigit(intp)))
	{
		bool isFloat = false;
		bool isHex = false;
		bool isNegated = c == '-';
		const char* start = input_ + cursorPos_;
		size_t size = 0;
		do
		{
			if(c == '.')
				isFloat = true;

			if(c == 'x' || c == 'X')
				isHex = true;

			c = GetChar();
			intc = std::char_traits<char>::to_int_type(c);

			++size;

		} while(std::isdigit(intc) ||
				(!isFloat && c == '.') ||
				(!isHex && (c == 'X' || c == 'x')) ||
				(isHex && std::isxdigit(intc)));

		if(!isFloat || (c != 'f' && c != 'F'))
			UngetChar();

		token.token = std::string_view(input_ + token.startPos, cursorPos_ - token.startPos);
		token.tokenType = TokenType::kConst;
		if(!isFloat)
		{
			try
			{
				if(isNegated)
				{
					token.int32Const = std::stoi(std::string(token.token), 0, 0);
					token.constType = ConstType::kInt32;
				}
				else
				{
					token.uint32Const = std::stoul(std::string(token.token), 0, 0);
					token.constType = ConstType::kUInt32;
				}
			}
			catch(std::out_of_range)
			{
				if(isNegated)
				{
					token.int64Const = std::stoll(std::string(token.token), 0, 0);
					token.constType = ConstType::kInt64;
				}
				else
				{
					token.uint64Const = std::stoull(std::string(token.token), 0, 0);
					token.constType = ConstType::kUInt64;
				}
			}
		}
		else
		{
			token.realConst = std::stod(std::string(token.token));
			token.constType = ConstType::kReal;
		}

		return true;
	}
	else if (c == '"' || (angleBracketsForStrings && c == '<'))
	{
		const char closingElement = c == '"' ? '"' : '>';

		c = GetChar();
		while (c != closingElement && std::char_traits<char>::not_eof(std::char_traits<char>::to_int_type(c)))
		{
			if(c == '\\')
			{
				c = GetChar();
				if(!std::char_traits<char>::not_eof(std::char_traits<char>::to_int_type(c)))
					break;
				else if(c == 'n')
					c = '\n';
				else if(c == 't')
					c = '\t';
				else if(c == 'r')
					c = '\r';
				else if(c == '"')
					c = '"';
			}

			c = GetChar();
		}

		if (c != closingElement)
			UngetChar();

		token.token = std::string_view(input_ + token.startPos + 1, cursorPos_ - token.startPos - 2);
		token.tokenType = TokenType::kConst;
		token.constType = ConstType::kString;
		token.stringConst = std::string(token.token);

		return true;
	}
	// Symbol
	else
	{
		// Push back the symbol
		#define PAIR(cc,dd) (c==cc&&d==dd) /* Comparison macro for two characters */
		const char d = GetChar();
		if(PAIR('<', '<') ||
			 PAIR('-', '>') ||
			 (!seperateBraces && PAIR('>', '>')) ||
			 PAIR('!', '=') ||
			 PAIR('<', '=') ||
			 PAIR('>', '=') ||
			 PAIR('+', '+') ||
			 PAIR('-', '-') ||
			 PAIR('+', '=') ||
			 PAIR('-', '=') ||
			 PAIR('*', '=') ||
			 PAIR('/', '=') ||
			 PAIR('^', '=') ||
			 PAIR('|', '=') ||
			 PAIR('&', '=') ||
			 PAIR('~', '=') ||
			 PAIR('%', '=') ||
			 PAIR('&', '&') ||
			 PAIR('|', '|') ||
			 PAIR('=', '=') ||
			 PAIR(':', ':') ||
			 PAIR('.', '.')
			)
		#undef PAIR
		{
			const char e = GetChar();
			if (e != '.') {
				UngetChar();
			}
		}
		else
			UngetChar();

		token.tokenType = TokenType::kSymbol;
		token.token = std::string_view(input_ + token.startPos, cursorPos_ - token.startPos);

		return true;
	}

	return false;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::is_eof() const
{
	return cursorPos_ >= inputLength_;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::GetConst(Token &token)
{
	if (!GetToken(token))
		return false;

	if (token.tokenType == TokenType::kConst)
		return true;

	UngetToken(token);
	return false;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::GetIdentifier(Token &token)
{
	if(!GetToken(token))
		return false;

	if(token.tokenType == TokenType::kIdentifier)
		return true;

	UngetToken(token);
	return false;
}

//--------------------------------------------------------------------------------------------------
void Tokenizer::UngetToken(const Token &token)
{
	cursorLine_ = token.startLine;
	cursorPos_ = token.startPos;
}

std::string_view Tokenizer::GetError()
{
	return error_;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::MatchIdentifier(const std::string_view &identifier)
{
	Token token;
	if(GetToken(token))
	{
		if(token.tokenType == TokenType::kIdentifier && token.token == identifier)
			return true;

		UngetToken(token);
	}

	return false;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::MatchSymbol(const std::string_view& symbol)
{
	Token token;
	if(GetToken(token, false, symbol.length() == 1 && symbol[0] == '>'))
	{
		if(token.tokenType == TokenType::kSymbol && token.token == symbol)
			return true;

		UngetToken(token);
	}

	return false;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::RequireIdentifier(const std::string_view& identifier)
{
	if(!MatchIdentifier(identifier))
		return Error("Expected '%.*s'", identifier.length(), identifier.data());
	return true;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::RequireSymbol(const std::string_view& symbol)
{
	if (!MatchSymbol(symbol))
		return Error("Expected '%.*s'", symbol.length(), symbol.data());
	return true;
}

void Tokenizer::SetMacroParsing(bool enabled)
{
	m_macrosEnabled = enabled;
}

//-------------------------------------------------------------------------------------------------
bool Tokenizer::Error(const char* fmt, ...)
{
	va_list args;
	char buffer[512];
	std::ostringstream str;
	va_start(args, fmt);
	vsnprintf(buffer, 512, fmt, args);
	str << "ParserError: " << (int)cursorLine_ << ":0: " << buffer;
	error_ = str.str();
	hasError_ = true;
	va_end(args);
	return false;
}
