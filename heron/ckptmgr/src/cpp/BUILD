package(default_visibility = ["//visibility:public"])

cc_library(
    name = "common-cxx",
    srcs = glob(["common/*.cpp"]),
    hdrs = glob(["common/*.h"]),
    copts = [
        "-Iheron",
        "-I$(GENDIR)/heron",
        "-Iheron/common/src/cpp",
        "-I$(GENDIR)/heron/common/src/cpp",
        "-Iheron/ckptmgr/src/cpp",
    ],
    deps = [
        "//heron/proto:proto-cxx",
        "//heron/common/src/cpp/config:config-cxx",
        "//heron/common/src/cpp/network:network-cxx",
    ],
    linkstatic = 1,
)

cc_library(
    name = "localfs-cxx",
    srcs = glob(["localfs/*.cpp"]),
    hdrs = glob(["localfs/*.h"]),
    copts = [
        "-Iheron",
        "-I$(GENDIR)/heron",
        "-Iheron/common/src/cpp",
        "-I$(GENDIR)/heron/common/src/cpp",
        "-Iheron/ckptmgr/src/cpp",
    ],
    deps = [
        ":common-cxx",
    ],
    linkstatic = 1,
)

cc_library(
    name = "hadoopfs-cxx",
    srcs = glob(["hadoopfs/*.cpp"]),
    hdrs = glob(["hadoopfs/*.h"]),
    copts = [
        "-Iheron",
        "-I$(GENDIR)/heron",
        "-Iheron/common/src/cpp",
        "-I$(GENDIR)/heron/common/src/cpp",
        "-Iheron/ckptmgr/src/cpp",
    ],
    deps = [
        ":common-cxx",
    ],
    linkstatic = 1,
)

cc_library(
    name = "manager-cxx",
    srcs = glob(["manager/*.cpp"]),
    hdrs = glob(["manager/*.h"]),
    copts = [
        "-Iheron",
        "-Iheron/common/src/cpp",
        "-Iheron/ckptmgr/src/cpp",
        "-I$(GENDIR)/heron",
        "-I$(GENDIR)/heron/common/src/cpp"
    ],
    deps = [
        ":common-cxx",
    ],
    linkstatic = 1,
)

cc_binary(
    name = "heron-ckptmgr",
    srcs = glob(["server/*.cpp"]),
    copts = [
        "-Iheron",
        "-Iheron/common/src/cpp",
        "-Iheron/ckptmgr/src/cpp",
        "-I$(GENDIR)/heron",
        "-I$(GENDIR)/heron/common/src/cpp",
    ],
    deps = [
        ":localfs-cxx",
        ":manager-cxx",
    ],
    linkstatic = 1,
)
