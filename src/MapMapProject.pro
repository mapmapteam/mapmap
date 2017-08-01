TEMPLATE = subdirs

SUBDIRS = mapmap tests

# Adds the tarball target
tarball.target = mapmap-$${VERSION}.tar.gz
tarball.commands = git archive --format=tar.gz -9 --prefix=mapmap-$${VERSION}/ --output=mapmap-$${VERSION}.tar.gz HEAD
tarball.depends = .git
QMAKE_EXTRA_TARGETS += tarball
