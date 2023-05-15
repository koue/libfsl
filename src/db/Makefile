#
MK_PROFILE=	no
LOCALBASE?=	/usr/local
LIB=            fsldb
LIBDIR=		${LOCALBASE}/lib
INCSDIR=	${LOCALBASE}/include
CFLAGS+=        -Wall -Wstrict-prototypes -fPIC -I${.CURDIR} -I${.CURDIR}/../base
CFLAGS+=	-I${LOCALBASE}/include
LDFLAGS+=	-L${LOCALBASE}/lib
LDADD=		-lsqlite3
SRCS+=		db.c fsldb.h
INCS=           fsldb.h
NO_OBJ=         yes

.include <bsd.lib.mk>
