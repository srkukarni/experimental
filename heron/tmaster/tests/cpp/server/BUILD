package(default_visibility = ["//visibility:public"])

cc_test(
    name = "tmaster_unittest",
    args = ["$(location //heron/config/src/yaml:test-config-internals-yaml)"],
    srcs = [
        "dummystmgr.cpp",
        "tmaster_unittest.cpp",
        "dummystmgr.h",
    ],
    deps = [
        "//heron/stmgr/src/cpp:manager-cxx",
        "//heron/tmaster/src/cpp:tmaster-cxx",
        "//third_party/gtest:gtest-cxx",
    ],
    data = [
        "//heron/config/src/yaml:test-config-internals-yaml",
    ],
    copts = [
        "-Iheron",
        "-Iheron/common/src/cpp",
        "-Iheron/statemgrs/src/cpp",
        "-Iheron/stmgr/src/cpp",
        "-Iheron/tmaster/src/cpp",
        "-Iheron/tmaster/tests/cpp",
        "-I$(GENDIR)/heron",
        "-I$(GENDIR)/heron/common/src/cpp",
    ],
    size = "small",
    flaky = 1,
    linkstatic = 1,
)

cc_test(
    name = "stateful_checkpointer_unittest",
    srcs = [
        "stateful_checkpointer_unittest.cpp",
    ],
    deps = [
        "//heron/stmgr/src/cpp:manager-cxx",
        "//heron/tmaster/src/cpp:tmaster-cxx",
        "//third_party/gtest:gtest-cxx",
    ],
    copts = [
        "-Iheron",
        "-Iheron/common/src/cpp",
        "-Iheron/statemgrs/src/cpp",
        "-Iheron/stmgr/src/cpp",
        "-Iheron/tmaster/src/cpp",
        "-Iheron/tmaster/tests/cpp",
        "-I$(GENDIR)/heron",
        "-I$(GENDIR)/heron/common/src/cpp",
    ],
    size = "small",
    flaky = 1,
    linkstatic = 1,
)

cc_test(
    name = "stateful_restorer_unittest",
    srcs = [
        "stateful_restorer_unittest.cpp",
        "dummytmaster.cpp",
        "dummystmgr.cpp",
        "dummytmaster.h",
        "dummystmgr.h",
    ],
    deps = [
        "//heron/tmaster/src/cpp:tmaster-cxx",
        "//third_party/gtest:gtest-cxx",
    ],
    copts = [
        "-Iheron",
        "-Iheron/common/src/cpp",
        "-Iheron/statemgrs/src/cpp",
        "-Iheron/tmaster/src/cpp",
        "-Iheron/tmaster/tests/cpp",
        "-I$(GENDIR)/heron",
        "-I$(GENDIR)/heron/common/src/cpp",
    ],
    size = "small",
    flaky = 1,
    linkstatic = 1,
)
