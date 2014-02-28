#include "jsi.h"
#include "jsparse.h"
#include "jscompile.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

static void jsB_Function(js_State *J, unsigned int argc)
{
	const char *source;
	js_Buffer *sb;
	js_Ast *parse;
	js_Function *fun;
	unsigned int i;

	if (js_isundefined(J, 1))
		source = "";
	else {
		source = js_tostring(J, argc);
		sb = NULL;
		if (argc > 1) {
			for (i = 1; i < argc; ++i) {
				if (i > 1) js_putc(J, &sb, ',');
				js_puts(J, &sb, js_tostring(J, i));
			}
			js_putc(J, &sb, ')');
		}
	}

	if (js_try(J)) {
		js_free(J, sb);
		jsP_freeparse(J);
		js_throw(J);
	}

	parse = jsP_parsefunction(J, "Function", sb->s, source);
	fun = jsC_compilefunction(J, parse);

	js_endtry(J);
	js_free(J, sb);
	jsP_freeparse(J);

	js_newfunction(J, fun, J->GE);
}

static void jsB_Function_prototype(js_State *J, unsigned int argc)
{
	js_pushundefined(J);
}

static void Fp_toString(js_State *J, unsigned int argc)
{
	js_Object *self = js_toobject(J, 0);
	char *s;
	unsigned int i, n;

	if (!js_iscallable(J, 0))
		js_typeerror(J, "not a function");

	if (self->type == JS_CFUNCTION || self->type == JS_CSCRIPT) {
		js_Function *F = self->u.f.function;
		n = strlen("function () { ... }");
		n += strlen(F->name);
		for (i = 0; i < F->numparams; ++i)
			n += strlen(F->vartab[i]) + 1;
		s = js_malloc(J, n);
		strcpy(s, "function ");
		strcat(s, F->name);
		strcat(s, "(");
		for (i = 0; i < F->numparams; ++i) {
			if (i > 0) strcat(s, ",");
			strcat(s, F->vartab[i]);
		}
		strcat(s, ") { ... }");
		if (js_try(J)) {
			js_free(J, s);
			js_throw(J);
		}
		js_pushstring(J, s);
		js_free(J, s);
	} else {
		js_pushliteral(J, "function () { ... }");
	}
}

static void Fp_apply(js_State *J, unsigned int argc)
{
	int i, n;
	char name[20];

	if (!js_iscallable(J, 0))
		js_typeerror(J, "not a function");

	js_copy(J, 0);
	js_copy(J, 1);

	js_getproperty(J, 2, "length");
	n = js_tonumber(J, -1);
	js_pop(J, 1);

	for (i = 0; i < n; ++i) {
		sprintf(name, "%d", i);
		js_getproperty(J, 2, name);
	}

	js_call(J, n);
}

static void Fp_call(js_State *J, unsigned int argc)
{
	unsigned int i;

	if (!js_iscallable(J, 0))
		js_typeerror(J, "not a function");

	js_copy(J, 0);
	js_copy(J, 1);
	for (i = 2; i <= argc; ++i)
		js_copy(J, i);

	js_call(J, argc - 1);
}

void jsB_initfunction(js_State *J)
{
	J->Function_prototype->u.c.function = jsB_Function_prototype;
	J->Function_prototype->u.c.constructor = NULL;

	js_pushobject(J, J->Function_prototype);
	{
		jsB_propf(J, "toString", Fp_toString, 2);
		jsB_propf(J, "apply", Fp_apply, 2);
		jsB_propf(J, "call", Fp_call, 1);
	}
	js_newcconstructor(J, jsB_Function, jsB_Function, 1);
	js_defglobal(J, "Function", JS_DONTENUM);
}
