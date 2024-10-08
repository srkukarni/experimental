licenses(["notice"])

package(default_visibility = ["//visibility:public"])

package_name = "libxml2"
package_version = "2.9.4"

package_file = package_name + "-" + package_version + ".tar.gz"
package_dir = package_name + "-" + package_version

genrule(
    name = "libxml2-srcs",
    srcs = [
        package_file,
    ],
    outs = [
        "include/libxml2/libxml/DOCBparser.h",
	"include/libxml2/libxml/catalog.h",
        "include/libxml2/libxml/globals.h",
        "include/libxml2/libxml/parserInternals.h",
        "include/libxml2/libxml/xmlautomata.h",
        "include/libxml2/libxml/xmlregexp.h",
        "include/libxml2/libxml/xmlversion.h",
        "include/libxml2/libxml/HTMLparser.h",
        "include/libxml2/libxml/chvalid.h",
        "include/libxml2/libxml/hash.h",
        "include/libxml2/libxml/pattern.h",
        "include/libxml2/libxml/uri.h",
        "include/libxml2/libxml/xmlerror.h",
        "include/libxml2/libxml/xmlsave.h",
        "include/libxml2/libxml/xmlwriter.h",
        "include/libxml2/libxml/HTMLtree.h",
        "include/libxml2/libxml/debugXML.h",
        "include/libxml2/libxml/list.h",
        "include/libxml2/libxml/relaxng.h",
        "include/libxml2/libxml/valid.h",
        "include/libxml2/libxml/xmlexports.h",
        "include/libxml2/libxml/xmlschemas.h",
        "include/libxml2/libxml/xpath.h",
        "include/libxml2/libxml/SAX.h",
        "include/libxml2/libxml/dict.h",
        "include/libxml2/libxml/nanoftp.h",
        "include/libxml2/libxml/schemasInternals.h",
        "include/libxml2/libxml/xinclude.h",
        "include/libxml2/libxml/xmlmemory.h",
        "include/libxml2/libxml/xmlschemastypes.h",
        "include/libxml2/libxml/xpathInternals.h",
        "include/libxml2/libxml/SAX2.h",
        "include/libxml2/libxml/encoding.h",
        "include/libxml2/libxml/nanohttp.h",
        "include/libxml2/libxml/schematron.h",
        "include/libxml2/libxml/xlink.h",
        "include/libxml2/libxml/xmlmodule.h",
        "include/libxml2/libxml/xmlstring.h",
        "include/libxml2/libxml/xpointer.h",
        "include/libxml2/libxml/c14n.h",
        "include/libxml2/libxml/entities.h",
        "include/libxml2/libxml/parser.h",
        "include/libxml2/libxml/threads.h",
        "include/libxml2/libxml/xmlIO.h",
        "include/libxml2/libxml/xmlreader.h",
        "include/libxml2/libxml/xmlunicode.h",
        "include/libxml2/libxml/tree.h",
        "lib/libxml2.a",
    ],
    cmd = "\n".join([
        "export INSTALL_DIR=$$(pwd)/$(@D)",
        "export TMP_DIR=$$(mktemp -d -t libxml2.XXXXX)",
        "mkdir -p $$TMP_DIR",
        "cp -R $(SRCS) $$TMP_DIR",
        "cd $$TMP_DIR",
        "tar xfz " + package_file,
        "cd " + package_dir,
        "./configure --prefix=$$INSTALL_DIR --enable-shared=no --without-python --without-lzma",
        "make install",
        "rm -rf $$TMP_DIR",
    ]),
)

cc_library(
    name = "libxml2-cxx",
    srcs = [
        "empty.cc",
        "lib/libxml2.a",
        "include/libxml2/libxml/DOCBparser.h",
	"include/libxml2/libxml/catalog.h",
        "include/libxml2/libxml/globals.h",
        "include/libxml2/libxml/parserInternals.h",
        "include/libxml2/libxml/xmlautomata.h",
        "include/libxml2/libxml/xmlregexp.h",
        "include/libxml2/libxml/xmlversion.h",
        "include/libxml2/libxml/HTMLparser.h",
        "include/libxml2/libxml/chvalid.h",
        "include/libxml2/libxml/hash.h",
        "include/libxml2/libxml/pattern.h",
        "include/libxml2/libxml/uri.h",
        "include/libxml2/libxml/xmlerror.h",
        "include/libxml2/libxml/xmlsave.h",
        "include/libxml2/libxml/xmlwriter.h",
        "include/libxml2/libxml/HTMLtree.h",
        "include/libxml2/libxml/debugXML.h",
        "include/libxml2/libxml/list.h",
        "include/libxml2/libxml/relaxng.h",
        "include/libxml2/libxml/valid.h",
        "include/libxml2/libxml/xmlexports.h",
        "include/libxml2/libxml/xmlschemas.h",
        "include/libxml2/libxml/xpath.h",
        "include/libxml2/libxml/SAX.h",
        "include/libxml2/libxml/dict.h",
        "include/libxml2/libxml/nanoftp.h",
        "include/libxml2/libxml/schemasInternals.h",
        "include/libxml2/libxml/xinclude.h",
        "include/libxml2/libxml/xmlmemory.h",
        "include/libxml2/libxml/xmlschemastypes.h",
        "include/libxml2/libxml/xpathInternals.h",
        "include/libxml2/libxml/SAX2.h",
        "include/libxml2/libxml/encoding.h",
        "include/libxml2/libxml/nanohttp.h",
        "include/libxml2/libxml/schematron.h",
        "include/libxml2/libxml/xlink.h",
        "include/libxml2/libxml/xmlmodule.h",
        "include/libxml2/libxml/xmlstring.h",
        "include/libxml2/libxml/xpointer.h",
        "include/libxml2/libxml/c14n.h",
        "include/libxml2/libxml/entities.h",
        "include/libxml2/libxml/threads.h",
        "include/libxml2/libxml/xmlIO.h",
        "include/libxml2/libxml/xmlreader.h",
        "include/libxml2/libxml/xmlunicode.h",
    ],
    hdrs = [
        "include/libxml2/libxml/parser.h",
        "include/libxml2/libxml/tree.h",
    ],
    includes = [
        "include/libxml2",
    ],
    linkstatic = 1,
)

filegroup(
    name = "libxml2",
    srcs = [
        ":libxml2-cxx",
    ]
)
