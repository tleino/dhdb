PROGRAMS = \
	test_dhdb \
	test_dhdb_json \
	test_dhdb_path \
	test_dhdb_xml \
	test_dhdb_ini

test_dhdb_OBJS = \
	test_dhdb.o \
	dhdb.o \
	dhdb_dump.o \
	dhdb_json.o \
	dhdb_path.o \
	dhdb_xml.o

test_dhdb_path_OBJS = \
	test_dhdb_path.o \
	dhdb_dump.o \
	dhdb.o \
	dhdb_path.o

test_dhdb_json_OBJS = \
	test_dhdb_json.o \
	dhdb.o \
	dhdb_dump.o \
	dhdb_json.o

test_dhdb_xml_OBJS = \
	test_dhdb_xml.o	\
	dhdb.o \
	dhdb_dump.o \
	dhdb_xml.o

test_dhdb_ini_OBJS = \
	test_dhdb_ini.o \
	dhdb.o \
	dhdb_dump.o \
	dhdb_ini.o

VALGRIND_AUTORUN = \
	test_dhdb \
	test_dhdb_path \
	test_dhdb_json \
	test_dhdb_xml \
	test_dhdb_ini

include rules.mk
