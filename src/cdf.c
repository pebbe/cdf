#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <netcdf.h>

typedef struct
{
    int rv;
    size_t *dimlen;
    double **values;
} data_double;

#define XXnc(cmd)                         \
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

#define XX(cmd)                          \
    {                                    \
        int status = (cmd);              \
        if (status)                      \
        {                                \
            fprintf(stderr,              \
                    "%s:%i: error %i\n", \
                    __FILE__,            \
                    __LINE__,            \
                    status);             \
            return 1;                    \
        }                                \
    }

data_double cdf_get_double(int ncid, char *varname);

void cdf_free_double(data_double data);

int main(int argc, char **argv)
{

    assert(argc == 2);

    int ncid;
    XXnc(nc_open(argv[1], 0, &ncid));

    //
    // station name
    //

    int varid;
    XXnc(nc_inq_varid(ncid, "stationname", &varid));

    int ndims;
    XXnc(nc_inq_varndims(ncid, varid, &ndims));

    int station_dimids[ndims];
    XXnc(nc_inq_vardimid(ncid, varid, station_dimids));

    size_t stationnames_len;
    XXnc(nc_inq_dimlen(ncid, station_dimids[0], &stationnames_len));

    char *stationnames[stationnames_len];
    XXnc(nc_get_var_string(ncid, varid, stationnames));

    //
    // minimum temp
    //

    data_double tmin = cdf_get_double(ncid, "tn");
    XX(tmin.rv);

    assert(tmin.dimlen[0] == stationnames_len);

    //
    // maximum temp
    //

    data_double tmax = cdf_get_double(ncid, "tx");
    XX(tmax.rv);

    assert(tmax.dimlen[0] == stationnames_len);

    //
    // uitvoer
    //

    for (size_t i = 0; i < stationnames_len; i++)
    {
        double temp = nan("");
        if (tmin.values[0][i] > -300 && tmax.values[0][i] > -300)
        {
            temp = (tmin.values[0][i] + tmax.values[0][i]) / 2;
        }
        printf("%2li:  %4.1f°C  %s\n", i + 1, temp, stationnames[i]);
    }

    //
    // time
    //

    data_double time = cdf_get_double(ncid, "time");
    XX(time.rv);

    time_t tijd = (time_t)(time.values[0][0]) - 631152000;
    printf("Tijd (UTC)  : %s", asctime(gmtime(&tijd)));
    printf("Tijd (local): %s", asctime(localtime(&tijd)));

    //
    // clean up
    //

    XXnc(nc_close(ncid));

    cdf_free_double(tmin);
    cdf_free_double(tmax);
    cdf_free_double(time);
    for (size_t i = 0; i < stationnames_len; i++)
    {
        free(stationnames[i]);
    }
}

data_double cdf_get_double(int ncid, char *varname)
{

#define XXresult(cmd)                        \
    {                                        \
        result.rv = cmd;                     \
        if (result.rv)                       \
        {                                    \
            fprintf(stderr,                  \
                    "%s:%i: error: %s\n",    \
                    __FILE__,                \
                    __LINE__,                \
                    nc_strerror(result.rv)); \
            return result;                   \
        }                                    \
    }

#define XXmalloc(var, cmd)                    \
    {                                         \
        var = cmd;                            \
        if (!var)                             \
        {                                     \
            fprintf(stderr,                   \
                    "%s:%i: malloc failed\n", \
                    __FILE__,                 \
                    __LINE__);                \
            result.rv = -1;                   \
            return result;                    \
        }                                     \
    }

    data_double result;

    int varid;
    XXresult(nc_inq_varid(ncid, varname, &varid));

    int ndims;
    XXresult(nc_inq_varndims(ncid, varid, &ndims));

    int dimids[ndims];
    XXresult(nc_inq_vardimid(ncid, varid, dimids));

    // twee dimensies gebruiken
    // als één, dan lengte 2e dimensie is 1
    // als meer dan twee, dan twee
    XXmalloc(result.dimlen, (size_t *)malloc((ndims < 2 ? 2 : ndims) * sizeof(size_t)))
        result.dimlen[1] = 1;
    for (int i = 0; i < ndims && i < 2; i++)
        XXresult(nc_inq_dimlen(ncid, dimids[i], &(result.dimlen[i])));

    XXmalloc(result.values, (double **)malloc(result.dimlen[1] * sizeof(double *)));
    for (int i = 0; i < result.dimlen[1]; i++)
    {
        XXmalloc(result.values[i], (double *)malloc(result.dimlen[0] * sizeof(double)));
        XXresult(nc_get_var_double(ncid, varid, result.values[i]));
    }
    return result;
}

void cdf_free_double(data_double data)
{
    for (int i = 0; i < data.dimlen[1]; i++)
    {
        free(data.values[i]);
    }
    free(data.values);
    free(data.dimlen);
}
