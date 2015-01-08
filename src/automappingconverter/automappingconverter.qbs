import qbs 1.0

QtGuiApplication {
    name: "automappingconverter"
    targetName: name

    Depends { name: "libtiled" }
    Depends { name: "Qt"; submodules: ["widgets"] }

    cpp.includePaths: ["."]
    cpp.rpaths: ["$ORIGIN/../lib"]

    consoleApplication: false

    files: [
        "convertercontrol.cpp",
        "convertercontrol.h",
        "converterdatamodel.cpp",
        "converterdatamodel.h",
        "converterwindow.cpp",
        "converterwindow.h",
        "converterwindow.ui",
        "main.cpp",
    ]

    Properties {
        condition: qbs.targetOS.contains("osx")
        targetName: "Automapping Converter"
    }

    Group {
        qbs.install: true
        qbs.installDir: {
            if (qbs.targetOS.contains("windows") || qbs.targetOS.contains("osx"))
                return ""
            else
                return "bin"
        }
        fileTagsFilter: product.type
    }
}
