#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netcdf.h>

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

int doDouble(int ncid, int varid, int ndims);
int doString(int ncid, int varid, int ndims);
char *quote(char *s);

int main(int argc, char **argv)
{
    assert(argc == 2);

    int ncid;
    XXnc(nc_open(argv[1], 0, &ncid));

    int ndims;
    XXnc(nc_inq_ndims(ncid, &ndims));

    char dimnames[ndims][NC_MAX_NAME + 1];
    size_t dimlens[ndims];
    for (int i = 0; i < ndims; i++)
    {
        XXnc(nc_inq_dimname(ncid, i, dimnames[i]));
        XXnc(nc_inq_dimlen(ncid, i, &dimlens[i]));
    }
    printf("{\n  \"dimensions\": {\n");
    for (int i = 0; i < ndims; i++)
    {
        printf("    \"%s\": %lu%s\n", quote(dimnames[i]), dimlens[i], i == ndims - 1 ? "" : ",");
    }
    printf("  },\n  \"variables\": {");

    int nvars;
    for (nvars = 0;; nvars++)
        if (nc_inq_varname(ncid, nvars, NULL))
            break;

    for (int varid = 0; varid < nvars; varid++)
    {
        char name[NC_MAX_NAME + 1];
        nc_type xtype;
        int ndims, natts;
        XXnc(nc_inq_var(ncid, varid, name, &xtype, &ndims, NULL, &natts));

        printf("%s\n    \"%s\": {", varid ? "," : "", quote(name));

        for (int attnum = 0;; attnum++)
        {
            char name[NC_MAX_NAME + 1];
            if (nc_inq_attname(ncid, varid, attnum, name))
                break;
            nc_type xtype;
            XXnc(nc_inq_atttype(ncid, varid, name, &xtype));
            printf("%s\n      \"%s\": ", attnum ? "," : "", quote(name));

            if (xtype == NC_CHAR)
            {
                size_t len;
                XXnc(nc_inq_attlen(ncid, varid, name, &len));
                char v[len + 1];
                v[len] = '\0';
                XXnc(nc_get_att_text(ncid, varid, name, v));
                printf("\"%s\"", quote(v));
            }
            else if (xtype == NC_DOUBLE)
            {
                double v;
                XXnc(nc_get_att_double(ncid, varid, name, &v));
                printf("%g", v);
            }
            else if (xtype == NC_FLOAT)
            {
                float v;
                XXnc(nc_get_att_float(ncid, varid, name, &v));
                printf("%g", v);
            }
            else
            {
                printf("null");
            }
        }

        if (xtype == NC_DOUBLE)
        {
            if (doDouble(ncid, varid, ndims))
                return 1;
        }
        else if (xtype == NC_STRING)
        {
            if (doString(ncid, varid, ndims))
                return 1;
        }

        printf("\n    }");
    }
    printf("\n  }\n}\n");
}

int doDouble(int ncid, int varid, int ndims)
{
    if (ndims < 1 || ndims > 2)
        return 1;

    if (ndims == 1)
    {
        int dimids[1];
        XXnc(nc_inq_vardimid(ncid, varid, dimids));

        size_t dimlen;
        XXnc(nc_inq_dimlen(ncid, dimids[0], &dimlen));

        double v[dimlen];
        XXnc(nc_get_var_double(ncid, varid, v));

        printf(",\n      \"values\": [");
        for (int i = 0; i < dimlen; i++)
        {
            long int li = v[i];
            if (v[i] - li)
                printf("%s\n        %g", i ? "," : "", v[i]);
            else
                printf("%s\n        %li", i ? "," : "", li);
        }
        printf("\n      ]");

        return 0;
    }

    int dimids[2];
    XXnc(nc_inq_vardimid(ncid, varid, dimids));

    size_t dimlen[2];
    for (int i = 0; i < 2; i++)
        XXnc(nc_inq_dimlen(ncid, dimids[i], &dimlen[i]));

    double v[dimlen[0]][dimlen[1]];
    XXnc(nc_get_var_double(ncid, varid, &(v[0][0])));

    printf(",\n      \"values\": [");
    for (int j = 0; j < dimlen[1]; j++)
    {
        if (dimlen[1] > 1) 
            printf("%s\n        [", j ? "," : "");
        for (int i = 0; i < dimlen[0]; i++)
        {
            long int li = v[i][j];
            if (v[i][j] - li)
                printf("%s\n          %g", i ? "," : "", v[i][j]);
            else
                printf("%s\n          %li", i ? "," : "", li);
        }
        if (dimlen[1] > 1)
            printf("\n        ]");
    }
    printf("\n      ]");

    return 0;
}

int doString(int ncid, int varid, int ndims)
{
    if (ndims != 1)
        return 1;

    int dimids[1];
    XXnc(nc_inq_vardimid(ncid, varid, dimids));

    size_t dimlen;
    XXnc(nc_inq_dimlen(ncid, dimids[0], &dimlen));

    char *v[dimlen];
    XXnc(nc_get_var_string(ncid, varid, v));

    printf(",\n      \"values\": [");
    for (int i = 0; i < dimlen; i++)
    {
        printf("%s\n        \"%s\"", i ? "," : "", quote(v[i]));
    }
    printf("\n      ]");
    return 0;
}

char *quote(char *s)
{
    char *s2 = (char *)malloc(1 + 2 * strlen(s));
    assert(s2);
    int j = 0;
    for (int i = 0; s[i]; i++)
    {
        if (s[i] == '\n')
        {
            s[j++] = '\\';
            s[j++] = 'n';
            continue;
        }
        if (s[i] == '"' || s[i] == '\\')
            s2[j++] = '\\';
        s2[j++] = s[i];
    }
    s2[j] = '\0';
    return s2;
}
