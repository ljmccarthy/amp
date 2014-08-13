#include <cstdio>
#include <readline/readline.h>
#include <readline/history.h>
#include "lisp.h"
#include "parse.h"
#include "util.h"

void
eval_file(const char *filename, Module *mod)
{
	FileParser p(filename);
	while (!p.eof())
		eval(p.parse(), mod);
}

static void
run(void)
{
	Module *toplevel = new Module;
	primitives(toplevel);

	try {
		eval_file("boot.lisp", toplevel);
	}
	catch (Error&) {
	}

	GC_gcollect();

	while (1) {
		malloc_ptr<char> line = readline(">>> ");
		if (!line) {
			putchar('\n');
			break;
		}
		try {
			StringParser p(line);
			if (!p.eof())
				add_history(line);
			while (!p.eof())
				println(eval(p.parse(), toplevel));
		}
		catch (Error&) {
		}
	}
}

int
main(int argc, char *argv[])
{
	GC_INIT();
	init_types();
	init_primitives();

	try {
		run();
	}
	catch (Exit& e) {
		return e.code;
	}
	return 0;
}
