// vd_c.h - A C syntax tree generator
// 
// -------------------------------------------------------------------------------------------------
// MIT License
// 
// Copyright (c) 2024 Michael Dodis
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#if defined(__INTELLISENSE__) || defined(__CLANGD__)
#define VD_C_IMPLEMENTATION
#endif

#ifndef VD_C_H
#define VD_C_H

enum {
	VD_C_TEOF = -1,
	VD_C_TUNKNOWN = 0,
	VD_C_TWHITESPACE,
	VD_C_TKEYWORD,
	VD_C_TIDENT,
	VD_C_TADDITION,
	VD_C_TSUBTRACTION,
	VD_C_TSTAR,
	VD_C_TDIVISION,
	VD_C_TINCREMENTATION,
	VD_C_TDECREMENTATION,
	VD_C_TPAREN_ENTER,
	VD_C_TPAREN_EXIT,
	VD_C_TBRACE_ENTER,
	VD_C_TBRACE_EXIT,
	VD_C_TID,
};

typedef struct VD_CAstNode VD_CAstNode;
struct VD_CAstNode {
	int type;
	struct {
		int   line;
		char *file;
	} src;

	VD_CAstNode *first_child;
	VD_CAstNode *last_child;
	VD_CAstNode *next_child;
	VD_CAstNode *prev_child;
	VD_CAstNode *parent;
};

VD_CAstNode *vd_c_parse(char *str, int len);

#ifdef VD_C_IMPLEMENTATION

static inline int vd_c__newline(int c) { return c == '\n'; }

typedef struct {
	int line;
	int column;
	int type;
} VD__CToken;

typedef struct {
	struct {
		char *str;
		int i;
		int len;
		int line;
		int column;
		int stash_cap;
		int stash_size;
		VD__CToken *stash;
	} lex;
} VD__CParserInstance;

int vd_c__getchar(VD__CParserInstance *inst)
{
	if (inst->lex.i >= inst->lex.len)
	{
		return -1;
	}

	return inst->lex.str[inst->lex.i++];
}

int vd_c__peekchar(VD__CParserInstance *inst)
{
	if (inst->lex.i >= inst->lex.len)
	{
		return -1;
	}

	return inst->lex.str[inst->lex.i];
}

int vd_c__retchar(VD__CParserInstance *inst)
{
	assert(inst->lex.i != 0 && "Too many return chars!");
	inst->lex.i--;

	return vd_c__peekchar(inst);
}

int vd_c__get(VD__CParserInstance *inst)
{
	int c = vd_c__getchar(inst);

	if (c == '\n')
	{
		inst->lex.line++;
		inst->lex.column = 1;
	} else if (c != -1) {
		inst->lex.column++;
	}
	return c;
}

void vd_c__unget(VD__CParserInstance *inst)
{
	int c = vd_c__retchar(inst);
	if (c == '\n')
	{
		inst->lex.line--;
		int i = inst->lex.i - 1;

		int col = 1;
		while (i >= 0 && inst->lex.str[i] != '\n') {
			col++;
		}

		inst->lex.column = col;
	}
}

VD__CToken vd_c__tnew(VD__CParserInstance *inst, VD__CToken t) {
	t.line = inst->lex.line;
	t.column = inst->lex.column;
	return t;
}

VD__CToken vd_c__tnext(VD__CParserInstance *inst)
{
	int c;
START:
	c = vd_c__get(inst);

	switch (c) {
		case '/': {
			if (vd_c__peekchar(inst) == '/') {
				// Single line comment
				int y;
				do {
					y = vd_c__get(inst);
				} while(!vd_c__newline(y) && (y != -1));

				goto START;
			} else {
				return vd_c__tnew(inst, (VD__CToken) {
					.type = VD_C_TDIVISION,
				});
			}
		} break;

		case ' ': case '\t': {
			goto START;
		} break;

		case '+': {
			if (vd_c__peekchar(inst) == '+') {
				vd_c__getchar(inst);
				return vd_c__tnew(inst, (VD__CToken) {
					.type = VD_C_TINCREMENTATION,
				});
			} else {
				return vd_c__tnew(inst, (VD__CToken) {
					.type = VD_C_TADDITION,
				});
			}
		} break;

		case '-': {
			if (vd_c__peekchar(inst) == '-') {
				vd_c__getchar(inst);
				return vd_c__tnew(inst, (VD__CToken) {
					.type = VD_C_TDECREMENTATION,
				});
			} else {
				return vd_c__tnew(inst, (VD__CToken) {
					.type = VD_C_TSUBTRACTION,
				});
			}
		}

		case '*': {
			return vd_c__tnew(inst, (VD__CToken) {
				.type = VD_C_TSTAR,
			});
		}

		case '(': return vd_c__tnew(inst, (VD__CToken) { .type = VD_C_TPAREN_ENTER}); break;
		case ')': return vd_c__tnew(inst, (VD__CToken) { .type = VD_C_TPAREN_EXIT}); break;
		case '{': return vd_c__tnew(inst, (VD__CToken) { .type = VD_C_TBRACE_ENTER}); break;
		case '}': return vd_c__tnew(inst, (VD__CToken) { .type = VD_C_TBRACE_EXIT}); break;
		default: break;
	}

	return vd_c__tnew(inst, (VD__CToken) {
		.type = VD_C_TUNKNOWN,
	});
}

void vd_c__tstash(VD__CParserInstance *inst, VD__CToken tok)
{
}

VD_CAstNode *vd_c_parse(char *str, int len)
{
	const int stash_cap = 4;
	// Initialize lexer
	VD__CParserInstance inst = {
		.lex = {
			.str = str,
			.len = len,
			.i = 0,
			.line = 1,
			.column = 1,
			.stash_cap = stash_cap,
			.stash_size = 0,
			.stash = (VD__CToken*)malloc(sizeof(VD__CToken) * stash_cap),
		},
	};
}


#endif // VD_C_IMPLEMENTATION
#endif // VD_C_H
