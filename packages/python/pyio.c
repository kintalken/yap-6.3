

#include "py4yap.h"
#include <VFS.h>

#include "YapStreams.h"
#include "YapUTF8.h"

YAP_Term TermErrStream, TermOutStream;

static void pyflush(StreamDesc *st) {
#if 0
  st->u.w_irl.ptr[0] = '\0';
  fprintf(stderr,"%s\n", st->u.w_irl.buf);
  term_t tg = python_acquire_GIL();
  if (st->user_name == TermOutStream){
    PySys_WriteStdout("%s", st->u.w_irl.buf);
  } else {
    PySys_WriteStderr("%s", st->u.w_irl.buf);

  }
  python_release_GIL(tg);
  st->u.w_irl.ptr =   st->u.w_irl.buf;
#endif
}

static int py_putc(int sno, int ch) {
  // PyObject *pyw; // buffer
  // int pyw_kind;
  // PyObject *pyw_data;
  StreamDesc *st = YAP_GetStreamFromId(sno);
#if 0
  if (false && (st->user_name == TermOutStream || st->user_name == TermErrStream)) {
     size_t sz = put_utf8(st->u.w_irl.ptr, ch);
     if (sz > 0) {
	st->u.w_irl.ptr += sz;
      if (ch == '\n' ||  st->u.w_irl.ptr - st->u.w_irl.buf > 256)
	{pyflush(st); }
     }
        return ch;
  }
#endif
  char s[2];
  PyObject *err;
  s[0] = ch;
  s[1] = '\0';
  term_t g0 = python_acquire_GIL();
  PyObject_CallMethodObjArgs(st->u.private_data, PyUnicode_FromString("write"),
                             PyUnicode_FromString(s), NULL);
  python_release_GIL(g0);
  if ((err = PyErr_Occurred())) {
    PyErr_SetString(
        err,
        "Error in put\n"); // %s:%s:%d!\n", __FILE__, __FUNCTION__, __LINE__);
  }
  return ch;
}

VFS_t pystream;
static void *py_open(VFS_t *me, const char *name, const char *io_mode,
                     int sno) {
#if HAVE_STRCASESTR
  if (strcasestr(name, "/python/") == name)
    name += strlen("/python/");
#else
  if (strstr(name, "/python/") == name)
    name += strlen("/python/");
#endif
  term_t ctk = python_acquire_GIL();
  PyObject *pystream = string_to_python(name, true, NULL);
  if (pystream == NULL || pystream == Py_None) {
    python_release_GIL(ctk);
    return NULL;
  }
  StreamDesc *st = YAP_RepStreamFromId(sno);
  st->name = YAP_LookupAtom(name);
  if (strcmp(name, "sys.stdout") == 0 || strcmp(name, "sys.stderr") == 0 ||
      strcmp(name, "input") == 0) {
    st->status |= Tty_Stream_f;
  }
  /*
    if (!outbuf)
       outbuf =   ( unsigned char *)malloc(1024);
    st->u.w_irl.ptr = st->u.w_irl.buf = outbuf;




]\]
    st->user_name = TermOutStream;
  } else if (strcmp(name, "sys.stderr") == 0) {
    st->user_name = TermErrStream;
    if (!errbuf)
     errbuf = ( unsigned char *)malloc(1024);
    st->u.w_irl.ptr = st->u.w_irl.buf = errbuf;
    //  } else if (strcmp(name, "input") == 0) {
    //pystream = PyObject_Call(pystream, PyTuple_New(0), NULL);
    } else */
  { st->user_name = YAP_MkAtomTerm(st->name); }
  st->u.private_data = pystream;
  st->vfs = me;
  st->file = NULL;
  python_release_GIL(ctk);
  return st;
}

static void py_flush(int sno) {
  StreamDesc *s = YAP_GetStreamFromId(sno);
  term_t tg = python_acquire_GIL();
  PyObject *flush = PyObject_GetAttrString(s->u.private_data, "flush");
  pyflush(s);
  PyObject_CallFunction(flush, NULL);
  python_release_GIL(tg);
}

static bool py_close(int sno) {
  StreamDesc *st = YAP_RepStreamFromId(sno);
  if (st->status & (Output_Stream_f | Append_Stream_f))
    py_flush(sno);
  if (strcmp(st->name, "sys.stdout") && strcmp(st->name, "sys.stderr")) {
    Py_XDECREF(st->u.private_data);
    st->u.w_irl.buf = st->u.w_irl.ptr = NULL;
  }
  st->u.private_data = NULL;
  st->vfs = NULL;

  return true;
}

static bool getLine(StreamDesc *rl_iostream, int sno) {
  char *myrl_line = NULL;
  // term_t ctk = python_acquire_GIL();
    PyObject *readl =
      PythonLookup("input", Py_None);
  PyObject_Print(readl, stderr, 0);fprintf(stderr," input\n");

  
      PyObject *user_inp = PyObject_CallFunction(readl,NULL);
  PyObject_Print(user_inp, stderr, 0);fprintf(stderr," input\n");
      myrl_line = PyUnicode_AsUTF8(user_inp);
  //PyObject_Print(myrl_line, stderr, 0);fprintf(stderr," myrl_line\n");
  // python_release_GIL(ctk);
  PyObject *err;
  if ((err = PyErr_Occurred())) {
    PyErr_SetString(err, "Error in getLine\n");
    Yap_ThrowError(SYSTEM_ERROR_GET_FAILED, YAP_MkIntTerm(sno), err);
  }
  size_t size = strlen(myrl_line) + 1;
  rl_iostream->u.irl.ptr = rl_iostream->u.irl.buf =
      (const unsigned char *)malloc(size);
  memmove((void *)rl_iostream->u.irl.buf, myrl_line, size);
  return true;
}

static int py_getc(int sno) {
  StreamDesc *s = YAP_RepStreamFromId(sno);
  int ch;
  bool fetch = (s->u.irl.buf == NULL);

  if (fetch) {
    if (!getLine(s, sno)) {
      return EOF;
    }
  }
  const unsigned char *ttyptr = s->u.irl.ptr++, *myrl_line = s->u.irl.buf;
  ch = *ttyptr;
  if (ch == '\0') {
    ch = '\n';
    free((void *)myrl_line);
    s->u.irl.ptr = s->u.irl.buf = NULL;
  }
  return ch;
}

/**
  @brief  Yap_ReadlinePeekChar peeks the next char from the
  readline buffer, but does not actually grab it.

  The idea is to take advantage of the buffering. Special care must be taken
  with EOF, though.

*/
static int py_peek(int sno) {
  StreamDesc *s = YAP_RepStreamFromId(sno);
  int ch;

  if (s->u.irl.buf) {
    const unsigned char *ttyptr = s->u.irl.ptr;
    ch = *ttyptr;
    if (ch == '\0') {
      ch = '\n';
    }
    return ch;
  }
  if (getLine(s, sno)) {
    ch = s->u.irl.ptr[0];
    if (ch == '\0') {
      ch = '\n';
    }
  } else {
    return EOF;
  }
  return ch;
}

static int64_t py_seek(int sno, int64_t where, int how) {
  StreamDesc *g0 = YAP_RepStreamFromId(sno);
  term_t s0 = python_acquire_GIL();
  PyObject *fseek = PyObject_GetAttrString(g0->u.private_data, "seek");
  PyObject *pyr = PyObject_CallFunctionObjArgs(fseek, PyLong_FromLong(where),
                                               PyLong_FromLong(how), NULL);
  python_release_GIL(s0);
  return PyLong_AsLong(pyr);
}

#if 0
static void python_output(void) {
  PyObject *stream = string_to_python("sys.stdout", true, NULL);
  StreamDesc *st = YAP_GetStreamFromId(1);
  st->u.private_data = stream;
  st->vfs = &pystream;
  stream = string_to_python("sys.stderr", true, NULL);
  st = YAP_GetStreamFromIds(2);
  st->u.private_data = stream;
  st->vfs = &pystream;
}
#endif

static bool initialized = false;

bool init_python_vfs(void) {
  // pyw = PyUnicode_FromString("x");
  // pyw_kind = PyUnicode_KIND(pyw);
  // pyw_data = PyUnicode_DATA(pyw);
  if (initialized)
    return false;
  initialized = true;
  pystream.name = "python stream";
  pystream.vflags =
      VFS_CAN_WRITE | VFS_CAN_EXEC | VFS_CAN_READ | VFS_HAS_PREFIX;
  pystream.prefix = "/python/";
  pystream.suffix = NULL;
  pystream.open = py_open;
  pystream.close = py_close;
  pystream.get_char = py_getc;
  pystream.peek_char = py_peek;
  pystream.put_char = py_putc;
  pystream.flush = py_flush;
  pystream.seek = py_seek;
  pystream.next = GLOBAL_VFS;
  GLOBAL_VFS = &pystream;
  TermOutStream = YAP_MkAtomTerm(YAP_LookupAtom("std.output"));
  TermErrStream = YAP_MkAtomTerm(YAP_LookupAtom("std.error"));
  // NULL;
  return true;
}
