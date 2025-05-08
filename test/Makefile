#
LOCALBASE?=	/usr/local
PROGS=		blob_test \
		printf_test

CFLAGS=		-I${.CURDIR}/../ \
		-I${.CURDIR}/../src/base

LDFLAGS+=	-L${.CURDIR}/../src/base

LDADD.blob_test=	-lfslbase
LDADD.printf_test=	-lfslbase

.ifndef NOSQLITE
PROGS+=		db_test
CFLAGS+=	-I${.CURDIR}/../src/db
LDFLAGS+=	-L${.CURDIR}/../src/db
LDADD.db_test=	-lfslbase -lfsldb
.  ifndef NOPRIVATE
CFLAGS+=	-I/usr/include/private/sqlite3
LDFLAGS+=	-L/usr/lib
LDADD.db_test+=	-lprivatesqlite3
.  else
CFLAGS+=	-I${LOCALBASE}/include
LDFLAGS+=	-L${LOCALBASE}/lib
LDADD.db_test+=	-lsqlite3
.  endif
.endif

MAN=
MK_DEBUG_FILES=	no
MAKEOBJDIR= .

.ifdef VALGRIND
VALGRIND_CMD?=	valgrind -q --tool=memcheck --leak-check=yes --num-callers=20
.else
VALGRIND_CMD?=
.endif

# keep this empty otherwise installation fails
install:

test:
	${VALGRIND_CMD} ./blob_test
.ifndef NOSQLITE
	${VALGRIND_CMD} ./db_test
.endif
	${VALGRIND_CMD} ./printf_test

.include <bsd.progs.mk>
