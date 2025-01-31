package(default_visibility = [
    "//:__subpackages__",
])

cc_binary(
    name = "asset_stream_manager",
    srcs = ["main.cc"],
    data = [":roots_pem"],
    deps = [
        ":asset_stream_config",
        ":session_management_server",
        "//cdc_stream",
        "//common:log",
        "//common:path",
        "//common:sdk_util",
        "//data_store:data_provider",
    ],
)

cc_library(
    name = "asset_stream_server",
    srcs = [
        "asset_stream_server.cc",
        "grpc_asset_stream_server.cc",
        "testing_asset_stream_server.cc",
    ],
    hdrs = [
        "asset_stream_server.h",
        "grpc_asset_stream_server.h",
        "testing_asset_stream_server.h",
    ],
    deps = [
        "//common:grpc_status",
        "//common:log",
        "//common:path",
        "//common:status",
        "//common:status_macros",
        "//common:thread_safe_map",
        "//data_store",
        "//manifest:manifest_updater",
        "//proto:asset_stream_service_grpc_proto",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "asset_stream_config",
    srcs = ["asset_stream_config.cc"],
    hdrs = ["asset_stream_config.h"],
    deps = [
        ":multi_session",
        "//absl_helper:jedec_size_flag",
        "//common:log",
        "//common:path",
        "//common:status_macros",
        "@com_github_jsoncpp//:jsoncpp",
        "@com_google_absl//absl/flags:parse",
    ],
)

cc_library(
    name = "cdc_fuse_manager",
    srcs = ["cdc_fuse_manager.cc"],
    hdrs = ["cdc_fuse_manager.h"],
    deps = [
        "//cdc_fuse_fs:constants",
        "//common:gamelet_component",
        "//common:remote_util",
        "//common:status_macros",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "session_management_server",
    srcs = [
        "background_service_impl.cc",
        "background_service_impl.h",
        "local_assets_stream_manager_service_impl.cc",
        "local_assets_stream_manager_service_impl.h",
        "session_management_server.cc",
        "session_manager.cc",
        "session_manager.h",
    ],
    hdrs = ["session_management_server.h"],
    deps = [
        ":multi_session",
        "//common:grpc_status",
        "//common:log",
        "//common:status_macros",
        "//common:util",
        "//manifest:manifest_updater",
        "//metrics",
        "//proto:background_service_grpc_proto",
        "//proto:local_assets_stream_manager_grpc_proto",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "multi_session",
    srcs = [
        "multi_session.cc",
        "session.cc",
        "session.h",
    ],
    hdrs = [
        "multi_session.h",
        "session_config.h",
    ],
    deps = [
        ":asset_stream_server",
        ":cdc_fuse_manager",
        ":metrics_recorder",
        "//common:file_watcher",
        "//common:log",
        "//common:path",
        "//common:port_manager",
        "//common:process",
        "//common:remote_util",
        "//common:sdk_util",
        "//common:status_macros",
        "//common:stopwatch",
        "//data_store:disk_data_store",
        "//manifest:manifest_printer",
        "//manifest:manifest_updater",
        "@com_google_absl//absl/status",
    ],
)

cc_test(
    name = "multi_session_test",
    srcs = ["multi_session_test.cc"],
    data = [":all_test_data"],
    deps = [
        ":multi_session",
        "//common:test_main",
        "//manifest:manifest_test_base",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "metrics_recorder",
    srcs = ["metrics_recorder.cc"],
    hdrs = ["metrics_recorder.h"],
    deps = [
        "//common:log",
        "//common:util",
        "//metrics",
        "//metrics:enums",
        "//metrics:messages",
        "@com_google_absl//absl/status",
    ],
)

cc_test(
    name = "metrics_recorder_test",
    srcs = ["metrics_recorder_test.cc"],
    deps = [
        ":metrics_recorder",
        "//common:status_test_macros",
        "//common:test_main",
        "//metrics",
        "@com_google_googletest//:gtest",
    ],
)

# Copy roots.pem to the output folder, required for authenticated gRPC.
genrule(
    name = "roots_pem",
    srcs = ["@com_github_grpc_grpc//:root_certificates"],
    outs = ["roots.pem"],
    cmd = "cp $(location @com_github_grpc_grpc//:root_certificates) $(location roots.pem)",
)

filegroup(
    name = "all_test_sources",
    srcs = glob(["*_test.cc"]),
)

filegroup(
    name = "all_test_data",
    srcs = glob(["testdata/**"]),
)
