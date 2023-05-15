#
MK_PROFILE=	no
LOCALBASE?=	/usr/local
LIB=            fsldb
LIBDIR=		${LOCALBASE}/lib
INCSDIR=	${LOCALBASE}/include
CFLAGS+=        -Wall -Wstrict-prototypes -fPIC -I${.CURDIR} -I${.CURDIR}/../base
.ifndef NOPRIVATE
CFLAGS+=	-I/usr/include/private/sqlite3
LDFLAGS+=	-L/usr/lib
LDADD=		-lprivatesqlite3
.else
CFLAGS+=	-I${LOCALBASE}/include
LDFLAGS+=	-L${LOCALBASE}/lib
LDADD=		-lsqlite3
.endif
SRCS+=		db.c fsldb.h
INCS=           fsldb.h
NO_OBJ=         yes

.include <bsd.lib.mk>
