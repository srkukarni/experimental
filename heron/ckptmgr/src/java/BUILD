licenses(["notice"])

package(default_visibility = ["//visibility:public"])

load("/tools/rules/jarjar_rules", "jarjar_binary")

java_library(
    name = "ckptmgr-java",
    srcs = glob(
        ["**/*.java"],
        exclude = ["**/CheckpointManager.java"],
    ),
    deps = [
        "//heron/api/src/java:api-java",
        "//heron/common/src/java:common-java",
        "//heron/proto:proto_common_java",
        "//heron/proto:proto_ckptmgr_java",
        "@com_google_protobuf_protobuf_java//jar",
        "@org_yaml_snakeyaml//jar",
        "//third_party/java:hadoop-core",  # for hdfs-state-backend
    ],
)

java_binary(
    name = "ckptmgr-unshaded",
    srcs = glob(["**/CheckpointManager.java"]),
    deps = [
        ":ckptmgr-java",
        "//heron/api/src/java:api-java",
        "//heron/common/src/java:common-java",
        "//heron/proto:proto_ckptmgr_java",
    ],
)

jarjar_binary(
    name = "heron-ckptmgr",
    src = ":ckptmgr-unshaded_deploy.jar",
    shade = "shade.conf",
    deps = ["@org_sonatype_plugins_jarjar_maven_plugin//jar"]
)
