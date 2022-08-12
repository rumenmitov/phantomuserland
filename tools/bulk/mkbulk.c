#include <stdio.h>
#include <stdlib.h>
#include <ph_string.h>
//#include "../../include/vm/bulk.h"

FILE *outf;

int fn2cn( char *out, const char *in, int outsz )
{
    char *p;

    p = rindex( in, '/' );
    if( p ) in = p+1;

    p = rindex( in, '\\' );
    if( p ) in = p+1;

    p = rindex( in, ':' );
    if( p ) in = p+1;

    ph_strlcpy( out, in, outsz );
    out[outsz-1] = '\0';

    p = rindex( out, '.' );

    if( (p != 0) && (0 == ph_strcmp( p, ".pc" )) )
    {
        *p = '\0';
        return 0;
    }

    ph_printf("input file (%s) has no '.pc' extension, skip, ", in);
    return 1;
}

/*
void save_hdr( char *classnm, long size )
{
    struct pvm_bulk_class_head h;
    ph_strncpy( h.name, classnm, PVM_BULK_CN_LENGTH );
    h.data_length = size;

    //return 1 ==
    fwrite( &h, sizeof(h), 1, outf );
}
*/

void copyf( FILE *outfp, FILE *infp, int lencheck )
{
    fseek( infp, 0L, SEEK_SET );

    const int bs = 8192;
    char buf[bs];

    int len;

    while(!feof(infp))
    {
        len = fread( buf, 1, bs, infp );
        if( len < 0 )
        {
            ph_printf("Read error\n");
            exit(2);
        }

        int res = fwrite( buf, 1, len, outfp );
        if( len != res )
        {
            ph_printf("Write error\n");
            exit(2);
        }

        lencheck -= res;
    }

    if( lencheck != 0 )
        {
            ph_printf("Length error\n");
            exit(2);
        }
}

int main( int ac, char **av )
{
    if( (ac < 3) || (av[0][0] == '-') )
    {
        ph_printf(
               "mkbulk: combine Phantom class files to a special\n"
               "bulk file to bundle with kernel (classes boot module)\n"
               "\n"
               "Usage: mkbulk outfile infile [...]\n"
              );
        exit(1);
    }

    ac--; av++;

    char *outfn = av[0];
    ac--;
    av++;

    //ph_printf("                       ----  Writing bulk\n");
    //ph_printf("                       ----  Writing bulk to %s: ", outfn);
    //ph_printf("                       ----  Writing bulk to %p: ", outfn);

    //outf = fopen( outfn, "wb" );
    outf = fopen( outfn, "w" );
    if( outf == NULL )
    {
        ph_printf("Can't open %s for write\n", outfn);
        exit(1);
    }

    //ph_printf("Writing bulk to %s: ", outfn);


    while( ac-- )
    {
        const char *infn = *av++;

        const cns = 1024;
        char cn[cns];
        int fail = fn2cn( cn, infn, cns );

        if(fail) continue;

        FILE *inf = fopen( infn, "rb" );
        if( outf == NULL )
        {
            ph_printf("can't open %s, skip%c ", infn, ac == 0 ? ' ' : ',');
            continue;
        }

        //ph_printf("%s%c ", infn, ac == 0 ? ' ' : ',' );

        if( fseek( inf, 0, SEEK_END ) )
        {
            ph_printf("can't seek %s, skip%c ", infn, ac == 0 ? ' ' : ',');
            fclose(inf);
            continue;
        }

        long size = ftell(inf);

        // TODO read class name from the class file!
        save_hdr( cn, size );
        copyf( outf, inf, (int)size );

        if(ferror(inf) || ferror(outf))
        {
            ph_printf("I/O error\n");
            exit(2);
        }

        fclose( inf );
    }

    fclose(outf);
    //ph_printf("done\n");
    return 0;
}

