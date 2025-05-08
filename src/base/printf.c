/*
** Copyright (c) 2017-2025 Nikola Kolev <koue@chaosophia.net>
** Copyright (c) 2006 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)

** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
**
** This file contains implementions of routines for formatting output
** (ex: mprintf()) and for output to the console.
*/
#if 0 /* libfsl */
#include "config.h"
#include "printf.h"
#if defined(_WIN32)
#   include <io.h>
#   include <fcntl.h>
#endif
#include <time.h>
#endif /* libfsl */

#include "fslbase.h"

/* Two custom conversions are used to show a prefix of artifact hashes:
**
**      %!S       Prefix of a length appropriate for URLs
**      %S        Prefix of a length appropriate for human display
**
** The following macros help determine those lengths.  FOSSIL_HASH_DIGITS
** is the default number of digits to display to humans.  This value can
** be overridden using the hash-digits setting.  FOSSIL_HASH_DIGITS_URL
** is the minimum number of digits to be used in URLs.  The number used
** will always be at least 6 more than the number used for human output,
** or HNAME_MAX, whichever is least.
*/
#ifndef FOSSIL_HASH_DIGITS
# define FOSSIL_HASH_DIGITS 10       /* For %S (human display) */
#endif
#ifndef FOSSIL_HASH_DIGITS_URL
# define FOSSIL_HASH_DIGITS_URL 16   /* For %!S (embedded in URLs) */
#endif

#define HNAME_MAX  64     /* Length for SHA3-256 */

/*
** Return the number of artifact hash digits to display.  The number is for
** human output if the bForUrl is false and is destined for a URL if
** bForUrl is false.
*/
int hash_digits(int bForUrl){
  static int nDigitHuman = 0;
  static int nDigitUrl = 0;
  if( nDigitHuman==0 ){
#if 0 /* libfsl */
    nDigitHuman = db_get_int("hash-digits", FOSSIL_HASH_DIGITS);
#else
    nDigitHuman = 10;
#endif /* libfsl */
    if( nDigitHuman < 6 ) nDigitHuman = 6;
    if( nDigitHuman > HNAME_MAX ) nDigitHuman = HNAME_MAX;
    nDigitUrl = nDigitHuman + 6;
    if( nDigitUrl < FOSSIL_HASH_DIGITS_URL ) nDigitUrl = FOSSIL_HASH_DIGITS_URL;
    if( nDigitUrl > HNAME_MAX ) nDigitUrl = HNAME_MAX;
  }
  return bForUrl ? nDigitUrl : nDigitHuman;
}

#if 0 /* libfsl */
/*
** Return the number of characters in a %S output.
*/
int length_of_S_display(void){
  return hash_digits(0);
}
#endif /* libfsl */

/*
** Conversion types fall into various categories as defined by the
** following enumeration.
*/
#define etRADIX       1 /* Integer types.  %d, %x, %o, and so forth */
#define etFLOAT       2 /* Floating point.  %f */
#define etEXP         3 /* Exponential notation. %e and %E */
#define etGENERIC     4 /* Floating or exponential, depending on exponent. %g */
#define etSIZE        5 /* Return number of characters processed so far. %n */
#define etSTRING      6 /* Strings. %s */
#define etDYNSTRING   7 /* Dynamically allocated strings. %z */
#define etPERCENT     8 /* Percent symbol. %% */
#define etCHARX       9 /* Characters. %c */
#define etERROR      10 /* Used to indicate no such conversion type */
/* The rest are extensions, not normally found in printf() */
#define etBLOB       11 /* Blob objects.  %b */
#define etBLOBSQL    12 /* Blob objects quoted for SQL.  %B */
#define etSQLESCAPE  13 /* Strings with '\'' doubled.  %q */
#define etSQLESCAPE2 14 /* Strings with '\'' doubled and enclosed in '',
                          NULL pointers replaced by SQL NULL.  %Q */
#define etSQLESCAPE3 15 /* Double '"' characters within an indentifier.  %w */
#define etPOINTER    16 /* The %p conversion */
#define etHTMLIZE    17 /* Make text safe for HTML */
#define etHTTPIZE    18 /* Make text safe for HTTP.  "/" encoded as %2f */
#define etURLIZE     19 /* Make text safe for HTTP.  "/" not encoded */
#define etFOSSILIZE  20 /* The fossil header encoding format. */
#define etPATH       21 /* Path type */
#define etWIKISTR    22 /* Timeline comment text rendered from a char*: %W */
#define etSTRINGID   23 /* String with length limit for a hash prefix: %S */
#define etROOT       24 /* String value of g.zTop: %R */
#define etJSONSTR    25 /* String encoded as a JSON string literal: %j
                           Use %!j to include double-quotes around it. */
#define etSHELLESC   26 /* Escape a filename for use in a shell command: %$
                           See blob_append_escaped_arg() for details
                           "%$"  -> adds "./" prefix if necessary.
                           "%!$" -> omits the "./" prefix. */
#define etHEX        27 /* Encode a string as hexadecimal */


/*
** An "etByte" is an 8-bit unsigned value.
*/
typedef unsigned char etByte;

/*
** Each builtin conversion character (ex: the 'd' in "%d") is described
** by an instance of the following structure
*/
typedef struct et_info {   /* Information about each format field */
  char fmttype;            /* The format field code letter */
  etByte base;             /* The base for radix conversion */
  etByte flags;            /* One or more of FLAG_ constants below */
  etByte type;             /* Conversion paradigm */
  etByte charset;          /* Offset into aDigits[] of the digits string */
  etByte prefix;           /* Offset into aPrefix[] of the prefix string */
} et_info;

/*
** Allowed values for et_info.flags
*/
#define FLAG_SIGNED  1     /* True if the value to convert is signed */
#define FLAG_INTERN  2     /* True if for internal use only */
#define FLAG_STRING  4     /* Allow infinity precision */


/*
** The following table is searched linearly, so it is good to put the
** most frequently used conversion types first.
**
** NB: When modifying this table is it vital that you also update the fmtchr[]
** variable to match!!!
*/
static const char aDigits[] = "0123456789ABCDEF0123456789abcdef";
static const char aPrefix[] = "-x0\000X0";
static const char fmtchr[] = "dsgzqQbBWhRtTwFSjcouxXfeEGin%p/$H";
static const et_info fmtinfo[] = {
  {  'd', 10, 1, etRADIX,      0,  0 },
  {  's',  0, 4, etSTRING,     0,  0 },
  {  'g',  0, 1, etGENERIC,    30, 0 },
  {  'z',  0, 6, etDYNSTRING,  0,  0 },
  {  'q',  0, 4, etSQLESCAPE,  0,  0 },
  {  'Q',  0, 4, etSQLESCAPE2, 0,  0 },
  {  'b',  0, 2, etBLOB,       0,  0 },
  {  'B',  0, 2, etBLOBSQL,    0,  0 },
  {  'W',  0, 2, etWIKISTR,    0,  0 },
  {  'h',  0, 4, etHTMLIZE,    0,  0 },
  {  'R',  0, 0, etROOT,       0,  0 },
  {  't',  0, 4, etHTTPIZE,    0,  0 },  /* "/" -> "%2F" */
  {  'T',  0, 4, etURLIZE,     0,  0 },  /* "/" unchanged */
  {  'w',  0, 4, etSQLESCAPE3, 0,  0 },
  {  'F',  0, 4, etFOSSILIZE,  0,  0 },
  {  'S',  0, 4, etSTRINGID,   0,  0 },
  {  'j',  0, 0, etJSONSTR,    0,  0 },
  {  'c',  0, 0, etCHARX,      0,  0 },
  {  'o',  8, 0, etRADIX,      0,  2 },
  {  'u', 10, 0, etRADIX,      0,  0 },
  {  'x', 16, 0, etRADIX,      16, 1 },
  {  'X', 16, 0, etRADIX,      0,  4 },
  {  'f',  0, 1, etFLOAT,      0,  0 },
  {  'e',  0, 1, etEXP,        30, 0 },
  {  'E',  0, 1, etEXP,        14, 0 },
  {  'G',  0, 1, etGENERIC,    14, 0 },
  {  'i', 10, 1, etRADIX,      0,  0 },
  {  'n',  0, 0, etSIZE,       0,  0 },
  {  '%',  0, 0, etPERCENT,    0,  0 },
  {  'p', 16, 0, etPOINTER,    0,  1 },
  {  '/',  0, 0, etPATH,       0,  0 },
  {  '$',  0, 0, etSHELLESC,   0,  0 },
  {  'H',  0, 0, etHEX,        0,  0 },
  {  etERROR, 0,0,0,0,0}  /* Must be last */
};
#if 0 /* libfsl */
#define etNINFO count(fmtinfo)
#else
#define etNINFO 34
#endif /* libfsl */

#if 0 /* libfsl */
/*
** Verify that the fmtchr[] and fmtinfo[] arrays are in agreement.
**
** This routine is a defense against programming errors.
*/
void fossil_printf_selfcheck(void){
  int i;
  for(i=0; fmtchr[i]; i++){
    assert( fmtchr[i]==fmtinfo[i].fmttype );
  }
}
#endif /* libfsl */


/*
** "*val" is a double such that 0.1 <= *val < 10.0
** Return the ascii code for the leading digit of *val, then
** multiply "*val" by 10.0 to renormalize.
**
** Example:
**     input:     *val = 3.14159
**     output:    *val = 1.4159    function return = '3'
**
** The counter *cnt is incremented each time.  After counter exceeds
** 16 (the number of significant digits in a 64-bit float) '0' is
** always returned.
*/
static int et_getdigit(long double *val, int *cnt){
  int digit;
  long double d;
  if( (*cnt)++ >= 16 ) return '0';
  digit = (int)*val;
  d = digit;
  digit += '0';
  *val = (*val - d)*10.0;
  return digit;
}

/*
** Size of temporary conversion buffer.
*/
#define etBUFSIZE 500

/*
** Find the length of a string as long as that length does not
** exceed N bytes.  If no zero terminator is seen in the first
** N bytes then return N.  If N is negative, then this routine
** is an alias for strlen().
*/
#if _XOPEN_SOURCE >= 700 || _POSIX_C_SOURCE >= 200809L
# define StrNLen32(Z,N) (int)strnlen(Z,N)
#else
static int StrNLen32(const char *z, int N){
  int n = 0;
  while( (N-- != 0) && *(z++)!=0 ){ n++; }
  return n;
}
#endif

typedef long long int i64;

/* True if the last character standard output cursor is setting at
** the beginning of a blank link.  False if a \r has been to move the
** cursor to the beginning of the line or if not at the beginning of
** a line.
** was a \n
*/
static int stdoutAtBOL = 1;

/*
** Write to standard output or standard error.
**
** On windows, transform the output into the current terminal encoding
** if the output is going to the screen.  If output is redirected into
** a file, no translation occurs. Switch output mode to binary to
** properly process line-endings, make sure to switch the mode back to
** text when done.
** No translation ever occurs on unix.
*/
void fossil_puts(const char *z, int toStdErr, int n){
  FILE* out = (toStdErr ? stderr : stdout);
  if( n==0 ) return;
  assert( toStdErr==0 || toStdErr==1 );
  if( toStdErr==0 ) stdoutAtBOL = (z[n-1]=='\n');
#if defined(_WIN32)
  if( fossil_utf8_to_console(z, n, toStdErr) >= 0 ){
    return;
  }
  fflush(out);
  _setmode(_fileno(out), _O_BINARY);
#endif
  fwrite(z, 1, n, out);
#if defined(_WIN32)
  fflush(out);
  _setmode(_fileno(out), _O_TEXT);
#endif
}

/*
** The root program.  All variations call this core.
**
** INPUTS:
**   pBlob  This is the blob where the output will be built.
**
**   fmt    This is the format string, as in the usual print.
**
**   ap     This is a pointer to a list of arguments.  Same as in
**          vfprint.
**
** OUTPUTS:
**          The return value is the total number of characters sent to
**          the function "func".  Returns -1 on error.
**
** Note that the order in which automatic variables are declared below
** seems to make a big difference in determining how fast this beast
** will run.
*/
int vxprintf(
  Blob *pBlob,                       /* Append output to this blob */
  const char *fmt,                   /* Format string */
  va_list ap                         /* arguments */
){
  int c;                     /* Next character in the format string */
  char *bufpt;               /* Pointer to the conversion buffer */
  int precision;             /* Precision of the current field */
  int length;                /* Length of the field */
  int idx;                   /* A general purpose loop counter */
  int count;                 /* Total number of characters output */
  int width;                 /* Width of the current field */
  etByte flag_leftjustify;   /* True if "-" flag is present */
  etByte flag_plussign;      /* True if "+" flag is present */
  etByte flag_blanksign;     /* True if " " flag is present */
  etByte flag_alternateform; /* True if "#" flag is present */
  etByte flag_altform2;      /* True if "!" flag is present */
  etByte flag_zeropad;       /* True if field width constant starts with zero */
  etByte flag_long;          /* True if "l" flag is present */
  etByte flag_longlong;      /* True if the "ll" flag is present */
  etByte done;               /* Loop termination flag */
  etByte cThousand;          /* Thousands separator for %d and %u */
  u64 longvalue;             /* Value for integer types */
  long double realvalue;     /* Value for real types */
  const et_info *infop;      /* Pointer to the appropriate info structure */
  char buf[etBUFSIZE];       /* Conversion buffer */
  char prefix;               /* Prefix character.  "+" or "-" or " " or '\0'. */
  etByte errorflag = 0;      /* True if an error is encountered */
  etByte xtype;              /* Conversion paradigm */
  char *zExtra;              /* Extra memory used for etTCLESCAPE conversions */
  static const char spaces[] =
   "                                                                         ";
#define etSPACESIZE (sizeof(spaces)-1)
  int  exp, e2;              /* exponent of real numbers */
  double rounder;            /* Used for rounding floating point values */
  etByte flag_dp;            /* True if decimal point should be shown */
  etByte flag_rtz;           /* True if trailing zeros should be removed */
  etByte flag_exp;           /* True to force display of the exponent */
  int nsd;                   /* Number of significant digits returned */
  char *zFmtLookup;

  count = length = 0;
  bufpt = 0;
  for(; (c=(*fmt))!=0; ++fmt){
    if( c!='%' ){
      bufpt = (char *)fmt;
#if HAVE_STRCHRNUL
      fmt = strchrnul(fmt, '%');
#else
      do{ fmt++; }while( *fmt && *fmt != '%' );
#endif
      blob_append(pBlob, bufpt, (int)(fmt - bufpt));
      if( *fmt==0 ) break;
    }
    if( (c=(*++fmt))==0 ){
      errorflag = 1;
      blob_append(pBlob,"%",1);
      count++;
      break;
    }
    /* Find out what flags are present */
    flag_leftjustify = flag_plussign = flag_blanksign = cThousand =
     flag_alternateform = flag_altform2 = flag_zeropad = 0;
    done = 0;
    do{
      switch( c ){
        case '-':   flag_leftjustify = 1;     break;
        case '+':   flag_plussign = 1;        break;
        case ' ':   flag_blanksign = 1;       break;
        case '#':   flag_alternateform = 1;   break;
        case '!':   flag_altform2 = 1;        break;
        case '0':   flag_zeropad = 1;         break;
        case ',':   cThousand = ',';          break;
        default:    done = 1;                 break;
      }
    }while( !done && (c=(*++fmt))!=0 );
    /* Get the field width */
    width = 0;
    if( c=='*' ){
      width = va_arg(ap,int);
      if( width<0 ){
        flag_leftjustify = 1;
        width = -width;
      }
      c = *++fmt;
    }else{
      while( c>='0' && c<='9' ){
        width = width*10 + c - '0';
        c = *++fmt;
      }
    }
    if( width > etBUFSIZE-10 ){
      width = etBUFSIZE-10;
    }
    /* Get the precision */
    if( c=='.' ){
      precision = 0;
      c = *++fmt;
      if( c=='*' ){
        precision = va_arg(ap,int);
        if( precision<0 ) precision = -precision;
        c = *++fmt;
      }else{
        while( c>='0' && c<='9' ){
          precision = precision*10 + c - '0';
          c = *++fmt;
        }
      }
    }else{
      precision = -1;
    }
    /* Get the conversion type modifier */
    if( c=='l' ){
      flag_long = 1;
      c = *++fmt;
      if( c=='l' ){
        flag_longlong = 1;
        c = *++fmt;
      }else{
        flag_longlong = 0;
      }
    }else{
      flag_long = flag_longlong = 0;
    }
    /* Fetch the info entry for the field */
    zFmtLookup = strchr(fmtchr,c);
    if( zFmtLookup ){
      infop = &fmtinfo[zFmtLookup-fmtchr];
      xtype = infop->type;
    }else{
      infop = 0;
      xtype = etERROR;
    }
    zExtra = 0;

    /* Limit the precision to prevent overflowing buf[] during conversion */
    if( precision>etBUFSIZE-40 && (infop->flags & FLAG_STRING)==0 ){
      precision = etBUFSIZE-40;
    }

    /*
    ** At this point, variables are initialized as follows:
    **
    **   flag_alternateform          TRUE if a '#' is present.
    **   flag_altform2               TRUE if a '!' is present.
    **   flag_plussign               TRUE if a '+' is present.
    **   flag_leftjustify            TRUE if a '-' is present or if the
    **                               field width was negative.
    **   flag_zeropad                TRUE if the width began with 0.
    **   flag_long                   TRUE if the letter 'l' (ell) prefixed
    **                               the conversion character.
    **   flag_longlong               TRUE if the letter 'll' (ell ell) prefixed
    **                               the conversion character.
    **   flag_blanksign              TRUE if a ' ' is present.
    **   width                       The specified field width.  This is
    **                               always non-negative.  Zero is the default.
    **   precision                   The specified precision.  The default
    **                               is -1.
    **   xtype                       The class of the conversion.
    **   infop                       Pointer to the appropriate info struct.
    */
    switch( xtype ){
      case etPOINTER:
        flag_longlong = sizeof(char*)==sizeof(i64);
        flag_long = sizeof(char*)==sizeof(long int);
        /* Fall through into the next case */
      case etRADIX:
        if( infop->flags & FLAG_SIGNED ){
          i64 v;
          if( flag_longlong )   v = va_arg(ap,i64);
          else if( flag_long )  v = va_arg(ap,long int);
          else                  v = va_arg(ap,int);
          if( v<0 ){
            longvalue = -v;
            prefix = '-';
          }else{
            longvalue = v;
            if( flag_plussign )        prefix = '+';
            else if( flag_blanksign )  prefix = ' ';
            else                       prefix = 0;
          }
        }else{
          if( flag_longlong )   longvalue = va_arg(ap,u64);
          else if( flag_long )  longvalue = va_arg(ap,unsigned long int);
          else                  longvalue = va_arg(ap,unsigned int);
          prefix = 0;
        }
        if( longvalue==0 ) flag_alternateform = 0;
        if( flag_zeropad && precision<width-(prefix!=0) ){
          precision = width-(prefix!=0);
        }
        bufpt = &buf[etBUFSIZE-1];
        {
          register const char *cset;      /* Use registers for speed */
          register int base;
          cset = &aDigits[infop->charset];
          base = infop->base;
          do{                                           /* Convert to ascii */
            *(--bufpt) = cset[longvalue%base];
            longvalue = longvalue/base;
          }while( longvalue>0 );
        }
        length = &buf[etBUFSIZE-1]-bufpt;
        while( precision>length ){
          *(--bufpt) = '0';                             /* Zero pad */
          length++;
        }
        if( cThousand ){
          int nn = (length - 1)/3;  /* Number of "," to insert */
          int ix = (length - 1)%3 + 1;
          bufpt -= nn;
          for(idx=0; nn>0; idx++){
            bufpt[idx] = bufpt[idx+nn];
            ix--;
            if( ix==0 ){
              bufpt[++idx] = cThousand;
              nn--;
              ix = 3;
            }
          }
        }
        if( prefix ) *(--bufpt) = prefix;               /* Add sign */
        if( flag_alternateform && infop->prefix ){      /* Add "0" or "0x" */
          const char *pre;
          char x;
          pre = &aPrefix[infop->prefix];
          if( *bufpt!=pre[0] ){
            for(; (x=(*pre))!=0; pre++) *(--bufpt) = x;
          }
        }
        length = &buf[etBUFSIZE-1]-bufpt;
        break;
      case etFLOAT:
      case etEXP:
      case etGENERIC:
        realvalue = va_arg(ap,double);
        if( precision<0 ) precision = 6;         /* Set default precision */
        if( precision>etBUFSIZE/2-10 ) precision = etBUFSIZE/2-10;
        if( realvalue<0.0 ){
          realvalue = -realvalue;
          prefix = '-';
        }else{
          if( flag_plussign )          prefix = '+';
          else if( flag_blanksign )    prefix = ' ';
          else                         prefix = 0;
        }
        if( xtype==etGENERIC && precision>0 ) precision--;
#if 0
        /* Rounding works like BSD when the constant 0.4999 is used.  Wierd! */
        for(idx=precision, rounder=0.4999; idx>0; idx--, rounder*=0.1);
#else
        /* It makes more sense to use 0.5 */
        for(idx=precision, rounder=0.5; idx>0; idx--, rounder*=0.1);
#endif
        if( xtype==etFLOAT ) realvalue += rounder;
        /* Normalize realvalue to within 10.0 > realvalue >= 1.0 */
        exp = 0;
        if( realvalue>0.0 ){
          while( realvalue>=1e32 && exp<=350 ){ realvalue *= 1e-32; exp+=32; }
          while( realvalue>=1e8 && exp<=350 ){ realvalue *= 1e-8; exp+=8; }
          while( realvalue>=10.0 && exp<=350 ){ realvalue *= 0.1; exp++; }
          while( realvalue<1e-8 && exp>=-350 ){ realvalue *= 1e8; exp-=8; }
          while( realvalue<1.0 && exp>=-350 ){ realvalue *= 10.0; exp--; }
          if( exp>350 || exp<-350 ){
            bufpt = "NaN";
            length = 3;
            break;
          }
        }
        bufpt = buf;
        /*
        ** If the field type is etGENERIC, then convert to either etEXP
        ** or etFLOAT, as appropriate.
        */
        flag_exp = xtype==etEXP;
        if( xtype!=etFLOAT ){
          realvalue += rounder;
          if( realvalue>=10.0 ){ realvalue *= 0.1; exp++; }
        }
        if( xtype==etGENERIC ){
          flag_rtz = !flag_alternateform;
          if( exp<-4 || exp>precision ){
            xtype = etEXP;
          }else{
            precision = precision - exp;
            xtype = etFLOAT;
          }
        }else{
          flag_rtz = 0;
        }
        if( xtype==etEXP ){
          e2 = 0;
        }else{
          e2 = exp;
        }
        nsd = 0;
        flag_dp = (precision>0) | flag_alternateform | flag_altform2;
        /* The sign in front of the number */
        if( prefix ){
          *(bufpt++) = prefix;
        }
        /* Digits prior to the decimal point */
        if( e2<0 ){
          *(bufpt++) = '0';
        }else{
          for(; e2>=0; e2--){
            *(bufpt++) = et_getdigit(&realvalue,&nsd);
          }
        }
        /* The decimal point */
        if( flag_dp ){
          *(bufpt++) = '.';
        }
        /* "0" digits after the decimal point but before the first
        ** significant digit of the number */
        for(e2++; e2<0 && precision>0; precision--, e2++){
          *(bufpt++) = '0';
        }
        /* Significant digits after the decimal point */
        while( (precision--)>0 ){
          *(bufpt++) = et_getdigit(&realvalue,&nsd);
        }
        /* Remove trailing zeros and the "." if no digits follow the "." */
        if( flag_rtz && flag_dp ){
          while( bufpt[-1]=='0' ) *(--bufpt) = 0;
          assert( bufpt>buf );
          if( bufpt[-1]=='.' ){
            if( flag_altform2 ){
              *(bufpt++) = '0';
            }else{
              *(--bufpt) = 0;
            }
          }
        }
        /* Add the "eNNN" suffix */
        if( flag_exp || (xtype==etEXP && exp) ){
          *(bufpt++) = aDigits[infop->charset];
          if( exp<0 ){
            *(bufpt++) = '-'; exp = -exp;
          }else{
            *(bufpt++) = '+';
          }
          if( exp>=100 ){
            *(bufpt++) = (exp/100)+'0';                /* 100's digit */
            exp %= 100;
          }
          *(bufpt++) = exp/10+'0';                     /* 10's digit */
          *(bufpt++) = exp%10+'0';                     /* 1's digit */
        }
        *bufpt = 0;

        /* The converted number is in buf[] and zero terminated. Output it.
        ** Note that the number is in the usual order, not reversed as with
        ** integer conversions. */
        length = bufpt-buf;
        bufpt = buf;

        /* Special case:  Add leading zeros if the flag_zeropad flag is
        ** set and we are not left justified */
        if( flag_zeropad && !flag_leftjustify && length < width){
          int i;
          int nPad = width - length;
          for(i=width; i>=nPad; i--){
            bufpt[i] = bufpt[i-nPad];
          }
          i = prefix!=0;
          while( nPad-- ) bufpt[i++] = '0';
          length = width;
        }
        break;
      case etSIZE:
        *(va_arg(ap,int*)) = count;
        length = width = 0;
        break;
      case etPERCENT:
        buf[0] = '%';
        bufpt = buf;
        length = 1;
        break;
      case etCHARX:
        c = buf[0] = va_arg(ap,int);
        if( precision>=0 ){
          for(idx=1; idx<precision; idx++) buf[idx] = c;
          length = precision;
        }else{
          length =1;
        }
        bufpt = buf;
        break;
      case etPATH: {
        int i;
        int limit = flag_alternateform ? va_arg(ap,int) : -1;
        char *e = va_arg(ap,char*);
        if( e==0 ){e="";}
        length = StrNLen32(e, limit);
        zExtra = bufpt = fossil_malloc(length+1);
        for( i=0; i<length; i++ ){
          if( e[i]=='\\' ){
            bufpt[i]='/';
          }else{
            bufpt[i]=e[i];
          }
        }
        bufpt[length]='\0';
        break;
      }
      case etROOT: {
#if 0 /* libfsl */
        bufpt = g.zTop ? g.zTop : "";
#else
        bufpt = "";
#endif /* libfsl */
        length = (int)strlen(bufpt);
        break;
      }
      case etSTRINGID:
      case etSTRING:
      case etDYNSTRING: {
        int limit = flag_alternateform ? va_arg(ap,int) : -1;
        bufpt = va_arg(ap,char*);
        if( bufpt==0 ){
          bufpt = "";
        }else if( xtype==etDYNSTRING ){
          zExtra = bufpt;
        }else if( xtype==etSTRINGID ){
          precision = hash_digits(flag_altform2);
        }
        length = StrNLen32(bufpt, limit);
        if( precision>=0 && precision<length ) length = precision;
        break;
      }
      case etBLOB: {
        int limit = flag_alternateform ? va_arg(ap, int) : -1;
        Blob *pBlob = va_arg(ap, Blob*);
        bufpt = blob_buffer(pBlob);
        length = blob_size(pBlob);
        if( limit>=0 && limit<length ) length = limit;
        break;
      }
      case etBLOBSQL: {
        int limit = flag_alternateform ? va_arg(ap, int) : -1;
        Blob *pBlob = va_arg(ap, Blob*);
        char *zOrig = blob_buffer(pBlob);
        int i, j, n, cnt;
        n = blob_size(pBlob);
        if( limit>=0 && limit<n ) n = limit;
        for(cnt=i=0; i<n; i++){ if( zOrig[i]=='\'' ) cnt++; }
        if( n+cnt+2 > etBUFSIZE ){
          bufpt = zExtra = fossil_malloc( n + cnt + 2 );
        }else{
          bufpt = buf;
        }
        bufpt[0] = '\'';
        for(i=0, j=1; i<n; i++, j++){
          if( zOrig[i]=='\'' ){ bufpt[j++] = '\''; }
          bufpt[j] = zOrig[i];
        }
        bufpt[j++] = '\'';
        length = j;
        assert( length==n+cnt+2 );
        break;
      }
      case etSQLESCAPE:
      case etSQLESCAPE2:
      case etSQLESCAPE3: {
        int i, j, n, ch, isnull;
        int needQuote;
        int limit = flag_alternateform ? va_arg(ap,int) : -1;
        char q = ((xtype==etSQLESCAPE3)?'"':'\'');  /* Quote characters */
        char *escarg = va_arg(ap,char*);
        isnull = escarg==0;
        if( isnull ) escarg = (xtype==etSQLESCAPE2 ? "NULL" : "(NULL)");
        if( limit<0 ) limit = strlen(escarg);
        for(i=n=0; i<limit; i++){
          if( escarg[i]==q )  n++;
        }
        needQuote = !isnull && xtype==etSQLESCAPE2;
        n += i + 1 + needQuote*2;
        if( n>etBUFSIZE ){
          bufpt = zExtra = fossil_malloc( n );
        }else{
          bufpt = buf;
        }
        j = 0;
        if( needQuote ) bufpt[j++] = q;
        for(i=0; i<limit; i++){
          bufpt[j++] = ch = escarg[i];
          if( ch==q ) bufpt[j++] = ch;
        }
        if( needQuote ) bufpt[j++] = q;
        bufpt[j] = 0;
        length = j;
        if( precision>=0 && precision<length ) length = precision;
        break;
      }
#if 0 /* libfsl */
      case etHTMLIZE: {
        int limit = flag_alternateform ? va_arg(ap,int) : -1;
        char *zMem = va_arg(ap,char*);
        if( zMem==0 ) zMem = "";
        zExtra = bufpt = htmlize(zMem, limit);
        length = strlen(bufpt);
        if( precision>=0 && precision<length ) length = precision;
        break;
      }
      case etHTTPIZE: {
        int limit = flag_alternateform ? va_arg(ap,int) : -1;
        char *zMem = va_arg(ap,char*);
        if( zMem==0 ) zMem = "";
        zExtra = bufpt = httpize(zMem, limit);
        length = strlen(bufpt);
        if( precision>=0 && precision<length ) length = precision;
        break;
      }
      case etURLIZE: {
        int limit = flag_alternateform ? va_arg(ap,int) : -1;
        char *zMem = va_arg(ap,char*);
        if( zMem==0 ) zMem = "";
        zExtra = bufpt = urlize(zMem, limit);
        length = strlen(bufpt);
        if( precision>=0 && precision<length ) length = precision;
        break;
      }
      case etFOSSILIZE: {
        int limit = flag_alternateform ? va_arg(ap,int) : -1;
        char *zMem = va_arg(ap,char*);
        if( zMem==0 ) zMem = "";
        zExtra = bufpt = fossilize(zMem, limit);
        length = strlen(bufpt);
        if( precision>=0 && precision<length ) length = precision;
        break;
      }
      case etJSONSTR: {
        int limit = flag_alternateform ? va_arg(ap,int) : -1;
        char *zMem = va_arg(ap,char*);
        if( limit!=0 ){
          /* Ignore the limit flag, if set, for JSON string
          ** output. This block exists to squelch the associated
          ** "unused variable" compiler warning. */
        }
        if( zMem==0 ) zMem = "";
        zExtra = bufpt =
          encode_json_string_literal(zMem, flag_altform2, &length);
        if( precision>=0 && precision<length ) length = precision;
        break;
      }
      case etWIKISTR: {
        int limit = flag_alternateform ? va_arg(ap,int) : -1;
        char *zWiki = va_arg(ap, char*);
        Blob wiki;
        blob_init(&wiki, zWiki, limit);
        wiki_convert(&wiki, pBlob, wiki_convert_flags(flag_altform2));
        blob_reset(&wiki);
        length = width = 0;
        break;
      }
      case etSHELLESC: {
        char *zArg = va_arg(ap, char*);
        blob_append_escaped_arg(pBlob, zArg, !flag_altform2);
        length = width = 0;
        break;
      }
      case etHEX: {
        char *zArg = va_arg(ap, char*);
        int szArg = (int)strlen(zArg);
        int szBlob = blob_size(pBlob);
        u8 *aBuf;
        blob_resize(pBlob, szBlob+szArg*2+1);
        aBuf = (u8*)&blob_buffer(pBlob)[szBlob];
        encode16((const u8*)zArg, aBuf, szArg);
        length = width = 0;
        break;
      }
#endif /* libfsl */
      case etERROR:
        buf[0] = '%';
        buf[1] = c;
        errorflag = 0;
        idx = 1+(c!=0);
        blob_append(pBlob,"%",idx);
        count += idx;
        if( c==0 ) fmt--;
        break;
    }/* End switch over the format type */
    /*
    ** The text of the conversion is pointed to by "bufpt" and is
    ** "length" characters long.  The field width is "width".  Do
    ** the output.
    */
    if( !flag_leftjustify ){
      register int nspace;
      nspace = width-length;
      if( nspace>0 ){
        count += nspace;
        while( nspace>=(int)etSPACESIZE ){
          blob_append(pBlob,spaces,etSPACESIZE);
          nspace -= etSPACESIZE;
        }
        if( nspace>0 ) blob_append(pBlob,spaces,nspace);
      }
    }
    if( length>0 ){
      blob_append(pBlob,bufpt,length);
      count += length;
    }
    if( flag_leftjustify ){
      register int nspace;
      nspace = width-length;
      if( nspace>0 ){
        count += nspace;
        while( nspace>=(int)etSPACESIZE ){
          blob_append(pBlob,spaces,etSPACESIZE);
          nspace -= etSPACESIZE;
        }
        if( nspace>0 ) blob_append(pBlob,spaces,nspace);
      }
    }
    if( zExtra ){
      fossil_free(zExtra);
    }
  }/* End for loop over the format string */
  return errorflag ? -1 : count;
} /* End of function */

/*
** Print into memory obtained from fossil_malloc().
*/
char *mprintf(const char *zFormat, ...){
  va_list ap;
  char *z;
  va_start(ap,zFormat);
  z = vmprintf(zFormat, ap);
  va_end(ap);
  return z;
}

/*
** Print into memory obtained from fossil_malloc().
*/
char *vmprintf(const char *zFormat, va_list ap){
  Blob blob = empty_blob;
  blob_vappendf(&blob, zFormat, ap);
  blob_materialize(&blob);
  return blob.aData;
}

