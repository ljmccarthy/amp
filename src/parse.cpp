#include <ctype.h>
#include <cstring>
#include "lisp.h"
#include "parse.h"

static inline int
isdelim(int c)
{
	return isspace(c) || strchr("()", c);
}

static inline int
issymchar(int c)
{
	return isalnum(c) || strchr("_-+*/&?!|<>=~%#", c);
}

#define parse_error(s) \
	errorf(ParseError(), "parse error: at %d,%d: " s, line, col)

Parser::Parser() : c(0), c1(0), line(1), col(-1) {}
Parser::~Parser() {}
void Parser::init() { read(); read(); }

bool
Parser::eof()
{
	eats();
	return c == EOF;
}

int
Parser::read()
{
	int c0 = c;
	if (c1 == EOF) {
		c = EOF;
		return c0;
	}
	c = c1;
	c1 = getc();
	if (c == '\n') {
		line++;
		col = 0;
	}
	else
		col++;
	return c0;
}

int
Parser::eats()
{
	while (1) {
		while (isspace(c)) read();
		if (c != ';') break;
		while (c != '\n' && c != EOF) read();
	}
	return c;
}

int
Parser::next()
{
	read();
	return eats();
}

Value
Parser::parse()
{
	switch (eats()) {
	case '(':
		read(); return list();
	case '\'':
		read(); return quoted(sym_quote);
	case '`':
		read(); return quoted(sym_quasiquote);
	case ',':
		read();
		if (c == '@') {
			read();
			return quoted(sym_unquote_splicing);
		}
		else
			return quoted(sym_unquote);
	case ')':
		parse_error("too many right-parens");
	case EOF:
		parse_error("unexpected end of file");
	}
	return atom();
}

Value
Parser::atom()
{
	if (c == '"')
		return string();
	if (isdigit(c) || (c == '-' && isdigit(c1)))
		return number();
	if (issymchar(c))
		return symbol();
	parse_error("unexpected character");
}

Value
Parser::list()
{
	Value p = NIL, t = NIL, x;

	while (eats() != ')') {
		x = parse();
		p = cons(x, p);
		if (eats() == '.') {
			if (next() == ')')
				parse_error("nothing after dot");
			t = parse();
			if (eats() != ')')
				parse_error("after dot terminator");
		}
	}
	read();
	return reverse(p, t);
}

Value
Parser::symbol()
{
	char buf[256];
	unsigned len = 0;

	while (len < sizeof(buf) && issymchar(c))
		buf[len++] = read();
	if (len == sizeof(buf))
		parse_error("symbol too long");
	buf[len] = 0;

	if (strcmp(buf, "true") == 0)
		return _T;
	if (strcmp(buf, "false") == 0)
		return _F;
	if (strcmp(buf, "nil") == 0)
		return NIL;

	return make_symbol(buf);
}

Value
Parser::string()
{
	char buf[256];
	unsigned len = 0;

	read();
	while (len < sizeof(buf) && c != '"')
		buf[len++] = read();
	read();
	if (len == sizeof(buf))
		parse_error("string too long");
	buf[len] = 0;
	return make_string(buf, len);
}

Value
Parser::number()
{
	Fixnum n = 0;
	int sign = 1;
	if (c == '-') {
		sign = -sign;
		read();
	}
	while (isdigit(c))
		n = n * 10 + (read() - '0');
	return make_fixnum(n * sign);
}

Value
Parser::quoted(Symbol *quote)
{
	return cons(quote, cons(parse(), NIL));
}

FileParser::FileParser(const char *filename)
	: file(filename) { init(); }
int FileParser::getc()
	{ return file.get(); }
bool FileParser::_eof()
	{ return file.eof(); }

StringParser::StringParser(const char *s, size_t n)
	: p(s), n(n) { init(); }
StringParser::StringParser(const char *s)
	: p(s), n(strlen(s)) { init(); }
int StringParser::getc()
	{ return n ? (n--, *p++) : EOF; }
bool StringParser::_eof()
	{ return n == 0; }
