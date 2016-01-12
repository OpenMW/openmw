#ifndef AMXCONS_H_INCLUDED
#define AMXCONS_H_INCLUDED

typedef struct tagFMTINFO {
  const cell *params;
  int numparams;
  int skip;     /* number of characters to skip from the beginning */
  int length;   /* number of characters to print */
  /* helper functions */
  int (*f_putstr)(void *dest,const TCHAR *);
  int (*f_putchar)(void *dest,TCHAR);
  void *user;   /* user data */
} AMX_FMTINFO;

int amx_printstring(AMX *amx,cell *cstr,AMX_FMTINFO *info);

#endif /* AMXCONS_H_INCLUDED */
