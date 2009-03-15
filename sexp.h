/*  sexp.h -- header for sexp library                    */
/*  Copyright (c) 2009 Alex Shinn.  All rights reserved. */
/*  BSD-style license: http://synthcode.com/license.txt  */

#ifndef SEXP_H
#define SEXP_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "config.h"

#if HAVE_ERR_H
#include <err.h>
#else
/* requires msg be a string literal, and at least one argument */
#define errx(code, msg, ...) (fprintf(stderr,msg"\n",__VA_ARGS__), exit(code))
#endif

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || defined(__OpenBSD__)
#define SEXP_BSD 1
#else
#define SEXP_BSD 0
#endif

#if USE_BOEHM
#include "gc/include/gc.h"
#define SEXP_ALLOC        GC_malloc
#define SEXP_ALLOC_ATOMIC GC_malloc_atomic
#define SEXP_REALLOC      GC_realloc
#define SEXP_FREE         GC_free
#else
#define SEXP_ALLOC        malloc
#define SEXP_ALLOC_ATOMIC SEXP_ALLOC
#define SEXP_REALLOC      realloc
#define SEXP_FREE         free
#endif

#define SEXP_NEW()       ((sexp) SEXP_ALLOC(sizeof(struct sexp_struct)))

/* tagging system
 *   bits end in  00:  pointer
 *                01:  fixnum
 *               011:  symbol
 *               111:  immediate symbol
 *              0110:  char
 *              1110:  other immediate object (NULL, TRUE, FALSE)
 */

#define SEXP_FIXNUM_BITS 2
#define SEXP_IMMEDIATE_BITS 3
#define SEXP_EXTENDED_BITS 4

#define SEXP_FIXNUM_MASK 3
#define SEXP_IMMEDIATE_MASK 7
#define SEXP_EXTENDED_MASK 15

#define SEXP_POINTER_TAG 0
#define SEXP_FIXNUM_TAG 1
#define SEXP_LSYMBOL_TAG 3
#define SEXP_ISYMBOL_TAG 7
#define SEXP_CHAR_TAG 6
#define SEXP_EXTENDED_TAG 14

enum sexp_types {
  SEXP_OBJECT,
  SEXP_FIXNUM,
  SEXP_CHAR,
  SEXP_BOOLEAN,
  SEXP_PAIR,
  SEXP_SYMBOL,
  SEXP_STRING,
  SEXP_VECTOR,
  SEXP_FLONUM,
  SEXP_BIGNUM,
  SEXP_IPORT,
  SEXP_OPORT,
  /* the following are used only by the evaluator */
  SEXP_EXCEPTION,
  SEXP_PROCEDURE,
  SEXP_MACRO,
  SEXP_ENV,
  SEXP_BYTECODE,
  SEXP_CORE,
  SEXP_OPCODE,
};

typedef unsigned long sexp_uint_t;
typedef long sexp_sint_t;
typedef char sexp_tag_t;
typedef struct sexp_struct *sexp;

struct sexp_struct {
  sexp_tag_t tag;
  union {
    double flonum;
    struct {
      sexp car, cdr;
    } pair;
    struct {
      sexp_uint_t length;
      sexp *data;
    } vector;
    struct {
      sexp_uint_t length;
      char *data;
    } string;
    struct {
      sexp_uint_t length;
      char *data;
    } symbol;
    struct {
      FILE *stream;
      char *name;
      sexp_uint_t line;
    } port;
    struct {
      sexp kind, message, irritants, file, line;
    } exception;
    struct {
      char flags;
      sexp parent, bindings;
    } env;
    struct {
      sexp_uint_t length;
      unsigned char data[];
    } bytecode;
    struct {
      char flags;
      unsigned short num_args;
      sexp bc, vars;
    } procedure;
    struct {
      sexp proc, env;
    } macro;
    struct {
      unsigned char op_class, code, num_args, flags,
        arg1_type, arg2_type, inverse;
      char *name;
      sexp data, proc;
    } opcode;
    struct {
      char code;
      char *name;
    } core;
  } value;
};

#define sexp_sizeof_field(field) (sizeof((sexp)NULL)->value.field)
#define sexp_sizeof(field) (sizeof(struct sexp_struct)-sexp_sizeof_field(exception)+sexp_sizeof_field(field))

#define SEXP_MAKE_IMMEDIATE(n)  ((sexp) ((n<<SEXP_EXTENDED_BITS) + SEXP_EXTENDED_TAG))
#define SEXP_NULL   SEXP_MAKE_IMMEDIATE(0)
#define SEXP_FALSE  SEXP_MAKE_IMMEDIATE(1)
#define SEXP_TRUE   SEXP_MAKE_IMMEDIATE(2)
#define SEXP_EOF    SEXP_MAKE_IMMEDIATE(3)
#define SEXP_UNDEF  SEXP_MAKE_IMMEDIATE(4)
#define SEXP_ERROR  SEXP_MAKE_IMMEDIATE(5)
#define SEXP_CLOSE  SEXP_MAKE_IMMEDIATE(6) /* internal use */
#define SEXP_RAWDOT SEXP_MAKE_IMMEDIATE(7) /* internal use */

#define sexp_nullp(x)    ((x) == SEXP_NULL)
#define sexp_pointerp(x) (((sexp_uint_t)(x) & SEXP_FIXNUM_MASK) == SEXP_POINTER_TAG)
#define sexp_integerp(x) (((sexp_uint_t)(x) & SEXP_FIXNUM_MASK) == SEXP_FIXNUM_TAG)
#define sexp_isymbolp(x) (((sexp_uint_t)(x) & SEXP_IMMEDIATE_MASK) == SEXP_ISYMBOL_TAG)
#define sexp_charp(x)    (((sexp_uint_t)(x) & SEXP_EXTENDED_MASK) == SEXP_CHAR_TAG)
#define sexp_booleanp(x) (((x) == SEXP_TRUE) || ((x) == SEXP_FALSE))

#define SEXP_CHECK_TAG(x,t) (sexp_pointerp(x) && (x)->tag == (t))

#define sexp_pairp(x)      (SEXP_CHECK_TAG(x, SEXP_PAIR))
#define sexp_stringp(x)    (SEXP_CHECK_TAG(x, SEXP_STRING))
#define sexp_lsymbolp(x)   (SEXP_CHECK_TAG(x, SEXP_SYMBOL))
#define sexp_vectorp(x)    (SEXP_CHECK_TAG(x, SEXP_VECTOR))
#define sexp_flonump(x)    (SEXP_CHECK_TAG(x, SEXP_FLONUM))
#define sexp_iportp(x)     (SEXP_CHECK_TAG(x, SEXP_IPORT))
#define sexp_oportp(x)     (SEXP_CHECK_TAG(x, SEXP_OPORT))
#define sexp_exceptionp(x) (SEXP_CHECK_TAG(x, SEXP_EXCEPTION))
#define sexp_procedurep(x) (SEXP_CHECK_TAG(x, SEXP_PROCEDURE))
#define sexp_envp(x)       (SEXP_CHECK_TAG(x, SEXP_ENV))
#define sexp_bytecodep(x)  (SEXP_CHECK_TAG(x, SEXP_BYTECODE))
#define sexp_corep(x)      (SEXP_CHECK_TAG(x, SEXP_CORE))
#define sexp_opcodep(x)    (SEXP_CHECK_TAG(x, SEXP_OPCODE))
#define sexp_macrop(x)     (SEXP_CHECK_TAG(x, SEXP_MACRO))

#define sexp_symbolp(x)  (sexp_isymbolp(x) || sexp_lsymbolp(x))

#if USE_HUFF_SYMS
#define SEXP_DOTP(x) (((sexp_uint_t)(x))==((0x5D00<<SEXP_IMMEDIATE_BITS)+SEXP_ISYMBOL_TAG))
#else
#define SEXP_DOTP(x) ((x)==sexp_the_dot_symbol)
#endif

#define sexp_make_integer(n)    ((sexp) (((sexp_sint_t) n<<SEXP_FIXNUM_BITS) + SEXP_FIXNUM_TAG))
#define sexp_unbox_integer(n)   ((sexp_sint_t) n>>SEXP_FIXNUM_BITS)
#define sexp_make_character(n)  ((sexp) (((sexp_sint_t) n<<SEXP_EXTENDED_BITS) + SEXP_CHAR_TAG))
#define sexp_unbox_character(n) ((sexp_sint_t) n>>SEXP_EXTENDED_BITS)

#define sexp_flonum_value(f) ((f)->value.flonum)

#define sexp_vector_length(x) ((x)->value.vector.length)
#define sexp_vector_data(x)   ((x)->value.vector.data)

#define sexp_vector_ref(x, i) (sexp_vector_data(x)[sexp_unbox_integer(i)])
#define sexp_vector_set(x, i, v) (sexp_vector_data(x)[sexp_unbox_integer(i)] = (v))

#define sexp_procedure_num_args(x) ((x)->value.procedure.num_args)
#define sexp_procedure_flags(x) ((x)->value.procedure.flags)
#define sexp_procedure_variadic_p(x) (sexp_unbox_integer(sexp_procedure_flags(x)) & 1)
#define sexp_procedure_code(x) ((x)->value.procedure.bc)
#define sexp_procedure_vars(x) ((x)->value.procedure.vars)

#define sexp_string_length(x) ((x)->value.string.length)
#define sexp_string_data(x)   ((x)->value.string.data)

#define sexp_string_ref(x, i) (sexp_make_character(sexp_string_data(x)[sexp_unbox_integer(i)]))
#define sexp_string_set(x, i, v) (sexp_string_data(x)[sexp_unbox_integer(i)] = sexp_unbox_character(v))

#define sexp_symbol_length(x)  ((x)->value.symbol.length)
#define sexp_symbol_data(x)    ((x)->value.symbol.data)

#define sexp_port_stream(p)    ((p)->value.port.stream)
#define sexp_port_name(p)      ((p)->value.port.name)
#define sexp_port_line(p)      ((p)->value.port.line)

#define sexp_exception_kind(p)      ((p)->value.exception.kind)
#define sexp_exception_message(p)   ((p)->value.exception.message)
#define sexp_exception_irritants(p) ((p)->value.exception.irritants)
#define sexp_exception_file(p)      ((p)->value.exception.file)
#define sexp_exception_line(p)      ((p)->value.exception.line)

#define sexp_bytecode_length(x)   ((x)->value.bytecode.length)
#define sexp_bytecode_data(x)     ((x)->value.bytecode.data)

#define sexp_env_flags(x)         ((x)->value.env.flags)
#define sexp_env_parent(x)        ((x)->value.env.parent)
#define sexp_env_bindings(x)      ((x)->value.env.bindings)
#define sexp_env_global_p(x)      (! sexp_env_parent(x))

#define sexp_macro_proc(x)        ((x)->value.macro.proc)
#define sexp_macro_env(x)         ((x)->value.macro.env)

#define sexp_core_code(x)         ((x)->value.core.code)
#define sexp_core_name(x)         ((x)->value.core.name)

#define sexp_opcode_class(x)      ((x)->value.opcode.op_class)
#define sexp_opcode_code(x)       ((x)->value.opcode.code)
#define sexp_opcode_num_args(x)   ((x)->value.opcode.num_args)
#define sexp_opcode_flags(x)      ((x)->value.opcode.flags)
#define sexp_opcode_arg1_type(x)  ((x)->value.opcode.arg1_type)
#define sexp_opcode_arg2_type(x)  ((x)->value.opcode.arg2_type)
#define sexp_opcode_inverse(x)    ((x)->value.opcode.inverse)
#define sexp_opcode_name(x)       ((x)->value.opcode.name)
#define sexp_opcode_data(x)       ((x)->value.opcode.data)
#define sexp_opcode_proc(x)       ((x)->value.opcode.proc)

#define sexp_opcode_variadic_p(x) (sexp_opcode_flags(x) & 1)

#if USE_STRING_STREAMS
#if SEXP_BSD
#define fmemopen(str, len, m) funopen(sexp_vector(3, (sexp)str, (sexp)len, (sexp)0), sstream_read, sstream_write, sstream_seek, sstream_close)
int sstream_read(void *vec, char *dst, int n);
int sstream_write(void *vec, const char *src, int n);
off_t sstream_seek(void *vec, off_t offset, int whence);
int sstream_close(void *vec);
#endif
#define sexp_read_char(p) (getc(sexp_port_stream(p)))
#define sexp_push_char(c, p) (ungetc(c, sexp_port_stream(p)))
#define sexp_write_char(c, p) (putc(c, sexp_port_stream(p)))
#define sexp_write_string(s, p) (fputs(s, sexp_port_stream(p)))
#define sexp_printf(p, s, ...) (fprintf(sexp_port_stream(p), s, __VA_ARGS__))
#define sexp_flush(p) (fflush(sexp_port_stream(p)))
#else
sexp sexp_read_char(sexp port);
void sexp_push_char(sexp ch, sexp port);
void sexp_write_char(sexp ch, sexp port);
void sexp_write_string(sexp str, sexp port);
void sexp_printf(sexp port, sexp fmt, ...);
#endif

#define sexp_add(a, b) ((sexp)(((sexp_sint_t)a)+((sexp_sint_t)b)-SEXP_FIXNUM_TAG))
#define sexp_sub(a, b) ((sexp)(((sexp_sint_t)a)-((sexp_sint_t)b)+SEXP_FIXNUM_TAG))
#define sexp_mul(a, b) ((sexp)((((((sexp_sint_t)a)-SEXP_FIXNUM_TAG)*(((sexp_sint_t)b)>>SEXP_FIXNUM_BITS))+SEXP_FIXNUM_TAG)))
#define sexp_div(a, b) ((sexp)(((((sexp_sint_t)a)>>SEXP_FIXNUM_BITS)/(((sexp_sint_t)b)>>SEXP_FIXNUM_BITS))<<SEXP_FIXNUM_BITS)+SEXP_FIXNUM_TAG)
#define sexp_mod(a, b) ((sexp)(((((sexp_sint_t)a)>>SEXP_FIXNUM_BITS)%(((sexp_sint_t)b)>>SEXP_FIXNUM_BITS))<<SEXP_FIXNUM_BITS)+SEXP_FIXNUM_TAG)

#define sexp_list1(a)          sexp_cons(a, SEXP_NULL)
#define sexp_list2(a, b)       sexp_cons(a, sexp_cons(b, SEXP_NULL))
#define sexp_list3(a, b, c)    sexp_cons(a, sexp_cons(b, sexp_cons(c, SEXP_NULL)))
#define sexp_list4(a, b, c, d) sexp_cons(a, sexp_cons(b, sexp_cons(c, sexp_cons(d, SEXP_NULL))))

#define sexp_car(x)       ((x)->value.pair.car)
#define sexp_cdr(x)       ((x)->value.pair.cdr)

#define sexp_caar(x)      (sexp_car(sexp_car(x)))
#define sexp_cadr(x)      (sexp_car(sexp_cdr(x)))
#define sexp_cdar(x)      (sexp_cdr(sexp_car(x)))
#define sexp_cddr(x)      (sexp_cdr(sexp_cdr(x)))

#define sexp_caaar(x)     (sexp_car(sexp_caar(x)))
#define sexp_caadr(x)     (sexp_car(sexp_cadr(x)))
#define sexp_cadar(x)     (sexp_car(sexp_cdar(x)))
#define sexp_caddr(x)     (sexp_car(sexp_cddr(x)))
#define sexp_cdaar(x)     (sexp_cdr(sexp_caar(x)))
#define sexp_cdadr(x)     (sexp_cdr(sexp_cadr(x)))
#define sexp_cddar(x)     (sexp_cdr(sexp_cdar(x)))
#define sexp_cdddr(x)     (sexp_cdr(sexp_cddr(x)))

#define sexp_cadddr(x)    (sexp_cadr(sexp_cddr(x)))
#define sexp_cddddr(x)    (sexp_cddr(sexp_cddr(x)))

sexp sexp_cons(sexp head, sexp tail);
int sexp_listp(sexp obj);
int sexp_list_index(sexp ls, sexp elt);
sexp sexp_lset_diff(sexp a, sexp b);
sexp sexp_reverse(sexp ls);
sexp sexp_nreverse(sexp ls);
sexp sexp_append(sexp a, sexp b);
sexp sexp_memq(sexp x, sexp ls);
sexp sexp_assq(sexp x, sexp ls);
sexp sexp_length(sexp ls);
sexp sexp_make_string(char *str);
sexp sexp_make_flonum(double f);
int sexp_string_hash(char *str, int acc);
sexp sexp_intern(char *str);
sexp sexp_make_vector(sexp len, sexp dflt);
sexp sexp_list_to_vector(sexp ls);
sexp sexp_vector(int count, ...);
void sexp_write(sexp obj, sexp out);
void sexp_free(sexp obj);
char* sexp_read_string(sexp in);
char* sexp_read_symbol(sexp in, int init);
sexp sexp_read_number(sexp in, int base);
sexp sexp_read_raw(sexp in);
sexp sexp_read(sexp in);
sexp sexp_read_from_string(char *str);
sexp sexp_make_input_port(FILE* in);
sexp sexp_make_output_port(FILE* out);
sexp sexp_make_input_string_port(sexp str);
sexp sexp_make_output_string_port();
sexp sexp_get_output_string(sexp port);
void sexp_init();

#endif /* ! SEXP_H */

