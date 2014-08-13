#ifndef SRC_PARSE_H
#define SRC_PARSE_H

#include <fstream>

class Parser {
	int c, c1;
	int line, col;

	int read();
	int eats();
	int next();
	Value atom();
	Value list();
	Value symbol();
	Value string();
	Value number();
	Value quoted(Symbol *quote);
protected:
	void init();
	virtual int getc() = 0;
	virtual bool _eof() = 0;
public:
	Parser();
	virtual ~Parser();
	Value parse();
	bool eof();
};

class FileParser : public Parser {
	std::ifstream file;
protected:
	int getc();
	bool _eof();
public:
	FileParser(const char *filename);
};

class StringParser : public Parser {
	const char *p;
	size_t n;
protected:
	int getc();
	bool _eof();
public:
	StringParser(const char *s, size_t n);
	StringParser(const char *s);
};

#endif /* SRC_PARSE_H */
