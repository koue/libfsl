VERSION=2.14
URL=https://www.fossil-scm.org/home/uv/
PACKAGE=fossil-src-$(VERSION).tar.gz
DIR=fossil-$(VERSION)
FUNCTIONS=blob db printf util

all: compile fetch extract grep

compile:
	$(CC) -o grep_function grep_function.c

grep:
	for func in $(FUNCTIONS); do \
		cat /dev/null > $${func}.h; \
		cat /dev/null > $${func}.c; \
		Y=license; \
		while read line; do \
			echo "$${line};" >> $${func}.h; \
			./grep_function "$${line}" $(DIR)/src/$${func}.c $${Y} >> $${func}.c; \
			echo >> $${func}.c; \
			Y=; \
		done < LIST_$${func}; \
	done;

fetch:
.if !exists(${PACKAGE})
	fetch $(URL)$(PACKAGE)
.endif

extract:
	tar zxf $(PACKAGE)

clean:
	rm -rf ./$(DIR)
	rm -f grep_function
	for func in $(FUNCTIONS); do \
		rm -f $${func}.[ch]; \
	done;

cleandir: clean
	rm -f $(PACKAGE)
