#ifndef _FLANG_WINTIMES_H
#define _FLANG_WINTIMES_H
  #include <Windows.h>
  #include <time.h> 

  typedef struct tms {
    clock_t tms_utime;  /* user time */
    clock_t tms_stime;  /* system time */
    clock_t tms_cutime; /* user time of children */
    clock_t tms_cstime; /* system time of children */
  } tms;

  clock_t convert_filetime( const FILETIME *ac_FileTime );

  /*
    Thin emulation of the unix times function
  */
  void times(tms *time_struct);
#endif
