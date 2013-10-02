TEMPLATE = subdirs
CONFIG += ordered
CONFIG -= qt

SUBDIRS = \
    project \
    test

test.depends = project
