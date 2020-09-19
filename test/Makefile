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
		-I${.CURDIR}/../src/db \
		-I${LOCALBASE}/include
LDFLAGS+=	-L${.CURDIR}/../src/cson \
		-L${.CURDIR}/../src/db \
		-L${LOCALBASE}/lib
LDADD.cson_test=-lfslbase -lfslcson -lsqlite3
LDADD.db_test=	-lfslbase -lfsldb -lsqlite3
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
