licenses(["notice"])

package(default_visibility = ["//visibility:public"])

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
        "//third_party/java:hadoop-core",
#        # for hdfs-state-backend, these jars should be attached in runtime
#        "@org_apache_hadoop_hadoop_common_2_6_1//jar",
#        "@org_apache_hadoop_hadoop_annotations_2_6_1//jar",
#        "@commons_logging_commons_logging//jar",
#        "@commons_collections_commons_collections//jar",
#        "@commons_lang_commons_lang//jar",
#        "@commons_configuration_commons_configuration_1_6//jar",
#        "@org_apache_hadoop_hadoop_auth_2_6_1//jar",
#        "//third_party/java:logging",
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

genrule(
    name = "heron-ckptmgr",
    srcs = [":ckptmgr-unshaded_deploy.jar"],
    outs = ["heron-ckptmgr.jar"],
    cmd  = "cp $< $@",
)
