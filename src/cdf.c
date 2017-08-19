#define _GNU_SOURCE // timegm

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <netcdf.h>

#define XX(cmd)                           \
    {                                     \
        int status = (cmd);               \
        if (status)                       \
        {                                 \
            fprintf(stderr,               \
                    "%s:%i: error: %s\n", \
                    __FILE__,             \
                    __LINE__,             \
                    nc_strerror(status)); \
            return 1;                     \
        }                                 \
    }

int main(int argc, char **argv)
{

    assert(argc == 2);

    int ncid;
    XX(nc_open(argv[1], 0, &ncid));

    //
    // station name
    //

    int varid;
    XX(nc_inq_varid(ncid, "stationname", &varid));

    int ndims;
    XX(nc_inq_varndims(ncid, varid, &ndims));

    int station_dimids[ndims];
    XX(nc_inq_vardimid(ncid, varid, station_dimids));

    size_t stationnames_len;
    XX(nc_inq_dimlen(ncid, station_dimids[0], &stationnames_len));

    char *stationnames[stationnames_len];
    XX(nc_get_var_string(ncid, varid, stationnames));

    //
    // minimum temp
    //

    XX(nc_inq_varid(ncid, "tn", &varid));

    XX(nc_inq_varndims(ncid, varid, &ndims));

    int tn_dimids[ndims];
    XX(nc_inq_vardimid(ncid, varid, tn_dimids));

    size_t tmin_len;
    XX(nc_inq_dimlen(ncid, tn_dimids[0], &tmin_len));
    assert(tmin_len == stationnames_len);

    double tmin[tmin_len];
    XX(nc_get_var_double(ncid, varid, tmin));

    //
    // maximum temp
    //

    XX(nc_inq_varid(ncid, "tx", &varid));

    XX(nc_inq_varndims(ncid, varid, &ndims));

    int tx_dimids[ndims];
    XX(nc_inq_vardimid(ncid, varid, tx_dimids));

    size_t tmax_len;
    XX(nc_inq_dimlen(ncid, tx_dimids[0], &tmax_len));
    assert(tmax_len == stationnames_len);

    double tmax[tmax_len];
    XX(nc_get_var_double(ncid, varid, tmax));

    //
    // uitvoer
    //

    for (size_t i = 0; i < stationnames_len; i++)
    {
        double temp = nan("");
        if (tmin[i] > -300 && tmax[i] > -300)
        {
            temp = (tmin[i] + tmax[i]) / 2;
        }
        printf("%2li:  %4.1f°C  %s\n", i + 1, temp, stationnames[i]);
    }

    //
    // time
    //

    XX(nc_inq_varid(ncid, "time", &varid));

    XX(nc_inq_varndims(ncid, varid, &ndims));

    int time_dimids[ndims];
    XX(nc_inq_vardimid(ncid, varid, time_dimids));

    size_t time_len;
    XX(nc_inq_dimlen(ncid, time_dimids[0], &time_len));

    double time[time_len];
    XX(nc_get_var_double(ncid, varid, time));

    // aanname: dst voor lokale tijd op 1950-1-1 is gelijk aan die op 1970-1-1
    struct tm ts = {
        .tm_sec = 0,
        .tm_min = 0,
        .tm_hour = 0,
        .tm_mday = 1,
        .tm_mon = 0,
        .tm_year = 50,
        .tm_isdst = 0};
    time_t tijd = timegm(&ts) + (time_t)(time[0]);
    printf("Tijd (UTC)  : %s", asctime(gmtime(&tijd)));
    printf("Tijd (local): %s", asctime(localtime(&tijd)));

    //
    // clean up
    //

    XX(nc_close(ncid));

    for (size_t i = 0; i < stationnames_len; i++)
    {
        free(stationnames[i]);
    }
}
