
package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "tbb",
    srcs = glob([
        "include/oneapi/tbb.h",
        "include/oneapi/tbb/*.h",
        "include/oneapi/tbb/detail/*.h",
        "include/tbb/*.h",
        "src/tbb/*.h",
        "src/tbb/*.cpp",
    ]),
    hdrs = glob([
        "include/tbb/*.h",
    ]),
    copts = [
        "-D__TBB_BUILD=1",
        "-w",
    ],
    defines = [
        "USE_PTHREAD",
        "__TBB_BUILD=1",
        "TBB_PREVIEW_GRAPH_NODES=1",
        "__TBB_NO_IMPLICIT_LINKAGE",
    ],
    includes = [
        "include",
    ],
    linkopts = [
        "-ldl",
        "-lpthread",
        "-lrt",
    ],
)
