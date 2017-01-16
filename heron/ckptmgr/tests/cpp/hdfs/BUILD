package(default_visibility = ["//visibility:public"])

cc_test(
    name = "hdfs_unittest",
    srcs = ["hdfs_unittest.cpp"],
    copts = [
        "-Iheron",
        "-I$(GENDIR)/heron",
        "-Iheron/common/src/cpp",
        "-I$(GENDIR)/heron/common/src/cpp",
        "-Iheron/ckptmgr/src/cpp",
    ],
    deps = [
        "//heron/common/src/cpp/basics:basics-cxx",
        "//third_party/libhdfs3:libhdfs3-cxx",
        "//third_party/gsasl:gsasl-cxx",
        "//third_party/libxml2:libxml2-cxx",
        "//third_party/glog:glog-cxx",
        "//third_party/gtest:gtest-cxx",
    ],
    data = [
        "function-test.xml",
    ],
    linkopts = ["-lz"],
    args = ["$(location function-test.xml)"],
    size = "small",
    linkstatic = 1,
)
