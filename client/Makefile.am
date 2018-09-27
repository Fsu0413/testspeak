AUTOMAKE_OPTIONS=foreign subdir-objects

bin_PROGRAMS=test1

test1_SOURCES= $(top_srcdir)/http_client.cpp \
$(top_srcdir)/http_client_utils.cpp \
$(top_srcdir)/http_header.cpp \
$(top_srcdir)/json_reader.cpp \
$(top_srcdir)/json_value.cpp \
$(top_srcdir)/json_writer.cpp \
$(top_srcdir)/main.cpp \
$(top_srcdir)/new_line.cpp \
$(top_srcdir)/tcp_client.cpp


test1_CPPFLAGS=-I$(top_srcdir)/include
test1_CXXFLAGS=--std=gnu++11
