#include "wintimes.h"

clock_t convert_filetime( const FILETIME *ac_FileTime )  {
    ULARGE_INTEGER    lv_Large ;

    lv_Large.LowPart  = ac_FileTime->dwLowDateTime   ;
    lv_Large.HighPart = ac_FileTime->dwHighDateTime  ;

    return (clock_t)lv_Large.QuadPart ;
}

/*
    Thin emulation of the unix times function
*/
void times(tms *time_struct) {
    FILETIME time_create, time_exit, accum_sys, accum_user;

    GetProcessTimes( GetCurrentProcess(),
            &time_create, &time_exit, &accum_sys, &accum_user );

    time_struct->tms_utime = convert_filetime(&accum_user);
    time_struct->tms_stime = convert_filetime(&accum_sys);
}