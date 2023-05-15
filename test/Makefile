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
# fossil cson requires sqlite
PROGS+=		cson_test \
		db_test
CFLAGS+=	-I${.CURDIR}/../src/cson \
		-I${.CURDIR}/../src/db
LDFLAGS+=	-L${.CURDIR}/../src/cson \
		-L${.CURDIR}/../src/db
LDADD.cson_test=-lfslbase -lfslcson
LDADD.db_test=	-lfslbase -lfsldb
.  ifndef NOPRIVATE
CFLAGS+=	-I/usr/include/private/sqlite3
LDFLAGS+=	-L/usr/lib
LDADD.cson_test+=	-lprivatesqlite3
LDADD.db_test+=	-lprivatesqlite3
.  else
CFLAGS+=	-I${LOCALBASE}/include
LDFLAGS+=	-L${LOCALBASE}/lib
LDADD.cson_test+=	-lsqlite3
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
	${VALGRIND_CMD} ./cson_test
	${VALGRIND_CMD} ./db_test
.endif
	${VALGRIND_CMD} ./printf_test

.include <bsd.progs.mk>
