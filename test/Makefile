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

# keep this empty otherwise installation fails
install:

test:
	./blob_test
.ifndef NOSQLITE
	./cson_test
	./db_test
.endif
	./printf_test

.include <bsd.progs.mk>
