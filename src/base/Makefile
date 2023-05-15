#
MK_PROFILE=	no
LOCALBASE?=	/usr/local
LIB=            fslbase
LIBDIR=		${LOCALBASE}/lib
INCSDIR=	${LOCALBASE}/include
CFLAGS+=        -Wall -Wstrict-prototypes -fPIC -I${.CURDIR}
SRCS=		blob.c printf.c util.c fslbase.h
INCS=           fslbase.h
NO_OBJ=         yes

.include <bsd.lib.mk>
