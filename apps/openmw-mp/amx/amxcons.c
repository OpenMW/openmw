/* Console output module (terminal I/O) for the Pawn AMX
 *
 *  Since some of these routines go further than those of standard C, they
 *  cannot always be implemented with portable C functions. In other words,
 *  these routines must be ported to other environments.
 *
 *  Copyright (c) ITB CompuPhase, 1997-2015
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy
 *  of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations
 *  under the License.
 *
 *  Version: $Id: amxcons.c 5181 2015-01-21 09:44:28Z thiadmer $
 */

#if defined _UNICODE || defined __UNICODE__ || defined UNICODE
# if !defined UNICODE   /* for Windows */
#   define UNICODE
# endif
# if !defined _UNICODE  /* for C library */
#   define _UNICODE
# endif
#endif

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined __WIN32__ || defined _WIN32 || defined WIN32 || defined __MSDOS__
  #define HAVE_CONIO
  #include <conio.h>
  #include <malloc.h>
#endif
#if defined USE_CURSES || defined HAVE_CURSES_H
  #include <curses.h>
  #if !defined CURSES
    #define CURSES  1
  #endif
#endif
#include "osdefs.h"
#if defined __ECOS__
  /* eCos puts include files in cyg/package_name */
  #include <cyg/hal/hal_if.h>
  #include <cyg/infra/diag.h>
  #include <cyg/hal/hal_diag.h>
  #include <cyg/pawn/amx.h>
#else
  #include "amx.h"
#endif
#if defined __WIN32__ || defined _WIN32 || defined WIN32
  #include <windows.h>
#endif

#if defined _UNICODE
# include <tchar.h>
#elif !defined __T
  typedef char          TCHAR;
# define __T(string)    string
# define _fgetts        fgets
# define _puttchar      putchar
# define _stprintf      sprintf
# define _tcschr        strchr
# define _tcscpy        strcpy
# define _tcsdup        strdup
# define _tcslen        strlen
# define _tprintf       printf
#endif
#include "amxcons.h"

#if defined AMX_TERMINAL
  #define EOL_CHAR       '\r'
#endif
#if defined __WIN32__ || defined _WIN32 || defined WIN32 || defined __MSDOS__
  #define EOL_CHAR       '\r'
#endif
#if !defined EOL_CHAR
  /* if not a "known" operating system, assume Linux/Unix */
  #define EOL_CHAR     '\n'
#endif

#if !defined AMX_STRING_LIB

#if defined AMX_TERMINAL
  /* required functions are implemented elsewhere */
  int amx_putstr(const TCHAR *);
  int amx_putchar(int);
  int amx_fflush(void);
  int amx_getch(void);
  TCHAR *amx_gets(TCHAR *,int);
  int amx_termctl(int,int);
  void amx_clrscr(void);
  void amx_clreol(void);
  int amx_gotoxy(int x,int y);
  void amx_wherexy(int *x,int *y);
  unsigned int amx_setattr(int foregr,int backgr,int highlight);
  void amx_console(int columns, int lines, int flags);
  void amx_viewsize(int *width,int *height);
  int amx_kbhit(void);
#elif defined CURSES && CURSES != 0
  /* Use the "curses" library to implement the console */
  static WINDOW *curseswin;
  #define amx_putstr(s)       printw("%s",(s))
  #define amx_putchar(c)      addch(c)
  #define amx_fflush()        refresh()
  #define amx_getch()         getch()
  #define amx_gets(s,n)       getnstr((s),(n))
  #define amx_clrscr()        clear()
  #define amx_clreol()        clrtoeol()
  #define amx_gotoxy(x,y)     move((y)-1,(x)-1)
  #define amx_console(c,l,f)  ((void)(c),(void)(l),(void)(f))
  unsigned int amx_setattr(int foregr,int backgr,int highlight)
  {
    if (highlight>0)
      attron(A_STANDOUT);
    else
      attroff(A_STANDOUT);
    //??? in future, also handle colours
  }
  void CreateConsole(void);
  int amx_kbhit(void)
  {
    int result;
    CreateConsole();
    nodelay(curseswin,TRUE);    /* enter non-blocking state */
    result=getch();             /* read key (if any) */
    nodelay(curseswin,FALSE);   /* leave non-blocking state */
    if (result!=ERR)
      ungetch(result);          /* a key is waiting, push it back */
    return (result==ERR) ? 0 : 1;
  }
  int amx_termctl(int code,int value)
  {
    switch (code) {
    case 0:           /* query terminal support */
      return 1;
    /* case 1: */     /* switch auto-wrap on/off (not supported in curses!) */
    /* case 2: */     /* create/switch to another console */
    case 3:           /* set emphasized font */
      if (value)
        attron(A_BOLD);
      else
        attroff(A_BOLD);
      return 1;
    /* case 4: */     /* query whether a terminal is "open" */
    default:
      return 0;
    } /* switch */
  }
  void amx_wherexy(int *x,int *y)
  {
    int row,col;
    getyx(curseswin,row,col);
    if (x!=NULL)
      *x=col+1;
    if (y!=NULL)
      *y=row+1;
  }
  void amx_viewsize(int *width,int *height)
  {
    int row,col;
    getmaxyx(curseswin,row,col);
    if (width!=NULL)
      *width=col;
    if (height!=NULL)
      *height=row;
  }
#elif defined VT100 || defined __LINUX__ || defined ANSITERM || defined __ECOS__
  /* ANSI/VT100 terminal, or shell emulating "xterm" */
  #if defined __ECOS__
    #define AMXCONSOLE_NOIDLE
  #endif

  #if CYGPKG_PAWN_AMXCONSOLE_DIAG==1
    /* eCos has basically two ways to make simple exchanges with a terminal:
     * - with the diag_*() functions (no input provided!)
     * - with f*() functions (fprintf(),fputs(), etc).
     */
    #define amx_fflush()

    static int amx_putstr(TCHAR *s)
    {
      diag_write_string(s);
      return 1;
    }
    static int amx_putchar(TCHAR c)
    {
      diag_write_char(c);
      return c;
    }
    static char amx_getch(void)
    {
      char c=-1;
      HAL_DIAG_READ_CHAR(c);
      return c;
    }
  #else

    #define amx_putstr(s)     fputs((s),stdout)
    #define amx_putchar(c)    putchar(c)
    #define amx_fflush()      fflush(stdout)
    #define amx_getch()       getch()
    #define amx_gets(s,n)     fgets(s,n,stdin)
    #define amx_kbhit()       kbhit()
  #endif

  int amx_termctl(int code,int value)
  {
    switch (code) {
    case 0:             /* query terminal support */
      return 1;

    case 1:             /* switch "auto-wrap" on or off */
      if (value)
        amx_putstr("\033[?7h"); /* enable "auto-wrap" */
      else
        amx_putstr("\033[?7l"); /* disable "auto-wrap" */
      return 1;

    #if 0
      /* next to swapping buffers, more information should be saved and swapped,
       * such as the cursor position and the current terminal attributes
       */
    case 2:             /* swap console buffers */
      amx_fflush();
      if (value==1) {
        amx_putstr("\033[?47h");
      } else {
        amx_putstr("\033[?47l");
      } /* if */
      amx_fflush();
      return 1;
    #endif

    case 3:             /* set bold/highlighted font */
      return 0;

    default:
      return 0;
    } /* switch */
  }
  void amx_clrscr(void)
  {
    amx_putstr("\033[2J");
    amx_fflush();        /* pump through the terminal codes */
  }
  void amx_clreol(void)
  {
    amx_putstr("\033[K");
    amx_fflush();        /* pump through the terminal codes */
  }
  int amx_gotoxy(int x,int y)
  {
    char str[30];
    _stprintf(str,"\033[%d;%dH",y,x);
    amx_putstr(str);
    amx_fflush();        /* pump through the terminal codes */
    return 1;
  }
  void amx_wherexy(int *x,int *y)
  {
    int val,i;
    char str[10];

    assert(x!=NULL && y!=NULL);
    amx_putstr("\033[6n");
    amx_fflush();
    while (amx_getch()!='\033')
      /* nothing */;
    val=amx_getch();
    assert(val=='[');
    for (i=0; i<8 && (val=amx_getch())!=';'; i++)
      str[i]=(char)val;
    str[i]='\0';
    if (y!=NULL)
      *y=atoi(str);
    for (i=0; i<8 && (val=amx_getch())!='R'; i++)
      str[i]=(char)val;
    str[i]='\0';
    if (x!=NULL)
      *x=atoi(str);
    #if defined ANSITERM
      val=amx_getch();
      assert(val=='\r');    /* ANSI driver adds CR to the end of the command */
    #endif
  }
  unsigned int amx_setattr(int foregr,int backgr,int highlight)
  {
    static short current=(0 << 8) | 7;
    short prev = current;
    char str[30];

    if (foregr>=0) {
      _stprintf(str,"\x1b[%dm",foregr+30);
      amx_putstr(str);
      current=(current & 0xff00) | (foregr & 0x0f);
    } /* if */
    if (backgr>=0) {
      _stprintf(str,"\x1b[%dm",backgr+40);
      amx_putstr(str);
      current=(current & 0x00ff) | ((backgr & 0x0f) << 8);
    } /* if */
    if (highlight>=0) {
      _stprintf(str,"\x1b[%dm",highlight);
      amx_putstr(str);
      current=(current & 0x7fff) | ((highlight & 0x01) << 15);
    } /* if */
    return prev;
  }
  void amx_console(int columns, int lines, int flags)
  {
    char str[30];

    (void)flags;
    /* There is no ANSI code (or VT100/VT220) to set the size of the console
     * (indeed, the terminal was that of the alphanumeric display). In xterm (a
     * terminal emulator) we can set the terminal size though, and most
     * terminals that in use today are in fact emulators.
     * Putty understands this code too, by many others do not.
     */
    sprintf(str,"\033[8;%d;%dt",lines,columns);
    amx_putstr(str);
    amx_fflush();
  }
  void amx_viewsize(int *width,int *height)
  {
    /* a trick to get the size of the terminal is to position the cursor far
     * away and then read it back
     */
    amx_gotoxy(999,999);
    amx_wherexy(width,height);
  }
#elif defined __WIN32__ || defined _WIN32 || defined WIN32
  /* Win32 console */
  #define amx_putstr(s)       _tprintf("%s",(s))
  #define amx_putchar(c)      _puttchar(c)
  #define amx_fflush()        fflush(stdout)
  #define amx_gets(s,n)       _fgetts(s,n,stdin)

  int amx_termctl(int code,int value)
  {
    switch (code) {
    case 0:             /* query terminal support */
      return 1;

    case 1: {           /* switch auto-wrap on/off */
      /* only works in Windows 2000/XP */
      HANDLE hConsole=GetStdHandle(STD_OUTPUT_HANDLE);
      DWORD mode;
      GetConsoleMode(hConsole,&mode);
      if (value)
        mode |= ENABLE_WRAP_AT_EOL_OUTPUT;
      else
        mode &= ~ENABLE_WRAP_AT_EOL_OUTPUT;
      SetConsoleMode(hConsole,mode);
      return 1;
    } /* case */

    /* case 2: */     /* create/switch to another console */
    /* case 3: */     /* set emphasized font */
    /* case 4: */     /* query whether a terminal is "open" */
    default:
      return 0;
    } /* switch */
  }
  void amx_clrscr(void)
  {
    COORD coordScreen={0,0};
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;
    HANDLE hConsole=GetStdHandle(STD_OUTPUT_HANDLE);

    amx_fflush();       /* make sure the I/O buffer is empty */
    GetConsoleScreenBufferInfo(hConsole,&csbi);
    dwConSize=csbi.dwSize.X*csbi.dwSize.Y;
    FillConsoleOutputCharacter(hConsole,' ',dwConSize,coordScreen,&cCharsWritten);
    FillConsoleOutputAttribute(hConsole,csbi.wAttributes,dwConSize,coordScreen, &cCharsWritten);
    SetConsoleCursorPosition(hConsole,coordScreen);
  }
  void amx_clreol(void)
  {
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;
    HANDLE hConsole=GetStdHandle(STD_OUTPUT_HANDLE);

    amx_fflush();       /* make sure all output is written */
    GetConsoleScreenBufferInfo(hConsole,&csbi);
    dwConSize=csbi.dwSize.X - csbi.dwCursorPosition.X;
    FillConsoleOutputCharacter(hConsole,' ',dwConSize,csbi.dwCursorPosition,&cCharsWritten);
    FillConsoleOutputAttribute(hConsole,csbi.wAttributes,dwConSize,csbi.dwCursorPosition,&cCharsWritten);
  }
  int amx_gotoxy(int x,int y)
  {
    COORD point;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hConsole=GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleScreenBufferInfo(hConsole, &csbi);
    if (x<=0 || x>csbi.dwSize.X || y<=0 || y>csbi.dwSize.Y)
      return 0;
    amx_fflush();       /* make sure all output is written */
    point.X=(short)(x-1);
    point.Y=(short)(y-1);
    SetConsoleCursorPosition(hConsole,point);
    return 1;
  }
  void amx_wherexy(int *x,int *y)
  {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    amx_fflush();       /* make sure all output is written */
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    if (x!=NULL)
      *x=csbi.dwCursorPosition.X+1;
    if (y!=NULL)
      *y=csbi.dwCursorPosition.Y+1;
  }
  unsigned int amx_setattr(int foregr,int backgr,int highlight)
  {
    static int ansi_colours[] = { 0, 4, 2, 6, 1, 5, 3, 7 };
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int f,b,h,prev;
    HANDLE hConsole=GetStdHandle(STD_OUTPUT_HANDLE);

    amx_fflush();       /* make sure all output is written */
    GetConsoleScreenBufferInfo(hConsole,&csbi);
    f=csbi.wAttributes & 0x07;
    b=(csbi.wAttributes >> 4) & 0x0f;
    h=(csbi.wAttributes & 0x08) ? 1 : 0;
    prev=(b << 8) | f | (h << 15);
    if (foregr>=0 && foregr<8)
      f=ansi_colours[foregr];
    if (backgr>=0 && backgr<8)
      b=ansi_colours[backgr];
    if (highlight>=0)
      h=highlight!=0;
    SetConsoleTextAttribute(hConsole, (WORD)((b << 4) | f | (h << 3)));
    return prev;
  }
  void amx_console(int columns, int lines, int flags)
  {
    SMALL_RECT rect;
    COORD dwSize;
    HANDLE hConsole;
    (void)flags;
    dwSize.X=(short)columns;
    dwSize.Y=(short)lines;
    hConsole=GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleScreenBufferSize(hConsole,dwSize);
    rect.Left=0;
    rect.Top=0;
    rect.Right=(short)(columns-1);
    rect.Bottom=(short)(lines-1);
    SetConsoleWindowInfo(hConsole,TRUE,&rect);
  }
  void amx_viewsize(int *width,int *height)
  {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&csbi);
    if (width!=NULL)
      *width=(int)csbi.dwSize.X;
    if (height!=NULL)
      *height=(int)(csbi.srWindow.Bottom-csbi.srWindow.Top+1);
  }
  int amx_getch(void)
  {
    TCHAR ch;
    DWORD count,mode;
    HANDLE hConsole=GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hConsole,&mode);
    SetConsoleMode(hConsole,mode & ~(ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT));
    while (ReadFile(hConsole,&ch,1,&count,NULL) && count==0)
      /* nothing */;
    SetConsoleMode(hConsole,mode);
    if (count>0)
      return ch;
    return EOF;
  }
  int amx_kbhit(void)
  {
    DWORD count=0;
    HANDLE hConsole;
    hConsole=GetStdHandle(STD_INPUT_HANDLE);
    if (GetFileType(hConsole)==FILE_TYPE_PIPE) {
      PeekNamedPipe(hConsole,NULL,0,NULL,&count,NULL);
    } else {
      INPUT_RECORD rec;
      while (PeekConsoleInput(hConsole,&rec,1,&count)) {
        if (count==0 || (rec.EventType==KEY_EVENT && rec.Event.KeyEvent.bKeyDown))
          break;
        ReadConsoleInput(hConsole,&rec,1,&count);
      }
    }
    return (count>0);
  }
#else
  /* assume a streaming terminal; limited features (no colour, no cursor
   * control)
   */
  #define amx_putstr(s)       printf("%s",(s))
  #define amx_putchar(c)      putchar(c)
  #define amx_fflush()        fflush(stdout)
  #define amx_gets(s,n)       fgets(s,n,stdin)
  #define amx_clrscr()        (void)(0)
  #define amx_clreol()        (void)(0)
  #define amx_gotoxy(x,y)     ((void)(x),(void)(y),(0))
  #define amx_wherexy(x,y)    (*(x)=*(y)=0)
  #define amx_setattr(c,b,h)  ((void)(c),(void)(b),(void)(h),(0))
  #define amx_termctl(c,v)    ((void)(c),(void)(v),(0))
  #define amx_console(c,l,f)  ((void)(c),(void)(l),(void)(f))
  #define amx_viewsize        (*(x)=80,*(y)=25)
  #if defined HAVE_CONIO
    #define amx_getch()       getch()
    #define amx_kbhit()       kbhit()
  #else
    #define amx_getch()       getchar()
    #define amx_kbhit()       (0)
  #endif
#endif

#if !defined AMX_TERMINAL && (defined __WIN32__ || defined _WIN32 || defined WIN32)
  void CreateConsole(void)
  { static int createdconsole=0;
    if (!createdconsole) {
  	  AllocConsole();
  	  createdconsole=1;
  	} /* if */
  }
#elif defined CURSES && CURSES != 0
  // The Mac OS X build variant uses curses.
  void CreateConsole(void)
  { static int createdconsole=0;
    if (!createdconsole) {
      curseswin=initscr();
      if (has_colors())
        start_color();
      cbreak();
      noecho();
      nonl();
      scrollok(curseswin,TRUE);
      intrflush(curseswin,FALSE);
      keypad(curseswin,TRUE);
      createdconsole=1;
    } /* if */
  }
#else
  #define CreateConsole()
#endif

static int cons_putstr(void *dest,const TCHAR *str)
{
  (void)dest;
  return amx_putstr(str);
}

static int cons_putchar(void *dest,TCHAR ch)
{
  (void)dest;
  return amx_putchar(ch);
}

#endif /* AMX_STRING_LIB */

enum {
  SV_DECIMAL,
  SV_HEX
};

static TCHAR *reverse(TCHAR *string,int stop)
{
	int start=0;
	TCHAR temp;

	/* swap the string */
	stop--;				/* avoid swapping the '\0' byte to the first position */
	while (stop - start > 0) {
		temp = string[start];
		string[start] = string[stop];
		string[stop] = temp;
		start++;
		stop--;
	} /* while */
	return string;
}

/* Converts an integral value to a string, with optional padding with spaces or
 * zeros.
 * The "format" must be decimal or hexadecimal
 * The number is right-aligned in the field with the size of the absolute value
 * of the "width" parameter.
 * If the width value is positive, the string is padded with spaces; if it is
 * negative, it is padded with zeros.
 */
static TCHAR *amx_strval(TCHAR buffer[], long value, int format, int width)
{
	int start, stop;
	TCHAR temp;

	start = stop = 0;
	if (format == SV_DECIMAL) {
		if (value < 0) {
			buffer[0] = __T('-');
			start = stop = 1;
			value = -value;
		} /* if */
		do {
			buffer[stop++] = (TCHAR)((value % 10) + __T('0'));
			value /= 10;
		} while (value > 0);
	} else {
		/* hexadecimal */
		unsigned long v = (unsigned long)value;	/* copy to unsigned value for shifting */
		do {
			buffer[stop] = (TCHAR)((v & 0x0f) + __T('0'));
			if (buffer[stop] > __T('9'))
				buffer[stop] += (TCHAR)(__T('A') - __T('0') - 10);
			v >>= 4;
			stop++;
		} while (v != 0);
	} /* if */

	/* pad to given width */
	if (width < 0) {
		temp = __T('0');
		width = -width;
	} else {
		temp = __T(' ');
	} /* if */
	while (stop < width)
		buffer[stop++] = temp;

	buffer[stop] = __T('\0');

	/* swap the string, and we are done */
	reverse(buffer+start,stop-start);
	return buffer;
}

#if defined FIXEDPOINT
  #define FIXEDMULT     1000
  #define FIXEDDIGITS   3

static TCHAR *formatfixed(TCHAR *string,cell value,TCHAR align,int width,TCHAR decpoint,int digits,TCHAR filler)
{
  int i, len;
  cell ipart,v;
  TCHAR vsign=__T('\0');

  /* make the value positive (but keep the sign) */
  if (value<0) {
    value=-value;
    vsign=__T('-');
  } /* if */

  /* "prepare" the value so that when it is truncated to the requested
   * number of digits, the result is rounded towards the dropped digits
   */
  assert(digits<INT_MAX);
  v=FIXEDMULT/2;
  for (i=0; i<digits; i++)
    v/=10;
  value+=v;

  /* get the integer part and remove it from the value */
  ipart=value/FIXEDMULT;
  value-=FIXEDMULT*ipart;
  assert(ipart>=0);
  assert(value>=0);

  /* truncate the fractional part to the requested number of digits */
  for (i=FIXEDDIGITS; i>digits; i--)
    value/=10;

  string[0]=__T('\0');

  /* add sign */
  i=(int)_tcslen(string);
  string[i]=vsign;
  string[i+1]=__T('\0');

  /* add integer part */
  amx_strval(string+_tcslen(string),(long)ipart,SV_DECIMAL,0);

  /* add fractional part */
  if (digits>0) {
    i=(int)_tcslen(string);
    string[i]=decpoint;
    amx_strval(string+i+1,(long)value,SV_DECIMAL,-digits);
  } /* if */

  len=(int)_tcslen(string);
  if (len<width) {
    /* pad to the requested width */
    for (i=len; i<width; i++)
      string[i]=filler;
    string[i]=__T('\0');
    /* optionally move the padding to the beginning of the string, using the handwaving algorithm */
    if (align!=__T('-')) {
      assert(i==(int)_tcslen(string));
      assert(i>=len);
      reverse(string,len);
      reverse(string+len,i-len);
      reverse(string,i);
    } /* if */
  } /* if */

  return string;
}
#endif


static int dochar(AMX *amx,TCHAR ch,cell param,TCHAR sign,TCHAR decpoint,int width,int digits,TCHAR filler,
                  int (*f_putstr)(void*,const TCHAR *),int (*f_putchar)(void*,TCHAR),void *user)
{
  cell *cptr;
  TCHAR buffer[40];
  #if defined FLOATPOINT
    TCHAR formatstring[40];
  #endif

  #if !defined FIXEDPOINT && !defined FLOATPOINT
    (void)decpoint;
  #endif
  assert(f_putstr!=NULL);
  assert(f_putchar!=NULL);

  switch (ch) {
  case __T('c'):
    cptr=amx_Address(amx,param);
    width--;            /* single character itself has a with of 1 */
    if (sign!=__T('-'))
      while (width-->0)
        f_putchar(user,filler);
    f_putchar(user,(TCHAR)*cptr);
    while (width-->0)
      f_putchar(user,filler);
    return 1;

  case __T('d'): {
    cell value;
    int length=1;
    cptr=amx_Address(amx,param);
    value=*cptr;
    if (value<0 || sign==__T('+'))
      length++;
    if (value<0)
      value=-value;
    while (value>=10) {
      length++;
      value/=10;
    } /* while */
    width-=length;
    if (sign!=__T('-'))
      while (width-->0)
        f_putchar(user,filler);
    amx_strval(buffer,*cptr,SV_DECIMAL,0);
    if (sign==__T('+') && *cptr>=0)
      f_putchar(user,sign);
    f_putstr(user,buffer);
    while (width-->0)
      f_putchar(user,filler);
    return 1;
  } /* case */

#if defined FLOATPOINT
  case __T('f'): /* 32-bit floating point number */
  case __T('r'): /* if floating point is enabled, %r == %f */
    /* build a format string */
    if (digits==INT_MAX)
      digits=5;
    else if (digits>25)
      digits=25;
    _tcscpy(formatstring,__T("%"));
    if (sign!=__T('\0'))
      _stprintf(formatstring+_tcslen(formatstring),__T("%c"),sign);
    if (width>0)
      _stprintf(formatstring+_tcslen(formatstring),__T("%d"),width);
    _stprintf(formatstring+_tcslen(formatstring),__T(".%df"),digits);
    cptr=amx_Address(amx,param);
    #if PAWN_CELL_SIZE == 64
      _stprintf(buffer,formatstring,*(double*)cptr);
    #else
      _stprintf(buffer,formatstring,*(float*)cptr);
    #endif
    if (decpoint==__T(',')) {
      TCHAR *ptr=_tcschr(buffer,__T('.'));
      if (ptr!=NULL)
        *ptr=__T(',');
    } /* if */
    f_putstr(user,buffer);
    return 1;
#endif

#if defined FIXEDPOINT
  #define FIXEDMULT 1000
  case __T('q'): /* 32-bit fixed point number */
#if !defined FLOATPOINT
  case __T('r'): /* if fixed point is enabled, and floating point is not, %r == %q */
#endif
    cptr=amx_Address(amx,param);
    /* format the number */
    if (digits==INT_MAX)
      digits=3;
    else if (digits>25)
      digits=25;
    formatfixed(buffer,*cptr,sign,width,decpoint,digits,filler);
    assert(_tcslen(buffer)<sizeof buffer);
    f_putstr(user,buffer);
    return 1;
#endif

#if !defined FLOATPOINT && !defined FIXEDPOINT
  case __T('f'):
  case __T('q'):
  case __T('r'):
    f_putstr(user,__T("(no rational number support)"));
    return 0; /* flag this as an error */
#endif

  case __T('s'): {
    AMX_FMTINFO info;
    memset(&info,0,sizeof info);
    info.length=digits;
    info.f_putstr=f_putstr;
    info.f_putchar=f_putchar;
    info.user=user;
    cptr=amx_Address(amx,param);
    amx_printstring(amx,cptr,&info);
    return 1;
  } /* case */

  case __T('x'): {
    ucell value;
    int length=1;
    cptr=amx_Address(amx,param);
    value=*(ucell*)cptr;
    while (value>=0x10) {
      length++;
      value>>=4;
    } /* while */
    width-=length;
    if (sign!=__T('-'))
      while (width-->0)
        f_putchar(user,filler);
    amx_strval(buffer,(long)*cptr,SV_HEX,0);
    f_putstr(user,buffer);
    while (width-->0)
      f_putchar(user,filler);
    return 1;
  } /* case */

  } /* switch */
  /* error in the string format, try to repair */
  f_putchar(user,ch);
  return 0;
}

enum {
  FMT_NONE,   /* not in format state; accept '%' */
  FMT_START,  /* found '%', accept '+', '-' (START), '0' (filler; START), digit (WIDTH), '.' (DECIM), or '%' or format letter (done) */
  FMT_WIDTH,  /* found digit after '%' or sign, accept digit (WIDTH), '.' (DECIM) or format letter (done) */
  FMT_DECIM,  /* found digit after '.', accept accept digit (DECIM) or format letter (done) */
};

static int formatstate(TCHAR c,int *state,TCHAR *sign,TCHAR *decpoint,int *width,int *digits,TCHAR *filler)
{
  assert(state!=NULL && sign!=NULL && decpoint!=NULL && width!=NULL && digits!=NULL && filler!=NULL);
  switch (*state) {
  case FMT_NONE:
    if (c==__T('%')) {
      *state=FMT_START;
      *sign=__T('\0');
      *decpoint=__T('.');
      *width=0;
      *digits=INT_MAX;
      *filler=__T(' ');
    } else {
      return -1;  /* print a single character */
    } /* if */
    break;
  case FMT_START:
    if (c==__T('+') || c==__T('-')) {
      *sign=c;
    } else if (c==__T('0')) {
      *filler=c;
    } else if (c>=__T('1') && c<=__T('9')) {
      *width=(int)(c-__T('0'));
      *state=FMT_WIDTH;
    } else if (c==__T('.') || c==__T(',')) {
      *decpoint=c;
      *digits=0;
      *state=FMT_DECIM;
    } else if (c==__T('%')) {
      *state=FMT_NONE;
      return -1;  /* print literal '%' */
    } else {
      return 1;   /* print formatted character */
    } /* if */
    break;
  case FMT_WIDTH:
    if (c>=__T('0') && c<=__T('9')) {
      *width=*width*10+(int)(c-__T('0'));
    } else if (c==__T('.') || c==__T(',')) {
      *decpoint=c;
      *digits=0;
      *state=FMT_DECIM;
    } else {
      return 1;   /* print formatted character */
    } /* if */
    break;
  case FMT_DECIM:
    if (c>=__T('0') && c<=__T('9')) {
      *digits=*digits*10+(int)(c-__T('0'));
    } else {
      return 1;   /* print formatted character */
    } /* if */
    break;
  } /* switch */

  return 0;
}

int amx_printstring(AMX *amx,cell *cstr,AMX_FMTINFO *info)
{
  int i,paramidx=0;
  int fmtstate=FMT_NONE,width,digits;
  TCHAR sign,decpoint,filler;
  int (*f_putstr)(void*,const TCHAR *);
  int (*f_putchar)(void*,TCHAR);
  void *user;
  int skip,length;

  if (info!=NULL) {
    f_putstr=info->f_putstr;
    f_putchar=info->f_putchar;
    user=info->user;
    skip=info->skip;
    length=info->length;
  } else {
    f_putstr=NULL;
    f_putchar=NULL;
    user=NULL;
    skip=0;
    length=INT_MAX;
  } /* if */
  #if !defined AMX_STRING_LIB
    if (f_putstr==NULL)
      f_putstr=cons_putstr;
    if (f_putchar==NULL)
      f_putchar=cons_putchar;
  #else
    assert(f_putstr!=NULL && f_putchar!=NULL);
  #endif

  /* if no placeholders appear, we can use a quicker routine */
  if (info==NULL || info->params==NULL) {

    TCHAR cache[100];
    int idx=0;

    if ((ucell)*cstr>UNPACKEDMAX) {
      int j=sizeof(cell)-sizeof(char);
      char c;
      /* the string is packed */
      i=0;
      for ( ; ; ) {
        c=(char)((ucell)cstr[i] >> 8*j);
        if (c==0)
          break;
        if (skip>0) {
          skip--;               /* skip a number of characters */
        } else {
          if (length--<=0)
            break;              /* print up to a certain length */
          assert(idx<sizeof cache);
          cache[idx++]=c;
          if (idx==sizeof cache - 1) {
            cache[idx]=__T('\0');
            f_putstr(user,cache);
            idx=0;
          } /* if */
        } /* if */
        if (j==0)
          i++;
        j=(j+sizeof(cell)-sizeof(char)) % sizeof(cell);
      } /* for */
    } else {
      /* unpacked string */
      for (i=0; cstr[i]!=0; i++) {
      	if (skip-->0)
      	  continue;
        assert(idx<sizeof cache);
        cache[idx++]=(TCHAR)cstr[i];
        if (idx==sizeof cache - 1) {
          cache[idx]=__T('\0');
          f_putstr(user,cache);
          idx=0;
        } /* if */
      } /* for */
    } /* if */
    if (idx>0) {
      cache[idx]=__T('\0');
      f_putstr(user,cache);
    } /* if */

  } else {

    /* check whether this is a packed string */
    if ((ucell)*cstr>UNPACKEDMAX) {
      int j=sizeof(cell)-sizeof(char);
      char c;
      /* the string is packed */
      i=0;
      for ( ; ; ) {
        c=(char)((ucell)cstr[i] >> 8*j);
        if (c==0)
          break;
        switch (formatstate(c,&fmtstate,&sign,&decpoint,&width,&digits,&filler)) {
        case -1:
          f_putchar(user,c);
          break;
        case 0:
          break;
        case 1:
          assert(info!=NULL && info->params!=NULL);
          if (paramidx>=info->numparams)  /* insufficient parameters passed */
            amx_RaiseError(amx, AMX_ERR_NATIVE);
          else
            paramidx+=dochar(amx,c,info->params[paramidx],sign,decpoint,width,digits,filler,
                             f_putstr,f_putchar,user);
          fmtstate=FMT_NONE;
          break;
        default:
          assert(0);
        } /* switch */
        if (j==0)
          i++;
        j=(j+sizeof(cell)-sizeof(char)) % sizeof(cell);
      } /* for */
    } else {
      /* the string is unpacked */
      for (i=0; cstr[i]!=0; i++) {
        switch (formatstate((TCHAR)cstr[i],&fmtstate,&sign,&decpoint,&width,&digits,&filler)) {
        case -1:
          f_putchar(user,(TCHAR)cstr[i]);
          break;
        case 0:
          break;
        case 1:
          assert(info!=NULL && info->params!=NULL);
          if (paramidx>=info->numparams)  /* insufficient parameters passed */
            amx_RaiseError(amx, AMX_ERR_NATIVE);
          else
            paramidx+=dochar(amx,(TCHAR)cstr[i],info->params[paramidx],sign,decpoint,width,digits,filler,
                             f_putstr,f_putchar,user);
          fmtstate=FMT_NONE;
          break;
        default:
          assert(0);
        } /* switch */
      } /* for */
    } /* if */

  } /* if (info==NULL || info->params==NULL) */

  return paramidx;
}

#if !defined AMX_STRING_LIB

#if defined AMX_ALTPRINT
/* print(const string[], start=0, end=cellmax) */
static cell AMX_NATIVE_CALL n_print(AMX *amx,const cell *params)
{
  cell *cstr;
  AMX_FMTINFO info;

  memset(&info,0,sizeof info);
  info.skip= ((size_t)params[0]>=2*sizeof(cell)) ? (int)params[2] : 0;
  info.length= ((size_t)params[0]>=3*sizeof(cell)) ? (int)(params[3]-info.skip) : INT_MAX;

  CreateConsole();
  cstr=amx_Address(amx,params[1]);
  amx_printstring(amx,cstr,&info);
  amx_fflush();
  return 0;
}
#else
/* print(const string[], foreground=-1, background=-1, highlight=-1) */
static cell AMX_NATIVE_CALL n_print(AMX *amx,const cell *params)
{
  cell *cstr;
  int oldcolours;

  CreateConsole();

  /* set the new colours */
  oldcolours=amx_setattr((int)params[2],(int)params[3],(int)params[4]);

  cstr=amx_Address(amx,params[1]);
  amx_printstring(amx,cstr,NULL);

  /* reset the colours */
  (void)amx_setattr(oldcolours & 0xff,(oldcolours >> 8) & 0x7f,(oldcolours >> 15) & 0x01);
  amx_fflush();
  return 0;
}
#endif

static cell AMX_NATIVE_CALL n_printf(AMX *amx,const cell *params)
{
  cell *cstr;
  AMX_FMTINFO info;

  memset(&info,0,sizeof info);
  info.params=params+2;
  info.numparams=(int)(params[0]/sizeof(cell))-1;
  info.skip=0;
  info.length=INT_MAX;

  CreateConsole();
  cstr=amx_Address(amx,params[1]);
  amx_printstring(amx,cstr,&info);
  amx_fflush();
  return 0;
}

/* getchar(bool:echo=true) */
static cell AMX_NATIVE_CALL n_getchar(AMX *amx,const cell *params)
{
  int c;

  (void)amx;
  CreateConsole();
  c=amx_getch();
  if (params[1]) {
    #if defined(SUPPRESS_ECHO)
 	  /* For Mac OS X, non-Curses, don't echo the character */
    #else
      amx_putchar((TCHAR)c);
      amx_fflush();
    #endif
  } /* if */
  return c;
}

/* getstring(string[], size=sizeof string, bool:pack=false) */
static cell AMX_NATIVE_CALL n_getstring(AMX *amx,const cell *params)
{
  int c,chars,max;
  cell *cptr;

  (void)amx;
  CreateConsole();
  chars=0;
  max=(int)params[2];
  if (max>0) {
    #if __STDC_VERSION__ >= 199901L
      TCHAR str[max];   /* use C99 feature if available */
    #else
      TCHAR *str=(TCHAR *)alloca(max*sizeof(TCHAR));
      if (str==NULL)
        return chars;
    #endif

    c=amx_getch();
    while (c!=EOF && c!=EOL_CHAR && chars<max-1) {
      str[chars++]=(TCHAR)c;
      #if defined(SUPPRESS_ECHO)
 	    /* For Mac OS X, non-Curses, don't echo the character */
      #else
        amx_putchar((TCHAR)c);
        amx_fflush();
      #endif
      if (chars<max-1)
        c=amx_getch();
    } /* while */

    if (c==EOL_CHAR)
      amx_putchar('\n');
    assert(chars<max);
    str[chars]='\0';

    cptr=amx_Address(amx,params[1]);
    amx_SetString(cptr,(char*)str,(int)params[3],sizeof(TCHAR)>1,max);

  } /* if */
  return chars;
}

static void acceptchar(int c,int *num)
{
  switch (c) {
  case '\b':
    amx_putchar('\b');
    *num-=1;
    #if defined amx_putchar && (defined __BORLANDC__ || defined __WATCOMC__)
      /* the backspace key does not erase the
       * character, so do this explicitly */
      amx_putchar(' ');     /* erase */
      amx_putchar('\b');    /* go back */
    #endif
    break;
  case EOL_CHAR:
    amx_putchar('\n');
    *num+=1;
    break;
  default:
    #if defined(SUPPRESS_ECHO)
 	  /* For Mac OS X, non-Curses, don't echo the character */
    #else
      amx_putchar((TCHAR)c);
    #endif
    *num+=1;
  } /* switch */
  amx_fflush();
}

static int inlist(AMX *amx,int c,const cell *params,int num)
{
  int i, key;

  (void)amx;
  for (i=0; i<num; i++) {
    if (i==0) {
      /* first key is passed by value, others are passed by reference */
      key = (int)params[i];
    } else {
      cell *cptr;
      cptr=amx_Address(amx,params[i]);
      key=(int)*cptr;
    } /* if */
    if (c==key || c==-key)
      return key;
  } /* for */
  return 0;
}

static cell AMX_NATIVE_CALL n_getvalue(AMX *amx,const cell *params)
{
  cell value;
  int base,sign,c,d;
  int chars,n;

  CreateConsole();
  base=(int)params[1];
  if (base<2 || base>36)
    return 0;

  chars=0;
  value=0;
  sign=1;       /* to avoid a compiler warning (Microsoft Visual C/C++ 6.0) */

  c=amx_getch();
  while (c!=EOF) {
    /* check for sign (if any) */
    if (chars==0) {
      if (c=='-') {
        sign=-1;
        acceptchar(c,&chars);
        c=amx_getch();
      } else {
        sign=1;
      } /* if */
    } /* if */

    /* check end of input */
    #if EOL_CHAR!='\r'
      if (c==EOL_CHAR && inlist(amx,'\r',params+2,(int)params[0]/sizeof(cell)-1)!=0)
        c='\r';
    #endif
    if ((chars>1 || chars>0 && sign>0)
        && (n=inlist(amx,c,params+2,(int)params[0]/sizeof(cell)-1))!=0)
    {
      if (n>0)
        acceptchar(c,&chars);
      break;
    } /* if */
    #if EOL_CHAR!='\r'
      if (c=='\r')
        c=EOL_CHAR;
    #endif

    /* get value */
    d=base;     /* by default, do not accept the character */
    if (c>='0' && c<='9') {
      d=c-'0';
    } else if (c>='a' && c<='z') {
      d=c-'a'+10;
    } else if (c>='A' && c<='Z') {
      d=c-'A'+10;
    } else if (c=='\b') {
      if (chars>0) {
        value/=base;
        acceptchar(c,&chars);
      } /* if */
    } /* if */
    if (d<base) {
      acceptchar(c,&chars);
      value=value*base + d;
    } /* if */
    c=amx_getch();
  } /* while */
  return sign*value;
}

static cell AMX_NATIVE_CALL n_clrscr(AMX *amx,const cell *params)
{
  (void)amx;
  (void)params;
  CreateConsole();
  amx_clrscr();
  return 0;
}

static cell AMX_NATIVE_CALL n_clreol(AMX *amx,const cell *params)
{
  (void)amx;
  (void)params;
  CreateConsole();
  amx_clreol();
  return 0;
}

static cell AMX_NATIVE_CALL n_gotoxy(AMX *amx,const cell *params)
{
  (void)amx;
  CreateConsole();
  return amx_gotoxy((int)params[1],(int)params[2]);
}

static cell AMX_NATIVE_CALL n_wherexy(AMX *amx,const cell *params)
{
  cell *px,*py;
  int x,y;

  (void)amx;
  CreateConsole();
  amx_wherexy(&x,&y);
  px=amx_Address(amx,params[1]);
  py=amx_Address(amx,params[2]);
  *px=x;
  *py=y;
  return 0;
}

static cell AMX_NATIVE_CALL n_setattr(AMX *amx,const cell *params)
{
  (void)amx;
  CreateConsole();
  (void)amx_setattr((int)params[1],(int)params[2],(int)params[3]);
  return 0;
}

static cell AMX_NATIVE_CALL n_consctrl(AMX *amx,const cell *params)
{
  (void)amx;
  CreateConsole();
  (void)amx_termctl((int)params[1],(int)params[2]);
  return 0;
}

static cell AMX_NATIVE_CALL n_console(AMX *amx,const cell *params)
{
  (void)amx;
  CreateConsole();
  amx_console((int)params[1],(int)params[2],(int)params[3]);
  return 0;
}


#if !defined AMXCONSOLE_NOIDLE
static AMX_IDLE PrevIdle = NULL;
static int idxKeyPressed = -1;

static int AMXAPI amx_ConsoleIdle(AMX *amx, int AMXAPI Exec(AMX *, cell *, int))
{
  int err=0, key;

  assert(idxKeyPressed >= 0);

  if (PrevIdle != NULL)
    PrevIdle(amx, Exec);

  if (amx_kbhit()) {
    key = amx_getch();
    amx_Push(amx, key);
    err = Exec(amx, NULL, idxKeyPressed);
    while (err == AMX_ERR_SLEEP)
      err = Exec(amx, NULL, AMX_EXEC_CONT);
  } /* if */

  return err;
}
#endif

#if defined __cplusplus
  extern "C"
#endif
const AMX_NATIVE_INFO console_Natives[] = {
  { "getchar",   n_getchar },
  { "getstring", n_getstring },
  { "getvalue",  n_getvalue },
  { "print",     n_print },
  { "printf",    n_printf },
  { "clrscr",    n_clrscr },
  { "clreol",    n_clreol },
  { "gotoxy",    n_gotoxy },
  { "wherexy",   n_wherexy },
  { "setattr",   n_setattr },
  { "console",   n_console },
  { "consctrl",  n_consctrl },
  { NULL, NULL }        /* terminator */
};

int AMXEXPORT AMXAPI amx_ConsoleInit(AMX *amx)
{
  #if !defined AMXCONSOLE_NOIDLE
    /* see whether there is an @keypressed() function */
    if (amx_FindPublic(amx, "@keypressed", &idxKeyPressed) == AMX_ERR_NONE) {
      if (amx_GetUserData(amx, AMX_USERTAG('I','d','l','e'), (void**)&PrevIdle) != AMX_ERR_NONE)
        PrevIdle = NULL;
      amx_SetUserData(amx, AMX_USERTAG('I','d','l','e'), (void*)amx_ConsoleIdle);
    } /* if */
  #endif

  return amx_Register(amx, console_Natives, -1);
}

int AMXEXPORT AMXAPI amx_ConsoleCleanup(AMX *amx)
{
  (void)amx;
  #if !defined AMXCONSOLE_NOIDLE
    PrevIdle = NULL;
  #endif
  return AMX_ERR_NONE;
}

#endif /* AMX_STRING_LIB */
