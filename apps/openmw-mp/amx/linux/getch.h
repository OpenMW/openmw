/* Extremely inefficient but portable POSIX getch(), see getch.c */
#ifndef GETCH_H
#define GETCH_H

#if defined __cplusplus
  extern "C" {
#endif
int getch(void);
int kbhit(void);

#if defined __cplusplus
}
#endif

#endif /* GETCH_H */
