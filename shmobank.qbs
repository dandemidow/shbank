import qbs
import "CxxStaticLibrary.qbs" as CxxStaticLibrary
import "ConsoleApplication.qbs" as ConsoleApplication

Project {
    CxxStaticLibrary {
        id: lib
        name: "shmobank"
        cpp.staticLibraries: ["sharedmem"]
        files: [
            "src/monmsg.c",
            "src/bank.c",
            "src/queue.c",
            "src/msg.c"
        ]

        Group {
            name: "headers"
            qbs.install: true
            qbs.installDir: "/usr/include/shmobank"
            files: [
                "src/limits.h",
                "src/monmsg.h",
                "src/bank.h",
                "src/queue.h",
                "src/msg.h"
            ]
        }
        Group {
            fileTagsFilter: product.type
            qbs.install: true
            qbs.installDir: "/usr/lib"
        }
    }

    ConsoleApplication {
        name: "test"
        type: ["application", "autotest"]
        cpp.libraryPaths: [ lib.buildDirectory ]
        cpp.staticLibraries: [ "shmobank", "sharedmem", "gpio" ]
        cpp.dynamicLibraries: ["pthread", "asound", "check", "rt"]
        cpp.includePaths: [ "src/" ]
        files: [ "src/tests/tst_monmsg.c" ]
        Group {
            name: "shmobank"
            prefix: "src/"
            files: [ "*.h", "*.c" ]
            excludeFiles: "acarsdec.c"
        }
    }
}
