#
MK_PROFILE=	no
LOCALBASE?=	/usr/local
LIB=            fslcson
LIBDIR=		${LOCALBASE}/lib
INCSDIR=	${LOCALBASE}/include
CFLAGS+=        -Wall -Wstrict-prototypes -fPIC -I${.CURDIR}
.ifndef NOPRIVATE
CFLAGS+=	-I/usr/include/private/sqlite3
LDFLAGS+=	-L/usr/lib
LDADD=		-lprivatesqlite3
.else
CFLAGS+=	-I${LOCALBASE}/include
LDFLAGS+=	-L${LOCALBASE}/lib
LDADD=		-lsqlite3
.endif
SRCS=		cson_amalgamation.c fslcson.h
INCS=		fslcson.h
NO_OBJ=         yes

.include <bsd.lib.mk>
